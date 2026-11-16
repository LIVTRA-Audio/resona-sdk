/*@block
 * kind      phaser
 * title     Phaser
 * short     PHSR
 * category  Modulation
 * legacy    yes
 * summary   A chain of all-pass stages swept by an LFO.
 *
 * in        in audio
 * in        mod cv
 * out       out audio
 *
 * param     rate "Rate" RATE Hz 0.05 5 0.4 expose set_rate
 * param     depth "Depth" DEPTH % 0 100 70 expose set_depth
 * param     fdbk "Feedback" FDBK % 0 90 30 hidden set_feedback
 * param     mix "Mix" MIX % 0 100 50 hidden set_mix
 * option    stages "Stages" default 4 values 2 4 6 8
 *
 * type      Phaser<{stages}>
 * state     4*stages + 56
 */
#ifndef RESONA_BLOCK_PHASER_H
#define RESONA_BLOCK_PHASER_H

#include "resona/blocks/common.h"
#include "resona/blocks/lfo.h"

namespace resona {
namespace blocks {

/* A phaser: S first-order all-pass stages swept over about four octaves. */
template <uint32_t S>
struct Phaser {
    float z[S];
    Lfo lfo;
    float depth;
    float feedback;
    float mix;
    float last;
    float a;
    uint32_t tick;

    void init()
    {
        for (uint32_t i = 0; i < S; ++i) z[i] = 0.0f;
        lfo.init(); lfo.set_rate(0.4f);
        depth = 0.7f; feedback = 0.3f; mix = 0.5f; last = 0.0f; a = 0.5f; tick = 0u;
    }
    void reset()
    {
        for (uint32_t i = 0; i < S; ++i) z[i] = 0.0f;
        lfo.reset();
        last = 0.0f;
    }
    void set_rate(float hz) { lfo.set_rate(hz); }
    void set_depth(float percent) { depth = clampf(percent * 0.01f, 0.0f, 1.0f); }
    void set_feedback(float percent) { feedback = clampf(percent * 0.01f, 0.0f, 0.9f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }

    float run(float x, float m)
    {
        if ((tick++ & (kControlEvery - 1u)) == 0u) {
            /* 200 Hz .. about 3.2 kHz at full depth, swept exponentially */
            const float f = 200.0f * resona_exp2f(4.0f * depth * (0.5f + 0.5f * clampf(m, -1.0f, 1.0f)));
            const float t = tan_hz(f);
            a = (1.0f - t) / (1.0f + t);
        }
        float v = clampf(x + last * feedback, -4.0f, 4.0f);
        for (uint32_t i = 0; i < S; ++i) {
            const float y = a * v + z[i];
            z[i] = flush(v - a * y);
            v = y;
        }
        last = flush(v);
        return x * (1.0f - 0.5f * mix) + v * (0.5f * mix);
    }
    float process(float x) { return run(x, lfo.process()); }
    float process_cv(float x, float mod) { return run(x, mod); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_PHASER_H */
