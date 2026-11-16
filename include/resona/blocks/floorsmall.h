/* Helper for the blocks that share it; not a block of its own. */
#ifndef RESONA_BLOCK_FLOORSMALL_H
#define RESONA_BLOCK_FLOORSMALL_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

inline float floor_small(float v)
{
    const int i = (int)v;
    return (float)(v < (float)i ? i - 1 : i);
}

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_FLOORSMALL_H */
