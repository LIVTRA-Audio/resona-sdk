/*@block
 * kind      samplehold
 * title     Sample & Hold
 * short     SHLD
 * category  Control
 * legacy    no
 * summary   Takes the input a few times a second and holds it in between. Noise through it
 *           gives random steps.
 *
 * in        in cv
 * out       out cv
 *
 * param     rate "Rate" RATE Hz 0.1 200 8 expose set_rate
 *
 * type      SampleHold
 * state     12
 */
#ifndef RESONA_BLOCK_SAMPLEHOLD_H
#define RESONA_BLOCK_SAMPLEHOLD_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Sample and hold on an internal clock: the input is taken `rate` times a second and
 * held in between. Noise through it gives stepped random control. */
struct SampleHold {
    float held, phase, inc;

    void init() { held = 0.0f; phase = 0.0f; inc = 8.0f / kRate; }
    void reset() { held = 0.0f; phase = 0.0f; }
    void set_rate(float hz) { inc = clampf(hz, 0.05f, 4000.0f) / kRate; }
    float process(float x)
    {
        phase += inc;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            held = x;
        }
        return held;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_SAMPLEHOLD_H */
