/* GPL-3.0. Host contract test: the preset chain, local persistence, timers and mixed drawing. */
#include <resona/platform.h>
#include <resona/libc.h>
#include "../../app_storage/contract.h"
static const resona_platform *P;
static unsigned forever,once;
static int init(void *state,const resona_platform *p) {
    (void)state;P=p;
    /* The playing preset is a chain: Factory Init is the five built-ins. */
    resona_program_info info;resona_chain_view chain;resona_preset_info pi;float v=0;
    if(p->program_info(&info)||info.effect_count!=5||p->program_chain(&chain)||chain.count!=5)return -10;
    if(chain.effects[0].source!=RESONA_CHAIN_BUILTIN||chain.effects[0].engine!=RESONA_ENGINE_GATE||chain.effects[4].engine!=RESONA_ENGINE_CAB)return -11;
    if(p->program_effect_param_set(1,0,0.73f)||p->program_effect_param_get(1,0,&v)||v<0.72f||v>0.74f)return -21;
    if(p->program_effect_param_set(5,0,0.5f)!=RESONA_ERR_ARG||p->program_effect_param_set(1,99,0.5f)!=RESONA_ERR_ARG)return -22;
    if(p->program_effect_enable(1,0)||p->program_chain(&chain)||chain.effects[1].enabled)return -23;
    if(p->program_map_encoder(1,0)||p->program_info(&info)||info.encoder_effect!=1||info.encoder_parameter!=0)return -24;
    /* Edits are jobs: MOVE reorders the factory chain (Encoder 2 follows), SET swaps the preset's effect. */
    resona_chain_edit ed={1,sizeof ed,RESONA_CHAIN_MOVE,1,3,0,0,0};resona_program_job job;memset(&job,0,sizeof job);job.version=1;job.size=sizeof job;
    if(p->program_edit(&ed)<=0||p->program_job(&job)||job.state!=RESONA_PROGRAM_COMPLETE||job.result||p->program_chain(&chain)||chain.effects[3].engine!=RESONA_ENGINE_COMP||p->program_info(&info)||info.encoder_effect!=3)return -25;
    ed.op=RESONA_CHAIN_SET;ed.source=RESONA_CHAIN_RPK;ed.entry=RESONA_ENTRY(RESONA_ENTRY_PACK,0);
    if(p->program_edit(&ed)<=0||p->program_chain(&chain)||chain.count!=1||chain.effects[0].source!=RESONA_CHAIN_RPK)return -26;
    /* As on the Box: an edit the preset cannot take is a job that fails, and the RPK keeps playing. */
    ed.op=RESONA_CHAIN_MOVE;if(p->program_edit(&ed)<=0||p->program_job(&job)||job.state!=RESONA_PROGRAM_FAILED||job.result!=RESONA_ERR_ARG||job.action!=RESONA_PROGRAM_EDIT||p->program_chain(&chain)||chain.count!=1)return -27;
    ed.op=4;if(p->program_edit(&ed)!=RESONA_ERR_ARG)return -27;
    ed.op=RESONA_CHAIN_SET;ed.source=RESONA_CHAIN_BUILTIN;ed.entry=0;
    if(p->program_edit(&ed)<=0||p->program_chain(&chain)||chain.count!=5||chain.effects[1].engine!=RESONA_ENGINE_COMP)return -27;
    if(p->program_submit(RESONA_PROGRAM_SAVE,7,"LOCAL ONLY",0)<=0)return -12;
    if(p->preset_info(7,&pi)||!pi.used||strcmp(pi.name,"LOCAL ONLY"))return -14;
    int32_t volume=0;unsigned x=0x12345678,y=0;
    if(p->setting_set(RESONA_SETTING_VOLUME,42)||p->setting_get(RESONA_SETTING_VOLUME,&volume)||volume!=42)return -16;
    if(p->app_store_write(4092,&x,4)||p->app_store_read(4092,&y,4)||y!=x||p->app_store_write(4093,&x,4)!=RESONA_ERR_ARG)return -17;
    if(resona_storage_contract(p))return -28;
    if(p->timer_start(11,100,0)||p->timer_start(12,100,1))return -18;
    uint16_t *px;uint32_t stride;resona_ui *u=p->screen_begin(&px,&stride);
    u->clear(u->ctx,0);px[0]=0xf800;u->rect(u->ctx,1,0,1,1,0x07e0);px[2]=0x001f;
    p->screen_present(0,0,320,172);p->log("platform contract: passed");return 0;
}
static void event(void *state,const resona_app_event *e) {
    (void)state;
    if(e->kind==RESONA_APP_EV_TIMER){if(e->u.timer.id==11)forever++;if(e->u.timer.id==12)once++;}
}
static void frame(void *state,uint32_t ms){(void)state;(void)ms;if(P->millis()>=1000){if(forever>=5&&once==1)P->log("timer contract: passed");else P->log("timer contract: FAILED");P->timer_stop(11);}}
static const resona_app app={RESONA_PLATFORM_VERSION,sizeof(resona_app),RESONA_APP_ID,RESONA_APP_NAME,RESONA_APP_VERSION,0,16384,init,event,frame};
RESONA_EXPORT const resona_app *resona_app_get(void){return &app;}
