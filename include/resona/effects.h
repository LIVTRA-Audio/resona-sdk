/* App-owned effect instances and their graph. GPL-3.0 licensed. */
#ifndef RESONA_EFFECTS_H
#define RESONA_EFFECTS_H
#include <stdint.h>
#include <stddef.h>

#define RESONA_EFFECTS_VERSION 1u
#define RESONA_EFFECT_FACTORY 1u
#define RESONA_EFFECT_PACK 2u
#define RESONA_EFFECT_AUTO_MODEL 0xffffffffu
#define RESONA_EFFECT_GUITAR 0xffff0000u
#define RESONA_EFFECT_USB 0xffff0001u
#define RESONA_EFFECT_BLUETOOTH 0xffff0002u
#define RESONA_EFFECT_MAIN 0xffff0003u /* post-EQ device mix, capture only */
#define RESONA_EFFECT_OUTPUT 0xffff0010u
#define RESONA_EFFECT_CAPTURE 0xffff0011u
/* Bounds control-call validation work, rather than the number of DSP slots. */
#define RESONA_EFFECT_MAX_ROUTES 1024u
#define RESONA_EFFECT_NAME_BYTES 60u

typedef uint32_t resona_effect_handle;
/* Pack identity is resolved in the installed public asset pool. Version may be
 * empty to select the unique matching ID. crc32=0 accepts that resolved version.
 * Factory engine values are the existing RESONA_ENGINE_* constants.
 * Pack errors distinguish missing ID, version/CRC mismatch and ambiguity;
 * malformed/empty IDs are ARG, actual storage failures remain STORAGE.
 * Selection order is ID, then optional version, then optional full-RPK CRC. */
typedef struct resona_effect_source {
    uint32_t version, size, kind, engine, model, variant, crc32;
    char id[RESONA_EFFECT_NAME_BYTES+1];
    char package_version[RESONA_EFFECT_NAME_BYTES+1];
    uint8_t reserved[2];
} resona_effect_source;

typedef struct resona_effect_route {
    uint32_t source, destination; /* handles, or the named bus constants above */
    float gain;
    uint8_t source_channel, destination_channel;
    uint16_t reserved;
} resona_effect_route;

typedef struct resona_effect_graph {
    uint32_t version, size, route_count;
    const resona_effect_route *routes; /* copied before activation */
} resona_effect_graph;

/* CPU reserved_ticks fields are zero on the runtime-only1.02 OS. */
typedef struct resona_effect_info {
    uint32_t version, size, handle, kind, entry, engine, model;
    uint32_t enabled, fault, calls, last_ticks, peak_ticks, reserved_ticks;
    uint32_t code_bytes, fast_bytes, psram_bytes, param_count;
    char id[61], name[61], package_version[61];
    uint8_t reserved;
} resona_effect_info;

typedef struct resona_effect_param {
    char name[16], unit[8];
    float min, max, def, step; /* immutable metadata defaults, not current values */
    uint32_t flags;
} resona_effect_param;

typedef struct resona_effects_status {
    uint32_t version, size, owned, active, generation, instances, routes;
    uint32_t last_ticks, peak_ticks, reserved_ticks, faults;
} resona_effects_status;

static inline int resona_effect_index(uint32_t handle,const uint32_t *handles,uint32_t count) {
    for(uint32_t i=0;i<count;i++)if(handles[i]==handle)return (int)i;
    return -1;
}

/* Pure control-side validation/topological ordering, also used by the host
 * model. order and incoming each hold count words; no DSP or allocation here.
 * Effect instances on this target are mono; buses retain explicit channels. */
static inline int resona_effect_graph_order(const resona_effect_graph *g,
        const uint32_t *handles,uint32_t count,uint32_t *order,uint32_t *incoming) {
    if(!g || g->version!=RESONA_EFFECTS_VERSION || g->size!=sizeof(*g) ||
       g->route_count>RESONA_EFFECT_MAX_ROUTES || (g->route_count&&!g->routes) ||
       (count&&(!handles||!order||!incoming)))return -1;
    for(uint32_t i=0;i<count;i++){
        if(!handles[i] || handles[i]>=0xffff0000u)return -1;
        for(uint32_t j=0;j<i;j++)if(handles[j]==handles[i])return -1;
        incoming[i]=0;
    }
    for(uint32_t i=0;i<g->route_count;i++){
        const resona_effect_route *r=&g->routes[i];
        if(r->reserved || !(r->gain>=-32767.0f&&r->gain<=32767.0f))return -1;
        int s=resona_effect_index(r->source,handles,count),d=resona_effect_index(r->destination,handles,count);
        if(s<0){
            if(r->source==RESONA_EFFECT_GUITAR){if(r->source_channel)return -1;}
            else if(r->source==RESONA_EFFECT_USB || r->source==RESONA_EFFECT_BLUETOOTH || r->source==RESONA_EFFECT_MAIN){if(r->source_channel>1)return -1;}
            else return -1;
        }else if(r->source_channel)return -1;
        if(d<0){
            if((r->destination!=RESONA_EFFECT_OUTPUT&&r->destination!=RESONA_EFFECT_CAPTURE)||r->destination_channel>1)return -1;
        }else if(r->destination_channel)return -1;
        if(r->source==RESONA_EFFECT_MAIN && r->destination!=RESONA_EFFECT_CAPTURE)return -1;
        for(uint32_t j=0;j<i;j++){
            const resona_effect_route *a=&g->routes[j];
            if(a->source==r->source && a->destination==r->destination &&
               a->source_channel==r->source_channel && a->destination_channel==r->destination_channel)return -1;
        }
        if(s>=0&&d>=0)incoming[d]++;
    }
    for(uint32_t k=0;k<count;k++){
        uint32_t node;
        for(node=0;node<count;node++)if(incoming[node]==0)break;
        if(node==count)return -1;
        order[k]=node;incoming[node]=0xffffffffu;
        for(uint32_t e=0;e<g->route_count;e++)if(g->routes[e].source==handles[node]){
            int d=resona_effect_index(g->routes[e].destination,handles,count);
            if(d>=0){if(incoming[d]==0xffffffffu)return -1;incoming[d]--;}
        }
    }
    return 0;
}

static inline void resona_effect_source_init(resona_effect_source *s,uint32_t kind) {
    uint8_t *p=(uint8_t*)s;
    for(size_t i=0;i<sizeof(*s);i++)p[i]=0;
    s->version=RESONA_EFFECTS_VERSION;s->size=sizeof(*s);s->kind=kind;
    s->model=RESONA_EFFECT_AUTO_MODEL;
}
#endif
