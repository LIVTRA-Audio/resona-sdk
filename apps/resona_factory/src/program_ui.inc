/* Public Factory App controller for preset jobs (platform API 1). No private DSP. */
static int g_main_param; /* the selected tile of the single-RPK main page */
static int program_request(uint32_t action,uint32_t slot,const char*name){
 int id=P->program_submit(action,slot,name,0);if(id<=0)return -1;
 g_requested_job=(uint32_t)id;g_job.id=(uint32_t)id;g_job.action=action;g_job.preset=slot;g_job.state=RESONA_PROGRAM_QUEUED;g_dirty=true;return 0;
}
/* A normal load finishes in a few hundred ms: hold the last frame and show the
 * loading page only when it really takes time (a CPU check). */
#define PROGRAM_LOADING_SCREEN_MS 1000U
static uint32_t g_loading_since;
static bool program_draw_loading(void){
 /* While browsing, keep the preset list moving; the load happens behind it. */
 if(g_preset_browse||!program_loading()){g_loading_since=0;return false;}
 uint32_t t=now();if(!g_loading_since)g_loading_since=t?t:1u;
 if(t-g_loading_since<PROGRAM_LOADING_SCREEN_MS)return true;
 /* Retain the normal header. This draws no chain metadata while the platform prepares it. */
 bool edit=g_edit_job&&g_job.id==g_edit_job;
 resona_ui*u=P->screen_begin(NULL,NULL);u->rect(u->ctx,0,61,320,111,RESONA_RGB(7,9,12));
 const char*label=g_job.state==RESONA_PROGRAM_CHECKING?"CHECKING CPU":g_job.state==RESONA_PROGRAM_STARTING?"STARTING AUDIO":edit?"UPDATING CHAIN":"LOADING PRESET";
 u->text(u->ctx,12,79,2,RESONA_RGB(232,227,211),label);
 if(!edit){char text[40];snprintf(text,sizeof text,"PRESET %02u",(unsigned)g_job.preset+1U);u->text(u->ctx,12,111,1,RESONA_RGB(116,130,136),text);}
 u->rect(u->ctx,12+(now()/50U)%268U,139,28,3,RESONA_RGB(39,206,212));P->screen_present(0,61,320,111);return true;
}
static void program_completion(void){
 resona_program_job job;memset(&job,0,sizeof job);job.version=1u;job.size=sizeof job;if(P->program_job(&job))return;g_job=job;
 if(job.id!=g_completed_job&&job.state>=RESONA_PROGRAM_COMPLETE&&!job.queued_id){
  bool edit=g_edit_job&&job.id==g_edit_job;
  g_completed_job=job.id;g_own_screen=-1; /* a load or edit finished: ask the effect again */
  if(edit&&job.result&&job.state!=RESONA_PROGRAM_CANCELLED){show_toast(chain_edit_error(job.result,g_edit_keep_reorder));g_reorder_mode=false;}
  else if(job.result&&job.state!=RESONA_PROGRAM_CANCELLED){g_save_error=job.result==-11?"CPU LIMIT":job.result==-3||job.result==-12?"NOT ENOUGH MEMORY":job.result==-15?"STORAGE FULL":job.result==-20?"SAVED: SELECTION NOT UPDATED":job.result==-24?"PRESET SKIPPED AT START":job.result==-25?"AUDIO NOT READY":job.result==-23?"LOAD TIMED OUT":"PROGRAM LOAD FAILED";g_save_error_until=now()+ERROR_MS;}
  else if(!job.result){
   sync();
   if(edit){
    /* Select the moved pedal, or the first tile of the new effect. A move keeps reorder mode. */
    int n=(int)effect_count(),to=g_edit_to<n?g_edit_to:n-1;
    if(to>=0){g_selected_main_item=(uint8_t)(MAIN_EFFECT0+to);g_selected_effect=(uint8_t)to;}
    g_reorder_mode=g_edit_keep_reorder&&g_reorder_mode;
    g_main_param=0;
   }
   else if(job.action==RESONA_PROGRAM_PREVIEW){g_preset_preview_slot=(int)job.preset;g_preset_preview_valid=true;}
   else{g_preset_target=g_preset_slot;g_preset_preview_slot=g_preset_slot;g_preset_preview_valid=g_preset_slot>=0;if(job.action==RESONA_PROGRAM_SAVE)preset_saved_show();}
  }
  if(edit){g_edit_job=0;g_edit_to=-1;g_edit_keep_reorder=false;}
  g_dirty=true;
 }
}

/* RPK main page, for a chain of exactly one RPK effect: tile 0 switches the effect
 * on/off, tiles 1.. are its parameters. Encoder 1 walks preset -> volume -> settings
 * -> tiles; press edits a parameter (press again returns) or toggles on/off.
 * Encoder 2 long press assigns the selected parameter to Encoder 2.
 * An effect that draws its own screen: press on tile 0 opens that screen instead (see
 * pack_screen_input), and long press on tile 0 switches it on/off. */
static bool program_controls_active(void){
 return g_view==BOX_VIEW_MAIN&&effect_count()==1u&&effect_is_rpk(0u);
}
/* Ask the effect once per load whether it draws its own screen. It draws into the back
 * buffer, which the page drawn next covers; nothing is presented. */
static void program_screen_probe(void){
 if(!program_controls_active())return;
 const char*name=g_chain.effects[0].name;
 if(g_own_screen>=0&&!strncmp(g_own_screen_name,name,sizeof g_own_screen_name))return;
 memcpy(g_own_screen_name,name,sizeof g_own_screen_name);
 g_own_screen=P->program_effect_screen_draw(0u,1u)>=0?1:0;
 g_pack_screen_effect=-1; /* the next draw of that screen is a full one */
}
static bool program_has_screen(void){
 if(!program_controls_active())return false;
 program_screen_probe();
 return g_own_screen>0;
}
/* Open the effect's own screen: the editor view, drawn by the effect alone. */
static void program_screen_open(void){
 g_editing=false;g_selected_effect=0u;g_view=BOX_VIEW_EDITOR;g_screen_open=true;g_pack_screen=true;g_pack_screen_effect=-1;
}
/* Tiles: EFFECT on/off, the parameters, and CHANGE EFFECT (opens the EFFECT picker). */
static uint32_t program_tiles(void){return g_chain.effects[0].parameter_count+2u;}
static bool program_change_tile(int t){return t==(int)program_tiles()-1;}
static void program_controls_render(void){
 box_ui_control_t c[BOX_UI_CONTROLS_PER_PAGE];resona_effect_param info[BOX_UI_CONTROLS_PER_PAGE];char values[BOX_UI_CONTROLS_PER_PAGE][16];uint8_t n=0;
 bool on=effect_enabled(0u);
 uint32_t total=program_tiles();
 if(g_main_param>=(int)total)g_main_param=(int)total-1;
 if(g_main_param<0)g_main_param=0;
 uint32_t first=((uint32_t)g_main_param/BOX_UI_CONTROLS_PER_PAGE)*BOX_UI_CONTROLS_PER_PAGE;
 memset(c,0,sizeof c);
 for(uint32_t t=first;t<first+BOX_UI_CONTROLS_PER_PAGE&&t<total;t++,n++){
  if(t==0u){c[n].label="EFFECT";c[n].value=on?"ON":"OFF";c[n].unit="";c[n].fill=on?255u:0u;c[n].mapped=false;
   if(g_own_screen>0){c[n].hint="PUSH SCREEN";c[n].hint2="HOLD ON/OFF";}continue;}
  if(program_change_tile((int)t)){c[n].label="CHANGE";c[n].value="EFFECT";c[n].unit="";c[n].fill=0u;c[n].mapped=false;continue;}
  uint32_t index=t-1u;float v;memset(&info[n],0,sizeof(info[n]));
  if(P->program_effect_param_info(0u,index,&info[n])||P->program_effect_param_get(0u,index,&v))break;
  info[n].name[sizeof(info[n].name)-1u]=0;info[n].unit[sizeof(info[n].unit)-1u]=0;
  format_pack_value(values[n],&info[n],v);
  char*space=NULL;for(char*k=values[n];*k;k++)if(*k==' '){*k=0;space=k;break;}
  float span=info[n].max-info[n].min;float f=span>0.f?(v-info[n].min)/span:0.f;if(f<0.f)f=0.f;if(f>1.f)f=1.f;
  c[n].label=info[n].name;c[n].value=values[n];c[n].unit=space?space+1:"";c[n].fill=(uint8_t)(f*255.f+.5f);c[n].mapped=effect_param_mapped(0u,index);
 }
 int sel=g_selected_main_item==MAIN_EFFECT0?(int)((uint32_t)g_main_param-first):-1;
 box_ui_render_controls(g_fb,c,n,sel,g_editing&&sel>=0,first,total);
}
/* Returns true when the turn was consumed by the tile area. */
static bool program_controls_turn(int delta){
 if(!program_controls_active())return false;
 if(g_selected_main_item!=MAIN_EFFECT0){
  if(g_selected_main_item==MAIN_GLOBAL&&delta>0&&!g_editing){g_selected_main_item=MAIN_EFFECT0;g_selected_effect=0u;g_main_param=0;g_dirty=true;return true;}
  return false;
 }
 if(g_editing){nudge_effect_param(0u,(uint32_t)g_main_param-1u,delta);return true;}
 int next=g_main_param+delta,last=(int)program_tiles()-1;
 if(next<0){g_selected_main_item=MAIN_GLOBAL;g_main_param=0;}
 else g_main_param=next>last?last:next;
 g_dirty=true;return true;
}
static void program_controls_click(void){
 if(program_change_tile(g_main_param)){g_editing=false;picker_open();return;}
 if(g_main_param==0&&program_has_screen()){program_screen_open();return;}
 if(g_main_param==0){toggle_effect(0u);g_editing=false;}
 else g_editing=!g_editing;
 g_dirty=true;
}
/* Encoder 1 long press on tile 0 of an effect with its own screen: on/off (press opens the
 * screen). Returns true when consumed. */
static bool program_controls_long(void){
 if(!program_controls_active()||g_selected_main_item!=MAIN_EFFECT0||g_main_param!=0||g_preset_browse||g_preset_save||!program_has_screen())return false;
 toggle_effect(0u);g_editing=false;g_dirty=true;return true;
}
/* Encoder 2 long press on a parameter tile: assign it to Encoder 2, or clear. */
static bool program_controls_map(void){
 if(!program_controls_active()||g_selected_main_item!=MAIN_EFFECT0||g_main_param<1||program_change_tile(g_main_param))return false;
 uint32_t index=(uint32_t)g_main_param-1u;
 if(effect_param_mapped(0u,index))(void)P->program_map_encoder(RESONA_PROGRAM_NO_PARAMETER,RESONA_PROGRAM_NO_PARAMETER);
 else if(P->program_map_encoder(0u,index)==RESONA_OK&&g_enc2_map_valid){g_enc2_map_valid=false;store_save();}
 sync();g_dirty=true;return true;
}
