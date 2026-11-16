/*@block
 * kind      compressor
 * title     Compressor
 * short     COMP
 * category  Dynamics
 * legacy    yes
 * summary   Turns loud parts down: a feed-forward compressor with a soft knee.
 *
 * in        in audio
 * out       out audio
 *
 * param     thresh "Threshold" THRESH dB -60 0 -20 expose set_threshold
 * param     ratio "Ratio" RATIO :1 1 20 4 expose set_ratio
 * param     attack "Attack" ATTACK ms 0.5 100 10 hidden set_attack
 * param     release "Release" REL ms 10 1000 120 hidden set_release
 * param     makeup "Makeup" MAKEUP dB 0 24 3 expose set_makeup
 *
 * type      Compressor
 * state     48
 */
#ifndef RESONA_BLOCK_COMPRESSOR_H
#define RESONA_BLOCK_COMPRESSOR_H

#include "resona/blocks/common.h"
#include "resona/blocks/env.h"

namespace resona {
namespace blocks {

/* A feed-forward compressor with a 6 dB soft knee. The gain is computed in decibels every
 * few samples and smoothed. */
struct Compressor {
    float env, att, rel;
    float thresh;     /* dB */
    float ratio;
    float makeup;     /* dB */
    float target, gain;
    uint32_t tick;

    void init()
    {
        env = 0.0f; att = Envelope::coef(10.0f); rel = Envelope::coef(120.0f);
        thresh = -20.0f; ratio = 4.0f; makeup = 0.0f; target = gain = 1.0f; tick = 0u;
    }
    void reset() { env = 0.0f; target = gain = 1.0f; }
    void set_threshold(float db) { thresh = db; }
    void set_ratio(float r) { ratio = clampf(r, 1.0f, 40.0f); }
    void set_attack(float ms) { att = Envelope::coef(ms); }
    void set_release(float ms) { rel = Envelope::coef(ms); }
    void set_makeup(float db) { makeup = db; }

    float process(float x)
    {
        const float a = absf(x);
        const float c = a > env ? att : rel;
        env = flush(c * env + (1.0f - c) * a);
        if ((tick++ & (kControlEvery - 1u)) == 0u) {
            const float knee = 6.0f;
            const float over = resona_lin_to_db(env) - thresh;
            float reduction;
            if (2.0f * over < -knee) {
                reduction = 0.0f;
            } else if (2.0f * absf(over) <= knee) {
                const float k = over + knee * 0.5f;
                reduction = (1.0f / ratio - 1.0f) * k * k / (2.0f * knee);
            } else {
                reduction = (1.0f / ratio - 1.0f) * over;
            }
            target = resona_db_to_lin(reduction + makeup);
        }
        gain += (target - gain) * 0.05f;
        return x * gain;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_COMPRESSOR_H */
