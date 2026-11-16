// GPL-3.0. Edit these three names. The platform assigns the PID; Lab uses CDC.
// Installation may take one additional reboot when the declared profile changes.
#include <resona/startup_helpers.h>
#include <resona/ui.h>
namespace {
const resona_usb_names names={"Code Synth","Code Synth Audio","Code Synth MIDI"};
const resona_platform*P;
struct State{uint32_t dirty;};
int init(void*state,const resona_platform*p){
    P=p;int result=resona_startup_usb_apply(p,&names);
    if(result<0)return result;
    if(result==RESONA_STARTUP_REBOOT_REQUIRED){p->log("USB names saved; restarting once");p->reboot();}
    ((State*)state)->dirty=1;return 0;
}
void event(void*,const resona_app_event*){}
void frame(void*state,uint32_t){State*s=(State*)state;if(!s->dirty)return;uint16_t*fb;uint32_t stride;const resona_ui*u=P->screen_begin(&fb,&stride);if(!u)return;u->clear(u->ctx,RESONA_RGB(8,18,24));u->text(u->ctx,16,20,2,RESONA_RGB(0,230,210),"USB NAMES IN CODE");u->text(u->ctx,16,62,1,0xffff,names.product);u->text(u->ctx,16,88,1,0xffff,names.audio);u->text(u->ctx,16,114,1,0xffff,names.midi);P->screen_present(0,0,0,0);s->dirty=0;}
const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,sizeof(State),0,init,event,frame};
}
extern "C" const resona_app*resona_app_get(){return &app;}
