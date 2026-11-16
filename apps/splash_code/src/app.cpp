// GPL-3.0. Edit the duration, colors and drawing code below, then Build/Install.
#include <resona/startup_helpers.h>
#include <resona/ui.h>
namespace {
const uint32_t duration_ms=4000;
const uint32_t frame_ms=200;
const uint16_t background=RESONA_RGB(8,18,24);
const uint16_t highlight=RESONA_RGB(0,230,210);
uint8_t animation[4096];
const resona_platform*P;
struct State{uint32_t dirty;};
int make_animation(){
    resona_splash_builder b;int rc=resona_splash_begin(&b,animation,sizeof animation,duration_ms);if(rc)return rc;
    for(unsigned frame=0;frame<4;frame++){
        rc=resona_splash_frame(&b,frame_ms);if(rc)return rc;
        const unsigned x=24+64*frame;
        for(unsigned y=0;y<172;y++){
            if(y<70||y>=102){rc=resona_splash_run(&b,320,background);if(rc)return rc;}
            else{
                rc=resona_splash_run(&b,x,background);if(rc)return rc;
                rc=resona_splash_run(&b,56,highlight);if(rc)return rc;
                rc=resona_splash_run(&b,320-x-56,background);if(rc)return rc;
            }
        }
    }
    return resona_splash_finish(&b);
}
int init(void*state,const resona_platform*p){
    P=p;int bytes=make_animation();if(bytes<0)return bytes;
    int result=resona_startup_splash_apply(p,animation,(uint32_t)bytes);
    if(result<0)return result;
    if(result==RESONA_STARTUP_REBOOT_REQUIRED){p->log("Splash changed; restarting once");p->reboot();}
    ((State*)state)->dirty=1;return 0;
}
void event(void*,const resona_app_event*){}
void frame(void*state,uint32_t){State*s=(State*)state;if(!s->dirty)return;uint16_t*fb;uint32_t stride;const resona_ui*u=P->screen_begin(&fb,&stride);if(!u)return;u->clear(u->ctx,background);u->text(u->ctx,16,24,2,highlight,"SPLASH IN CODE");u->text(u->ctx,16,76,1,0xffff,"EDIT FRAMES IN SRC/APP.CPP");P->screen_present(0,0,0,0);s->dirty=0;}
const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,sizeof(State),0,init,event,frame};
}
extern "C" const resona_app*resona_app_get(){return &app;}
