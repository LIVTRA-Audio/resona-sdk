/* GPL-3.0. Control-only preview model. No factory DSP or device CPU qualification. */
#include <resona/factory_params.h>
typedef struct preview_effect {
    struct preview_effect *next;
    resona_effect_info info;
    float values[RESONA_MAX_PARAMS];
    uint32_t variant;
} preview_effect;
static preview_effect *effect_nodes;
static resona_effects_status effect_status;
static resona_effect_route *effect_routes;
static uint32_t effect_next=1;
static void effects_reset(void) {
    preview_effect_owned=0;
    while(effect_nodes){preview_effect*n=effect_nodes;effect_nodes=n->next;free(n);}
    free(effect_routes);effect_routes=NULL;memset(&effect_status,0,sizeof effect_status);
    effect_status.version=1;effect_status.size=sizeof effect_status;effect_next=1;
}
static preview_effect *effect_find(uint32_t h){for(preview_effect*n=effect_nodes;n;n=n->next)if(n->info.handle==h)return n;return NULL;}
static int effects_begin(void) {
    if(effect_status.owned)return RESONA_ERR_STATE;
    preview_effect_owned=1;effect_status.owned=1;effect_status.generation++;return 0;
}
static int effect_create(const resona_effect_source*s,uint32_t*out) {
    if(!s||!out)return RESONA_ERR_ARG;*out=0;
    if(!effect_status.owned)return RESONA_ERR_STATE;
    if(s->version!=1||s->size!=sizeof *s||s->reserved[0]||s->reserved[1]||!memchr(s->id,0,sizeof s->id)||!memchr(s->package_version,0,sizeof s->package_version))return RESONA_ERR_ARG;
    if(s->kind==RESONA_EFFECT_PACK){if(!s->id[0])return RESONA_ERR_ARG;return unavailable();}
    if(s->kind!=RESONA_EFFECT_FACTORY||s->engine<1||s->engine>5||s->variant>(s->engine==1?1u:0u)||(s->model!=RESONA_EFFECT_AUTO_MODEL&&s->model>=30))return RESONA_ERR_ARG;
    if(effect_next>=0xffff0000u)return RESONA_ERR_FULL;
    preview_effect*n=calloc(1,sizeof *n);if(!n)return RESONA_ERR_NO_MEMORY;
    resona_effect_info*i=&n->info;i->version=1;i->size=sizeof *i;i->handle=effect_next++;i->kind=s->kind;i->engine=s->engine;
    n->variant=s->variant;
    i->enabled=1;i->model=s->engine<3?255u:s->model;i->param_count=s->engine==RESONA_ENGINE_AMP?5U:resona_factory_param_count(s->engine,s->variant);
    strcpy(i->name,names[s->engine]);strcpy(i->package_version,"preview");
    for(uint32_t k=0;k<i->param_count;k++){resona_effect_param param;resona_factory_param_info(i->engine,n->variant,k,&param);n->values[k]=param.def;}
    n->next=effect_nodes;effect_nodes=n;*out=i->handle;effect_status.instances++;effect_status.generation++;return 0;
}
static int effect_destroy(uint32_t h) {
    preview_effect*n=effect_find(h);if(!n)return RESONA_ERR_ARG;
    for(uint32_t i=0;i<effect_status.routes;i++)if(effect_routes[i].source==h||effect_routes[i].destination==h)return RESONA_ERR_STATE;
    preview_effect**p=&effect_nodes;while(*p!=n)p=&(*p)->next;*p=n->next;free(n);effect_status.instances--;effect_status.generation++;return 0;
}
static int effect_param_set(uint32_t h,uint32_t index,float value) {
    preview_effect*n=effect_find(h);if(!n||index>=n->info.param_count||!(value>=0&&value<=1))return RESONA_ERR_ARG;
    /* The Box keeps built-in values in 1/100 steps (the Cabinet's LEVEL in 1/48). */
    uint32_t steps=n->info.engine==RESONA_ENGINE_CAB&&index==2?48u:100u;
    n->values[index]=(float)(uint32_t)(value*steps+.5f)/(float)steps;return 0;
}
static int effect_enable(uint32_t h,uint32_t on){preview_effect*n=effect_find(h);if(!n||on>1)return RESONA_ERR_ARG;n->info.enabled=on;return 0;}
static int effects_graph_set(const resona_effect_graph*g) {
    if(!effect_status.owned)return RESONA_ERR_STATE;
    uint32_t count=effect_status.instances,*work=calloc(count?count*3u:1u,sizeof(uint32_t));if(!work)return RESONA_ERR_NO_MEMORY;
    uint32_t k=0;for(preview_effect*n=effect_nodes;n;n=n->next)work[k++]=n->info.handle;
    int rc=resona_effect_graph_order(g,work,count,work+count,work+2u*count);free(work);if(rc)return RESONA_ERR_ARG;
    resona_effect_route*next=NULL;
    if(g->route_count){next=malloc(g->route_count*sizeof *next);if(!next)return RESONA_ERR_NO_MEMORY;memcpy(next,g->routes,g->route_count*sizeof *next);}
    free(effect_routes);effect_routes=next;effect_status.routes=g->route_count;effect_status.active=1;effect_status.generation++;return 0;
}
static int effect_info(uint32_t h,resona_effect_info*out){preview_effect*n=effect_find(h);if(!n||!out)return RESONA_ERR_ARG;*out=n->info;return 0;}
static int effect_param_info(uint32_t h,uint32_t index,resona_effect_param*out) {
    preview_effect*n=effect_find(h);if(!n||!out||index>=n->info.param_count)return RESONA_ERR_ARG;
    return resona_factory_param_info(n->info.engine,n->variant,index,out);
}
static int effects_status(resona_effects_status*out){if(!out)return RESONA_ERR_ARG;*out=effect_status;return 0;}
