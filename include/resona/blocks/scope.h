/*@block
 * kind      scope
 * title     Waveform
 * short     WAVE
 * category  View
 * legacy    no
 * summary   Shows the signal's waveform, live in the Flow page. Changes nothing.
 *
 * in        in audio
 *
 * type      Scope
 * state     0
 */
#ifndef RESONA_BLOCK_SCOPE_H
#define RESONA_BLOCK_SCOPE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

// A view block only looks: it has no output, so it cannot change the sound. In the Lab's
// desktop build it keeps the last 2048 samples for the Flow page to draw; on the device it is
// empty and costs nothing.
struct Scope {
#ifdef RESONA_LAB_HOST
    float lab_ring[2048];
    uint32_t lab_pos;

    void init() { reset(); }
    void reset()
    {
        for (uint32_t k = 0; k < 2048u; ++k) lab_ring[k] = 0.0f;
        lab_pos = 0;
    }
    void process(float x)
    {
        lab_ring[lab_pos] = x;
        lab_pos = (lab_pos + 1u) & 2047u;
    }
#else
    void init() {}
    void reset() {}
    void process(float) {}
#endif
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_SCOPE_H */
