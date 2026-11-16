/*@block
 * kind      gate
 * title     Gate
 * short     GATE
 * category  Dynamics
 * legacy    yes
 * summary   Closes when the signal falls below a threshold. Good after a reverb or a
 *           distortion.
 *
 * in        in audio
 * out       out audio
 *
 * param     thresh "Threshold" THRESH dB -80 -10 -50 expose set_threshold
 * param     release "Release" REL ms 5 500 80 expose set_release
 *
 * type      Gate
 * state     40
 */
#ifndef RESONA_BLOCK_GATE_H
#define RESONA_BLOCK_GATE_H

#include "resona/blocks/common.h"
#include "resona/blocks/env.h"

namespace resona {
namespace blocks {

/* A noise gate: closes below the threshold, opens quickly. */
struct Gate {
    float env, att, rel, thresh, gain, release_coef;
    uint32_t tick;

    void init() { env = 0.0f; att = Envelope::coef(1.0f); rel = Envelope::coef(20.0f); thresh = 0.003f; gain = 0.0f; release_coef = 0.002f; tick = 0u; }
    void reset() { env = 0.0f; gain = 0.0f; }
    void set_threshold(float db) { thresh = resona_db_to_lin(db); }
    void set_release(float ms) { release_coef = 1.0f - Envelope::coef(ms); }
    float process(float x)
    {
        const float a = absf(x);
        const float c = a > env ? att : rel;
        env = flush(c * env + (1.0f - c) * a);
        const float target = env > thresh ? 1.0f : 0.0f;
        gain += (target - gain) * (target > gain ? 0.02f : release_coef);
        gain = flush(gain);
        return x * gain;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_GATE_H */
