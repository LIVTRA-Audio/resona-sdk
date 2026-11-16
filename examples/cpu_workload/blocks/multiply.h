/*@block
 * kind      multiply
 * title     Multiply
 * short     MUL
 * category  Level
 * legacy    no
 * summary   Multiplies two signals: an oscillator gives ring modulation, an envelope gives
 *           a VCA.
 *
 * in        a audio
 * in        b audio label "gain"
 * out       out audio
 *
 * type      Multiply
 * state     4
 */
#ifndef RESONA_BLOCK_MULTIPLY_H
#define RESONA_BLOCK_MULTIPLY_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Two signals multiplied: ring modulation with an oscillator, a VCA with an envelope. */
struct Multiply {
    void init() {}
    void reset() {}
    float process(float a, float b) const { return a * b; }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_MULTIPLY_H */
