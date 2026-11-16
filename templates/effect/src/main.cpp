// main.cpp: the glue between the module interface and your code. You rarely change it.
//
//   dsp.cpp    the algorithm: dsp_init, dsp_param, dsp_process
//   ui.cpp     the screen and the encoders
//   params.h   the parameters
#include <resona/math.h>
#include <resona/module.h>
#include <resona/parameters.h>
#include <resona/ui.h>

#include "dsp.h"
#include "ui.h"

namespace {

struct State {
    Dsp dsp;
    resona_parameter_inbox changes;
    Screen screen;
};

void init(void *s, const resona_host *)
{
    State *st = static_cast<State *>(s);
    st->changes = {};
    dsp_init(st->dsp);
    screen_init(st->screen, "@TITLE@");
    for (uint32_t i = 0; i < P_COUNT; ++i) {
        dsp_param(st->dsp, i, kParams[i].def);
    }
}

void reset(void *s)
{
    dsp_reset(static_cast<State *>(s)->dsp);
}

void set_param(void *s, uint32_t index, float value)
{
    State *st = static_cast<State *>(s);
    if (index < P_COUNT) {
        resona_parameter_publish(&st->changes, index, value);
        screen_set(st->screen, index, value);
    }
}

void process(void *s, const float *const *inputs, float *const *outputs, uint32_t frames)
{
    const float *in = inputs[0];   // mono: one channel
    float *out = outputs[0];
    State *st = static_cast<State *>(s);
    float values[12];
    const uint32_t pending = resona_parameter_consume(&st->changes, values);
    for (uint32_t i=0; i<P_COUNT; ++i)
        if (pending & (1u<<i)) dsp_param(st->dsp, i, values[i]);
    dsp_process(st->dsp, in, out, frames);
    screen_tick(st->screen);
}

void ui_draw(void *s, const resona_ui *ui)
{
    screen_draw(static_cast<const State *>(s)->screen, ui);
}

void on_event(void *s, const resona_event *e)
{
    State *st = static_cast<State *>(s);
    const int changed = screen_event(st->screen, *e);
    if (changed >= 0 && changed < (int)P_COUNT) {
        resona_parameter_publish(&st->changes, (uint32_t)changed, screen_value(st->screen, (uint32_t)changed));
    }
}

float get_param(const void *s, uint32_t index)
{
    const State *st = static_cast<const State *>(s);
    return index < (uint32_t)P_COUNT ? screen_value(st->screen, index) : 0.0f;
}

const resona_module kModule = {
    RESONA_MODULE_ABI, sizeof(resona_module),
    "@ID@", "@NAME@", "0.1.0",
    RESONA_LAYOUT_MONO, RESONA_RATE_48000,
    sizeof(State), P_COUNT, kParams, nullptr,
    init, reset, set_param, process, ui_draw, on_event, get_param,
};

}  // namespace

extern "C" RESONA_EXPORT const resona_module *resona_module_get(void)
{
    return &kModule;
}
