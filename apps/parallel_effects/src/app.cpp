/* GPL-3.0. Two independent factory branches, mixed by an App through its effect graph. */
#include <resona/effects.hpp>
static const resona_platform *P;
static resona::Effects fx;
static uint32_t gate,compressor;
static unsigned wet=50;
static int result;
static int connect_graph() {
    fx.clear_routes();
    fx.connect(fx.Guitar,gate);fx.connect(fx.Guitar,compressor);
    fx.output(gate,1.0f-wet*0.01f);fx.output(compressor,wet*0.01f);
    fx.connect(fx.Main,fx.Capture,1,0,0);fx.connect(fx.Main,fx.Capture,1,1,1);
    return fx.commit();
}
static int init(void*,const resona_platform*p) {
    P=p;if(fx.begin(p))return fx.error();
    gate=fx.factory(RESONA_ENGINE_GATE);compressor=fx.factory(RESONA_ENGINE_COMP);
    if(!gate||!compressor)return fx.error();
    result=connect_graph();if(!result)P->log("Parallel Effects: two independent factory branches running");return result;
}
static void event(void*,const resona_app_event*e) {
    if(e->kind==RESONA_APP_EV_INPUT&&e->u.input.kind==RESONA_EVENT_TURN){
        int next=(int)wet+e->u.input.value;wet=next<0?0:next>100?100:(unsigned)next;result=connect_graph();
    }
}
static void frame(void*,uint32_t) {
    resona_ui*u=P->screen_begin(nullptr,nullptr);if(!u)return;u->clear(u->ctx,0);
    u->text(u->ctx,12,16,2,0xffff,"Parallel Effects");char line[64];
    P->format(line,sizeof line,"Gate %u%% / Compressor %u%%",100-wet,wet);u->text(u->ctx,12,52,1,0xffff,line);
    P->format(line,sizeof line,"Graph status %d",result);u->text(u->ctx,12,78,1,0xffff,line);
    P->screen_present(0,0,320,172);
}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,8192,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(){return &app;}
