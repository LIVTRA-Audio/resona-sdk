// dsp.h: what your algorithm remembers, and the entry points main.cpp calls.
#ifndef DSP_H
#define DSP_H

#include <stdint.h>

#include "params.h"
#include "blocks/delay.h"

// Everything the algorithm keeps between calls lives here. The host gives the module one
// block of memory for it, so there is no heap: use arrays, and size them here.
struct Dsp {
    // add your own state, or press Add block: an LFO, a delay line, a filter ... are declared here
    resona::blocks::Delay<24000> delay1;   // Delay
};

void dsp_init(Dsp &d);                                  // once, when the module loads
void dsp_reset(Dsp &d);                                 // clear the signal history, keep the settings
void dsp_param(Dsp &d, uint32_t index, float value);    // a parameter changed (also once at start)
void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames);

#endif
