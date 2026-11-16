// dsp.h: what your algorithm remembers, and the entry points main.cpp calls.
#ifndef DSP_H
#define DSP_H

#include <stdint.h>

#include "params.h"
#include "blocks/osc.h"
#include "blocks/filter.h"
#include "blocks/slew.h"
#include "blocks/multiply.h"
#include "blocks/gain.h"

// Everything the algorithm keeps between calls lives here. The host gives the module one
// block of memory for it, so there is no heap: use arrays, and size them here.
struct Dsp {
    float gate;   // 1 while GATE is on: the Slew turns it into the envelope
    resona::blocks::Osc osc1;   // Oscillator
    resona::blocks::Filter filter1;   // Filter
    resona::blocks::Slew slew1;   // Slew
    resona::blocks::Multiply multiply1;   // Multiply
    resona::blocks::Gain gain1;   // Gain
};

void dsp_init(Dsp &d);                                  // once, when the module loads
void dsp_reset(Dsp &d);                                 // clear the signal history, keep the settings
void dsp_param(Dsp &d, uint32_t index, float value);    // a parameter changed (also once at start)
void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames);

#endif
