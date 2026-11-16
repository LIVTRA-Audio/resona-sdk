/* Helper for the blocks that share it; not a block of its own. */
#ifndef RESONA_BLOCK_MODLINE_H
#define RESONA_BLOCK_MODLINE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A modulated short delay with a few shared bits: the core of chorus and flanger. */
template <uint32_t N>
struct ModLine {
    float line[N];
    uint32_t write;

    void clear()
    {
        for (uint32_t i = 0; i < N; ++i) line[i] = 0.0f;
        write = 0u;
    }
    /* Read `delay` samples back (fractional, 1 .. N - 2). */
    float read(float delay) const
    {
        const float t = clampf(delay, 1.0f, (float)(N - 2u));
        float r = (float)write - t;
        if (r < 0.0f) r += (float)N;
        const uint32_t i0 = (uint32_t)r;
        const float f = r - (float)i0;
        const uint32_t i1 = i0 + 1u >= N ? 0u : i0 + 1u;
        return line[i0] + f * (line[i1] - line[i0]);
    }
    void push(float x)
    {
        line[write] = flush(clampf(x, -4.0f, 4.0f));
        if (++write >= N) write = 0u;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_MODLINE_H */
