// GPL-3.0 licensed. Code and immutable lookup data share the chosen RX image.
#include <resona/module.h>
namespace {
// Change only this region to RESONA_CODE_SRAM or RESONA_CODE_FAST to compare.
const resona_code_profile code={RESONA_CODE_VERSION,sizeof(resona_code_profile),RESONA_CODE_PSRAM};
// A deliberately large lookup image makes placement visible (>128 KiB).
// Volatile reads retain the whole immutable table during optimization.
const uint8_t lookup[132*1024]={1};
struct State {uint32_t cursor;float level;};
const resona_param params[]={{"LEVEL",0,1,0.5f,""}};
void init(void *p,const resona_host*){auto &s=*static_cast<State*>(p);s.cursor=0;s.level=0.5f;}
void reset(void *p){static_cast<State*>(p)->cursor=0;}
void set(void *p,uint32_t n,float v){if(n==0)static_cast<State*>(p)->level=v;}
float get(const void *p,uint32_t n){return n==0?static_cast<const State*>(p)->level:0;}
void process(void *p,const float *const *in,float *const *out,uint32_t frames){
 auto &s=*static_cast<State*>(p);const volatile uint8_t *table=lookup;
 for(uint32_t i=0;i<frames;++i){
   float trim=1.0f-static_cast<float>(table[s.cursor])/32768.0f;
   s.cursor=(s.cursor+1)%sizeof(lookup);out[0][i]=in[0][i]*s.level*trim;
 }
}
const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),
 "org.resona.example.flexible-code","Flexible Code","0.1.0",
 RESONA_LAYOUT_MONO,RESONA_RATE_48000,sizeof(State),1,params,nullptr,
 init,reset,set,process,nullptr,nullptr,get,nullptr,nullptr,RESONA_MEMORY_FAST,&code};
}
extern "C" RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
