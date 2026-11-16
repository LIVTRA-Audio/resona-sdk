/*@block
 * kind      mixer
 * title     Mixer
 * short     MIX
 * category  Level
 * legacy    no
 * summary   Crossfades between two signals: 0 % is A, 100 % is B.
 *
 * in        a audio
 * in        b audio
 * out       out audio
 *
 * param     mix "Mix" MIX % 0 100 50 expose set_mix
 *
 * type      Mixer
 * state     4
 */
#ifndef RESONA_BLOCK_MIXER_H
#define RESONA_BLOCK_MIXER_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Crossfade between two signals: 0 % is `a`, 100 % is `b`. */
struct Mixer {
    float mix;

    void init() { mix = 0.5f; }
    void reset() {}
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }
    float process(float a, float b) const { return a + (b - a) * mix; }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_MIXER_H */
