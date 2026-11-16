/*@block
 * kind      slew
 * title     Slew
 * short     SLEW
 * category  Control
 * legacy    no
 * summary   Smooths a control signal: it rises and falls no faster than the times set.
 *           Rounds a square wave, makes a glide.
 *
 * in        in cv
 * out       out cv
 *
 * param     rise "Rise" RISE ms 0.1 2000 10 expose set_rise
 * param     fall "Fall" FALL ms 0.1 5000 100 expose set_fall
 *
 * type      Slew
 * state     12
 */
#ifndef RESONA_BLOCK_SLEW_H
#define RESONA_BLOCK_SLEW_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A slew limiter: a one-pole smoother with separate rise and fall times. Smooths a
 * control, rounds off a square wave, makes a portamento. */
struct Slew {
    float y, up, down;

    static float alpha(float ms)
    {
        return 1.0f - resona_exp2f(-1.442695f / (clampf(ms, 0.05f, 10000.0f) * 0.001f * kRate));
    }
    void init() { y = 0.0f; up = alpha(10.0f); down = alpha(100.0f); }
    void reset() { y = 0.0f; }
    void set_rise(float ms) { up = alpha(ms); }
    void set_fall(float ms) { down = alpha(ms); }
    float process(float x)
    {
        y = flush(y + (x > y ? up : down) * (x - y));
        return y;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_SLEW_H */
