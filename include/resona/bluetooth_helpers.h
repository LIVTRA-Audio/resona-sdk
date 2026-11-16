/* GPL-3.0. Code-first naming; never restarts the device or changes radio identity. */
#ifndef RESONA_BLUETOOTH_HELPERS_H
#define RESONA_BLUETOOTH_HELPERS_H
#include <resona/platform.h>
#define RESONA_BLUETOOTH_POWER_CYCLE_REQUIRED 1
static inline int resona_bluetooth_apply(const resona_platform*p,const resona_bluetooth_names*n){
    if(!p||p->version!=RESONA_PLATFORM_VERSION||p->size<offsetof(resona_platform,bluetooth_names_set)+sizeof(p->bluetooth_names_set)||!p->bluetooth_names_get||!p->bluetooth_names_set)return RESONA_ERR_UNSUPPORTED;
    if(n&&!resona_bluetooth_names_valid(n))return RESONA_ERR_ARG;
    int rc=p->bluetooth_names_set(n);if(rc)return rc;
    resona_bluetooth_info info;rc=p->bluetooth_names_get(&info);
    return rc?rc:(info.power_cycle_required?RESONA_BLUETOOTH_POWER_CYCLE_REQUIRED:RESONA_OK);
}
#endif
