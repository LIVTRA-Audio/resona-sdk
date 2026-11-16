// GPL-3.0. Run scalar/native equivalence checks at init, then profile a long dot product.
#include <resona/module.h>
namespace {
struct State {const resona_tools *tools;int16_t a[4096],b[4096];int64_t sum;float native;uint32_t passed;};
const resona_param params[]={{"NATIVE",0,1,1,""},{"CHECK",0,1,1,""}};
void init(void *p,const resona_host *h){
 auto &s=*(State*)p;s.tools=resona_host_tools(h);s.native=1;s.passed=1;
 uint32_t seed=123;
 for(uint32_t i=0;i<4096;i++){seed=seed*1664525u+1013904223u;s.a[i]=(int16_t)(seed>>16);seed=seed*1664525u+1013904223u;s.b[i]=(int16_t)(seed>>16);}
 s.a[0]=s.a[1]=s.b[0]=s.b[1]=-32768;
 int64_t expected=0,actual=0;
 for(uint32_t n=1;n<=4096;n=n<8?n+1: n==4096?4097:4096){
  resona_dsp_dot_q15(nullptr,s.a,s.b,n,&expected);resona_dsp_dot_q15(s.tools,s.a,s.b,n,&actual);if(actual!=expected)s.passed=0;
 }
 int16_t ref[17],run[17],outA[256],outB[256];
 for(uint32_t i=0;i<17;i++)ref[i]=run[i]=s.a[i];
 resona_dsp_ring ra={ref,17,16,32686},rb={run,17,16,32686};
 resona_dsp_ring_q15(nullptr,&ra,1,outA,256);resona_dsp_ring_q15(s.tools,&rb,1,outB,256);
 for(uint32_t i=0;i<256;i++)if(outA[i]!=outB[i])s.passed=0;
 if(ra.index!=rb.index)s.passed=0;
 if(h->log)h->log(s.passed?"DSP Tools self-test PASS":"DSP Tools self-test FAIL");
}
void reset(void*){}
void set(void*p,uint32_t i,float v){if(i==0)((State*)p)->native=v;}
float get(const void*p,uint32_t i){return i==0?((const State*)p)->native:(float)((const State*)p)->passed;}
void process(void*p,const float*const*in,float*const*out,uint32_t n){auto&s=*(State*)p;
 resona_profile_begin(s.tools,0,"dot-4096");
 resona_dsp_dot_q15(s.native>=0.5f?s.tools:nullptr,s.a,s.b,4096,&s.sum);
 resona_profile_end(s.tools,0);
 for(uint32_t i=0;i<n;i++)out[0][i]=in[0][i];
}
const resona_module module={1,sizeof(resona_module),"org.resona.example.dsp-tools","DSP Tools","0.1.0",1,2,sizeof(State),2,params,nullptr,init,reset,set,process,nullptr,nullptr,get,nullptr,nullptr};
}
extern "C" RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
