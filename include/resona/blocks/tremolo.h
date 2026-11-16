/*@block
 * kind      tremolo
 * title     Tremolo
 * short     TREM
 * category  Modulation
 * legacy    yes
 * summary   Moves the level up and down. Its own LFO, or the mod input.
 *
 * in        in audio
 * in        mod cv
 * out       out audio
 *
 * param     rate "Rate" RATE Hz 0.1 20 5 expose set_rate
 * param     depth "Depth" DEPTH % 0 100 50 expose set_depth
 *
 * type      Tremolo
 * state     40
 */
#ifndef RESONA_BLOCK_TREMOLO_H
#define RESONA_BLOCK_TREMOLO_H

#include "resona/blocks/common.h"
#include "resona/blocks/lfo.h"

namespace resona {
namespace blocks {

/* Amplitude modulation: out = in * (1 - depth * (0.5 + 0.5 * lfo)). */
struct Tremolo {
    Lfo lfo;
    float depth;

    void init() { lfo.init(); lfo.set_rate(5.0f); depth = 0.5f; }
    void reset() { lfo.reset(); }
    void set_rate(float hz) { lfo.set_rate(hz); }
    void set_depth(float percent) { depth = clampf(percent * 0.01f, 0.0f, 1.0f); }
    float process_cv(float x, float mod) const { return x * (1.0f - depth * (0.5f + 0.5f * mod)); }
    float process(float x) { return process_cv(x, lfo.process()); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_TREMOLO_H */
