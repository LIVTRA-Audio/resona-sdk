/* GPL-3.0. No persistent EQ writes. */
#include "../contract.h"
static int init(void*s,const resona_platform*p){(void)s;return resona_format_eq_contract(p);}
static void event(void*s,const resona_app_event*e){(void)s;(void)e;}
static void frame(void*s,uint32_t t){(void)s;(void)t;}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,8192,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(void){return &app;}
