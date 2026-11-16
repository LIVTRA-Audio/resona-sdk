/* GPL-3.0. Bounded UTF-8 decoding; no allocation, locale or platform dependency. */
#ifndef RESONA_UTF8_H
#define RESONA_UTF8_H
#include <stdint.h>
#define RESONA_UNICODE_REPLACEMENT 0xfffdu
static inline uint32_t resona_utf8_decode(const uint8_t *s, uint32_t bytes, uint32_t *used)
{
    uint32_t first, count, value;
    *used = 0;
    if (!bytes) return 0;
    first = s[0]; *used = 1;
    if (first < 0x80u) return first;
    if (first >= 0xc2u && first <= 0xdfu) { count=2; value=first&31u; }
    else if (first >= 0xe0u && first <= 0xefu) { count=3; value=first&15u; }
    else if (first >= 0xf0u && first <= 0xf4u) { count=4; value=first&7u; }
    else return RESONA_UNICODE_REPLACEMENT;
    for (uint32_t i=1; i<count; ++i) {
        if (i==bytes) { *used=i; return RESONA_UNICODE_REPLACEMENT; }
        if ((s[i]&0xc0u)!=0x80u) { *used=i; return RESONA_UNICODE_REPLACEMENT; }
        if (i==1 && ((first==0xe0u && s[i]<0xa0u) ||
                     (first==0xedu && s[i]>=0xa0u) ||
                     (first==0xf0u && s[i]<0x90u) ||
                     (first==0xf4u && s[i]>=0x90u))) return RESONA_UNICODE_REPLACEMENT;
        value=(value<<6)|(s[i]&63u);
    }
    *used=count; return value;
}
/* Largest prefix within limit, without splitting a scalar at that boundary. */
static inline uint32_t resona_utf8_prefix(const uint8_t *s, uint32_t bytes, uint32_t limit)
{
    uint32_t at=0;
    while (at<bytes && at<limit) {
        uint32_t used; (void)resona_utf8_decode(s+at,bytes-at,&used);
        if (used>limit-at) break;
        at+=used;
    }
    return at;
}
#endif
