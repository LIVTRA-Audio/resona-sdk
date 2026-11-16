/* Helper for the blocks that share it; not a block of its own. */
#ifndef RESONA_BLOCK_POLYBLEP_H
#define RESONA_BLOCK_POLYBLEP_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* PolyBLEP residual: removes the aliasing of a step at phase 0 (t in [0, 1), dt = increment). */
inline float poly_blep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_POLYBLEP_H */
