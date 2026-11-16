/*@block
 * kind      value
 * title     Value
 * short     VAL
 * category  Source
 * legacy    no
 * summary   A constant control you set by hand. Patch it into a mod input, a Scale or a
 *           Multiply to fix an amount.
 *
 * out       out cv
 *
 * param     value "Value" VALUE % -100 100 0 expose set_value
 *
 * type      Value
 * state     4
 */
#ifndef RESONA_BLOCK_VALUE_H
#define RESONA_BLOCK_VALUE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A constant control, -100 % .. 100 % as -1 .. 1: a knob you can patch anywhere. */
struct Value {
    float v;

    void init() { v = 0.0f; }
    void reset() {}
    void set_value(float percent) { v = clampf(percent * 0.01f, -1.0f, 1.0f); }
    float process() const { return v; }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_VALUE_H */
