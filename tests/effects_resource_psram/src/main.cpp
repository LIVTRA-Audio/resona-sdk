/* GPL-3.0. Deliberate resource-admission boundary fixture. */
#include <resona/module.h>
static const resona_buffer_request request={1,2,2752512,128};
static const resona_memory_profile memory={1,sizeof(memory),1,&request};
static void init(void*,const resona_host*){}
static void reset(void*){}
static void param(void*,uint32_t,float){}
static void process(void*,const float*const*in,float*const*out,uint32_t frames){for(uint32_t i=0;i<frames;i++)out[0][i]=in[0][i];}
static const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),"org.resona.test.effect-resource-psram","Resource Psram Fixture","0.1.0",RESONA_LAYOUT_MONO,RESONA_RATE_48000,4,0,0,0,init,reset,param,process,0,0,0,0,&memory,0,0};
RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
