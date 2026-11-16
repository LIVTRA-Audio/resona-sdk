/*@block
 * kind      onepole
 * title     One-pole Filter
 * short     POLE
 * category  Tone
 * legacy    no
 * summary   A first-order low-pass or high-pass, 6 dB per octave. The part tone shelves,
 *           loop filters and smoothers are made of.
 *
 * in        in audio
 * out       out audio
 *
 * param     cutoff "Cutoff" CUTOFF Hz 1 12000 1000 expose set_cutoff
 * param     mode "Mode" MODE - 0 1 0 hidden set_mode choices "Low-pass" "High-pass"
 *
 * type      OnePole
 * state     24
 */
#ifndef RESONA_BLOCK_ONEPOLE_H
#define RESONA_BLOCK_ONEPOLE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A first-order low-pass or high-pass (bilinear transform, prewarped at the corner):
 * 6 dB per octave, the building block of tone shelves and of the filters in feedback
 * paths. LP: y = g/(1+g) (x + x1) + (1-g)/(1+g) y1; HP: y = 1/(1+g) (x - x1) + (1-g)/(1+g) y1,
 * with g = tan(pi fc / fs). */
struct OnePole {
    float x1, y1;
    float b, a;      /* feed-forward and feedback coefficients */
    float cutoff;
    uint32_t high;

    void coefficients()
    {
        const float g = tan_hz(cutoff);
        a = (1.0f - g) / (1.0f + g);
        b = high ? 1.0f / (1.0f + g) : g / (1.0f + g);
    }
    void init() { x1 = y1 = 0.0f; cutoff = 1000.0f; high = 0u; coefficients(); }
    void reset() { x1 = y1 = 0.0f; }
    void set_cutoff(float hz) { cutoff = clampf(hz, 1.0f, 12000.0f); coefficients(); }
    void set_mode(float m) { high = m < 0.5f ? 0u : 1u; coefficients(); }
    float process(float x)
    {
        const float y = b * (high ? x - x1 : x + x1) + a * y1;
        x1 = x;
        y1 = flush(y);
        return y;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_ONEPOLE_H */
