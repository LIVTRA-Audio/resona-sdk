#!/usr/bin/env python3
"""GPL-3.0. Build a Resona .rsp animated splash using only Python's standard library.
Usage: python3 splash.py animation.json animation.rsp
JSON: {"duration_ms":5000,"frames":[{"hold_ms":100,"background":"#001020",
       "rects":[{"x":20,"y":70,"w":100,"h":8,"color":"#00ffcc"}]}]}
Frames loop for duration_ms; zero skips. Maximum installed asset24KiB.
"""
import json, struct, sys, zlib
W,H=320,172
MAX_BYTES=24576

def color(value):
    if not isinstance(value,str) or len(value)!=7 or value[0]!='#':
        raise ValueError('colors use #RRGGBB')
    n=int(value[1:],16)
    return ((n>>19)&31)<<11 | ((n>>10)&63)<<5 | ((n>>3)&31)

def number(value, low, high):
    if type(value) is not int or not low<=value<=high:
        raise ValueError(f'integer must be {low}..{high}')
    return value

def build(doc):
    if set(doc)-{'duration_ms','frames'}: raise ValueError('unknown animation field')
    duration=number(doc['duration_ms'],0,0x7fffffff)
    frames=doc['frames']
    if not isinstance(frames,list) or not 1<=len(frames)<=128: raise ValueError('use1..128 frames')
    payload=bytearray()
    for f in frames:
        if set(f)-{'hold_ms','background','rects'}: raise ValueError('unknown frame field')
        hold=number(f['hold_ms'],16,60000)
        pixels=[color(f.get('background','#000000'))]*(W*H)
        rects=f.get('rects',[])
        if not isinstance(rects,list) or len(rects)>256: raise ValueError('maximum256 rectangles per frame')
        for r in rects:
            if set(r)!={'x','y','w','h','color'}: raise ValueError('rectangle fields: x,y,w,h,color')
            x=number(r['x'],0,W-1);y=number(r['y'],0,H-1)
            w=number(r['w'],1,W-x);h=number(r['h'],1,H-y);c=color(r['color'])
            for row in range(y,y+h): pixels[row*W+x:row*W+x+w]=[c]*w
        runs=[];last=pixels[0];count=0
        for p in pixels:
            if p!=last or count==65535:
                runs.append((count,last));count=0;last=p
            count+=1
        runs.append((count,last))
        payload.extend(struct.pack('<II',hold,len(runs)))
        for run in runs: payload.extend(struct.pack('<HH',*run))
        if len(payload)+32>MAX_BYTES: raise ValueError('splash exceeds24KiB; simplify frames or use fewer frames')
    return struct.pack('<4s7I',b'RSP1',32+len(payload),zlib.crc32(payload),duration,W,H,len(frames),0)+payload

if __name__=='__main__':
    if len(sys.argv)!=3: raise SystemExit(__doc__)
    try:
        with open(sys.argv[1],encoding='utf-8') as f: data=build(json.load(f))
        with open(sys.argv[2],'wb') as f: f.write(data)
        print(f'{len(data)} bytes')
    except (ValueError,KeyError,TypeError,OSError) as e: raise SystemExit(str(e))
