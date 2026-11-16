/* GPL-3.0. Resona DSP tools v1: portable semantics, optional protected acceleration. */
#ifndef RESONA_DSP_H
#define RESONA_DSP_H
#include <stdint.h>
#include <stddef.h>
#define RESONA_TOOLS_VERSION 1u
#define RESONA_DSP_DOT_Q15 1u
#define RESONA_DSP_RING_Q15 2u
#define RESONA_DSP_CONVERT 4u
#define RESONA_PROFILE_REGIONS 8u
#define RESONA_DSP_MAX_FRAMES 256u
/* A two-tap feedback ring. coefficient is0..32767. Each output is
 * floor(coefficient*(current+previous)/65536); original current becomes previous.
 * The ring is updated in place; index is advanced. Ring/output must not overlap. */
typedef struct resona_dsp_ring {
    int16_t *data;
    uint32_t length, index;
    int32_t coefficient;
} resona_dsp_ring;
/* A DSP job for the platform's optional accelerated kernels. */
typedef struct resona_dsp_job {
    uint32_t version, struct_size, operation, count;
    const void *a, *b;
    void *out;
    uint32_t voices;
} resona_dsp_job;
typedef struct resona_tools {
    uint32_t version, struct_size, capabilities;
    volatile uint32_t profiling_enabled;
    int (*run)(const resona_dsp_job *);
    int (*profile_begin)(uint32_t id,const char *name);
    int (*profile_end)(uint32_t id);
} resona_tools;
/* Defined semantics: arithmetic floor, including negative values. */
static inline int32_t resona_dsp_floor16(int32_t x) {
    return x>=0?x/65536:-(int32_t)(((uint32_t)(-(int64_t)x)+65535u)/65536u);
}
static inline int resona_dsp_run(const resona_tools *t,uint32_t op,uint32_t n,const void *a,const void *b,void *out,uint32_t voices) {
    resona_dsp_job j={1,sizeof(resona_dsp_job),op,n,a,b,out,voices};
    return t->run(&j);
}
static inline int resona_dsp_dot_q15(const resona_tools *t,const int16_t *a,const int16_t *b,uint32_t n,int64_t *out) {
    if(!a||!b||!out||!n||n>4096u)return -1;
    if(t && (t->capabilities&RESONA_DSP_DOT_Q15))return resona_dsp_run(t,1,n,a,b,out,0);
    int64_t sum=0;for(uint32_t i=0;i<n;i++)sum+=(int32_t)a[i]*(int32_t)b[i];*out=sum;return 0;
}
/* Up to8 independent rings, frame-major output: out[frame*voices+voice]. */
static inline int resona_dsp_ring_q15(const resona_tools *t,resona_dsp_ring *rings,uint32_t voices,int16_t *out,uint32_t n) {
    if(!rings||!out||!voices||voices>8u||!n||n>256u)return -1;
    for(uint32_t k=0;k<voices;k++)if(!rings[k].data||!rings[k].length||rings[k].length>16384u||rings[k].index>=rings[k].length||rings[k].coefficient<0||rings[k].coefficient>32767)return -1;
    if(t && (t->capabilities&RESONA_DSP_RING_Q15))return resona_dsp_run(t,2,n,rings,0,out,voices);
    for(uint32_t k=0;k<voices;k++) {
        resona_dsp_ring *r=&rings[k];uint32_t p=r->index;
        int16_t prior=r->data[p?p-1:r->length-1];
        for(uint32_t i=0;i<n;i++) {int16_t in=r->data[p];int16_t v=(int16_t)resona_dsp_floor16(r->coefficient*((int32_t)in+prior));out[i*voices+k]=v;r->data[p]=v;prior=in;if(++p==r->length)p=0;}
        r->index=p;
    }
    return 0;
}
static inline int resona_dsp_f32_to_q15(const resona_tools *t,const float *a,int16_t *out,uint32_t n) {
    if(!a||!out||!n||n>256u)return -1;
    if(t && (t->capabilities&RESONA_DSP_CONVERT))return resona_dsp_run(t,3,n,a,0,out,0);
    for(uint32_t i=0;i<n;i++){float v=a[i];if(v!=v)v=0;if(v>=0.999969482421875f)v=0.999969482421875f;if(v< -1)v=-1;out[i]=(int16_t)(v*32768.0f);}return 0;
}
static inline int resona_dsp_q15_to_f32(const resona_tools *t,const int16_t *a,float *out,uint32_t n) {
    if(!a||!out||!n||n>256u)return -1;
    if(t && (t->capabilities&RESONA_DSP_CONVERT))return resona_dsp_run(t,4,n,a,0,out,0);
    for(uint32_t i=0;i<n;i++){out[i]=(float)a[i]*(1.0f/32768.0f);}
    return 0;
}
/* Profiling is opt-in from Lab/CLI, process() only. Times are elapsed time and include
 * platform and interrupt overhead. IDs0..7, nonempty names up to23 ASCII bytes. */
static inline int resona_profile_begin(const resona_tools*t,uint32_t id,const char*name) {
    return t&&t->profiling_enabled&&id<8u?t->profile_begin(id,name):0;
}
static inline int resona_profile_end(const resona_tools*t,uint32_t id) {
    return t&&t->profiling_enabled&&id<8u?t->profile_end(id):0;
}
#endif
