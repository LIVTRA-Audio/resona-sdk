/*@block
 * kind      clip
 * title     Hard Clipper
 * short     CLIP
 * category  Distortion
 * legacy    no
 * summary   Gain in, then a flat ceiling: everything past it is cut off. Harsher than the
 *           soft saturator.
 *
 * in        in audio
 * out       out audio
 *
 * param     drive "Drive" DRIVE dB 0 40 12 expose set_drive
 * param     ceiling "Ceiling" CEIL dB -24 0 -6 expose set_ceiling
 *
 * type      Clip
 * state     8
 */
#ifndef RESONA_BLOCK_CLIP_H
#define RESONA_BLOCK_CLIP_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A hard clipper: gain in, then a flat ceiling. */
struct Clip {
    float pre, ceiling;

    void init() { pre = 1.0f; ceiling = 0.5f; }
    void reset() {}
    void set_drive(float db) { pre = resona_db_to_lin(db); }
    void set_ceiling(float db) { ceiling = clampf(resona_db_to_lin(db), 0.001f, 4.0f); }
    float process(float x) const { return clampf(x * pre, -ceiling, ceiling); }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_CLIP_H */
