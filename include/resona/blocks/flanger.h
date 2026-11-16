/*@block
 * kind      flanger
 * title     Flanger
 * short     FLNG
 * category  Modulation
 * legacy    yes
 * summary   A swept comb filter with feedback.
 *
 * in        in audio
 * in        mod cv
 * out       out audio
 *
 * param     rate "Rate" RATE Hz 0.05 5 0.3 expose set_rate
 * param     depth "Depth" DEPTH % 0 100 70 expose set_depth
 * param     fdbk "Feedback" FDBK % -90 90 40 expose set_feedback
 * param     mix "Mix" MIX % 0 100 50 hidden set_mix
 *
 * type      Flanger
 * state     4*512 + 72
 */
#ifndef RESONA_BLOCK_FLANGER_H
#define RESONA_BLOCK_FLANGER_H

#include "resona/blocks/common.h"
#include "resona/blocks/lfo.h"
#include "resona/blocks/modline.h"

namespace resona {
namespace blocks {

/* A swept comb: 0.4 ms to 4 ms, with feedback (negative gives the hollow, inverted sound). */
struct Flanger {
    ModLine<512> line;
    Lfo lfo;
    float depth;
    float feedback;
    float mix;
    float last;

    void init() { line.clear(); lfo.init(); lfo.set_rate(0.3f); depth = 0.7f; feedback = 0.4f; mix = 0.5f; last = 0.0f; }
    void reset() { line.clear(); lfo.reset(); last = 0.0f; }
    void set_rate(float hz) { lfo.set_rate(hz); }
    void set_depth(float percent) { depth = clampf(percent * 0.01f, 0.0f, 1.0f); }
    void set_feedback(float percent) { feedback = clampf(percent * 0.01f, -0.95f, 0.95f); }
    void set_mix(float percent) { mix = clampf(percent * 0.01f, 0.0f, 1.0f); }

    float run(float x, float m)
    {
        const float d = 19.0f + depth * 172.0f * (0.5f + 0.5f * clampf(m, -1.0f, 1.0f));
        const float wet = line.read(d);
        last = flush(wet);
        line.push(x + last * feedback);
        return (x + wet * mix) * (1.0f / (1.0f + 0.5f * mix));
    }
    float process(float x) { return run(x, lfo.process()); }
    float process_cv(float x, float mod) { return run(x, mod); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_FLANGER_H */
