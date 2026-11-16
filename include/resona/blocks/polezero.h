/*@block
 * kind      polezero
 * title     Pole-Zero Filter
 * short     PZ
 * category  Tone
 * legacy    no
 * summary   A first-order section set by where its zero and its pole are, and the gain at
 *           high frequencies. One zero and one pole make a shelf, a damping filter or a
 *           tilt; a zero at 0 Hz is a DC blocker.
 *
 * in        in audio
 * out       out audio
 *
 * param     zero "Zero" ZERO Hz 0 12000 200 expose set_zero
 * param     pole "Pole" POLE Hz 1 12000 200 expose set_pole
 * param     gain "HF gain" GAIN dB -24 24 0 hidden set_gain
 * param     design "Design" DESIGN - 0 1 0 hidden set_design choices "Pre-warped" "Plain K = pi f / fs"
 *
 * type      PoleZero
 * state     48
 */
#ifndef RESONA_BLOCK_POLEZERO_H
#define RESONA_BLOCK_POLEZERO_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A first-order section given by where its zero and its pole are: H(s) = A (s + wz) / (s + wp),
 * A being the gain at high frequencies (the gain at DC is A wz / wp). One zero and one pole make
 * a low or high shelf, a damping filter, a 6 dB tilt. `plain` places the corners with
 * K = pi f / fs, no pre-warping, the way some hardware designs do. A zero at 0 Hz is a DC
 * blocker. */
struct PoleZero {
    float x1, y1;
    float b0, b1, a1;
    float zero, pole, gain;
    uint32_t plain;

    static float warp(float f, bool plain_)
    {
        if (f <= 0.0f) return 0.0f;
        return plain_ ? clampf(f, 0.0f, 23000.0f) * (RESONA_PI / kRate) : tan_hz(f);
    }
    void coefficients()
    {
        const float kz = warp(zero, plain != 0u);
        const float kp = warp(pole, plain != 0u);
        const float g = gain / (1.0f + kp);
        b0 = g * (1.0f + kz);
        b1 = -g * (1.0f - kz);
        a1 = (1.0f - kp) / (1.0f + kp);
    }
    void init() { x1 = y1 = 0.0f; zero = 200.0f; pole = 200.0f; gain = 1.0f; plain = 0u; coefficients(); }
    void reset() { x1 = y1 = 0.0f; }
    void set_zero(float hz) { zero = hz; coefficients(); }
    void set_pole(float hz) { pole = hz; coefficients(); }
    void set_gain(float db) { gain = resona_db_to_lin(db); coefficients(); }
    void set_design(float m) { plain = m < 0.5f ? 0u : 1u; coefficients(); }
    float process(float x)
    {
        const float y = b0 * x + b1 * x1 + a1 * y1;
        x1 = x;
        y1 = flush(y);
        return y;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_POLEZERO_H */
