/*@block
 * kind      scale
 * title     Scale
 * short     SCL
 * category  Control
 * legacy    no
 * summary   Scales and offsets a control signal: use it to set how far a modulator
 *           reaches.
 *
 * in        in cv
 * out       out cv
 *
 * param     amount "Amount" AMOUNT % -200 200 100 expose set_amount
 * param     offset "Offset" OFFSET % -100 100 0 hidden set_offset
 *
 * type      Scale
 * state     8
 */
#ifndef RESONA_BLOCK_SCALE_H
#define RESONA_BLOCK_SCALE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Scales and offsets a control signal: out = in * amount + offset. */
struct Scale {
    float amount, offset;

    void init() { amount = 1.0f; offset = 0.0f; }
    void reset() {}
    void set_amount(float percent) { amount = percent * 0.01f; }
    void set_offset(float percent) { offset = percent * 0.01f; }
    float process(float x) const { return clampf(x * amount + offset, -4.0f, 4.0f); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_SCALE_H */
