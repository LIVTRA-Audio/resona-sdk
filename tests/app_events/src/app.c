/* GPL-3.0. No device storage writes or transport calls. */
#include "../contract.h"
static int init(void *state,const resona_platform*p){(void)state;event_contract_begin(p);return 0;}
static void event(void *state,const resona_app_event*e){(void)state;event_contract_event(e);}
static void frame(void *state,uint32_t ms){(void)state;(void)ms;event_contract_frame();}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,8192,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(void){return &app;}
