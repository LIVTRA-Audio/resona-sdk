/*@block
 * kind      reverb
 * title     Plate Reverb
 * short     RVB
 * category  Time
 * legacy    yes
 * summary   A compact four-line feedback network with diffusion and damping: smooth and
 *           dense.
 *
 * in        in audio
 * out       out audio
 *
 * param     decay "Decay" DECAY s 0.2 10 2 expose set_decay
 * param     damp "Damping" DAMP % 0 100 40 expose set_damping
 * param     mix "Mix" MIX % 0 100 30 expose set_mix
 *
 * type      Reverb
 * state     4*(1087 + 1307 + 1523 + 1741 + 142 + 107) + 72
 */
#ifndef RESONA_BLOCK_REVERB_H
#define RESONA_BLOCK_REVERB_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A compact reverb: two input diffusers into a four-line feedback delay network
 * with a Hadamard mix and a damping low-pass in each line. About 24 KB. */
struct Reverb {
    static constexpr uint32_t L0 = 1087u, L1 = 1307u, L2 = 1523u, L3 = 1741u;
    static constexpr uint32_t D0 = 142u, D1 = 107u;
    float l0[L0], l1[L1], l2[L2], l3[L3];
    float d0[D0], d1[D1];
    uint32_t i0, i1, i2, i3, j0, j1;
    float lp[4];
    float gain[4];
    float damp;   /* low-pass coefficient: 1 is bright, small is dark */
    float mix;

    void set_decay(float seconds)
    {
        const float t = clampf(seconds, 0.1f, 20.0f);
        /* 60 dB of decay over t seconds: g = 10^(-3 L / (t rate)) */
        const float k = -9.96578428f / (t * kRate);
        gain[0] = resona_exp2f(k * (float)L0);
        gain[1] = resona_exp2f(k * (float)L1);
        gain[2] = resona_exp2f(k * (float)L2);
        gain[3] = resona_exp2f(k * (float)L3);
    }
    void init() { clear(); damp = 0.6f; mix = 0.3f; set_decay(2.0f); }
    void reset() { clear(); }
    void clear()
    {
        for (uint32_t i = 0; i < L0; ++i) l0[i] = 0.0f;
        for (uint32_t i = 0; i < L1; ++i) l1[i] = 0.0f;
        for (uint32_t i = 0; i < L2; ++i) l2[i] = 0.0f;
        for (uint32_t i = 0; i < L3; ++i) l3[i] = 0.0f;
        for (uint32_t i = 0; i < D0; ++i) d0[i] = 0.0f;
        for (uint32_t i = 0; i < D1; ++i) d1[i] = 0.0f;
        i0 = i1 = i2 = i3 = j0 = j1 = 0u;
        lp[0] = lp[1] = lp[2] = lp[3] = 0.0f;
    }
    void set_damping(float percent) { damp = 1.0f - 0.92f * clampf(percent * 0.01f, 0.0f, 1.0f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }

    float process(float x)
    {
        /* two Schroeder all-pass diffusers */
        float w = x + 0.6f * d0[j0];
        float y = d0[j0] - 0.6f * w;
        d0[j0] = flush(w);
        if (++j0 >= D0) j0 = 0u;
        w = y + 0.6f * d1[j1];
        y = d1[j1] - 0.6f * w;
        d1[j1] = flush(w);
        if (++j1 >= D1) j1 = 0u;
        const float in = y * 0.3f;

        const float o0 = l0[i0], o1 = l1[i1], o2 = l2[i2], o3 = l3[i3];
        lp[0] = flush(lp[0] + damp * (o0 - lp[0]));
        lp[1] = flush(lp[1] + damp * (o1 - lp[1]));
        lp[2] = flush(lp[2] + damp * (o2 - lp[2]));
        lp[3] = flush(lp[3] + damp * (o3 - lp[3]));
        const float v0 = lp[0] * gain[0], v1 = lp[1] * gain[1];
        const float v2 = lp[2] * gain[2], v3 = lp[3] * gain[3];
        l0[i0] = flush(clampf(in + 0.5f * (v0 + v1 + v2 + v3), -4.0f, 4.0f));
        l1[i1] = flush(clampf(in + 0.5f * (v0 - v1 + v2 - v3), -4.0f, 4.0f));
        l2[i2] = flush(clampf(in + 0.5f * (v0 + v1 - v2 - v3), -4.0f, 4.0f));
        l3[i3] = flush(clampf(in + 0.5f * (v0 - v1 - v2 + v3), -4.0f, 4.0f));
        if (++i0 >= L0) i0 = 0u;
        if (++i1 >= L1) i1 = 0u;
        if (++i2 >= L2) i2 = 0u;
        if (++i3 >= L3) i3 = 0u;
        const float wet = (o0 - o1 + o2 - o3) * 0.6f;
        return x + (wet - x) * mix;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_REVERB_H */
