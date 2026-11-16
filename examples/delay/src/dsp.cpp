// dsp.cpp: Delay, a feedback echo: one Delay block (its line is in Dsp, owned by the host).
#include "dsp.h"

#include <resona/math.h>

void dsp_init(Dsp &d)
{
    d.delay1.init();
    d.delay1.set_time(250.0f);   // time
    d.delay1.set_feedback(35.0f);   // feedback
    d.delay1.set_mix(30.0f);   // mix
    d.delay1.set_tone(6000.0f);   // tone
}

void dsp_reset(Dsp &d)
{
    d.delay1.reset();
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_TIME:
        d.delay1.set_time(value);
        break;
    case P_FEEDBACK:
        d.delay1.set_feedback(value);
        break;
    case P_MIX:
        d.delay1.set_mix(value);
        break;
    default:
        break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i) {
        const float input_out = in[i];  // Input
        const float delay1_out = d.delay1.process(input_out);  // Delay
        out[i] = delay1_out;  // Output
    }
}
