/*@block
 * kind      crush
 * title     Bit Crusher
 * short     CRSH
 * category  Distortion
 * legacy    no
 * summary   Fewer bits and a lower sample rate: lo-fi grit.
 *
 * in        in audio
 * out       out audio
 *
 * param     bits "Bits" BITS bit 1 16 8 expose set_bits
 * param     rate "Rate" RATE x 1 32 4 expose set_rate
 * param     mix "Mix" MIX % 0 100 100 hidden set_mix
 *
 * type      Crush
 * state     24
 */
#ifndef RESONA_BLOCK_CRUSH_H
#define RESONA_BLOCK_CRUSH_H

#include "resona/blocks/common.h"
#include "resona/blocks/floorsmall.h"

namespace resona {
namespace blocks {

/* Bit reduction and sample-rate reduction. */
struct Crush {
    float step;       /* quantiser step */
    float held;
    uint32_t every;
    uint32_t count;
    float mix;

    void init() { step = 1.0f / 128.0f; held = 0.0f; every = 4u; count = 0u; mix = 1.0f; }
    void reset() { held = 0.0f; count = 0u; }
    void set_bits(float bits)
    {
        const float b = clampf(bits, 1.0f, 16.0f);
        step = 2.0f / (resona_exp2f(b) - 1.0f + (b < 1.5f ? 1.0f : 0.0f));
    }
    void set_rate(float n) { every = (uint32_t)clampf(n, 1.0f, 64.0f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }
    float process(float x)
    {
        if (count == 0u) held = floor_small(clampf(x, -1.0f, 1.0f) / step + 0.5f) * step;
        if (++count >= every) count = 0u;
        return x + (held - x) * mix;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_CRUSH_H */
