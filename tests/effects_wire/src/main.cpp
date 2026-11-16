/* GPL-3.0. Minimal stateful linear gain for hardware graph acceptance. */
#include <resona/module.h>
struct State {float gain;uint32_t frames;};
static void init(void*s,const resona_host*h){(void)h;*(State*)s={1,0};}
static void reset(void*s){((State*)s)->frames=0;}
static void param(void*s,uint32_t i,float v){if(!i)((State*)s)->gain=v;}
static void process(void*s,const float*const*in,float*const*out,uint32_t frames){
    State*state=(State*)s;state->frames+=frames;
    for(uint32_t i=0;i<frames;i++)out[0][i]=in[0][i]*state->gain;
}
static const resona_param parameters[]={{"GAIN",0,2,1,""}};
static const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),"org.resona.test.effect-wire","Effect Wire Fixture","0.2.0",
    RESONA_LAYOUT_MONO,RESONA_RATE_48000,sizeof(State),1,parameters,0,init,reset,param,process,0,0,0,0,0,0,0};
RESONA_EXPORT const resona_module *resona_module_get(void){return &module;}
