// GPL-3.0. Edit these names, build/install, then disconnect ALL device power.
#include <resona/bluetooth_helpers.h>
#include <resona/ui.h>
namespace {
const resona_bluetooth_names names={"Resona Studio Audio","Resona Studio MIDI"};
const resona_platform*P;
struct State{int result;uint32_t dirty;};
void apply(State*s){s->result=resona_bluetooth_apply(P,&names);s->dirty=1;
    if(s->result==1)P->log("Bluetooth names saved. Disconnect USB-C and 9 V power, then reconnect.");
    else if(s->result<0)P->log("Bluetooth name request unavailable; enable Bluetooth and click to retry.");
}
int init(void*state,const resona_platform*p){P=p;apply((State*)state);return 0;}
void event(void*state,const resona_app_event*e){if(e->kind==RESONA_APP_EV_INPUT&&e->u.input.kind==RESONA_EVENT_CLICK)apply((State*)state);}
void frame(void*state,uint32_t){State*s=(State*)state;if(!s->dirty)return;const resona_ui*u=P->screen_begin(nullptr,nullptr);if(!u)return;
    u->clear(u->ctx,RESONA_RGB(8,18,24));u->text(u->ctx,12,14,2,RESONA_RGB(0,230,210),"BLUETOOTH NAMES");
    u->text(u->ctx,12,52,1,0xffff,names.audio);u->text(u->ctx,12,76,1,0xffff,names.midi);
    const char*status=s->result==1?"Reconnect ALL power to apply":s->result==0?"Names active":"Enable Bluetooth; click to retry";
    u->text(u->ctx,12,118,1,0xffff,status);P->screen_present(0,0,0,0);s->dirty=0;
}
const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,sizeof(State),0,init,event,frame};
}
extern "C" const resona_app*resona_app_get(){return &app;}
