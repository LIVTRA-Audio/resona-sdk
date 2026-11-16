/*@block
 * kind      osc
 * title     Oscillator
 * short     OSC
 * category  Source
 * legacy    no
 * summary   An audio oscillator: sine, triangle, band-limited saw and square. The pitch
 *           input bends it an octave.
 *
 * in        mod cv label "pitch" moves freq octaves 1
 * out       out audio
 *
 * param     freq "Pitch" PITCH Hz 20 5000 220 expose set_freq
 * param     shape "Shape" SHAPE - 0 3 0 hidden set_shape choices "Sine" "Triangle" "Saw" "Square"
 * param     level "Level" LEVEL dB -40 0 -12 hidden set_level
 *
 * type      Osc
 * state     32
 */
#ifndef RESONA_BLOCK_OSC_H
#define RESONA_BLOCK_OSC_H

#include "resona/blocks/common.h"
#include "resona/blocks/polyblep.h"

namespace resona {
namespace blocks {

/* An audio-rate oscillator: sine, triangle, and band-limited saw and square. The mod input
 * moves the pitch by up to an octave each way. */
struct Osc {
    float phase;
    float hz;
    float level;
    float scale;       /* pitch factor from the mod input, refreshed every few samples */
    uint32_t shape;    /* 0 sine, 1 triangle, 2 saw, 3 square */
    uint32_t tick;

    void init() { phase = 0.0f; hz = 220.0f; level = 0.25f; scale = 1.0f; shape = 0u; tick = 0u; }
    void reset() { phase = 0.0f; }
    void set_freq(float f) { hz = clampf(f, 1.0f, 12000.0f); }
    void set_shape(float s) { shape = s < 0.5f ? 0u : (s < 1.5f ? 1u : (s < 2.5f ? 2u : 3u)); }
    void set_level(float db) { level = resona_db_to_lin(db); }

    float run()
    {
        const float dt = clampf(hz * scale, 1.0f, 16000.0f) / kRate;
        const float t = phase;
        float y;
        switch (shape) {
        case 1u: y = t < 0.25f ? 4.0f * t : (t < 0.75f ? 2.0f - 4.0f * t : 4.0f * t - 4.0f); break;
        case 2u: y = 2.0f * t - 1.0f - poly_blep(t, dt); break;
        case 3u: {
            float u = t + 0.5f;
            if (u >= 1.0f) u -= 1.0f;
            y = (t < 0.5f ? 1.0f : -1.0f) + poly_blep(t, dt) - poly_blep(u, dt);
            break;
        }
        default: y = sin_unit(t); break;
        }
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        return y * level;
    }
    float process() { return run(); }
    float process_cv(float mod)
    {
        if ((tick++ & (kControlEvery - 1u)) == 0u) scale = resona_exp2f(clampf(mod, -1.0f, 1.0f));
        return run();
    }
};

}  // namespace blocks
}  // namespace resona

#endif /* RESONA_BLOCK_OSC_H */
