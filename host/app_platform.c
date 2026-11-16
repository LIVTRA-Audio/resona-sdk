#include "resona/splash.h"
/* GPL-3.0. Desktop-only platform model. Never opens USB or real device storage. */
#include <resona/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static const resona_app *app;
static void *state;
static resona_ui *screen;
static uint16_t *pixels;
static uint32_t now_ms, frame_elapsed, led;
static const uint8_t led_colors[6][3]={{0,0,0},{0,255,255},{255,255,0},{0,255,0},{255,0,255},{255,0,0}};
static resona_rgb_state rgb;
static uint32_t rgb_last;
static int active=-1;
static uint8_t store[4096];
static int32_t settings[10];
static uint32_t eq_on;
static int8_t eq[5], eq_level;
static char logs[8192];
static uint32_t log_len, unsupported;
static struct {uint32_t id, period, next, repeat, used;} timers[8];
static struct { resona_app_event ev; uint8_t data[RESONA_APP_MIDI_EVENT_MAX_BYTES]; } queue[32];
static unsigned queued;
static struct {void *base,*ptr; uint32_t bytes,bank;} heap[RESONA_APP_MAX_ALLOCATIONS];
static struct {void*base;uint8_t*data;uint32_t budget,used,reserved;} banks[RESONA_APP_MEMORY_BANKS];
static uint32_t power2(uint32_t n){uint32_t s=128;while(s<n)s<<=1;return s;}
static int memory_reserve(uint32_t bank,uint32_t bytes){
    if(!bank||bank>=RESONA_APP_MEMORY_BANKS)return RESONA_ERR_ARG;
    if(banks[bank].used)return RESONA_ERR_STATE;
    if(banks[bank].base){if(bytes==banks[bank].budget)return 0;if(bytes)return RESONA_ERR_STATE;free(banks[bank].base);memset(&banks[bank],0,sizeof banks[bank]);return 0;}
    if(!bytes)return 0;
    if(bytes>(bank==RESONA_APP_MEMORY_FAST?512U*1024U:128U*1024U))return RESONA_ERR_NO_MEMORY;
    uint32_t span=bank==RESONA_APP_MEMORY_FAST?power2(bytes):(bytes+127U)&~127U,align=bank==RESONA_APP_MEMORY_FAST?span:128U;
    void*base=malloc((size_t)span+align);if(!base)return RESONA_ERR_NO_MEMORY;
    banks[bank].base=base;banks[bank].data=(uint8_t*)(((uintptr_t)base+align-1U)&~((uintptr_t)align-1U));
    banks[bank].budget=bytes;banks[bank].reserved=span;memset(banks[bank].data,0,span);return 0;
}
static int memory_info(uint32_t bank,resona_app_memory_info*out){
    if(bank>=RESONA_APP_MEMORY_BANKS||!out)return RESONA_ERR_ARG;
    /* Preview models App ownership/quotas; it does not estimate live board RAM. */
    *out=(resona_app_memory_info){1,sizeof(*out),bank,bank?banks[bank].budget:app->heap_bytes,banks[bank].used,bank?banks[bank].reserved:app->heap_bytes,0,0};return 0;
}
static uint32_t preview_effect_owned;
static const char *names[]={"Empty","Gate","Compressor","Resona","NAM","Cabinet"};
static void log_line(const char *s) {
    if(!s) return;
    size_t n=strlen(s); if(n>1024)n=1024;
    if(log_len+n+2>=sizeof logs) {log_len=0;}
    memcpy(logs+log_len,s,n);log_len+=(uint32_t)n;logs[log_len++]='\n';logs[log_len]=0;
}
static int unavailable(void) {unsupported++;return RESONA_ERR_UNSUPPORTED;}
#include "effects_model.inc"
static void drain(void) {
    for(unsigned n=0;queued && n<256;n++) {
        resona_app_event ev=queue[0].ev;
        uint8_t data[RESONA_APP_MIDI_EVENT_MAX_BYTES];
        if(ev.kind==RESONA_APP_EV_MIDI){memcpy(data,queue[0].data,ev.u.midi.len);ev.u.midi.data=data;}
        else if(ev.kind==RESONA_APP_EV_CTRL){memcpy(data,queue[0].data,ev.u.ctrl.len);ev.u.ctrl.data=data;}
        if(app->event)app->event(state,&ev);
        memmove(queue,queue+1,(--queued)*sizeof queue[0]);
    }
    if(queued){queued=0;log_line("Preview event recursion limit reached");}
}
static void device_info(resona_device_info *o) {
    memset(o,0,sizeof *o);strcpy(o->product,"Resona Box preview");strcpy(o->family,"resona-box");
    strcpy(o->platform_version,"preview API 1");strcpy(o->serial,"LOCAL-PREVIEW");
    o->sample_rate=48000;o->block_frames=settings[9];o->screen_width=320;o->screen_height=172;o->encoders=2;o->developer=1;
}
static uint32_t millis(void){return now_ms;}
static void reboot(void){log_line("Preview: reboot requested; use Restart preview");}
/* An out-of-range argument to these calls ends the App on the Box; the preview stops it too. */
static int app_stopped;
static void stop_app(const char*why){if(app_stopped)return;app_stopped=1;char line[128];snprintf(line,sizeof line,"Preview: App stopped: %s (the Box ends an App for this)",why);log_line(line);}
static void sleep_ms(uint32_t ms){if(ms>50){stop_app("sleep_ms over 50 ms");return;}log_line("Preview: sleep is not scheduled; use timers");unsupported++;}
static int timer_start(uint32_t id,uint32_t period,uint32_t repeat){
    if(!period||period>RESONA_APP_TIMER_MAX_PERIOD_MS)return RESONA_ERR_ARG;
    for(int i=0;i<8;i++)if(timers[i].used&&timers[i].id==id){timers[i].used=0;break;}
    for(int i=0;i<8;i++)if(!timers[i].used){timers[i].id=id;timers[i].period=period;timers[i].next=now_ms+period;timers[i].repeat=repeat;timers[i].used=1;return 0;}
    return RESONA_ERR_NO_MEMORY;
}
static void timer_stop(uint32_t id){for(int i=0;i<8;i++)if(timers[i].id==id)timers[i].used=0;}
static void *memory_alloc(uint32_t bank,uint32_t bytes,uint32_t align){
    if(bank>=RESONA_APP_MEMORY_BANKS||!bytes||!align||(align&(align-1))||align>RESONA_APP_ALLOC_MAX_ALIGN)return NULL;
    uint32_t budget=bank?banks[bank].budget:app->heap_bytes;
    if(banks[bank].used>budget||bytes>budget-banks[bank].used)return NULL;
    unsigned slot=RESONA_APP_MAX_ALLOCATIONS;
    for(unsigned i=0;i<RESONA_APP_MAX_ALLOCATIONS;i++)if(!heap[i].ptr){slot=i;break;}
    if(slot==RESONA_APP_MAX_ALLOCATIONS)return NULL;
    void*base=NULL;uint8_t*p=NULL;
    if(bank){
        uint32_t offset=0;
        for(;;){
            if(offset>budget)return NULL;
            uint32_t pad=(uint32_t)(-(uintptr_t)(banks[bank].data+offset)&(align-1U));
            if(pad>budget-offset)return NULL;offset+=pad;if(bytes>budget-offset)return NULL;
            p=banks[bank].data+offset;int moved=0;
            for(unsigned i=0;i<RESONA_APP_MAX_ALLOCATIONS;i++)if(heap[i].ptr&&heap[i].bank==bank && p<(uint8_t*)heap[i].ptr+heap[i].bytes && p+bytes>(uint8_t*)heap[i].ptr){offset=(uint32_t)((uint8_t*)heap[i].ptr-banks[bank].data)+heap[i].bytes;moved=1;break;}
            if(!moved)break;
        }
    }else{base=malloc((size_t)bytes+align);if(!base)return NULL;p=(uint8_t*)(((uintptr_t)base+align-1)&~((uintptr_t)align-1));}
    heap[slot].base=base;heap[slot].ptr=p;heap[slot].bytes=bytes;heap[slot].bank=bank;
    banks[bank].used+=bytes;memset(p,0,bytes);return p;
}
static void*allocate(uint32_t bytes,uint32_t align){return memory_alloc(0,bytes,align);}
static void release(void*p){if(!p)return;for(unsigned i=0;i<RESONA_APP_MAX_ALLOCATIONS;i++)if(heap[i].ptr==p){memset(p,0,heap[i].bytes);free(heap[i].base);banks[heap[i].bank].used-=heap[i].bytes;memset(&heap[i],0,sizeof heap[i]);return;}}
static void memory_reset(void){
    for(unsigned i=0;i<RESONA_APP_MAX_ALLOCATIONS;i++)free(heap[i].base);
    for(unsigned bank=1;bank<RESONA_APP_MEMORY_BANKS;bank++)free(banks[bank].base);
    memset(heap,0,sizeof heap);memset(banks,0,sizeof banks);
}
static void post(const resona_app_event *ev){
    if(!ev||queued>=32)return;
    const uint8_t *data=NULL;uint32_t n=0;
    if(ev->kind==RESONA_APP_EV_MIDI){data=ev->u.midi.data;n=ev->u.midi.len;}
    else if(ev->kind==RESONA_APP_EV_CTRL){data=ev->u.ctrl.data;n=ev->u.ctrl.len;}
    if(n>RESONA_APP_MIDI_EVENT_MAX_BYTES||(n&&!data))return;
    queue[queued].ev=*ev;queue[queued].ev.time_ms=now_ms;
    if(n)memcpy(queue[queued].data,data,n);
    queued++;
}
static resona_ui *screen_begin(uint16_t **px,uint32_t *stride){if(px)*px=pixels;if(stride)*stride=320;return screen;}
static void present(int32_t x,int32_t y,int32_t w,int32_t h){(void)x;(void)y;(void)w;(void)h;}
static void brightness(uint32_t p){if(p>100)stop_app("screen_brightness over 100");}
static int block_draw(uint32_t role,uint32_t force){(void)role;(void)force;return RESONA_ERR_UNSUPPORTED;}
static int block_event(uint32_t role,const resona_event *ev){(void)role;(void)ev;return RESONA_ERR_UNSUPPORTED;}
static void input_config(uint32_t input,uint32_t flags){if(input>=2||(flags&~3u))stop_app("input_config input or flags out of range");}
static void rgb_tick(void){
    uint32_t delta=now_ms-rgb_last;
    if(rgb.owner && rgb.config.animation && delta>=16){uint64_t phase=(uint64_t)rgb.phase_ms+delta;rgb_last=now_ms;rgb.phase_ms=rgb.config.animation==RESONA_RGB_FADE?(phase>=rgb.config.period_ms?rgb.config.period_ms:(uint32_t)phase):(uint32_t)(phase%rgb.config.period_ms);}
    uint8_t out[3];resona_rgb_evaluate(&rgb.config,rgb.phase_ms,out);rgb.red=out[0];rgb.green=out[1];rgb.blue=out[2];rgb.flags=rgb.owner?RESONA_RGB_CUSTOM:0;
    if(rgb.owner && rgb.config.animation)rgb.flags|=rgb.config.animation==RESONA_RGB_FADE && rgb.phase_ms>=rgb.config.period_ms?RESONA_RGB_FINISHED:RESONA_RGB_ANIMATING;
}
static int rgb_set(const resona_rgb_config*c){if(c && !resona_rgb_valid(c))return RESONA_ERR_ARG;rgb.config=c?*c:resona_rgb_solid(led_colors[led][0],led_colors[led][1],led_colors[led][2],255);rgb.phase_ms=0;rgb_last=now_ms;rgb.owner=c?RESONA_RGB_OWNER_APP:0;rgb_tick();return 0;}
static int rgb_get(resona_rgb_state*s){if(!s)return RESONA_ERR_ARG;rgb_tick();*s=rgb;return 0;}
static void led_set(uint32_t mode){if(mode>RESONA_LED_BYPASS){stop_app("led_set mode out of range");return;}led=mode;rgb_set(NULL);}

static int rename_entry(uint32_t i,const char*n){(void)i;(void)n;return RESONA_ERR_ARG;}
static int color_entry(uint32_t i,const uint8_t*c){(void)i;(void)c;return RESONA_ERR_ARG;}
static int delete_entry(uint32_t i){(void)i;return RESONA_ERR_ARG;}
static void storage_info(resona_storage_info*o){memset(o,0,sizeof *o);o->pool_bytes=o->pool_free=272*1024;}
static int setting_get(uint32_t i,int32_t *v){if(!v||i<1||i>9)return RESONA_ERR_ARG;*v=settings[i];return 0;}
static int setting_set(uint32_t i,int32_t v){
    if(i<1||i>9)return RESONA_ERR_ARG;
    int min=i==3?-20:0,max=i==3?20:i==8?16:(i==2||i==4||i==6)?1:100;
    if(i==9){if(v!=64)return RESONA_ERR_ARG;}else if(v<min||v>max)return RESONA_ERR_ARG;
    settings[i]=v;return 0;
}
static int eq_set(uint32_t on,const int8_t*b,int8_t level,uint32_t persist){if(!resona_global_eq_valid(on,b,level,persist))return RESONA_ERR_ARG;eq_on=on;memcpy(eq,b,5);eq_level=level;return 0;}
static int eq_get(uint32_t*on,int8_t*b,int8_t*l){if(!on||!b||!l)return RESONA_ERR_ARG;*on=eq_on;memcpy(b,eq,5);*l=eq_level;return 0;}
static void meters(uint8_t*i,uint8_t*o,uint8_t*c){if(i)*i=0;if(o)*o=0;if(c)*c=0;}
static int midi_send(uint32_t port,const uint8_t*data,uint32_t n){if(!data||!n||n>RESONA_APP_MIDI_SEND_MAX_BYTES||port>RESONA_MIDI_UART)return RESONA_ERR_ARG;return unavailable();}
static int ctrl_send(const uint8_t*data,uint32_t n){(void)data;if(n>64){stop_app("ctrl_send over 64 bytes");return RESONA_ERR_ARG;}return unavailable();}
static int format(char*out,uint32_t cap,const char*fmt,...){va_list ap;va_start(ap,fmt);int n=vsnprintf(out,cap,fmt,ap);va_end(ap);return n;}
static uint32_t store_size(void){return sizeof store;}
static int store_read(uint32_t at,void*out,uint32_t len){if(!out||!resona_app_store_range_valid(sizeof store,at,len))return RESONA_ERR_ARG;if(!len)return RESONA_OK;memcpy(out,store+at,len);return 0;}
static int store_write(uint32_t at,const void*src,uint32_t len){if(!src||!resona_app_store_range_valid(sizeof store,at,len))return RESONA_ERR_ARG;if(!len)return RESONA_OK;memcpy(store+at,src,len);return 0;}
static resona_bluetooth_info bluetooth;
static int bluetooth_names_get(resona_bluetooth_info*out){if(!out)return RESONA_ERR_ARG;*out=bluetooth;return 0;}
static int bluetooth_names_set(const resona_bluetooth_names*n){
    const resona_bluetooth_names factory={"Resona Box Audio","Resona Box Midi"};if(!n)n=&factory;
    if(!resona_bluetooth_names_valid(n))return RESONA_ERR_ARG;
    memset(&bluetooth.saved,0,sizeof(bluetooth.saved));strcpy(bluetooth.saved.audio,n->audio);strcpy(bluetooth.saved.midi,n->midi);
    bluetooth.power_cycle_required=strcmp(bluetooth.active.audio,n->audio)||strcmp(bluetooth.active.midi,n->midi);return 0;
}
static resona_startup_info startup;
static int startup_info(resona_startup_info*out){if(!out)return RESONA_ERR_ARG;*out=startup;return 0;}
static int startup_usb_set(const resona_usb_names*n){
    if(n){if(!resona_usb_names_valid(n))return RESONA_ERR_ARG;startup.names=*n;startup.next_pid=0x1400;}
    else{memset(&startup.names,0,sizeof startup.names);strcpy(startup.names.product,"Resona Box");strcpy(startup.names.audio,"Resona Box");strcpy(startup.names.midi,"Resona Box");startup.next_pid=0x1307;}return 0;
}
static int startup_splash_set(const void*data,uint32_t n){
 if(!n&&!data){startup.splash_bytes=startup.splash_crc32=startup.splash_duration_ms=0;return 0;}
 if(!resona_splash_valid(data,n))return RESONA_ERR_ARG;
 startup.splash_bytes=n;startup.splash_crc32=resona_splash_crc32(data,n);startup.splash_duration_ms=resona_splash_word((const uint8_t*)data+12);return 0;
}
#include "program_model.inc"

static resona_platform platform;
RESONA_EXPORT int resona_preview_init(resona_ui *ui,uint16_t *fb){
    effects_reset();memset(&effect_hooks,0,sizeof effect_hooks);
    app=resona_app_get();if(!app||app->platform_version!=RESONA_PLATFORM_VERSION||app->struct_size!=sizeof *app||!app->init||!app->frame||app->state_size>64*1024||app->heap_bytes>512*1024)return RESONA_ERR_ARG;
    screen=ui;pixels=fb;
    memory_reset();
    free(state);state=calloc(1,app->state_size?app->state_size:1);if(!state)return RESONA_ERR_NO_MEMORY;
    memset(timers,0,sizeof timers);queued=0;now_ms=0;frame_elapsed=0;log_len=0;logs[0]=0;unsupported=0;app_stopped=0;
    memset(&platform,0,sizeof platform);platform.version=RESONA_PLATFORM_VERSION;platform.size=sizeof platform;
#define SET(field,fn) platform.field=fn
    SET(device_info,device_info);SET(millis,millis);SET(log,log_line);SET(reboot,reboot);SET(reserved_0,unavailable);
    SET(sleep_ms,sleep_ms);SET(timer_start,timer_start);SET(timer_stop,timer_stop);SET(alloc,allocate);SET(free,release);SET(post,post);SET(format,format);
    SET(memory_reserve,memory_reserve);SET(memory_alloc,memory_alloc);SET(memory_info,memory_info);
    SET(screen_begin,screen_begin);SET(screen_present,present);SET(screen_brightness,brightness);
    SET(input_config,input_config);SET(led_set,led_set);SET(rgb_set,rgb_set);SET(rgb_get,rgb_get);
    SET(setting_get,setting_get);SET(setting_set,setting_set);SET(global_eq_set,eq_set);SET(global_eq_get,eq_get);SET(meters,meters);
    SET(midi_send,midi_send);SET(ctrl_send,ctrl_send);SET(app_store_size,store_size);SET(app_store_read,store_read);SET(app_store_write,store_write);
    SET(entry_count,entry_count);SET(entry_info,entry_info);SET(entry_rename,rename_entry);SET(entry_color,color_entry);SET(entry_delete,delete_entry);SET(storage_info,storage_info);
    memset(&startup,0,sizeof startup);startup.version=1;startup.size=sizeof startup;startup.current_pid=0x1307;startup_usb_set(NULL);
    memset(&bluetooth,0,sizeof bluetooth);bluetooth.version=1;bluetooth.size=sizeof bluetooth;bluetooth_names_set(NULL);bluetooth.active=bluetooth.saved;bluetooth.power_cycle_required=0;
    SET(bluetooth_names_get,bluetooth_names_get);SET(bluetooth_names_set,bluetooth_names_set);
    SET(startup_info,startup_info);SET(startup_usb_set,startup_usb_set);SET(startup_splash_set,startup_splash_set);
    memset(&rgb,0,sizeof rgb);led=RESONA_LED_HOVER;rgb.version=1;rgb.size=sizeof rgb;rgb.pwm_hz=0;rgb.config=resona_rgb_solid(0,255,255,255);rgb_last=0;rgb_tick();
    SET(preset_info,program_preset_info);SET(program_info,program_info);SET(program_submit,program_submit);SET(program_edit,program_edit);
    SET(program_job,program_job);SET(program_enable,program_enable);SET(program_map_encoder,program_map);SET(program_resources,program_resources);
    SET(program_chain,program_chain);SET(program_effect_param_info,program_effect_param_info);SET(program_effect_param_get,program_effect_param_get);SET(program_effect_param_set,program_effect_param_set);
    SET(program_effect_enable,program_effect_enable);SET(program_effect_model,program_effect_model);SET(program_effect_variant,program_effect_variant);
    SET(program_effect_screen_draw,program_effect_draw);SET(program_effect_screen_event,program_effect_event);
    SET(effects_begin,effects_begin);SET(effect_create,effect_create);SET(effect_destroy,effect_destroy);
    SET(effect_param_set,effect_param_set);SET(effect_enable,effect_enable);SET(effects_graph_set,effects_graph_set);
    SET(effect_info,effect_info);SET(effect_param_info,effect_param_info);SET(effects_status,effects_status);
#undef SET
    settings[1]=settings[5]=settings[7]=80;settings[9]=64;
    program_model_reset();queued=0;
    int rc=app->init(state,&platform);if(app_stopped)return RESONA_ERR_STATE;if(!rc){drain();app->frame(state,0);}return rc;
}
RESONA_EXPORT void resona_preview_input(uint32_t input,uint32_t kind,int32_t value){
    if(app_stopped)return;
    resona_app_event ev={0};ev.kind=RESONA_APP_EV_INPUT;ev.time_ms=now_ms;ev.u.input.input=input;ev.u.input.kind=kind;ev.u.input.value=value;
    if(app->event)app->event(state,&ev);drain();
}
RESONA_EXPORT void resona_preview_step(uint32_t ms){
    while(ms&&!app_stopped){
        uint32_t step=ms>33-frame_elapsed?33-frame_elapsed:ms;
        for(int i=0;i<8;i++)if(timers[i].used){
            int32_t wait=(int32_t)(timers[i].next-now_ms);
            uint32_t due=wait>0?(uint32_t)wait:1;
            if(due<step)step=due;
        }
        ms-=step;now_ms+=step;frame_elapsed+=step;rgb_tick();
        for(int i=0;i<8;i++)if(timers[i].used&&(int32_t)(now_ms-timers[i].next)>=0){
            resona_app_event ev={0};ev.kind=RESONA_APP_EV_TIMER;ev.time_ms=now_ms;ev.u.timer.id=timers[i].id;
            if(timers[i].repeat && --timers[i].repeat==0)timers[i].used=0;else timers[i].next=now_ms+timers[i].period;
            if(app->event)app->event(state,&ev);
        }drain();if(frame_elapsed>=33){app->frame(state,frame_elapsed);frame_elapsed=0;drain();}
    }
}
RESONA_EXPORT const char *resona_preview_log(void){return logs;}
RESONA_EXPORT uint32_t resona_preview_unsupported(void){return unsupported;}
RESONA_EXPORT int resona_preview_stopped(void){return app_stopped;}
RESONA_EXPORT void resona_preview_close(void){memset(&effect_hooks,0,sizeof effect_hooks);program_model_close();effects_reset();memory_reset();free(state);state=NULL;}

RESONA_EXPORT void resona_preview_rgb(uint8_t out[48]){rgb_tick();memcpy(out,&rgb,48);}
