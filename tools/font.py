#!/usr/bin/env python3
"""Generate an App-owned 1-bit font header from a user-supplied TTF/OTF.

Requires Pillow for this offline authoring step. The generated C/C++ needs only
the GPL-3.0 SDK. Font rights remain with the source font; its notice is copied beside
the output. No download, firmware write or font installation is performed.
"""
import argparse
import json
from pathlib import Path
import re
import shutil

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('font', type=Path)
    text = ap.add_mutually_exclusive_group(required=True)
    text.add_argument('--text')
    text.add_argument('--text-file', type=Path)
    ap.add_argument('--size', type=int, default=16)
    ap.add_argument('--name', default='kAppFont')
    ap.add_argument('--license', type=Path, required=True, help='Source font license/notice to keep with generated glyphs')
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', a.name):
        ap.error('--name must be a C identifier')
    if not 8 <= a.size <= 64:
        ap.error('--size must be8..64')
    if not a.font.is_file() or not a.license.is_file():
        ap.error('font and license must be existing files')
    notice = a.out.with_name(a.out.stem+'.LICENSE.txt')
    if a.out.resolve() in (a.font.resolve(), a.license.resolve()) or notice.resolve()==a.font.resolve():
        ap.error('outputs must not overwrite the source font or its notice')
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        ap.error('Install Pillow for font authoring: python -m pip install Pillow')
    source = a.text_file.read_text(encoding='utf-8') if a.text_file else a.text
    codepoints = sorted({ord(c) for c in source if c not in '\r\n'})
    if not codepoints or len(codepoints) > 65536:
        ap.error('select1..65536 glyphs')
    font = ImageFont.truetype(str(a.font), a.size)
    ascent, descent = font.getmetrics()
    bitmap = bytearray()
    glyphs = []
    for cp in codepoints:
        if 0xd800 <= cp <= 0xdfff:
            ap.error('text contains an invalid Unicode scalar')
        ch = chr(cp)
        x0, y0, x1, y1 = font.getbbox(ch, anchor='ls')
        width, height = x1-x0, y1-y0
        image = Image.new('L', (max(1, width), max(1, height)), 0)
        ImageDraw.Draw(image).text((-x0, -y0), ch, font=font, fill=255, anchor='ls')
        offset = len(bitmap)
        for y in range(height):
            for start in range(0, width, 8):
                value = sum((0x80 >> k) for k in range(8) if start+k < width and image.getpixel((start+k,y)) >= 128)
                bitmap.append(value)
        advance = int(font.getlength(ch)+.5)
        glyphs.append((cp,offset,width,height,advance,x0,ascent+y0,0))
    if any(g[2] > 256 or g[3] > 256 or g[4] > 1024 for g in glyphs):
        ap.error('glyph metrics exceed SDK bounds')
    source_name = a.font.name.replace('*/', '* /').replace('\n', ' ').replace('\r', ' ')
    notice_name = notice.name.replace('*/', '* /').replace('\n', ' ').replace('\r', ' ')
    lines = [f'/* Generated font subset: {source_name}. Keep {notice_name} with it. */',
             '#pragma once', '#include <resona/font.h>',
             f'static const uint8_t {a.name}Bitmap[] = {{']
    for i in range(0,len(bitmap),16):
        lines.append('    '+','.join(f'0x{v:02x}' for v in bitmap[i:i+16])+',')
    if not bitmap:
        lines.append('    0,')
    lines.extend(['};',f'static const resona_font_glyph {a.name}Glyphs[] = {{'])
    for g in glyphs:
        lines.append('    {'+','.join(str(v) for v in g)+'},')
    lines.extend(['};',f'static const resona_bitmap_font {a.name} = {{',
                  f'    {a.name}Glyphs,{a.name}Bitmap,{len(glyphs)},{len(bitmap)},{ascent+descent},{ascent}', '};', ''])
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text('\n'.join(lines),encoding='utf-8')
    if a.license.resolve()!=notice.resolve():
        shutil.copyfile(a.license,notice)
    print(json.dumps({'header':str(a.out),'notice':str(notice),'glyphs':len(glyphs),'bitmap_bytes':len(bitmap)},ensure_ascii=False))

if __name__ == '__main__':
    main()
