/*@block
 * kind      unitdelay
 * title     Unit Delay
 * short     Z1
 * category  Time
 * legacy    no
 * summary   One sample of delay (z^-1). A cable may loop back only through this block: put
 *           it in a feedback path.
 *
 * in        in audio
 * out       out audio
 *
 * type      UnitDelay
 * state     4
 * late      yes
 */
#ifndef RESONA_BLOCK_UNITDELAY_H
#define RESONA_BLOCK_UNITDELAY_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A one-sample delay (z^-1). The only block a cable may loop back through: its output is
 * what its input was one sample ago, so a loop that passes through it is computable. The
 * generated code reads `out()` where the block sits in the order and `write()` at the end
 * of the sample. */
struct UnitDelay {
    float y;

    void init() { y = 0.0f; }
    void reset() { y = 0.0f; }
    float out() const { return y; }
    void write(float x) { y = flush(clampf(x, -8.0f, 8.0f)); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_UNITDELAY_H */
