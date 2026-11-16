/*@block
 * kind      drive
 * title     Drive
 * short     DRV
 * category  Distortion
 * legacy    no
 * summary   A soft saturator.
 *
 * in        in audio
 * out       out audio
 *
 * param     drive "Drive" DRIVE dB 0 40 12 expose set_drive
 * param     level "Level" LEVEL dB -24 6 -6 expose set_level
 *
 * type      Drive
 * state     8
 */
#ifndef RESONA_BLOCK_DRIVE_H
#define RESONA_BLOCK_DRIVE_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A soft saturator: gain in, a smooth clip, gain out. */
struct Drive {
    float pre;
    float post;

    void init() { pre = 1.0f; post = 1.0f; }
    void reset() {}
    void set_drive(float db) { pre = resona_db_to_lin(db); }
    void set_level(float db) { post = resona_db_to_lin(db); }
    float process(float x) const
    {
        float v = x * pre;
        if (v > 3.0f) v = 3.0f;
        if (v < -3.0f) v = -3.0f;
        /* x (27 + x^2) / (27 + 9 x^2): tanh to within a few percent, one divide */
        return v * (27.0f + v * v) / (27.0f + 9.0f * v * v) * post;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_DRIVE_H */
