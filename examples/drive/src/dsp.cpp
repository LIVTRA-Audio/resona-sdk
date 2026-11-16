// dsp.cpp: Drive, a soft saturator (the Drive block) with a dry/wet Mixer and an output Gain.
#include "dsp.h"

#include <resona/math.h>

void dsp_init(Dsp &d)
{
    d.drive1.init();
    d.drive1.set_drive(12.0f);   // drive
    d.drive1.set_level(0.0f);   // level
    d.mixer1.init();
    d.mixer1.set_mix(50.0f);   // mix
    d.gain1.init();
    d.gain1.set_level(0.0f);   // level
}

void dsp_reset(Dsp &d)
{
    d.drive1.reset();
    d.mixer1.reset();
    d.gain1.reset();
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_DRIVE:
        d.drive1.set_drive(value);
        break;
    case P_MIX:
        d.mixer1.set_mix(value);
        break;
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
        const float drive1_out = d.drive1.process(input_out);  // Drive
        const float mixer1_out = d.mixer1.process(input_out, drive1_out);  // Mixer: dry against driven
        const float gain1_out = d.gain1.process(mixer1_out);  // Gain: the output level
        out[i] = gain1_out;  // Output
    }
}
