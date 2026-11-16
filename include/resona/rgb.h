/* Status-light colors and animation math. GPL-3.0 licensed; no hardware access. */
#ifndef RESONA_RGB_H
#define RESONA_RGB_H
#include <stdint.h>
#define RESONA_RGB_VERSION 1u
#define RESONA_RGB_SOLID 0u
#define RESONA_RGB_BLINK 1u
#define RESONA_RGB_BREATHE 2u
#define RESONA_RGB_RAINBOW 3u
#define RESONA_RGB_FADE 4u /* one-shot primary -> secondary, then hold */
#define RESONA_RGB_CUSTOM 1u
#define RESONA_RGB_ANIMATING 2u
#define RESONA_RGB_FINISHED 4u
#define RESONA_RGB_OWNER_LEGACY 0u
#define RESONA_RGB_OWNER_APP 1u
#define RESONA_RGB_OWNER_LAB 2u

typedef struct resona_rgb_config {
    uint32_t version, size;
    uint32_t animation, period_ms; /* animated period: 20..60000 ms */
    uint8_t red, green, blue, brightness; /* all 0..255, linear intensity */
    uint8_t red2, green2, blue2, reserved; /* secondary for blink/fade; reserved=0 */
} resona_rgb_config;
typedef struct resona_rgb_state {
    uint32_t version, size;
    resona_rgb_config config;
    uint32_t phase_ms; /* cyclic phase, or clamped elapsed time for fade */
    uint8_t red, green, blue, flags; /* brightness-scaled current output */
    uint32_t pwm_hz, owner;
} resona_rgb_state;

static inline resona_rgb_config resona_rgb_solid(uint8_t r,uint8_t g,uint8_t b,uint8_t brightness) {
    resona_rgb_config c={RESONA_RGB_VERSION,sizeof(resona_rgb_config),RESONA_RGB_SOLID,0,r,g,b,brightness,0,0,0,0};return c;
}
static inline int resona_rgb_valid(const resona_rgb_config *c) {
    return c && c->version==RESONA_RGB_VERSION && c->size==sizeof(*c) && !c->reserved &&
        c->animation<=RESONA_RGB_FADE && (c->animation==RESONA_RGB_SOLID ? c->period_ms==0 : c->period_ms>=20 && c->period_ms<=60000);
}
static inline uint8_t resona_rgb_scale(uint32_t v,uint32_t brightness) {return (uint8_t)((v*brightness+127u)/255u);}
/* Deterministic portable evaluation used by preview. Call only with a valid config.
 * Cyclic effects use phase modulo period; fade clamps at the secondary color. */
static inline void resona_rgb_evaluate(const resona_rgb_config *c,uint32_t phase,uint8_t out[3]) {
    uint32_t r=c->red,g=c->green,b=c->blue;
    if(c->animation!=RESONA_RGB_SOLID){
        uint32_t period=c->period_ms;
        uint32_t t=c->animation==RESONA_RGB_FADE ? (phase<period?phase:period) : phase%period;
        if(c->animation==RESONA_RGB_BLINK){if(t>=period/2u){r=c->red2;g=c->green2;b=c->blue2;}}
        else if(c->animation==RESONA_RGB_BREATHE){uint32_t x=(t*510u)/period;uint32_t level=x<=255u?x:510u-x;r=resona_rgb_scale(r,level);g=resona_rgb_scale(g,level);b=resona_rgb_scale(b,level);}
        else if(c->animation==RESONA_RGB_FADE){
            r=(r*(period-t)+(uint32_t)c->red2*t+period/2u)/period;
            g=(g*(period-t)+(uint32_t)c->green2*t+period/2u)/period;
            b=(b*(period-t)+(uint32_t)c->blue2*t+period/2u)/period;
        }else if(c->animation==RESONA_RGB_RAINBOW){
            uint32_t h=t*1536u/period,s=h/256u,x=h%256u;
            switch(s){case 0:r=255;g=x;b=0;break;case 1:r=255-x;g=255;b=0;break;case 2:r=0;g=255;b=x;break;case 3:r=0;g=255-x;b=255;break;case 4:r=x;g=0;b=255;break;default:r=255;g=0;b=255-x;break;}
        }
    }
    out[0]=resona_rgb_scale(r,c->brightness);out[1]=resona_rgb_scale(g,c->brightness);out[2]=resona_rgb_scale(b,c->brightness);
}
#endif
