// dsp.cpp: the algorithm of @NAME@. This is the file to write in.
//
// The audio is mono at 48 kHz and arrives in blocks of 1 to 256 samples (64, 128 or
// 256 on the Resona Box, as its owner sets; 64 is about every 1.3 ms). Keep dsp_process() short and predictable: no malloc, no new, no C library math (use
// <resona/math.h>: resona_sinf, resona_expf, resona_db_to_lin ...), and no waiting.
#include "dsp.h"

#include <resona/math.h>

void dsp_init(Dsp &d)
{
    d.gain = 1.0f;
}

void dsp_reset(Dsp &d)
{
    (void)d;          // clear delay lines and filter states here
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_GAIN:
        d.gain = resona_db_to_lin(value);   // dB -> a multiplier
        break;
    default:
        break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i) {
        out[i] = in[i] * d.gain;
    }
}
