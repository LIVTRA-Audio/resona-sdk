// dsp.cpp: UI Kit's sound is one Gain block (LEVEL); the point of the example is the screen,
// in ui.cpp. CUTOFF, RES and MIX only move the knobs.
#include "dsp.h"

#include <resona/math.h>

void dsp_init(Dsp &d)
{
    d.gain1.init();
    d.gain1.set_level(0.0f);   // level
}

void dsp_reset(Dsp &d)
{
    d.gain1.reset();
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_LEVEL:
        d.gain1.set_level(value);
        break;
    default:
        break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i) {
        const float input_out = in[i];  // Input
        const float gain1_out = d.gain1.process(input_out);  // Gain (LEVEL)
        out[i] = gain1_out;  // Output
    }
}
