// GPL-3.0 licensed. Large state is explicit; there is no automatic memory fallback.
#include <resona/module.h>
namespace {
constexpr uint32_t samples=131072;
struct State {float history[samples];uint32_t cursor;float mix;};
const resona_param params[]={{"MIX",0,1,0.5f,""}};
void init(void *p,const resona_host*){auto &s=*static_cast<State*>(p);s.cursor=0;s.mix=0.5f;}
void reset(void *p){auto &s=*static_cast<State*>(p);for(uint32_t i=0;i<samples;++i)s.history[i]=0;s.cursor=0;}
void set(void *p,uint32_t n,float v){if(n==0)static_cast<State*>(p)->mix=v;}
float get(const void *p,uint32_t n){return n==0?static_cast<const State*>(p)->mix:0;}
void process(void *p,const float *const *in,float *const *out,uint32_t frames){
 auto &s=*static_cast<State*>(p);
 for(uint32_t i=0;i<frames;++i){float dry=in[0][i],wet=s.history[s.cursor];s.history[s.cursor]=dry;s.cursor=(s.cursor+1)&(samples-1);out[0][i]=dry*(1-s.mix)+wet*s.mix;}
}
const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),
 "org.resona.example.shared-state","Shared State Delay","0.1.0",
 RESONA_LAYOUT_MONO,RESONA_RATE_48000,sizeof(State),1,params,nullptr,
 init,reset,set,process,nullptr,nullptr,get,nullptr,nullptr,RESONA_MEMORY_PSRAM};
}
extern "C" RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
