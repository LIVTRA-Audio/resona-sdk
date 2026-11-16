/* GPL-3.0 licensed. Persistent startup assets, independent of the installed Box App.
 * Names never grant access to USB descriptors or IDs.
 * Writes replace only the startup configuration; presets and app storage stay intact.
 */
#ifndef RESONA_STARTUP_H
#define RESONA_STARTUP_H
#include <stdint.h>
#define RESONA_USB_NAME_BYTES 48u
#define RESONA_SPLASH_MAX_BYTES 24576u
#define RESONA_SPLASH_WIDTH 320u
#define RESONA_SPLASH_HEIGHT 172u
/* UTF-8 names, NUL terminated, at most47 bytes each; no control characters.
 * PID allocation belongs to the platform/Lab. Apply takes effect after reboot.
 */
typedef struct resona_usb_names {
    char product[RESONA_USB_NAME_BYTES];
    char audio[RESONA_USB_NAME_BYTES];
    char midi[RESONA_USB_NAME_BYTES];
} resona_usb_names;
/* UTF-8 validation shared by SDK tools/models and the protected platform.
 * C0/C1 control characters, overlong forms and invalid scalar values are refused. */
static inline int resona_usb_name_valid(const char *s){
    unsigned n=0;while(n<RESONA_USB_NAME_BYTES&&s[n])n++;
    if(!n||n==RESONA_USB_NAME_BYTES)return 0;
    unsigned i=0;while(i<n){uint32_t c=(uint8_t)s[i++];unsigned more=0;uint32_t minimum=0;
        if(c<0x80u){if(c<0x20u||c==0x7fu)return 0;continue;}
        if(c>=0xc2u&&c<=0xdfu){c&=31u;more=1;minimum=0x80u;}
        else if(c>=0xe0u&&c<=0xefu){c&=15u;more=2;minimum=0x800u;}
        else if(c>=0xf0u&&c<=0xf4u){c&=7u;more=3;minimum=0x10000u;}else return 0;
        while(more--){if(i==n||((uint8_t)s[i]&0xc0u)!=0x80u)return 0;c=(c<<6)|((uint8_t)s[i++]&63u);}
        if(c<minimum||c>0x10ffffu||(c>=0xd800u&&c<=0xdfffu)||(c>=0x80u&&c<=0x9fu))return 0;
    }return 1;
}
static inline int resona_usb_names_valid(const resona_usb_names *n){return n&&resona_usb_name_valid(n->product)&&resona_usb_name_valid(n->audio)&&resona_usb_name_valid(n->midi);}
typedef struct resona_startup_info {
    uint32_t version, size;
    resona_usb_names names;
    uint32_t current_pid, next_pid, profiles_used;
    uint32_t splash_bytes, splash_crc32, splash_duration_ms;
    char unit_id[33]; /* Opaque product identity; never use a display name as identity. */
    uint8_t reserved[3];
} resona_startup_info;
/* .rsp: little endian 32-byte header: "RSP1", total bytes, CRC32(payload),
 * duration_ms (0 skips; <=0x7fffffff), width=320, height=172, frame count, reserved=0.
 * Each frame: hold_ms u32 (16..60000), run count u32; runs are count u16/color565 u16.
 * Counts must cover exactly320*172 pixels, no zero runs. At most128 frames.
 * Frames loop to the selected total duration. No code is executed from this asset.
 * NULL/0 restores the factory splash. Use tools/splash.py or generate this public format.
 */
#endif
