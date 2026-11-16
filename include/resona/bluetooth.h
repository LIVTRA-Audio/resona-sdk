/* GPL-3.0. Persistent Bluetooth display names. No radio identity or sample-rate control. */
#ifndef RESONA_BLUETOOTH_H
#define RESONA_BLUETOOTH_H
#include <stdint.h>
#define RESONA_BLUETOOTH_NAME_BYTES 32u
/* Limits are UTF-8 bytes, excluding NUL: audio31, MIDI28 (advertising budget). */
typedef struct resona_bluetooth_names { char audio[32]; char midi[32]; } resona_bluetooth_names;
typedef struct resona_bluetooth_info {
    uint32_t version, size, power_cycle_required;
    resona_bluetooth_names active, saved;
} resona_bluetooth_info;
static inline int resona_bluetooth_name_valid(const char*s,unsigned limit){
    unsigned n=0;if(!s)return 0;while(n<=limit&&s[n])n++;
    if(!n||n>limit)return 0;
    unsigned i=0;while(i<n){uint32_t c=(uint8_t)s[i++],minimum=0;unsigned more=0;
        if(c<0x80u){if(c<0x20u||c==0x7fu)return 0;continue;}
        if(c>=0xc2u&&c<=0xdfu){c&=31u;more=1;minimum=0x80u;}
        else if(c>=0xe0u&&c<=0xefu){c&=15u;more=2;minimum=0x800u;}
        else if(c>=0xf0u&&c<=0xf4u){c&=7u;more=3;minimum=0x10000u;}else return 0;
        while(more--){if(i==n||((uint8_t)s[i]&0xc0u)!=0x80u)return 0;c=(c<<6)|((uint8_t)s[i++]&63u);}
        if(c<minimum||c>0x10ffffu||(c>=0xd800u&&c<=0xdfffu)||(c>=0x80u&&c<=0x9fu))return 0;
    }return 1;
}
static inline int resona_bluetooth_names_valid(const resona_bluetooth_names*n){
    return n&&resona_bluetooth_name_valid(n->audio,31)&&resona_bluetooth_name_valid(n->midi,28);
}
#endif
