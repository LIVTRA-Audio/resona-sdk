/*@block
 * kind      dyncurve
 * title     Dynamics Curve
 * short     DYN
 * category  Dynamics
 * legacy    no
 * summary   Turns a level (from an Envelope) into a gain (for a Multiply). Compress lowers
 *           what is loud; Gate lowers what is quiet.
 *
 * in        in cv label "level"
 * out       out cv label "gain"
 *
 * param     thresh "Threshold" THRESH dB -80 0 -20 expose set_threshold
 * param     ratio "Ratio" RATIO :1 1 20 4 expose set_ratio
 * param     knee "Knee" KNEE dB 0 24 6 hidden set_knee
 * param     mode "Mode" MODE - 0 1 0 hidden set_mode choices "Compress" "Gate"
 *
 * type      DynCurve
 * state     28
 */
#ifndef RESONA_BLOCK_DYNCURVE_H
#define RESONA_BLOCK_DYNCURVE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* The static curve of a dynamics processor: a level in (the Envelope block's output)
 * becomes a gain out, to feed a Multiply. Compress turns what is above the threshold
 * down by the ratio; Gate turns what is below it down, so quiet parts fall away. A soft
 * knee blends the corner. The gain is smoothed so a change never clicks. */
struct DynCurve {
    float thresh;   /* dB */
    float ratio;
    float knee;     /* dB */
    uint32_t gate;
    float target, gain;
    uint32_t tick;

    void init() { thresh = -20.0f; ratio = 4.0f; knee = 6.0f; gate = 0u; target = gain = 1.0f; tick = 0u; }
    void reset() { target = gain = 1.0f; }
    void set_threshold(float db) { thresh = db; }
    void set_ratio(float r) { ratio = clampf(r, 1.0f, 40.0f); }
    void set_knee(float db) { knee = clampf(db, 0.0f, 24.0f); }
    void set_mode(float m) { gate = m < 0.5f ? 0u : 1u; }

    /* Gain change in dB (zero or negative) for a level `db` decibels full scale. */
    float reduction(float db) const
    {
        const float over = (gate ? thresh - db : db - thresh);   /* distance past the corner */
        const float slope = gate ? (1.0f - ratio) : (1.0f / ratio - 1.0f);
        float r;
        if (2.0f * over < -knee) {
            r = 0.0f;
        } else if (knee > 0.0f && 2.0f * absf(over) <= knee) {
            const float k = over + knee * 0.5f;
            r = slope * k * k / (2.0f * knee);
        } else {
            r = over > 0.0f ? slope * over : 0.0f;
        }
        return clampf(r, -90.0f, 0.0f);
    }
    float process(float level)
    {
        if ((tick++ & (kControlEvery - 1u)) == 0u) {
            target = resona_db_to_lin(reduction(resona_lin_to_db(absf(level))));
        }
        gain = flush(gain + (target - gain) * 0.1f);
        return gain;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_DYNCURVE_H */
