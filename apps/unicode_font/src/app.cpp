// GPL-3.0. Font assets belong to this App; no LVGL or firmware dependency.
#include <resona/platform.h>
#include <resona/ui.h>
#include "../assets/demo_font.h"
static const resona_platform*P;
static int init(void*,const resona_platform*p){P=p;return resona_font_valid(&kDemoFont)?0:-1;}
static void event(void*,const resona_app_event*){}
static void frame(void*,uint32_t){
 uint16_t*pixels;uint32_t stride;resona_ui*u=P->screen_begin(&pixels,&stride);if(!u)return;
 u->clear(u->ctx,RESONA_RGB(12,18,24));
 u->text(u->ctx,16,12,2,RESONA_RGB(210,220,230),"APP-OWNED FONT");
 resona_ui_text_font(u,16,54,3,RESONA_RGB(60,230,200),"音量 50%",&kDemoFont);
 resona_ui_dots(u,264,76,24,24,0,360,8,8,3,RESONA_RGB(255,200,100));
 u->text(u->ctx,16,126,1,RESONA_RGB(150,160,170),"FONT IN APP / NO FLASH WRITE");
 P->screen_present(0,0,320,172);
}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,8192,init,event,frame};
RESONA_EXPORT const resona_app*resona_app_get(){return &app;}
