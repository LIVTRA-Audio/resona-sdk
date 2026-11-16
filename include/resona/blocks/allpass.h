/*@block
 * kind      allpass
 * title     All-pass
 * short     APF
 * category  Time
 * legacy    no
 * summary   Passes every frequency but smears them in time. Stack a few to diffuse a
 *           sound.
 *
 * in        in audio
 * out       out audio
 *
 * param     time "Time" TIME ms 0.5 40 10 expose set_time
 * param     gain "Gain" GAIN % 0 95 60 expose set_gain
 *
 * type      Allpass<2048>
 * state     4*2048 + 32
 */
#ifndef RESONA_BLOCK_ALLPASS_H
#define RESONA_BLOCK_ALLPASS_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A Schroeder all-pass: flat magnitude, smeared in time. Chain a few to build a diffuser. */
template <uint32_t N>
struct Allpass {
    float line[N];
    uint32_t write;
    uint32_t delay;
    float gain;

    void init() { write = 0u; delay = 480u; gain = 0.6f; }
    void reset()
    {
        for (uint32_t i = 0; i < N; ++i) line[i] = 0.0f;
        write = 0u;
    }
    void set_time(float ms) { delay = (uint32_t)clampf(ms * (kRate / 1000.0f), 1.0f, (float)(N - 1u)); }
    void set_gain(float percent) { gain = clampf(percent * 0.01f, 0.0f, 0.95f); }
    float process(float x)
    {
        const uint32_t r = write >= delay ? write - delay : write + N - delay;
        const float d = line[r];
        const float w = clampf(x + gain * d, -8.0f, 8.0f);
        line[write] = flush(w);
        if (++write >= N) write = 0u;
        return d - gain * w;
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_ALLPASS_H */
