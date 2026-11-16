/* GPL-3.0. Public scalar IO/allocation contract; no firmware or real storage access. */
#include <resona/platform.h>
#include <resona/libc.h>
static int init(void *state,const resona_platform *p) {
 (void)state;unsigned failed=0;char line[128];uint8_t bytes[8]={0xf0,0x7d,0x01,0,0,0,0,0xf7};
 const uint32_t lengths[]={0,RESONA_APP_MIDI_SEND_MAX_BYTES+1,UINT32_MAX};
 for(unsigned i=0;i<3;i++){int rc=p->midi_send(RESONA_MIDI_USB,bytes,lengths[i]);if(rc!=RESONA_ERR_ARG)failed++;p->format(line,sizeof line,"IO MIDI invalid_len=%u rc=%d",lengths[i],rc);p->log(line);}
 if(p->midi_send(RESONA_MIDI_UART+1,bytes,8)!=RESONA_ERR_ARG)failed++;
 for(unsigned align=1;align<=RESONA_APP_ALLOC_MAX_ALIGN;align*=2){void*b=p->alloc(257,align);int ok=b&&!((uintptr_t)b&(align-1));if(!ok)failed++;if(b){memset(b,0xA7,257);p->free(b);}p->format(line,sizeof line,"IO ALIGN %u ok=%d",align,ok);p->log(line);}
 if(p->alloc(1,3)||p->alloc(1,RESONA_APP_ALLOC_MAX_ALIGN*2))failed++;
 p->format(line,sizeof line,"IO SUMMARY failed=%u",failed);p->log(line);return failed?RESONA_ERR_STATE:0;
}
static void event(void*s,const resona_app_event*e){(void)s;(void)e;}
static void frame(void*s,uint32_t t){(void)s;(void)t;}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,32768,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(void){return &app;}
