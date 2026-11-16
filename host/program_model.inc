/* GPL-3.0. Public data-only programme fixture; never reaches USB or private engines. */
#include "resona/factory_params.h"
#define RESONA_PROGRAM_FACTORY 0x100u /* preview-private: the record is Factory Init */
static resona_program_info preview_program;
static resona_effect_param *preview_parameters;
static float *preview_values,*preview_saved[50];
static resona_effect_param *preview_saved_parameters[50];
static uint32_t preview_saved_flags[50];
static uint32_t preview_count,preview_saved_count[50],preview_saved_enable[50],preview_saved_map[50];
static char preview_names[50][17];
static resona_program_job preview_job;
/* The factory chain's order (built-in index per position) and the one RPK's name. */
static uint8_t chain_order[5]={0,1,2,3,4},preview_saved_order[50][5];
static char preview_rpk[24],preview_saved_rpk[50][24];
/* Per built-in parameter counts and flat offsets, from resona/factory_params.h. */
static uint8_t chain_counts[5],chain_offsets[5];
static void chain_layout(void){unsigned at=0;for(unsigned g=0;g<5;g++){chain_counts[g]=g+1U==RESONA_ENGINE_AMP?5U:(uint8_t)resona_factory_param_count(g+1U,0);chain_offsets[g]=(uint8_t)at;at+=chain_counts[g];}}
/* RPK entries on the preview Box: a demo pack (pool 0) and the programme fixture (pool 1). */
static const resona_effect_param demo_pack[2]={{"DEPTH","%",0.f,100.f,50.f,1.f,0},{"RATE","Hz",.1f,10.f,1.f,.1f,0}};
static resona_effect_param *fixture_params;static uint32_t fixture_count;static char fixture_name[17];
/* The Lab can attach the project's own effect (its desktop build) as the fixture RPK: its
 * parameters and its own screen then answer for the fixture whenever a preset plays it. */
typedef struct resona_preview_effect_hooks{
 uint32_t size;void*ctx;
 float(*get)(void*ctx,uint32_t index);
 float(*set)(void*ctx,uint32_t index,float value); /* returns the value the effect took */
 int(*draw)(void*ctx,uint32_t force);              /* <0: no screen of its own */
 int(*event)(void*ctx,const resona_event*ev);      /* <0: no event handler */
}resona_preview_effect_hooks;
static resona_preview_effect_hooks effect_hooks;
static uint8_t rpk_fixture,preview_saved_live[50]; /* the playing (saved) RPK is the fixture */
static uint8_t rpk_on=1,preview_saved_rpk_on[50]; /* the one RPK effect's own on/off */
static int effect_live(void);
static void effect_push(void);
static void program_model_close(void){free(preview_parameters);free(preview_values);preview_parameters=NULL;preview_values=NULL;for(unsigned i=0;i<50;i++){free(preview_saved[i]);free(preview_saved_parameters[i]);preview_saved_parameters[i]=NULL;preview_saved[i]=NULL;preview_saved_count[i]=0;}free(fixture_params);fixture_params=NULL;fixture_count=0;fixture_name[0]=0;}
static int entry_count(void){return fixture_name[0]?2:1;}
static int entry_info(uint32_t i,resona_entry_info*o){if(!o||(int)i>=entry_count())return RESONA_ERR_ARG;memset(o,0,sizeof(*o));o->id=RESONA_ENTRY(RESONA_ENTRY_PACK,i);o->kind=RESONA_ENTRY_PACK;o->bytes=4096;strcpy(o->name,i?fixture_name:"02_velvet_comb.rpk");return 0;}
static int chain_kind(void);static uint32_t chain_count(void);static int program_info(resona_program_info*out){if(!out)return RESONA_ERR_ARG;*out=preview_program;out->flags&=~RESONA_PROGRAM_FACTORY;out->effect_count=chain_count();out->encoder_effect=out->encoder_parameter=UINT32_MAX;if(preview_program.encoder_parameter!=UINT32_MAX){uint32_t f=preview_program.encoder_parameter;if(chain_kind()==2){out->encoder_effect=0;out->encoder_parameter=f;}else if(chain_kind()==1){for(uint32_t e=0;e<5;e++){uint32_t g=chain_order[e];if(f>=chain_offsets[g]&&f<chain_offsets[g]+chain_counts[g]){out->encoder_effect=e;out->encoder_parameter=f-chain_offsets[g];}}}}if(preview_effect_owned){out->active_preset=-1;out->effect_count=out->enabled=out->flags=0;}return 0;}
static int program_param_info(uint32_t i,resona_effect_param*out){if(!out||i>=preview_count)return RESONA_ERR_ARG;*out=preview_parameters[i];return 0;}
static int program_param_get(uint32_t i,float*out){if(!out||i>=preview_count)return RESONA_ERR_ARG;if(effect_live())preview_values[i]=effect_hooks.get(effect_hooks.ctx,i);*out=preview_values[i];return 0;}
static int program_param_set(uint32_t i,float v){if(i>=preview_count||!(v>=preview_parameters[i].min&&v<=preview_parameters[i].max))return RESONA_ERR_ARG;resona_effect_param*q=&preview_parameters[i];if(q->step>0.f){float steps=(v-q->min)/q->step;v=q->min+(float)(uint32_t)(steps+.5f)*q->step;if(v>q->max)v=q->max;}if(effect_live())v=effect_hooks.set(effect_hooks.ctx,i,v);preview_values[i]=v;return 0;}
static int program_enable(uint32_t on){if(on>1||!(preview_program.flags&RESONA_PROGRAM_USED))return RESONA_ERR_ARG;preview_program.enabled=on;return 0;}
static int program_map_flat(uint32_t index){if(index!=UINT32_MAX&&index>=preview_count)return RESONA_ERR_ARG;preview_program.encoder_parameter=index;return 0;}
static int program_job(resona_program_job*out){if(!out)return RESONA_ERR_ARG;*out=preview_job;out->version=1;out->size=sizeof(*out);out->active_preset=(uint32_t)preview_program.active_preset;out->committed_preset=(uint32_t)active;out->owner=preview_effect_owned?2U:1U;return 0;}
static int program_save_fixture(uint32_t slot,const char*name){
 if(effect_live())for(uint32_t i=0;i<preview_count;i++)preview_values[i]=effect_hooks.get(effect_hooks.ctx,i);
 uint32_t bytes=0;for(unsigned i=0;i<50;i++)if(i!=slot)bytes+=preview_saved_count[i]*8U+68U;
 if((uint64_t)bytes+preview_count*8U+68U>272U*1024U)return RESONA_ERR_FULL;
 float*v=malloc(preview_count?preview_count*4U:1U);resona_effect_param*params=malloc(preview_count?preview_count*sizeof(*params):1U);if(!v||!params){free(v);free(params);return RESONA_ERR_NO_MEMORY;}if(preview_count){memcpy(v,preview_values,preview_count*4U);memcpy(params,preview_parameters,preview_count*sizeof(*params));}free(preview_saved_parameters[slot]);preview_saved_parameters[slot]=params;preview_saved_flags[slot]=preview_program.flags;free(preview_saved[slot]);preview_saved[slot]=v;preview_saved_count[slot]=preview_count;preview_saved_enable[slot]=preview_program.enabled;preview_saved_map[slot]=preview_program.encoder_parameter;
 memcpy(preview_saved_order[slot],chain_order,5);memcpy(preview_saved_rpk[slot],preview_rpk,24);preview_saved_live[slot]=rpk_fixture;preview_saved_rpk_on[slot]=rpk_on;
 strncpy(preview_names[slot],name&&*name?name:preview_program.name,16);preview_names[slot][16]=0;return 0;
}
/* Replace the playing parameters (taking ownership); with v NULL they start at their defaults. */
static int program_replace(resona_effect_param*params,float*v,uint32_t count){
 if(!v){v=malloc(count?count*4U:1U);if(!v){free(params);return RESONA_ERR_NO_MEMORY;}for(uint32_t i=0;i<count;i++)v[i]=params[i].def;}
 free(preview_parameters);free(preview_values);preview_parameters=params;preview_values=v;preview_count=count;return 0;
}
/* Factory Init: the five built-ins (20 parameters, 5 on-switches, 3 models, Auto Gate). */
static int factory_params(void){
 resona_effect_param*params=calloc(29U,sizeof(*params));float*v=calloc(29U,4U);
 if(!params||!v){free(params);free(v);return RESONA_ERR_NO_MEMORY;}
 unsigned at=0;for(unsigned r=0;r<5;r++)for(unsigned p=0;p<chain_counts[r];p++,at++){resona_factory_param_info(r+1U,0,p,&params[at]);v[at]=params[at].def;}
 for(unsigned i=25;i<28;i++)v[i]=-1.f; /* no model chosen yet, as on the Box */
 for(unsigned i=20;i<28;i++){snprintf(params[i].name,16,i<25?"Effect %u on":"Model %u",i<25?i-19:i-24);params[i].min=i<25?0.f:-1.f;params[i].max=i<25?1.f:29.f;params[i].step=1;params[i].flags=RESONA_PARAM_STEPPED;}
 strcpy(params[28].name,"Auto Gate");params[28].max=1;params[28].step=1;params[28].flags=RESONA_PARAM_TOGGLE;
 (void)program_replace(params,v,29);for(unsigned i=0;i<5;i++)chain_order[i]=(uint8_t)i;preview_rpk[0]=0;rpk_fixture=0;
 preview_program.flags=RESONA_PROGRAM_FACTORY|RESONA_PROGRAM_USED;preview_program.enabled=1;rpk_on=1;preview_program.encoder_parameter=UINT32_MAX;return 0;
}
static void program_job_done(uint32_t action,uint32_t slot,int rc,uint32_t what){
 uint32_t id=preview_job.id+1;memset(&preview_job,0,sizeof preview_job);preview_job.id=id;preview_job.action=action;preview_job.preset=slot;
 preview_job.state=rc?RESONA_PROGRAM_FAILED:RESONA_PROGRAM_COMPLETE;preview_job.result=rc;
 resona_app_event ev={0};ev.kind=RESONA_APP_EV_CHANGED;ev.u.changed.what=RESONA_CHANGED_PROGRAM_JOB;ev.u.changed.index=slot;post(&ev);
 if(!rc&&what){ev.u.changed.what=what;post(&ev);}
}
static int program_submit(uint32_t action,uint32_t slot,const char*name,uint32_t expected){
 (void)expected;if(preview_effect_owned)return RESONA_ERR_STATE;if(action<1U||action>5U||slot>=50U||(name&&strlen(name)>16))return RESONA_ERR_ARG;
 if(action!=RESONA_PROGRAM_SAVE&&action!=RESONA_PROGRAM_FACTORY_RESET&&!preview_saved[slot])return RESONA_ERR_STATE;
 int rc=0;
 if(action==RESONA_PROGRAM_SAVE)rc=program_save_fixture(slot,name);
 else if(action==RESONA_PROGRAM_FACTORY_RESET){
  resona_effect_param*old_params=preview_parameters;float*old_values=preview_values;uint32_t old_count=preview_count;resona_program_info old=preview_program;uint8_t old_order[5];char old_effect[24];uint8_t old_live=rpk_fixture;
  memcpy(old_order,chain_order,5);memcpy(old_effect,preview_rpk,24);preview_parameters=NULL;preview_values=NULL;
  rc=factory_params();if(!rc){strcpy(preview_program.name,"Factory Init");rc=program_save_fixture(slot,"Factory Init");}
  if(rc){free(preview_parameters);free(preview_values);preview_parameters=old_params;preview_values=old_values;preview_count=old_count;preview_program=old;memcpy(chain_order,old_order,5);memcpy(preview_rpk,old_effect,24);rpk_fixture=old_live;}else{free(old_params);free(old_values);}
 }
 else{uint32_t count=preview_saved_count[slot];float*v=malloc(count?count*4U:1U);resona_effect_param*params=malloc(count?count*sizeof(*params):1U);if(!v||!params){free(v);free(params);rc=RESONA_ERR_NO_MEMORY;}else{if(count){memcpy(v,preview_saved[slot],count*4U);memcpy(params,preview_saved_parameters[slot],count*sizeof(*params));}(void)program_replace(params,v,count);preview_program.flags=preview_saved_flags[slot];preview_program.enabled=preview_saved_enable[slot];preview_program.encoder_parameter=preview_saved_map[slot];memcpy(chain_order,preview_saved_order[slot],5);memcpy(preview_rpk,preview_saved_rpk[slot],24);rpk_fixture=preview_saved_live[slot];rpk_on=preview_saved_rpk_on[slot];effect_push();}}
 if(!rc){preview_program.active_preset=(int32_t)slot;strcpy(preview_program.name,preview_names[slot]);if(action!=RESONA_PROGRAM_PREVIEW)active=(int)slot;}
 program_job_done(action,slot,rc,0);return (int)preview_job.id;
}
static void program_model_reset(void){chain_layout();
 program_model_close();memset(&preview_program,0,sizeof preview_program);memset(&preview_job,0,sizeof preview_job);
 preview_program.version=1;preview_program.size=sizeof preview_program;preview_program.active_preset=0;strcpy(preview_program.name,"Factory Init");(void)factory_params();
 for(unsigned i=0;i<50;i++)(void)program_save_fixture(i,"Factory Init");active=0;
}
RESONA_EXPORT int resona_preview_program(const char*name,const resona_effect_param*params,const float*values,uint32_t count){
 if(!name||strlen(name)>16||!params||!values||count>(272U*1024U-48U)/sizeof(resona_effect_param))return RESONA_ERR_ARG;
 resona_effect_param*p=malloc(count?(size_t)count*sizeof(*p):1U);float*v=malloc(count?count*4U:1U);resona_effect_param*f=malloc(count?(size_t)count*sizeof(*f):1U);if(!p||!v||!f){free(p);free(v);free(f);return RESONA_ERR_NO_MEMORY;}
 for(uint32_t i=0;i<count;i++){if(!memchr(params[i].name,0,16)||!memchr(params[i].unit,0,8)||!(params[i].min<=values[i]&&values[i]<=params[i].max)){free(p);free(v);free(f);return RESONA_ERR_ARG;}}
 memcpy(p,params,count*sizeof(*p));memcpy(v,values,count*4U);memcpy(f,params,count*sizeof(*f));program_model_close();preview_parameters=p;preview_values=v;preview_count=count;preview_program.active_preset=0;preview_program.enabled=1;preview_program.flags=RESONA_PROGRAM_USED;preview_program.encoder_parameter=count?0U:UINT32_MAX;strcpy(preview_program.name,name);
 /* The fixture is also an RPK on the preview Box, so the EFFECT picker can choose it again. */
 fixture_params=f;fixture_count=count;strcpy(fixture_name,name);strcpy(preview_rpk,name);rpk_fixture=1;rpk_on=1;(void)program_save_fixture(0,name);active=0;
 resona_app_event ev={0};ev.kind=RESONA_APP_EV_CHANGED;ev.u.changed.what=RESONA_CHANGED_PRESET;post(&ev);return 0;
}
static int program_preset_info(uint32_t slot,resona_preset_info*out){if(slot>=50||!out)return RESONA_ERR_ARG;memset(out,0,sizeof(*out));out->used=preview_saved[slot]!=NULL;strcpy(out->name,preview_names[slot]);out->enabled=preview_saved_enable[slot];out->effect_count=!out->used?0u:(preview_saved_flags[slot]&RESONA_PROGRAM_FACTORY)?5u:1u;return 0;}
/* Chain view over the same flat model as the device: Factory Init is the five
 * built-ins in chain_order, an RPK is one. */
static int chain_kind(void){if(preview_effect_owned||preview_program.active_preset<0||!(preview_program.flags&RESONA_PROGRAM_USED))return 0;return (preview_program.flags&RESONA_PROGRAM_FACTORY)?1:2;}
static uint32_t chain_count(void){int k=chain_kind();return k==1?5u:k==2?1u:0u;}
static int chain_flat(uint32_t e,uint32_t i,uint32_t*flat){int k=chain_kind();if(e>=chain_count())return RESONA_ERR_ARG;if(k==1){uint32_t g=chain_order[e];if(i>=chain_counts[g])return RESONA_ERR_ARG;*flat=chain_offsets[g]+i;return 0;}if(i>=preview_count)return RESONA_ERR_ARG;*flat=i;return 0;}
static int program_chain(resona_chain_view*out){
 if(!out)return RESONA_ERR_ARG;memset(out,0,sizeof(*out));out->version=1;out->size=sizeof(*out);out->active_preset=chain_kind()?active:-1;
 int k=chain_kind();out->count=chain_count();
 if(k==1){static const char*const n[5]={"Gate","Compressor","Resona","NAM","Cabinet"};for(uint32_t e=0;e<5;e++){uint32_t g=chain_order[e];resona_chain_effect*x=&out->effects[e];x->source=RESONA_CHAIN_BUILTIN;x->engine=g+1;x->parameter_count=chain_counts[g];x->enabled=preview_count>20+g&&preview_values[20+g]>=.5f;x->model=g>=2&&preview_count>23+g?(int32_t)preview_values[23+g]:RESONA_CHAIN_NO_MODEL;x->variant=g==0&&preview_count>28?(uint32_t)preview_values[28]:0;strncpy(x->name,n[g],sizeof(x->name)-1);}}
 else if(k==2){resona_chain_effect*x=&out->effects[0];x->source=RESONA_CHAIN_RPK;x->enabled=rpk_on;x->parameter_count=preview_count;x->model=RESONA_CHAIN_NO_MODEL;strncpy(x->name,preview_rpk[0]?preview_rpk:preview_program.name,sizeof(x->name)-1);}
 return 0;
}
static int program_effect_param_info(uint32_t e,uint32_t i,resona_effect_param*out){uint32_t f;int rc=chain_flat(e,i,&f);return rc?rc:program_param_info(f,out);}
static int program_effect_param_get(uint32_t e,uint32_t i,float*out){uint32_t f;int rc=chain_flat(e,i,&f);return rc?rc:program_param_get(f,out);}
static int program_effect_param_set(uint32_t e,uint32_t i,float v){uint32_t f;int rc=chain_flat(e,i,&f);return rc?rc:program_param_set(f,v);}
static int program_effect_enable(uint32_t e,uint32_t on){int k=chain_kind();if(on>1)return RESONA_ERR_ARG;if(k==1&&e<5)return program_param_set(20U+chain_order[e],(float)on);if(k==2&&e==0){rpk_on=(uint8_t)on;return 0;}return RESONA_ERR_ARG;}
static int program_effect_model(uint32_t e,uint32_t m){if(m>=30||chain_kind()!=1||e>=5||chain_order[e]<2)return RESONA_ERR_ARG;return program_param_set(23U+chain_order[e],(float)m);}
static int program_effect_variant(uint32_t e,uint32_t v){if(v>1||chain_kind()!=1||e>=5||chain_order[e])return RESONA_ERR_ARG;if(preview_count<=28)return RESONA_ERR_STATE;if((uint32_t)preview_values[28]!=v){uint32_t f=preview_program.encoder_parameter;if(f!=UINT32_MAX&&f>=chain_offsets[0]&&f<chain_offsets[0]+chain_counts[0])preview_program.encoder_parameter=UINT32_MAX;}preview_values[28]=(float)v;return 0;}
static int program_map(uint32_t e,uint32_t i){uint32_t f=UINT32_MAX;if(e!=UINT32_MAX||i!=UINT32_MAX){int rc=chain_flat(e,i,&f);if(rc)return rc;}return program_map_flat(f);}
/* A preset is the factory chain or one RPK. SET and MOVE are jobs that complete at once.
 * As on the Box, only the version, size and op are checked when the edit is submitted;
 * anything else the edit cannot do ends its job FAILED with RESONA_ERR_ARG, leaving the
 * playing preset as it was. The Encoder 2 mapping is flat, so it follows a moved built-in. */
static int program_edit(const resona_chain_edit*e){
 if(!e||e->version!=1||e->size!=sizeof(*e)||(e->op!=RESONA_CHAIN_MOVE&&e->op!=RESONA_CHAIN_SET))return RESONA_ERR_ARG;
 if(preview_effect_owned)return RESONA_ERR_STATE;
 int rc=RESONA_ERR_ARG;
 if(e->op==RESONA_CHAIN_MOVE){
  if(chain_kind()==1&&e->effect<5&&e->to<5){uint8_t g=chain_order[e->effect];if(e->effect<e->to)memmove(chain_order+e->effect,chain_order+e->effect+1,e->to-e->effect);else memmove(chain_order+e->to+1,chain_order+e->to,e->effect-e->to);chain_order[e->to]=g;rc=0;}
 }else if(e->source==RESONA_CHAIN_BUILTIN){
  if(!e->engine){rc=factory_params();if(!rc)strcpy(preview_program.name,"Factory Init");}
 }else if(e->source==RESONA_CHAIN_RPK){
  uint32_t pool=RESONA_ENTRY_INDEX(e->entry);
  if(e->entry==RESONA_ENTRY(RESONA_ENTRY_PACK,pool)&&(int)pool<entry_count()){
   uint32_t count=pool?fixture_count:2U;resona_effect_param*p=malloc(count?count*sizeof(*p):1U);
   if(!p)rc=RESONA_ERR_NO_MEMORY;else{if(count)memcpy(p,pool?fixture_params:demo_pack,count*sizeof(*p));rc=program_replace(p,NULL,count);}
   if(!rc){strcpy(preview_rpk,pool?fixture_name:"VELVET COMB");strncpy(preview_program.name,pool?fixture_name:"Velvet Comb",16);preview_program.name[16]=0;preview_program.flags=RESONA_PROGRAM_USED;preview_program.enabled=1;rpk_on=1;preview_program.encoder_parameter=UINT32_MAX;rpk_fixture=pool?1:0;effect_push();}
  }
 }
 program_job_done(RESONA_PROGRAM_EDIT,preview_program.active_preset<0?0U:(uint32_t)preview_program.active_preset,rc,RESONA_CHANGED_CHAIN);return (int)preview_job.id;
}
static void program_resources(resona_program_resources*o){if(o){memset(o,0,sizeof(*o));o->block_frames=64;}}
static int effect_live(void){return effect_hooks.ctx&&rpk_fixture&&chain_kind()==2;}
static void effect_push(void){if(effect_live())for(uint32_t i=0;i<preview_count;i++)preview_values[i]=effect_hooks.set(effect_hooks.ctx,i,preview_values[i]);}
static int program_effect_draw(uint32_t e,uint32_t force){if(chain_kind()!=2||e)return RESONA_ERR_ARG;if(!effect_live())return block_draw(0,1);return effect_hooks.draw(effect_hooks.ctx,force)<0?RESONA_ERR_UNSUPPORTED:1;}
static int program_effect_event(uint32_t e,const resona_event*ev){if(chain_kind()!=2||e||!ev)return RESONA_ERR_ARG;if(!effect_live())return block_event(0,ev);return effect_hooks.event(effect_hooks.ctx,ev)<0?RESONA_ERR_UNSUPPORTED:0;}
/* Attach (NULL detaches) the effect the fixture RPK stands for; set the fixture first. */
RESONA_EXPORT int resona_preview_effect(const resona_preview_effect_hooks*h){
 if(!h){memset(&effect_hooks,0,sizeof effect_hooks);return 0;}
 if(h->size!=sizeof *h||!h->ctx||!h->get||!h->set||!h->draw||!h->event)return RESONA_ERR_ARG;
 effect_hooks=*h;effect_push();
 resona_app_event ev={0};ev.kind=RESONA_APP_EV_CHANGED;ev.u.changed.what=RESONA_CHANGED_PRESET;post(&ev);return 0;
}
/* The Lab's own control (its parameter sliders) moving the playing effect, as a host edit. */
RESONA_EXPORT int resona_preview_effect_param(uint32_t e,uint32_t i,float v){
 resona_effect_param q;int rc=program_effect_param_info(e,i,&q);if(rc)return rc;
 if(!(v==v))return RESONA_ERR_ARG;if(v<q.min)v=q.min;if(v>q.max)v=q.max;
 rc=program_effect_param_set(e,i,v);if(rc)return rc;
 resona_app_event ev={0};ev.kind=RESONA_APP_EV_CHANGED;ev.u.changed.what=RESONA_CHANGED_EFFECT;ev.u.changed.index=e;post(&ev);return 0;
}
