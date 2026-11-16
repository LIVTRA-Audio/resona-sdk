/*@block
 * kind      delayline
 * title     Delay Line
 * short     DLN
 * category  Time
 * legacy    no
 * summary   The signal a set time ago, and only that: mix it with the dry signal yourself.
 *           Feedback makes echoes; a modulated short line is the heart of a chorus, a
 *           flanger and a vibrato.
 *
 * in        in audio
 * in        mod cv label "time" moves time by depth
 * out       out audio
 *
 * param     time "Time" TIME ms 0.02 500 20 expose set_time
 * param     fdbk "Feedback" FDBK % -95 95 0 hidden set_feedback
 * param     depth "Mod depth" MOD ms 0 500 0 expose set_mod_depth
 * param     damp "Damping" DAMP Hz 500 20000 20000 hidden set_damping
 * option    max "Longest, ms" default 50 values 10 50 250 500
 *
 * type      DelayLine<{max*48}>
 * state     4*max*48 + 32
 */
#ifndef RESONA_BLOCK_DELAYLINE_H
#define RESONA_BLOCK_DELAYLINE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A delay line and nothing else: the signal `time` ms ago, with optional feedback and a
 * low-pass in the feedback path. It outputs only the delayed signal (mix it with the dry
 * signal with a Mixer). The mod input moves the time by up to `depth` ms, either way.
 * N is the longest delay in samples. */
template <uint32_t N>
struct DelayLine {
    float line[N];
    uint32_t write;
    float base;      /* samples, smoothed towards `target` */
    float target;
    float depth;     /* samples of time change for a mod of 1 */
    float feedback;
    float damp;      /* one-pole coefficient of the feedback low-pass, 1 = off */
    float lp;
    uint32_t primed; /* the time glides only once audio is running */

    void clear()
    {
        for (uint32_t i = 0; i < N; ++i) line[i] = 0.0f;
        write = 0u;
        lp = 0.0f;
    }
    void init()
    {
        clear();
        base = target = 480.0f;
        depth = 0.0f;
        feedback = 0.0f;
        damp = 1.0f;
        primed = 0u;
    }
    void reset()
    {
        clear();
        base = target;
        primed = 0u;
    }
    void set_time(float ms)
    {
        target = clampf(ms * (kRate / 1000.0f), 1.0f, (float)(N - 2u));
        if (!primed) base = target;
    }
    void set_mod_depth(float ms) { depth = clampf(ms, 0.0f, 500.0f) * (kRate / 1000.0f); }
    void set_feedback(float percent) { feedback = clampf(percent * 0.01f, -0.95f, 0.95f); }
    void set_damping(float hz)
    {
        if (hz >= 19999.0f) {
            damp = 1.0f;
        } else {
            const float x = clampf(hz, 100.0f, 20000.0f) * (2.0f * RESONA_PI / kRate);
            damp = x / (1.0f + x);
        }
    }

    float run(float x, float mod)
    {
        primed = 1u;
        base += (target - base) * 0.002f;
        const float t = clampf(base + depth * mod, 1.0f, (float)(N - 2u));
        float r = (float)write - t;
        if (r < 0.0f) r += (float)N;
        const uint32_t i0 = (uint32_t)r;
        const float f = r - (float)i0;
        const uint32_t i1 = i0 + 1u >= N ? 0u : i0 + 1u;
        const float echo = line[i0] + f * (line[i1] - line[i0]);
        lp = flush(lp + damp * (echo - lp));
        line[write] = flush(clampf(x + lp * feedback, -4.0f, 4.0f));
        if (++write >= N) write = 0u;
        return echo;
    }
    float process(float x) { return run(x, 0.0f); }
    float process_cv(float x, float mod) { return run(x, clampf(mod, -1.0f, 1.0f)); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_DELAYLINE_H */
