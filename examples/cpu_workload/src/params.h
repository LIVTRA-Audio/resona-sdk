// params.h: the parameters of Tiny Synth, in one place.
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
    P_NOTE,
    P_GATE,
    P_WAVE,
    P_CUTOFF,
    P_RES,
    P_ATTACK,
    P_RELEASE,
    P_LEVEL,
    P_COUNT,
};

// name (12 characters at most), minimum, maximum, default, unit
const resona_param kParams[P_COUNT] = {
    {"NOTE", 24.0f, 96.0f, 57.0f, ""},
    {"GATE", 0.0f, 1.0f, 0.0f, ""},
    {"WAVE", 0.0f, 3.0f, 2.0f, ""},
    {"CUTOFF", 80.0f, 12000.0f, 1800.0f, "Hz"},
    {"RES", 0.0f, 95.0f, 35.0f, "%"},
    {"ATTACK", 1.0f, 1000.0f, 8.0f, "ms"},
    {"RELEASE", 5.0f, 2000.0f, 250.0f, "ms"},
    {"LEVEL", -24.0f, 0.0f, -6.0f, "dB"},
};

// How the host and the Lab show each parameter: NOTE in whole steps, GATE as a switch,
// WAVE by name, CUTOFF on a log scale. main.cpp passes this table with kParams.
const char *const kWaves[4] = {"SINE", "TRIANGLE", "SAW", "SQUARE"};

const resona_param_ext kExt[P_COUNT] = {
    {RESONA_PARAM_STEPPED, 1.0f, 0, nullptr},
    {RESONA_PARAM_TOGGLE, 0.0f, 0, nullptr},
    {RESONA_PARAM_CHOICE, 0.0f, 4, kWaves},
    {RESONA_PARAM_LOG, 0.0f, 0, nullptr},
    {0, 0.0f, 0, nullptr},
    {0, 0.0f, 0, nullptr},
    {0, 0.0f, 0, nullptr},
    {0, 0.0f, 0, nullptr},
};

#endif
