/*@block
 * kind      fold
 * title     Wavefolder
 * short     FOLD
 * category  Distortion
 * legacy    no
 * summary   Folds the signal back on itself when it goes past full scale, for a metallic,
 *           rich tone.
 *
 * in        in audio
 * out       out audio
 *
 * param     drive "Drive" DRIVE dB 0 30 12 expose set_drive
 * param     level "Level" LEVEL dB -24 6 -6 expose set_level
 *
 * type      Fold
 * state     8
 */
#ifndef RESONA_BLOCK_FOLD_H
#define RESONA_BLOCK_FOLD_H

#include "resona/blocks/common.h"
#include "resona/blocks/floorsmall.h"

namespace resona {
namespace blocks {

/* A wavefolder: the signal, pushed past full scale, folds back instead of clipping. */
struct Fold {
    float pre, post;

    void init() { pre = 1.0f; post = 1.0f; }
    void reset() {}
    void set_drive(float db) { pre = resona_db_to_lin(db); }
    void set_level(float db) { post = resona_db_to_lin(db); }
    float process(float x) const
    {
        const float v = clampf(x * pre, -60.0f, 60.0f);
        const float u = (v + 1.0f) * 0.25f;
        const float f = u - floor_small(u + 0.5f);   /* -0.5 .. 0.5 */
        return (4.0f * absf(f) - 1.0f) * post;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_FOLD_H */
