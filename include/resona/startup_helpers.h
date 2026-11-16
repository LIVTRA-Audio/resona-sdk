/* GPL-3.0. Optional code-first USB naming and splash helpers. No VID/PID parameter is exposed. */
#ifndef RESONA_STARTUP_HELPERS_H
#define RESONA_STARTUP_HELPERS_H
#include "resona/platform.h"
#include "resona/splash.h"
#define RESONA_STARTUP_REBOOT_REQUIRED 1
static inline int resona_startup_name_equal(const char*a,const char*b){
    for(unsigned i=0;i<RESONA_USB_NAME_BYTES;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;
}
/* Call from Box App init/control code. Does not reboot automatically.
 * Return0: active;1: saved, reboot required; negative: unchanged/refused.
 * Compare saved names before writing, so repeated init does not wear flash.
 * After one reboot the same code returns0, avoiding a reboot loop.
 * NULL selects the factory profile. Check errors before deciding to reboot.
 */
static inline int resona_startup_usb_apply(const resona_platform*p,const resona_usb_names*names){
    if(!p||p->version!=RESONA_PLATFORM_VERSION||p->size<offsetof(resona_platform,startup_usb_set)+sizeof(p->startup_usb_set)||!p->startup_info||!p->startup_usb_set)return RESONA_ERR_UNSUPPORTED;
    if(names&&!resona_usb_names_valid(names))return RESONA_ERR_ARG;
    resona_startup_info info;int rc=p->startup_info(&info);if(rc)return rc;
    const resona_usb_names factory={"Resona Box","Resona Box","Resona Box"};const resona_usb_names*want=names?names:&factory;
    int same=resona_startup_name_equal(want->product,info.names.product)&&resona_startup_name_equal(want->audio,info.names.audio)&&resona_startup_name_equal(want->midi,info.names.midi);
    if(!same||!names){rc=p->startup_usb_set(names);if(rc)return rc;rc=p->startup_info(&info);if(rc)return rc;}
    return info.current_pid==info.next_pid?RESONA_OK:RESONA_STARTUP_REBOOT_REQUIRED;
}
/* Apply an asset only when its complete CRC/size differs from stored data.
 * Return0: stored asset already matches;1: changed, next boot will use it;
 * negative: invalid/unsupported/write refused. Reboot is the App's choice.
 * NULL/0 restores the factory splash, also without repeated flash writes.
 */
static inline int resona_startup_splash_apply(const resona_platform*p,const void*data,uint32_t bytes){
    if(!p||p->version!=RESONA_PLATFORM_VERSION||p->size<offsetof(resona_platform,startup_splash_set)+sizeof(p->startup_splash_set)||!p->startup_info||!p->startup_splash_set)return RESONA_ERR_UNSUPPORTED;
    if((!data&&bytes)||(data&&!resona_splash_valid(data,bytes)))return RESONA_ERR_ARG;
    uint32_t crc=bytes?resona_splash_crc32(data,bytes):0;
    resona_startup_info info;int rc=p->startup_info(&info);if(rc)return rc;
    if(info.splash_bytes==bytes&&info.splash_crc32==crc)return RESONA_OK;
    rc=p->startup_splash_set(data,bytes);return rc?rc:RESONA_STARTUP_REBOOT_REQUIRED;
}
#endif
