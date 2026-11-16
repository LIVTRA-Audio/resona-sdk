// params.h: the parameters of UI Kit, in one place.
//
// To add one: put a name in the enum and a line in kParams, in the same order. The screen
// grows a knob for it, the host and the Lab app show it as a control, and dsp_param() in
// dsp.cpp is called whenever it changes. A module can have up to 12.
#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

#include <resona/module.h>

// One name per parameter, in the order of kParams. Keep P_COUNT last.
enum Param : uint32_t {
    P_CUTOFF,
    P_RES,
    P_LEVEL,
    P_MIX,
    P_COUNT,
};

// name (12 characters at most), minimum, maximum, default, unit
const resona_param kParams[P_COUNT] = {
    {"CUTOFF", 20.0f, 12000.0f, 1000.0f, "Hz"},
    {"RES", 0.0f, 100.0f, 20.0f, "%"},
    {"LEVEL", -24.0f, 12.0f, 0.0f, "dB"},
    {"MIX", 0.0f, 100.0f, 50.0f, "%"},
};

#endif
