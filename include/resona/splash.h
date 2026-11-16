/* GPL-3.0. Bounded RSP1 animation builder and validation; no firmware or drivers. */
#ifndef RESONA_SPLASH_H
#define RESONA_SPLASH_H
#include "resona/platform.h"
static inline uint32_t resona_splash_word(const uint8_t*d){return (uint32_t)d[0]|(uint32_t)d[1]<<8|(uint32_t)d[2]<<16|(uint32_t)d[3]<<24;}
static inline void resona_splash_put(uint8_t*d,uint32_t v){for(unsigned i=0;i<4;i++)d[i]=(uint8_t)(v>>(8*i));}
static inline uint32_t resona_splash_crc32(const void*data,uint32_t n){const uint8_t*d=(const uint8_t*)data;uint32_t c=~0u;while(n--){c^=*d++;for(unsigned i=0;i<8;i++)c=(c>>1)^(0xedb88320u&-(c&1u));}return ~c;}
static inline int resona_splash_valid(const void*data,uint32_t n){
    const uint8_t*d=(const uint8_t*)data;
    if(!d||n<32||n>RESONA_SPLASH_MAX_BYTES||d[0]!='R'||d[1]!='S'||d[2]!='P'||d[3]!='1'||resona_splash_word(d+4)!=n||resona_splash_word(d+8)!=resona_splash_crc32(d+32,n-32)||resona_splash_word(d+12)>0x7fffffffu||resona_splash_word(d+16)!=320||resona_splash_word(d+20)!=172||!resona_splash_word(d+24)||resona_splash_word(d+24)>128||resona_splash_word(d+28))return 0;
    uint32_t off=32;for(uint32_t f=0;f<resona_splash_word(d+24);f++){
        if(n-off<8)return 0;uint32_t hold=resona_splash_word(d+off),runs=resona_splash_word(d+off+4);off+=8;
        if(hold<16||hold>60000||!runs||runs>(n-off)/4)return 0;
        uint32_t pixels=0;while(runs--){uint32_t count=d[off]|(uint32_t)d[off+1]<<8;off+=4;if(!count||count>55040-pixels)return 0;pixels+=count;}if(pixels!=55040)return 0;
    }return off==n;
}
/* A frame is a row-major stream of exactly320*172 RGB565 pixels.
 * Append runs (count,color), or generate pixels with your own drawing code and
 * group adjacent equal pixels. No allocation, automatic fallback or device write.
 */
typedef struct resona_splash_builder{uint8_t*data;uint32_t capacity,used,frames,frame_offset,pixels;}resona_splash_builder;
static inline int resona_splash_begin(resona_splash_builder*b,void*data,uint32_t capacity,uint32_t duration_ms){
    if(!b)return RESONA_ERR_ARG;b->data=0;b->capacity=b->used=b->frames=b->frame_offset=b->pixels=0;
    if(!data||capacity<32||duration_ms>0x7fffffffu)return RESONA_ERR_ARG;
    b->data=(uint8_t*)data;b->capacity=capacity>RESONA_SPLASH_MAX_BYTES?RESONA_SPLASH_MAX_BYTES:capacity;b->used=32;
    for(unsigned i=0;i<32;i++)b->data[i]=0;
    b->data[0]='R';b->data[1]='S';b->data[2]='P';b->data[3]='1';resona_splash_put(b->data+12,duration_ms);resona_splash_put(b->data+16,320);resona_splash_put(b->data+20,172);return RESONA_OK;
}
static inline int resona_splash_frame(resona_splash_builder*b,uint32_t hold_ms){
    if(!b||!b->data||hold_ms<16||hold_ms>60000)return RESONA_ERR_ARG;
    if(b->frames&&b->pixels!=55040)return RESONA_ERR_STATE;
    if(b->frames>=128)return RESONA_ERR_FULL;
    if(b->capacity-b->used<8)return RESONA_ERR_NO_MEMORY;
    b->frame_offset=b->used;b->used+=8;b->frames++;b->pixels=0;resona_splash_put(b->data+b->frame_offset,hold_ms);resona_splash_put(b->data+b->frame_offset+4,0);return RESONA_OK;
}
static inline int resona_splash_run(resona_splash_builder*b,uint32_t count,uint16_t color){
    if(!b||!b->data||!b->frames||!count||count>55040-b->pixels)return RESONA_ERR_ARG;
    if(b->capacity-b->used<4)return RESONA_ERR_NO_MEMORY;
    uint8_t*d=b->data+b->used;d[0]=(uint8_t)count;d[1]=(uint8_t)(count>>8);d[2]=(uint8_t)color;d[3]=(uint8_t)(color>>8);b->used+=4;b->pixels+=count;
    uint8_t*r=b->data+b->frame_offset+4;resona_splash_put(r,resona_splash_word(r)+1);return RESONA_OK;
}
static inline int resona_splash_finish(resona_splash_builder*b){
    if(!b||!b->data||!b->frames||b->pixels!=55040)return RESONA_ERR_STATE;
    resona_splash_put(b->data+4,b->used);resona_splash_put(b->data+24,b->frames);resona_splash_put(b->data+8,resona_splash_crc32(b->data+32,b->used-32));return (int)b->used;
}
#endif
