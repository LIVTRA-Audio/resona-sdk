/*@block
 * kind      eq
 * title     EQ Band
 * short     EQ
 * category  Tone
 * legacy    no
 * summary   One parametric band: a bell, or a low or high shelf (RBJ biquad).
 *
 * in        in audio
 * out       out audio
 *
 * param     freq "Frequency" FREQ Hz 40 12000 1000 expose set_freq
 * param     gain "Gain" GAIN dB -18 18 6 expose set_gain
 * param     q "Width (Q)" Q - 0.3 8 1 hidden set_q
 * param     shape "Shape" SHAPE - 0 2 0 hidden set_shape choices "Bell" "Low shelf" "High shelf"
 *
 * type      EqBand
 * state     56
 */
#ifndef RESONA_BLOCK_EQ_H
#define RESONA_BLOCK_EQ_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* One biquad EQ band (RBJ cookbook): bell, low shelf or high shelf. */
struct EqBand {
    float b0, b1, b2, a1, a2, z1, z2;
    float freq, gain, q;
    uint32_t shape;   /* 0 bell, 1 low shelf, 2 high shelf */

    void design()
    {
        const float f = clampf(freq, 20.0f, 0.45f * kRate);
        const float w0 = 2.0f * RESONA_PI * f / kRate;
        const float cs = resona_cosf(w0), sn = resona_sinf(w0);
        const float A = resona_exp2f(gain * (1.0f / 40.0f) * 3.321928095f);   /* 10^(dB/40) */
        float c0, c1, c2, d0, d1, d2;
        if (shape == 0u) {
            const float alpha = sn / (2.0f * clampf(q, 0.1f, 20.0f));
            c0 = 1.0f + alpha * A; c1 = -2.0f * cs; c2 = 1.0f - alpha * A;
            d0 = 1.0f + alpha / A; d1 = -2.0f * cs; d2 = 1.0f - alpha / A;
        } else {
            const float alpha = sn * 0.70710678f;   /* shelf slope S = 1 */
            const float t = 2.0f * resona_sqrtf(A) * alpha;
            if (shape == 1u) {
                c0 = A * ((A + 1.0f) - (A - 1.0f) * cs + t);
                c1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
                c2 = A * ((A + 1.0f) - (A - 1.0f) * cs - t);
                d0 = (A + 1.0f) + (A - 1.0f) * cs + t;
                d1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
                d2 = (A + 1.0f) + (A - 1.0f) * cs - t;
            } else {
                c0 = A * ((A + 1.0f) + (A - 1.0f) * cs + t);
                c1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
                c2 = A * ((A + 1.0f) + (A - 1.0f) * cs - t);
                d0 = (A + 1.0f) - (A - 1.0f) * cs + t;
                d1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
                d2 = (A + 1.0f) - (A - 1.0f) * cs - t;
            }
        }
        const float inv = 1.0f / d0;
        b0 = c0 * inv; b1 = c1 * inv; b2 = c2 * inv; a1 = d1 * inv; a2 = d2 * inv;
    }
    void init() { z1 = z2 = 0.0f; freq = 1000.0f; gain = 0.0f; q = 1.0f; shape = 0u; design(); }
    void reset() { z1 = z2 = 0.0f; }
    void set_freq(float hz) { freq = hz; design(); }
    void set_gain(float db) { gain = clampf(db, -24.0f, 24.0f); design(); }
    void set_q(float v) { q = v; design(); }
    void set_shape(float s) { shape = s < 0.5f ? 0u : (s < 1.5f ? 1u : 2u); design(); }
    float process(float x)
    {
        const float y = b0 * x + z1;
        z1 = flush(b1 * x - a1 * y + z2);
        z2 = flush(b2 * x - a2 * y);
        return y;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_EQ_H */
