/*@block
 * kind      chorus
 * title     Chorus
 * short     CHOR
 * category  Modulation
 * legacy    yes
 * summary   Two detuned, slowly moving copies of the signal.
 *
 * in        in audio
 * in        mod cv
 * out       out audio
 *
 * param     rate "Rate" RATE Hz 0.05 5 0.8 expose set_rate
 * param     depth "Depth" DEPTH % 0 100 50 expose set_depth
 * param     mix "Mix" MIX % 0 100 50 expose set_mix
 *
 * type      Chorus
 * state     4*2048 + 56
 */
#ifndef RESONA_BLOCK_CHORUS_H
#define RESONA_BLOCK_CHORUS_H

#include "resona/blocks/common.h"
#include "resona/blocks/lfo.h"
#include "resona/blocks/modline.h"

namespace resona {
namespace blocks {

/* Two detuned voices (14 ms and 20 ms bases, up to +-5 ms of sweep). */
struct Chorus {
    ModLine<2048> line;
    Lfo lfo;
    float depth;
    float mix;

    void init() { line.clear(); lfo.init(); lfo.set_rate(0.8f); depth = 0.5f; mix = 0.5f; }
    void reset() { line.clear(); lfo.reset(); }
    void set_rate(float hz) { lfo.set_rate(hz); }
    void set_depth(float percent) { depth = clampf(percent * 0.01f, 0.0f, 1.0f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }

    float run(float x, float a, float b)
    {
        const float sweep = depth * 240.0f;  /* 5 ms */
        const float v1 = line.read(672.0f + sweep * a);
        const float v2 = line.read(960.0f + sweep * b);
        line.push(x);
        return x * (1.0f - 0.5f * mix) + (v1 + v2) * (0.5f * 0.8f * mix);
    }
    float process(float x)
    {
        const float a = lfo.at(0.0f);
        const float b = lfo.at(0.25f);
        lfo.process();
        return run(x, a, b);
    }
    float process_cv(float x, float mod) { return run(x, mod, -mod); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_CHORUS_H */
