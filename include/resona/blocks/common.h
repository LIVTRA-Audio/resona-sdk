/* Shared by every block: the sample rate, the control-rate step, and the few functions a
 * block needs that the device build cannot get from a C library. Include it from a block
 * file; <resona/blocks.h> includes it too. */
#ifndef RESONA_BLOCKS_COMMON_H
#define RESONA_BLOCKS_COMMON_H

#include <stdint.h>

#include "resona/math.h"

namespace resona {
namespace blocks {

constexpr float kRate = 48000.0f;          /* RESONA_SAMPLE_RATE */
constexpr uint32_t kControlEvery = 8u;     /* samples between control-rate updates */

inline float flush(float x) { return resona_fabsf(x) < 1.0e-20f ? 0.0f : x; }
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float absf(float x) { return resona_fabsf(x); }

/* sin(2*pi*phase) for phase in [0, 1): a parabola with one correction step,
 * within 0.1 % of a sine, and no libm. */
inline float sin_unit(float phase)
{
    const float x = phase * 2.0f - 1.0f;
    float y = 4.0f * x * (1.0f - absf(x));
    y = 0.225f * (y * absf(y) - y) + y;
    return -y;
}

/* tan(pi * fc / rate) for fc up to about 12 kHz (a series, no libm): x + x^3/3 + 2x^5/15
 * + 17x^7/315 + 62x^9/2835, within 0.1 % at 12 kHz and 1e-5 below 4 kHz. */
inline float tan_hz(float fc)
{
    const float x = clampf(fc, 1.0f, 12000.0f) * (RESONA_PI / kRate);
    const float x2 = x * x;
    return x * (1.0f + x2 * (0.333333333f + x2 * (0.133333333f + x2 * (0.053968254f + x2 * 0.021869489f))));
}

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCKS_COMMON_H */
