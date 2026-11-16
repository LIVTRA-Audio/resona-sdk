/*@block
 * kind      lfo
 * title     LFO
 * short     LFO
 * category  Source
 * legacy    no
 * summary   A slow oscillator. Patch it into a block's mod input.
 *
 * out       out cv
 *
 * param     rate "Rate" RATE Hz 0.05 200 1 expose set_rate
 * param     shape "Shape" SHAPE - 0 4 0 hidden set_shape choices "Sine" "Triangle" "Saw" "Square" "Random"
 * param     phase "Start phase" PHASE % 0 100 0 hidden set_phase
 *
 * type      Lfo
 * state     32
 */
#ifndef RESONA_BLOCK_LFO_H
#define RESONA_BLOCK_LFO_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A low-frequency oscillator. Output in [-1, 1]. The phase is a 64-bit integer (one cycle is
 * 2^64), the way a hardware DSP counts it: adding the same step every sample never drifts. A
 * float phase does, and a pitch shifter's ramp, which steps by a few parts in ten thousand
 * per sample, shows it as a wrong ratio. */
struct Lfo {
    uint64_t acc;
    uint64_t step;
    uint32_t shape; /* 0 sine, 1 triangle, 2 saw, 3 square, 4 random (sample and hold) */
    uint32_t rng;
    float held;
    float offset;   /* start phase of the waveform, 0 .. 1 of a cycle */

    /* hz / rate * 2^64 from a 32-bit float without a 64-bit conversion: the integer part of
     * hz / rate * 2^32, then the fraction of that scaled by 2^32. */
    static uint64_t step_for(float hz)
    {
        const float t = clampf(hz, 0.0f, 12000.0f) / kRate * 4294967296.0f;   /* < 2^30 */
        const uint32_t hi = (uint32_t)t;
        const uint32_t lo = (uint32_t)((t - (float)hi) * 4294967296.0f);
        return ((uint64_t)hi << 32) | (uint64_t)lo;
    }
    void init() { acc = 0u; step = step_for(1.0f); shape = 0u; rng = 12345u; held = 0.0f; offset = 0.0f; }
    void reset() { acc = 0u; rng = 12345u; held = 0.0f; }
    void set_rate(float hz) { step = step_for(hz); }
    void set_phase(float percent) { offset = clampf(percent * 0.01f, 0.0f, 0.9999f); }
    void set_shape(float s)
    {
        shape = s < 0.5f ? 0u : (s < 1.5f ? 1u : (s < 2.5f ? 2u : (s < 3.5f ? 3u : 4u)));
    }

    float at(float off) const
    {
        float p = (float)(uint32_t)(acc >> 40) * (1.0f / 16777216.0f) + off;
        if (p >= 1.0f) p -= 1.0f;
        switch (shape) {
        case 1u: return p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f);
        case 2u: return 2.0f * p - 1.0f;
        case 3u: return p < 0.5f ? 1.0f : -1.0f;
        case 4u: return held;
        default: return sin_unit(p);
        }
    }
    float process()
    {
        const float v = at(offset);
        const uint64_t before = acc;
        acc += step;
        if (acc < before) {
            rng = rng * 1664525u + 1013904223u;   /* a new value each cycle */
            held = (float)(rng >> 8) * (2.0f / 16777216.0f) - 1.0f;
        }
        return v;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_LFO_H */
