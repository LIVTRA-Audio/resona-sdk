/* GPL-3.0. Control model test; hardware DSP/resource tests are separate. */
#include <resona/effects.hpp>
static const resona_platform *P;
static resona::Effects fx;
static int init(void*,const resona_platform*p) {
    P=p;if(fx.begin(p))return -10;
    /* While the App owns audio, presets cannot load or change the effects. */
    if(p->program_submit(RESONA_PROGRAM_LOAD,0,nullptr,0)!=RESONA_ERR_STATE||p->program_effect_enable(0,1)==0)return -24;
    uint32_t h[10];for(unsigned i=0;i<10;i++){h[i]=fx.factory(RESONA_ENGINE_GATE);if(!h[i])return -11;}
    if(fx.connect(fx.Usb,h[0]))return -12;
    for(unsigned i=1;i<10;i++)if(fx.connect(h[i-1],h[i]))return -13;
    if(fx.output(h[9])||fx.capture(h[9])||fx.commit())return -14;
    resona_effects_status before{};if(fx.status(&before)||before.instances!=10||!before.active)return -15;
    if(fx.destroy(h[4])!=RESONA_ERR_STATE)return -16;
    fx.clear_error();if(fx.param(h[0],0,0.2f)||fx.param(h[1],0,0.8f))return -17;
    if(fx.param(h[0],256,0.5f)!=RESONA_ERR_ARG)return -18;
    fx.clear_error();fx.connect(h[9],h[0]);if(fx.commit()!=RESONA_ERR_ARG)return -19;
    resona_effects_status after{};if(fx.status(&after)||after.generation!=before.generation||after.routes!=before.routes)return -20;
    fx.clear_routes();fx.connect(fx.Usb,h[0],1,0);fx.connect(fx.Usb,h[1],1,1);fx.output(h[0],0.25f);fx.output(h[1],0.75f);if(fx.commit())return -21;
    if(fx.destroy(h[9]))return -22;
    resona_effect_param param{};if(fx.param_info(h[0],0,&param)||param.max!=1)return -23;
    P->log("effect graph control contract: passed");return 0;
}
static void event(void*,const resona_app_event*){}
static void frame(void*,uint32_t) {resona_ui*u=P->screen_begin(nullptr,nullptr);if(u){u->clear(u->ctx,0);u->text(u->ctx,8,8,1,0xffff,"Effects contract passed");}}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,65536,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(){return &app;}
