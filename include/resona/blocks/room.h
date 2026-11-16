/*@block
 * kind      room
 * title     Room Reverb
 * short     ROOM
 * category  Time
 * legacy    yes
 * summary   Freeverb: eight damped combs and four all-passes. Bright and roomy; bigger
 *           than the plate.
 *
 * in        in audio
 * out       out audio
 *
 * param     size "Size" SIZE % 0 100 50 expose set_size
 * param     damp "Damping" DAMP % 0 100 50 expose set_damping
 * param     mix "Mix" MIX % 0 100 30 expose set_mix
 *
 * type      Room
 * state     4*(12000 + 1701) + 120
 */
#ifndef RESONA_BLOCK_ROOM_H
#define RESONA_BLOCK_ROOM_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Freeverb: eight damped feedback combs in parallel into four all-passes in series. */
struct Room {
    static constexpr uint32_t C0 = 1215u, C1 = 1293u, C2 = 1390u, C3 = 1476u;
    static constexpr uint32_t C4 = 1548u, C5 = 1623u, C6 = 1695u, C7 = 1760u;
    static constexpr uint32_t A0 = 605u, A1 = 480u, A2 = 371u, A3 = 245u;
    float c0[C0], c1[C1], c2[C2], c3[C3], c4[C4], c5[C5], c6[C6], c7[C7];
    float a0[A0], a1[A1], a2[A2], a3[A3];
    uint32_t ci[8], ai[4];
    float store[8];
    float feedback, damp1, damp2, mix;

    void clear()
    {
        for (uint32_t i = 0; i < C0; ++i) c0[i] = 0.0f;
        for (uint32_t i = 0; i < C1; ++i) c1[i] = 0.0f;
        for (uint32_t i = 0; i < C2; ++i) c2[i] = 0.0f;
        for (uint32_t i = 0; i < C3; ++i) c3[i] = 0.0f;
        for (uint32_t i = 0; i < C4; ++i) c4[i] = 0.0f;
        for (uint32_t i = 0; i < C5; ++i) c5[i] = 0.0f;
        for (uint32_t i = 0; i < C6; ++i) c6[i] = 0.0f;
        for (uint32_t i = 0; i < C7; ++i) c7[i] = 0.0f;
        for (uint32_t i = 0; i < A0; ++i) a0[i] = 0.0f;
        for (uint32_t i = 0; i < A1; ++i) a1[i] = 0.0f;
        for (uint32_t i = 0; i < A2; ++i) a2[i] = 0.0f;
        for (uint32_t i = 0; i < A3; ++i) a3[i] = 0.0f;
        for (uint32_t i = 0; i < 8; ++i) { ci[i] = 0u; store[i] = 0.0f; }
        for (uint32_t i = 0; i < 4; ++i) ai[i] = 0u;
    }
    void init() { clear(); set_size(50.0f); set_damping(50.0f); mix = 0.3f; }
    void reset() { clear(); }
    /* room size 0 .. 100 %: feedback 0.7 .. 0.98 (Freeverb's own scaling) */
    void set_size(float percent) { feedback = clampf(percent * 0.01f, 0.0f, 1.0f) * 0.28f + 0.7f; }
    void set_damping(float percent)
    {
        damp1 = clampf(percent * 0.01f, 0.0f, 1.0f) * 0.4f;
        damp2 = 1.0f - damp1;
    }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }

    float comb(float *buf, uint32_t n, uint32_t &i, float &s, float x)
    {
        const float out = buf[i];
        s = flush(out * damp2 + s * damp1);
        buf[i] = flush(clampf(x + s * feedback, -4.0f, 4.0f));
        if (++i >= n) i = 0u;
        return out;
    }
    float allpass(float *buf, uint32_t n, uint32_t &i, float x)
    {
        const float b = buf[i];
        buf[i] = flush(clampf(x + b * 0.5f, -8.0f, 8.0f));
        if (++i >= n) i = 0u;
        return b - x;
    }
    float process(float x)
    {
        const float in = x * 0.015f;
        float s = comb(c0, C0, ci[0], store[0], in);
        s += comb(c1, C1, ci[1], store[1], in);
        s += comb(c2, C2, ci[2], store[2], in);
        s += comb(c3, C3, ci[3], store[3], in);
        s += comb(c4, C4, ci[4], store[4], in);
        s += comb(c5, C5, ci[5], store[5], in);
        s += comb(c6, C6, ci[6], store[6], in);
        s += comb(c7, C7, ci[7], store[7], in);
        s = allpass(a0, A0, ai[0], s);
        s = allpass(a1, A1, ai[1], s);
        s = allpass(a2, A2, ai[2], s);
        s = allpass(a3, A3, ai[3], s);
        return x + (s * 3.0f - x) * mix;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_ROOM_H */
