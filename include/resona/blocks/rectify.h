/*@block
 * kind      rectify
 * title     Rectifier
 * short     RECT
 * category  Distortion
 * legacy    no
 * summary   Full-wave flips the negative half up (an octave higher on a tone); half-wave
 *           drops it.
 *
 * in        in audio
 * out       out audio
 *
 * param     mode "Mode" MODE - 0 1 0 hidden set_mode choices "Full-wave" "Half-wave"
 *
 * type      Rectifier
 * state     4
 */
#ifndef RESONA_BLOCK_RECTIFY_H
#define RESONA_BLOCK_RECTIFY_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A rectifier: full-wave takes the absolute value (an octave up on a tone), half-wave
 * keeps the positive half. */
struct Rectifier {
    uint32_t half;

    void init() { half = 0u; }
    void reset() {}
    void set_mode(float m) { half = m < 0.5f ? 0u : 1u; }
    float process(float x) const { return half ? (x > 0.0f ? x : 0.0f) : absf(x); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_RECTIFY_H */
