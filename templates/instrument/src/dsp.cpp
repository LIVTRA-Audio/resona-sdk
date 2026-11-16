// dsp.cpp: the algorithm of @NAME@. This is the file to write in.
//
// An instrument makes sound without an input: dsp_process() fills `out` and ignores `in`.
// The audio is mono at 48 kHz in blocks of 1 to 256 samples (64, 128 or 256 on the Resona Box). No malloc, no new, no C library
// math: use <resona/math.h> (resona_sinf, resona_expf, resona_db_to_lin ...).
#include "dsp.h"

#include <resona/math.h>

void dsp_init(Dsp &d)
{
    d.phase = 0.0f;
    d.step = 220.0f / 48000.0f;
    d.level = 0.25f;
}

void dsp_reset(Dsp &d)
{
    d.phase = 0.0f;
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_FREQ:
        d.step = value / 48000.0f;            // cycles per sample
        break;
    case P_LEVEL:
        d.level = resona_db_to_lin(value);
        break;
    default:
        break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    (void)in;
    for (uint32_t i = 0; i < frames; ++i) {
        out[i] = d.level * resona_sinf(6.2831853f * d.phase);
        d.phase += d.step;
        if (d.phase >= 1.0f) {
            d.phase -= 1.0f;
        }
    }
}
