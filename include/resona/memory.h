/* GPL-3.0 licensed. Declarative, load-time DSP buffer placement, version 1. */
#ifndef RESONA_MEMORY_H
#define RESONA_MEMORY_H
#include <stdint.h>
#include <stddef.h>
#define RESONA_MEMORY_VERSION 1u
#define RESONA_MEMORY_FAST 1u
#define RESONA_MEMORY_PSRAM 2u
#define RESONA_MEMORY_MAX_BUFFERS 16u
/* Sanity bounds, not reservations: admission uses the device's available heap. */
#define RESONA_MEMORY_FAST_MAX 1048576u
#define RESONA_MEMORY_PSRAM_MAX 4194304u
typedef struct resona_buffer_request {
    uint32_t id, region, bytes, alignment;
} resona_buffer_request;
typedef struct resona_memory_profile {
    uint32_t version, struct_size, count;
    const resona_buffer_request *requests;
} resona_memory_profile;
typedef struct resona_buffer {
    uint32_t id, region, bytes;
    void *data;
} resona_buffer;
typedef struct resona_memory_context {
    uint32_t version, struct_size, count;
    const resona_buffer *buffers;
} resona_memory_context;
/* Validate and compute independent arena offsets. Each arena starts aligned to
 * 128 bytes. No fallback between regions, no allocation on the audio thread.
 * Returns 0 on success; all outputs are undefined on failure. */
static inline int resona_memory_layout(const resona_buffer_request *r, uint32_t n,
    uint32_t offsets[RESONA_MEMORY_MAX_BUFFERS], uint32_t totals[2])
{
    uint32_t i,j; totals[0]=totals[1]=0;
    if(n>RESONA_MEMORY_MAX_BUFFERS || (n && !r)) return -1;
    for(i=0;i<n;i++) {
        uint32_t a=r[i].alignment,k=r[i].region-1u,t;
        if(!r[i].id || k>1u || !r[i].bytes || a<16u || a>128u || (a&(a-1u))) return -1;
        for(j=0;j<i;j++) if(r[j].id==r[i].id) return -1;
        if(totals[k]>UINT32_MAX-(a-1u)) return -1;
        t=(totals[k]+a-1u)&~(a-1u);
        if(r[i].bytes>UINT32_MAX-t) return -1;
        offsets[i]=t; totals[k]=t+r[i].bytes;
        if(totals[k]>(k?RESONA_MEMORY_PSRAM_MAX:RESONA_MEMORY_FAST_MAX)) return -1;
    }
    return 0;
}
/* Combined state/buffer declaration check against physical sanity bounds only.
 * Live free heaps, reserves, runtime stacks and fragmentation decide admission. */
static inline int resona_memory_requirements(uint32_t state_bytes,uint32_t state_region,
    const resona_buffer_request *r,uint32_t n,uint32_t totals[2])
{
    uint32_t offsets[RESONA_MEMORY_MAX_BUFFERS],k,t;
    if(!state_region)state_region=RESONA_MEMORY_FAST;
    if(state_region>RESONA_MEMORY_PSRAM || resona_memory_layout(r,n,offsets,totals))return -1;
    k=state_region-1U;
    if(state_bytes>UINT32_MAX-127U)return -1;
    t=(state_bytes+127U)&~127U;
    if(totals[k]>UINT32_MAX-t)return -1;
    totals[k]+=t;
    return totals[k]>(k?RESONA_MEMORY_PSRAM_MAX:RESONA_MEMORY_FAST_MAX)?-1:0;
}
static inline const resona_buffer *resona_buffer_find(const resona_memory_context *m,uint32_t id)
{
    uint32_t i;
    if(!m || m->version!=RESONA_MEMORY_VERSION || m->struct_size<sizeof(*m) || m->count>RESONA_MEMORY_MAX_BUFFERS) return NULL;
    for(i=0;i<m->count;i++) if(m->buffers[i].id==id) return &m->buffers[i];
    return NULL;
}
#endif
