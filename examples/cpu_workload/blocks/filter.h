/*@block
 * kind      filter
 * title     Filter
 * short     FILT
 * category  Tone
 * legacy    no
 * summary   A resonant state-variable filter. The mod input sweeps the cutoff.
 *
 * in        in audio
 * in        mod cv moves cutoff octaves 2
 * out       out audio
 *
 * param     cutoff "Cutoff" CUTOFF Hz 50 12000 1000 expose set_cutoff
 * param     res "Resonance" RES % 0 100 20 hidden set_resonance
 * param     mode "Mode" MODE - 0 3 0 hidden set_mode choices "Low-pass" "High-pass" "Band-pass" "Notch"
 *
 * type      Filter
 * state     44
 */
#ifndef RESONA_BLOCK_FILTER_H
#define RESONA_BLOCK_FILTER_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A state-variable filter (topology-preserving). */
struct Filter {
    float ic1, ic2;
    float cutoff;       /* Hz, the setting */
    float k;            /* damping, 2 .. 0.1 */
    float g, a1, a2, a3;
    uint32_t mode;      /* 0 low-pass, 1 high-pass, 2 band-pass, 3 notch */
    uint32_t tick;

    void coefficients(float fc)
    {
        g = tan_hz(fc);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void init() { ic1 = 0.0f; ic2 = 0.0f; cutoff = 1000.0f; k = 1.0f; mode = 0u; tick = 0u; coefficients(cutoff); }
    void reset() { ic1 = 0.0f; ic2 = 0.0f; }
    void set_cutoff(float hz) { cutoff = clampf(hz, 20.0f, 12000.0f); coefficients(cutoff); }
    void set_resonance(float percent) { k = 2.0f * (1.0f - clampf(percent * 0.01f, 0.0f, 1.0f) * 0.95f); coefficients(cutoff); }
    void set_mode(float m) { mode = m < 0.5f ? 0u : (m < 1.5f ? 1u : (m < 2.5f ? 2u : 3u)); }

    float run(float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flush(2.0f * v1 - ic1);
        ic2 = flush(2.0f * v2 - ic2);
        return mode == 0u ? v2 : (mode == 1u ? x - k * v1 - v2 : (mode == 2u ? v1 : x - k * v1));
    }
    float process(float x) { return run(x); }
    /* `mod` sweeps the cutoff by up to two octaves each way. */
    float process_cv(float x, float mod)
    {
        if ((tick++ & (kControlEvery - 1u)) == 0u) {
            coefficients(cutoff * resona_exp2f(clampf(mod, -1.0f, 1.0f) * 2.0f));
        }
        return run(x);
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_FILTER_H */
