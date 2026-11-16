/* GPL-3.0. App/module-owned 1-bit Unicode bitmap fonts, drawn through public UI calls.
 * Store glyphs/bitmap in the project; validate once before drawing. No LVGL or
 * firmware-font dependency. UTF-8 scalars only; no shaping, bidi or kerning. */
#ifndef RESONA_FONT_H
#define RESONA_FONT_H
#include <stdint.h>
#include "resona/module.h"
#include "resona/utf8.h"

typedef struct resona_font_glyph {
    uint32_t codepoint, offset;
    uint16_t width, height, advance;
    int16_t x_offset, y_offset;
    uint16_t reserved;
} resona_font_glyph;
typedef struct resona_bitmap_font {
    const resona_font_glyph *glyphs; /* sorted, unique Unicode scalar values */
    const uint8_t *bitmap;          /* rows: ceil(width/8) bytes, MSB first */
    uint32_t glyph_count, bitmap_bytes;
    uint16_t line_height, baseline; /* distances from line top, in unscaled pixels */
} resona_bitmap_font;

static inline int resona_font_header_valid(const resona_bitmap_font *f)
{
    return f&&f->glyphs&&f->bitmap&&f->glyph_count&&f->glyph_count<=65536u&&
        f->line_height&&f->line_height<=1024u&&f->baseline<=f->line_height;
}

static inline int resona_font_glyph_valid(const resona_bitmap_font *f,const resona_font_glyph *g)
{
    uint32_t bytes=((uint32_t)g->width+7u)/8u*g->height;
    return g->codepoint<=0x10ffffu && !(g->codepoint>=0xd800u&&g->codepoint<=0xdfffu) &&
        !g->reserved && g->width<=256u && g->height<=256u && g->advance<=1024u &&
        g->offset<=f->bitmap_bytes && bytes<=f->bitmap_bytes-g->offset;
}
static inline int resona_font_valid(const resona_bitmap_font *f)
{
    if(!resona_font_header_valid(f))return 0;
    for(uint32_t i=0;i<f->glyph_count;i++)if(!resona_font_glyph_valid(f,f->glyphs+i)||
        (i&&f->glyphs[i-1].codepoint>=f->glyphs[i].codepoint))return 0;
    return 1;
}
static inline const resona_font_glyph *resona_font_find(const resona_bitmap_font *f,uint32_t cp)
{
    uint32_t lo=0,hi=f->glyph_count;
    while(lo<hi){uint32_t mid=lo+(hi-lo)/2;if(f->glyphs[mid].codepoint<cp)lo=mid+1;else hi=mid;}
    return lo<f->glyph_count&&f->glyphs[lo].codepoint==cp?f->glyphs+lo:0;
}
/* Longest line's pen advance, including final spacing. Missing glyphs use the
 * small built-in font (one fallback per scalar), with advance6*scale. */
static inline int32_t resona_font_text_advance(const resona_bitmap_font *f,uint32_t scale,const char *text)
{
    if(!resona_font_header_valid(f)||!text||!scale||scale>8u)return -1;
    uint32_t bytes=0,at=0;int64_t line=0,maximum=0;
    while(text[bytes])bytes++;
    while(at<bytes){uint32_t used,cp=resona_utf8_decode((const uint8_t*)text+at,bytes-at,&used);at+=used;
        if(cp=='\n'){if(line>maximum)maximum=line;line=0;continue;}
        const resona_font_glyph *g=resona_font_find(f,cp);
        if(g&&!resona_font_glyph_valid(f,g))return -1;
        line+=(int64_t)(g?g->advance:6u)*scale;if(line>INT32_MAX)return -1;
    }
    return (int32_t)(line>maximum?line:maximum);
}
/* Call with a validated immutable font. Rectangle runs reduce draw-call cost;
 * DSP module screens retain their usual per-frame command budget. */
static inline int32_t resona_ui_text_font(const resona_ui *ui,int32_t x,int32_t y,
                                         uint32_t scale,uint16_t color,const char *text,
                                         const resona_bitmap_font *f)
{
    if(!ui||!ui->rect||!ui->text||!f||!text||!scale||scale>8u)return -1;
    int32_t width=resona_font_text_advance(f,scale,text);if(width<0)return -1;
    uint32_t bytes=0,at=0;int64_t cx=x,cy=y;
    while(text[bytes])bytes++;
    while(at<bytes){
        uint32_t used,cp=resona_utf8_decode((const uint8_t*)text+at,bytes-at,&used);at+=used;
        if(cp=='\n'){cx=x;cy+=(int64_t)f->line_height*scale;continue;}
        const resona_font_glyph *g=resona_font_find(f,cp);
        if(!g){char fallback[2]={(char)(cp>=32u&&cp<127u?cp:'?'),0};
            int64_t yy=cy+(f->baseline>7u?(int64_t)(f->baseline-7u)*scale:0);
            if(cx<ui->width&&cx+5u*scale>0&&yy<ui->height&&yy+7u*scale>0)
                ui->text(ui->ctx,(int32_t)cx,(int32_t)yy,scale,color,fallback);
            cx+=6u*scale;continue;
        }
        if(!resona_font_glyph_valid(f,g))return -1;
        uint32_t stride=((uint32_t)g->width+7u)/8u;
        for(uint32_t row=0;row<g->height;row++){
            const uint8_t *bits=f->bitmap+g->offset+row*stride;uint32_t col=0;
            while(col<g->width){
                while(col<g->width&&!(bits[col/8]&(0x80u>>(col%8))))col++;
                uint32_t begin=col;
                while(col<g->width&&(bits[col/8]&(0x80u>>(col%8))))col++;
                if(col>begin){int64_t xx=cx+((int64_t)g->x_offset+begin)*scale,yy=cy+((int64_t)g->y_offset+row)*scale;
                    int64_t right=xx+(col-begin)*scale,bottom=yy+scale;
                    if(xx<0)xx=0;if(yy<0)yy=0;
                    if(right>ui->width)right=ui->width;if(bottom>ui->height)bottom=ui->height;
                    if(right>xx&&bottom>yy)
                        ui->rect(ui->ctx,(int32_t)xx,(int32_t)yy,(int32_t)(right-xx),(int32_t)(bottom-yy),color);
                }
            }
        }
        cx+=(int64_t)g->advance*scale;
    }
    return width;
}
#endif
