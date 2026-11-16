// RGB Studio: protected status-light control, without touching sound or presets.
#include <resona/platform.h>
static const resona_platform *P;
struct State {resona_rgb_config color;uint32_t selected;int result;};
static const char *names[]={"RED","GREEN","BLUE","BRIGHTNESS","ANIMATION","PERIOD MS"};
static const char *modes[]={"SOLID","BLINK","BREATHE","RAINBOW","FADE"};
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static void apply(State*s){s->color.period_ms=s->color.animation ? (s->color.period_ms?s->color.period_ms:1200):0;s->result=P->rgb_set(&s->color);}
static int init(void *state,const resona_platform *p){
    if(p->version!=RESONA_PLATFORM_VERSION || p->size<offsetof(resona_platform,rgb_get)+sizeof(p->rgb_get) || !p->rgb_set || !p->rgb_get)return RESONA_ERR_UNSUPPORTED;
    P=p;State*s=(State*)state;s->color=resona_rgb_solid(255,80,20,128);s->color.blue2=255;apply(s);P->log("RGB Studio ready; USB MIDI CC16..21 controls colors/brightness/mode/period");return s->result;
}
static void event(void *state,const resona_app_event*e){State*s=(State*)state;
    if(e->kind==RESONA_APP_EV_INPUT){const resona_event*i=&e->u.input;
        if(i->input==RESONA_INPUT_SELECTOR){if(i->kind==RESONA_EVENT_TURN)s->selected=(uint32_t)clamp((int)s->selected+i->value,0,5);else if(i->kind==RESONA_EVENT_CLICK)s->selected=(s->selected+1)%6;return;}
        if(i->kind==RESONA_EVENT_LONG){s->result=P->rgb_set(NULL);return;}
        if(i->kind==RESONA_EVENT_CLICK){s->color.animation=(s->color.animation+1)%5;apply(s);return;}
        if(i->kind!=RESONA_EVENT_TURN)return;
        int d=i->value;
        if(s->selected<4){uint8_t *v=&s->color.red+s->selected;*v=(uint8_t)clamp(*v+d,0,255);}
        else if(s->selected==4)s->color.animation=(uint32_t)clamp((int)s->color.animation+d,0,4);
        else {if(!s->color.animation)s->color.animation=RESONA_RGB_BLINK;s->color.period_ms=(uint32_t)clamp((int)s->color.period_ms+d*100,20,60000);}
        apply(s);
    }else if(e->kind==RESONA_APP_EV_MIDI && e->u.midi.len==3){const uint8_t*d=e->u.midi.data;
        if((d[0]&0xf0)!=0xb0 || d[1]<16 || d[1]>22)return;
        if(d[1]==22){s->result=P->rgb_set(NULL);return;}
        uint32_t cc=d[1]-16,v=d[2];
        if(cc<4)(&s->color.red)[cc]=(uint8_t)((v*255u+63u)/127u);
        else if(cc==4)s->color.animation=v%5u;
        else {if(!s->color.animation)s->color.animation=RESONA_RGB_BLINK;s->color.period_ms=20+v*100;}
        apply(s);
    }
}
static void frame(void *state,uint32_t ms){(void)ms;State*s=(State*)state;resona_rgb_state light={0};P->rgb_get(&light);
    uint16_t *pixels;uint32_t stride;resona_ui*u=P->screen_begin(&pixels,&stride);if(!u)return;
    u->clear(u->ctx,RESONA_RGB(10,16,18));u->text(u->ctx,12,9,2,RESONA_RGB(240,240,230),"RGB STUDIO");
    u->rect(u->ctx,234,40,70,86,RESONA_RGB(light.red,light.green,light.blue));
    for(unsigned i=0;i<6;i++){char line[48];uint32_t v=i<4?(&s->color.red)[i]:s->color.period_ms;
        if(i==4)P->format(line,sizeof(line),"%s %s",names[i],modes[s->color.animation]);else P->format(line,sizeof(line),"%s %u",names[i],v);
        u->text(u->ctx,12,42+(int)i*16,1,i==s->selected?RESONA_RGB(255,122,47):RESONA_RGB(150,176,180),line);
    }
    u->text(u->ctx,12,148,1,RESONA_RGB(150,176,180),s->result?"LIGHT OVERRIDE OR ERROR":"SELECT: TOP / EDIT: BOTTOM");P->screen_present(0,0,320,172);
}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,sizeof(State),0,init,event,frame};
extern "C" RESONA_EXPORT const resona_app*resona_app_get(void){return &app;}
