#include <resona/module.h>

namespace {
struct State { float values[12]; uint32_t selected; };
const resona_param params[] = {
    {"P01",0,255,1,""},{"P02",0,255,2,""},{"P03",0,255,3,""},
    {"P04",0,255,4,""},{"P05",0,255,5,""},{"P06",0,255,6,""},
    {"P07",0,255,7,""},{"P08",0,255,8,""},{"P09",0,255,9,""},
    {"P10",0,255,10,""},{"P11",0,255,11,""},{"P12",0,255,12,""},
};
void init(void *,const resona_host *) {}
void reset(void *) {}
void set_param(void *s,uint32_t index,float value) {
    if(index<12) static_cast<State *>(s)->values[index]=value;
}
float get_param(const void *s,uint32_t index) {
    return index<12 ? static_cast<const State *>(s)->values[index] : 0.0f;
}
void process(void *,const float *const *in,float *const *out,uint32_t n) {
    for(uint32_t i=0;i<n;++i) out[0][i]=in[0][i]; // Parameter/UI example: dry audio.
}
void on_event(void *s,const resona_event *e) {
    State *p=static_cast<State *>(s);
    if(e->kind!=RESONA_EVENT_TURN) return;
    if(e->input==RESONA_INPUT_SELECTOR) {
        int next=(int)p->selected+e->value;
        p->selected=(uint32_t)(next<0?0:next>11?11:next);
    } else if(e->input==RESONA_INPUT_CUSTOMIZE) {
        float next=p->values[p->selected]+(float)e->value;
        p->values[p->selected]=next<0?0:next>255?255:next;
    }
    // No special preset call: get_param reports these UI edits to the host.
}
void draw(void *s,const resona_ui *ui) {
    const State *p=static_cast<const State *>(s);
    ui->clear(ui->ctx,0x0000);ui->text(ui->ctx,8,4,2,0xffff,"UI PARAMETERS");
    for(uint32_t i=0;i<12;++i) {
        int x=8+(int)(i/6)*156,y=30+(int)(i%6)*22;
        uint16_t ink=p->selected==i?0x07ff:0xffff;
        ui->text(ui->ctx,x,y,1,ink,params[i].name);
        int v=(int)p->values[i];char text[4]={(char)('0'+v/100),(char)('0'+v/10%10),(char)('0'+v%10),0};
        ui->text(ui->ctx,x+68,y,1,ink,text);
    }
}
const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),
    "org.resona.example.ui-parameters","UI Parameters","0.1.0",
    RESONA_LAYOUT_MONO,RESONA_RATE_48000,sizeof(State),12,params,nullptr,
    init,reset,set_param,process,draw,on_event,get_param};
}
extern "C" RESONA_EXPORT const resona_module *resona_module_get(){return &module;}
