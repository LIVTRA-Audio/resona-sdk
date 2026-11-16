/* GPL-3.0. Deliberate hardware containment fixture; never use as a normal effect.
 * Values0.375/0.625/0.875 trigger access/deadline/non-finite faults respectively.
 * Endpoint/default qualification values remain ordinary unity processing. */
#include <resona/module.h>
static void init(void*s,const resona_host*h){(void)h;*(float*)s=1;}
static void reset(void*){}
static void param(void*s,uint32_t i,float v){if(!i)*(float*)s=v;}
static void process(void*s,const float*const*in,float*const*out,uint32_t frames){
    float mode=*(float*)s;
    if(mode==0.375f)*(volatile uint32_t*)0=1;
    if(mode==0.625f)for(;;){__asm__ volatile("" ::: "memory");}
    for(uint32_t i=0;i<frames;i++)out[0][i]=mode==0.875f?__builtin_nanf(""):in[0][i];
}
static const resona_param parameters[]={{"FAULT",0,2,1,""}};
static const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),"org.resona.test.effect-fault","Effect Fault Fixture","0.1.0",
    RESONA_LAYOUT_MONO,RESONA_RATE_48000,4,1,parameters,0,init,reset,param,process,0,0,0,0,0,0,0};
RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
