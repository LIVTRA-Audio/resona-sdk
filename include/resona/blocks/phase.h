/*@block
 * kind      phase
 * title     Phase Stage
 * short     PHAS
 * category  Tone
 * legacy    no
 * summary   A first-order all-pass: the level stays, the phase turns around the frequency
 *           set. Several in series with a swept frequency make a phaser. The mod input can
 *           also set the pole K directly.
 *
 * in        in audio
 * in        mod cv moves freq octaves sweep when control 0
 * out       out audio
 *
 * param     freq "Frequency" FREQ Hz 50 12000 1000 expose set_freq
 * param     sweep "Sweep" SWEEP oct 0 4 1.5 hidden set_sweep
 * param     control "Mod input is" CTRL - 0 1 0 hidden set_control choices "Frequency sweep" "Coefficient K"
 *
 * type      PhaseStage
 * state     24
 */
#ifndef RESONA_BLOCK_PHASE_H
#define RESONA_BLOCK_PHASE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A first-order all-pass: every frequency passes at full level, the phase turns by 90
 * degrees at `freq`. Several in series with a swept `freq` are a phaser. The mod input
 * moves `freq` by up to `sweep` octaves either way. */
struct PhaseStage {
    float z, a, freq, sweep;
    uint32_t tick;
    uint32_t direct;   /* 1: the mod input is the coefficient itself (the pole position K) */
    float x1, y1;      /* the direct form's history (coefficient mode) */

    static float coefficient(float fc)
    {
        const float t = tan_hz(fc);
        return (t - 1.0f) / (t + 1.0f);
    }
    void init() { z = 0.0f; freq = 1000.0f; sweep = 1.5f; tick = 0u; direct = 0u; x1 = y1 = 0.0f; a = coefficient(freq); }
    void reset() { z = 0.0f; x1 = y1 = 0.0f; }
    void set_freq(float hz) { freq = clampf(hz, 20.0f, 12000.0f); if (!direct) a = coefficient(freq); }
    void set_sweep(float octaves) { sweep = clampf(octaves, 0.0f, 4.0f); }
    void set_control(float m) { direct = m < 0.5f ? 0u : 1u; a = direct ? 0.0f : coefficient(freq); }
    float run(float x)
    {
        if (direct) {
            /* y[n] = x[n-1] + K (y[n-1] - x[n]) with the K of this sample in both terms: the
             * form the hardware phasers use, which differs from the transposed form only
             * while K is moving. */
            const float k = -a;
            const float y = x1 + k * (y1 - x);
            x1 = x;
            y1 = flush(y);
            return y;
        }
        const float y = a * x + z;
        z = flush(x - a * y);
        return y;
    }
    float process(float x) { return run(x); }
    float process_cv(float x, float mod)
    {
        if (direct) {
            /* A(z) = (-K + z^-1) / (1 - K z^-1): K is the pole, updated every sample */
            a = -clampf(mod, -0.99997f, 0.99997f);
        } else if ((tick++ & (kControlEvery - 1u)) == 0u) {
            a = coefficient(freq * resona_exp2f(clampf(mod, -1.0f, 1.0f) * sweep));
        }
        return run(x);
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_PHASE_H */
