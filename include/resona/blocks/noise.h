/*@block
 * kind      noise
 * title     Noise
 * short     NOIS
 * category  Source
 * legacy    no
 * summary   White or pink noise. Through a filter it is wind and snare; through Sample &
 *           Hold it is random steps.
 *
 * out       out audio
 *
 * param     level "Level" LEVEL dB -60 0 -12 expose set_level
 * param     color "Colour" COLOR - 0 1 0 hidden set_color choices "White" "Pink"
 *
 * type      Noise
 * state     72
 */
#ifndef RESONA_BLOCK_NOISE_H
#define RESONA_BLOCK_NOISE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* White or pink noise at a set level. White is a xorshift32 generator. Pink is the
 * Voss-McCartney sum: twelve held values, row k refreshed every 2^k samples, plus one
 * fresh white sample; flat to about 25 Hz and falling 3 dB per octave above it. */
struct Noise {
    uint32_t rng;
    uint32_t counter;
    float rows[12];
    float sum;
    float level;
    uint32_t pink;

    float white()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (float)(int32_t)rng * (1.0f / 2147483648.0f);
    }
    void clear()
    {
        rng = 2463534242u;
        counter = 0u;
        sum = 0.0f;
        for (uint32_t i = 0; i < 12u; ++i) rows[i] = 0.0f;
    }
    void init() { clear(); level = resona_db_to_lin(-12.0f); pink = 0u; }
    void reset() { clear(); }
    void set_level(float db) { level = resona_db_to_lin(db); }
    void set_color(float c) { pink = c < 0.5f ? 0u : 1u; }
    float process()
    {
        const float w = white();
        if (pink == 0u) return w * level;
        uint32_t c = ++counter;
        uint32_t row = 0u;
        while ((c & 1u) == 0u && row < 11u) {
            c >>= 1;
            ++row;
        }
        sum -= rows[row];
        rows[row] = white();
        sum += rows[row];
        return clampf((sum + w) * (1.0f / 6.0f), -1.0f, 1.0f) * level;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_NOISE_H */
