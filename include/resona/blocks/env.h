/*@block
 * kind      env
 * title     Envelope
 * short     ENV
 * category  Dynamics
 * legacy    no
 * summary   Follows how loud a signal is and outputs it as a control (0 to 1). Patch it
 *           into a mod input.
 *
 * in        in audio
 * out       out cv
 *
 * param     attack "Attack" ATTACK ms 0.5 200 5 hidden set_attack
 * param     release "Release" REL ms 10 2000 150 expose set_release
 * param     gain "Gain" GAIN dB -12 36 6 expose set_gain
 *
 * type      Envelope
 * state     24
 */
#ifndef RESONA_BLOCK_ENV_H
#define RESONA_BLOCK_ENV_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* Follows the level of a signal: attack and release in milliseconds. Output 0 .. 1. */
struct Envelope {
    float level, att, rel, gain;

    static float coef(float ms) { return resona_exp2f(-1.442695f / (clampf(ms, 0.1f, 5000.0f) * 0.001f * kRate)); }
    void init() { level = 0.0f; att = coef(5.0f); rel = coef(150.0f); gain = 1.0f; }
    void reset() { level = 0.0f; }
    void set_attack(float ms) { att = coef(ms); }
    void set_release(float ms) { rel = coef(ms); }
    void set_gain(float db) { gain = resona_db_to_lin(db); }
    float process(float x)
    {
        const float a = absf(x);
        const float c = a > level ? att : rel;
        level = flush(c * level + (1.0f - c) * a);
        return clampf(level * gain, 0.0f, 1.0f);
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_ENV_H */
