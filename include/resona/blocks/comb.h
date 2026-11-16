/*@block
 * kind      comb
 * title     Comb
 * short     COMB
 * category  Time
 * legacy    no
 * summary   A short delay fed back or forward: metallic resonance, the heart of a flanger
 *           or a reverb.
 *
 * in        in audio
 * out       out audio
 *
 * param     time "Time" TIME ms 0.1 40 4 expose set_time
 * param     gain "Gain" GAIN % -95 95 60 expose set_gain
 * param     mode "Mode" MODE - 0 1 0 hidden set_mode choices "Feedback" "Feed-forward"
 *
 * type      Comb<2048>
 * state     4*2048 + 32
 */
#ifndef RESONA_BLOCK_COMB_H
#define RESONA_BLOCK_COMB_H

#include "resona/blocks/common.h"

namespace resona {
namespace blocks {

/* A comb filter: the delayed signal fed back (0) or forward (1). The core of flanging,
 * of metallic resonances and of the reverb. N is the longest delay in samples. */
template <uint32_t N>
struct Comb {
    float line[N];
    uint32_t write;
    float delay;
    float gain;
    uint32_t mode;

    void init() { write = 0u; delay = 192.0f; gain = 0.6f; mode = 0u; }
    void reset()
    {
        for (uint32_t i = 0; i < N; ++i) line[i] = 0.0f;
        write = 0u;
    }
    void set_time(float ms) { delay = clampf(ms * (kRate / 1000.0f), 1.0f, (float)(N - 2u)); }
    void set_gain(float percent) { gain = clampf(percent * 0.01f, -0.97f, 0.97f); }
    void set_mode(float m) { mode = m < 0.5f ? 0u : 1u; }
    float process(float x)
    {
        float r = (float)write - delay;
        if (r < 0.0f) r += (float)N;
        const uint32_t i0 = (uint32_t)r;
        const float f = r - (float)i0;
        const uint32_t i1 = i0 + 1u >= N ? 0u : i0 + 1u;
        const float d = line[i0] + f * (line[i1] - line[i0]);
        const float y = x + gain * d;
        line[write] = flush(clampf(mode == 0u ? y : x, -4.0f, 4.0f));
        if (++write >= N) write = 0u;
        return clampf(y, -8.0f, 8.0f);
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_COMB_H */
