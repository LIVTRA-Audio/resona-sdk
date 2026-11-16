/*@block
 * kind      gain
 * title     Gain
 * short     GAIN
 * category  Level
 * legacy    no
 * summary   Turns the signal up or down.
 *
 * in        in audio
 * out       out audio
 *
 * param     level "Level" LEVEL dB -60 24 0 expose set_level
 *
 * type      Gain
 * state     4
 */
#ifndef RESONA_BLOCK_GAIN_H
#define RESONA_BLOCK_GAIN_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

struct Gain {
    float gain;

    void init() { gain = 1.0f; }
    void reset() {}
    void set_level(float db) { gain = resona_db_to_lin(db); }
    float process(float x) const { return x * gain; }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_GAIN_H */
