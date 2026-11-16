/*@block
 * kind      delay
 * title     Delay
 * short     DLY
 * category  Time
 * legacy    yes
 * summary   A feedback delay with a darkening tone control.
 *
 * in        in audio
 * in        mod cv
 * out       out audio
 *
 * param     time "Time" TIME ms 10 500 250 expose set_time
 * param     fdbk "Feedback" FDBK % 0 95 35 expose set_feedback
 * param     mix "Mix" MIX % 0 100 30 expose set_mix
 * param     tone "Tone" TONE Hz 500 16000 6000 hidden set_tone
 * option    max "Longest" default 250 values 100 250 500 labels "100 ms" "250 ms" "500 ms"
 *
 * type      Delay<{max*48}>
 * state     4*max*48 + 48
 */
#ifndef RESONA_BLOCK_DELAY_H
#define RESONA_BLOCK_DELAY_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A feedback delay. N is the longest delay in samples (the line's memory). */
template <uint32_t N>
struct Delay {
    float line[N];
    uint32_t write;
    float time;      /* samples, smoothed towards `target` */
    float target;
    float feedback;
    float mix;
    float tone;      /* one-pole coefficient of the feedback low-pass */
    float lp;

    void init() { write = 0u; time = target = 12000.0f; feedback = 0.35f; mix = 0.3f; tone = 0.5f; lp = 0.0f; }
    void reset()
    {
        for (uint32_t i = 0; i < N; ++i) line[i] = 0.0f;
        write = 0u;
        lp = 0.0f;
    }
    void set_time(float ms)
    {
        target = clampf(ms * (kRate / 1000.0f), 2.0f, (float)(N - 2u));
    }
    void set_feedback(float percent) { feedback = clampf(percent * 0.01f, 0.0f, 0.95f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }
    void set_tone(float hz)
    {
        const float x = clampf(hz, 100.0f, 20000.0f) * (2.0f * RESONA_PI / kRate);
        tone = x / (1.0f + x);
    }

    float run(float x, float scale)
    {
        time += (target * scale - time) * 0.0015f;
        float t = clampf(time, 1.0f, (float)(N - 2u));
        float r = (float)write - t;
        if (r < 0.0f) r += (float)N;
        const uint32_t i0 = (uint32_t)r;
        const float f = r - (float)i0;
        const uint32_t i1 = i0 + 1u >= N ? 0u : i0 + 1u;
        const float echo = line[i0] + f * (line[i1] - line[i0]);
        lp += tone * (echo - lp);
        lp = flush(lp);
        float w = x + lp * feedback;
        w = clampf(w, -4.0f, 4.0f);
        line[write] = flush(w);
        if (++write >= N) write = 0u;
        return x + lp * mix;
    }
    float process(float x) { return run(x, 1.0f); }
    /* `mod` wobbles the delay time by up to 5 %: tape-style pitch movement. */
    float process_cv(float x, float mod) { return run(x, 1.0f + 0.05f * clampf(mod, -1.0f, 1.0f)); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_DELAY_H */
