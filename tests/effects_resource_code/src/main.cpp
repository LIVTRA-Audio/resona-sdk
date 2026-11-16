/* GPL-3.0. Deliberate resource-admission boundary fixture. */
#include <resona/module.h>
static const volatile unsigned char padding[65536]={0};
static void init(void*,const resona_host*){}
static void reset(void*){}
static void param(void*,uint32_t,float){}
static void process(void*,const float*const*in,float*const*out,uint32_t frames){for(uint32_t i=0;i<frames;i++)out[0][i]=in[0][i]+padding[0];}
static const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),"org.resona.test.effect-resource-code","Resource Code Fixture","0.1.0",RESONA_LAYOUT_MONO,RESONA_RATE_48000,4,0,0,0,init,reset,param,process,0,0,0,0,0,0,0};
RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
