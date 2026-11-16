// GPL-3.0. Persistent startup settings are separate from presets and Box App storage.
// No flash writes during init. USB MIDI CC80 installs names for the next boot;
// CC81 installs an animated splash; CC82 restores the factory splash;
// CC83 restores factory names. CC84 reboots. Each requires value127.
#include <resona/platform.h>
#include <resona/ui.h>
#include <resona/libc.h>
#include <resona/splash.h>
namespace {
const resona_platform*P;
struct State{resona_startup_info info;int result;uint32_t dirty;};
static void word(uint8_t*d,uint32_t v){resona_splash_put(d,v);}
static int install_splash(){uint8_t data[56]={};memcpy(data,"RSP1",4);word(data+4,sizeof data);word(data+12,5000);word(data+16,320);word(data+20,172);word(data+24,2);
 for(unsigned i=0;i<2;i++){uint8_t*f=data+32+i*12;word(f,400);word(f+4,1);f[8]=0;f[9]=215;uint16_t color=i?RESONA_RGB(0,120,100):RESONA_RGB(8,18,24);f[10]=color;f[11]=color>>8;}word(data+8,resona_splash_crc32(data+32,24));return P->startup_splash_set(data,sizeof data);}
int init(void*memory,const resona_platform*p){P=p;State*s=(State*)memory;if(p->version!=RESONA_PLATFORM_VERSION||p->size<offsetof(resona_platform,startup_splash_set)+sizeof p->startup_splash_set)return RESONA_ERR_UNSUPPORTED;s->result=P->startup_info(&s->info);s->dirty=1;return 0;}
void event(void*memory,const resona_app_event*e){State*s=(State*)memory;if(e->kind!=RESONA_APP_EV_MIDI||e->u.midi.port!=RESONA_MIDI_USB||e->u.midi.len!=3)return;const uint8_t*d=e->u.midi.data;if((d[0]&0xf0)!=0xb0||d[2]!=127)return;
 if(d[1]==80){resona_usb_names names={"Startup Studio","Studio Audio","Studio MIDI"};s->result=P->startup_usb_set(&names);}
 else if(d[1]==81)s->result=install_splash();else if(d[1]==82)s->result=P->startup_splash_set(NULL,0);else if(d[1]==83)s->result=P->startup_usb_set(NULL);else if(d[1]==84)P->reboot();else return;
 P->startup_info(&s->info);s->dirty=1;P->log(s->result?"startup change refused":"startup settings saved; reboot to apply");}
void frame(void*memory,uint32_t){State*s=(State*)memory;if(!s->dirty)return;uint16_t*px;uint32_t stride;const resona_ui*u=P->screen_begin(&px,&stride);if(!u)return;u->clear(u->ctx,RESONA_RGB(8,18,24));u->text(u->ctx,16,16,2,RESONA_RGB(0,230,210),"STARTUP STUDIO");u->text(u->ctx,16,52,1,0xffff,s->info.names.product);char line[48];P->format(line,sizeof line,"SPLASH %u MS  RESULT %d",(unsigned)s->info.splash_duration_ms,s->result);u->text(u->ctx,16,78,1,0xffff,line);u->text(u->ctx,16,110,1,0xffff,"USB MIDI CC80-84, VALUE127");P->screen_present(0,0,0,0);s->dirty=0;}
const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,sizeof(State),0,init,event,frame};
}
extern "C" const resona_app*resona_app_get(){return &app;}
