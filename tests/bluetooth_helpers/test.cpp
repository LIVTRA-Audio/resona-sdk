// GPL-3.0. Public API safety, byte budgets, persistence notification and refusal behavior.
#include <resona/bluetooth_helpers.h>
#include <cassert>
#include <cstring>
static resona_bluetooth_info info;
static int error;
static unsigned writes;
static int get(resona_bluetooth_info*out){*out=info;return 0;}
static int set(const resona_bluetooth_names*n){if(error)return error;if(n&&!resona_bluetooth_names_valid(n))return RESONA_ERR_ARG;const resona_bluetooth_names d={"Resona Box Audio","Resona Box Midi"};if(!n)n=&d;
 if(std::strcmp(info.saved.audio,n->audio)||std::strcmp(info.saved.midi,n->midi)){info.saved=*n;writes++;}info.power_cycle_required=std::strcmp(info.saved.audio,info.active.audio)||std::strcmp(info.saved.midi,info.active.midi);return 0;}
int main(){resona_platform p={};p.version=RESONA_PLATFORM_VERSION;p.size=sizeof p;p.bluetooth_names_get=get;p.bluetooth_names_set=set;
 set(nullptr);info.active=info.saved;info.power_cycle_required=0;writes=0;
 assert(resona_bluetooth_apply(&p,nullptr)==0&&writes==0);
 resona_bluetooth_names n={"My Audio","My MIDI"};assert(resona_bluetooth_apply(&p,&n)==1&&writes==1);
 assert(resona_bluetooth_apply(&p,&n)==1&&writes==1);info.active=info.saved;assert(resona_bluetooth_apply(&p,&n)==0&&writes==1);
 n.midi[0]='\n';assert(resona_bluetooth_apply(&p,&n)==RESONA_ERR_ARG&&writes==1);
 std::memset(n.audio,'a',32);assert(!resona_bluetooth_names_valid(&n));
 n={"音频","MIDI"};assert(resona_bluetooth_names_valid(&n));n.midi[0]=(char)0xc0;n.midi[1]=(char)0x80;n.midi[2]=0;assert(!resona_bluetooth_names_valid(&n));
 error=RESONA_ERR_STORAGE;assert(resona_bluetooth_apply(&p,nullptr)==error&&writes==1);p.version=RESONA_PLATFORM_VERSION+1;assert(resona_bluetooth_apply(&p,nullptr)==RESONA_ERR_UNSUPPORTED);p.version=RESONA_PLATFORM_VERSION;p.size=offsetof(resona_platform,bluetooth_names_set);assert(resona_bluetooth_apply(&p,nullptr)==RESONA_ERR_UNSUPPORTED);
}
