// ui.h: the screen and the two encoders: every widget class of <resona/widgets.h>.
#ifndef UI_H
#define UI_H

#include <resona/module.h>
#include <resona/ui.h>
#include <resona/widgets.h>

#include "params.h"

struct Screen {
    // page 0: knobs (the four parameters)
    resona::ui::Knob cutoff, res, level, mix;
    // page 1: faders, meters, a toggle
    resona::ui::Fader attack, release, gain_fader;
    resona::ui::Meter in_meter, out_meter;
    resona::ui::Toggle gate;
    // page 2: tiles and a pad
    resona::ui::Steps steps;
    resona::ui::XYPad pad;
    resona::ui::Form form;   // holds them, and routes the encoders
};

void screen_init(Screen &s, const char *title);
void screen_set(Screen &s, uint32_t index, float value);   // a parameter changed from outside
void screen_tick(Screen &s);                               // once a block: eases the animation
void screen_draw(const Screen &s, const resona_ui *ui);
int screen_event(Screen &s, const resona_event &e);        // the index of the parameter changed, or -1
float screen_value(const Screen &s, uint32_t index);

#endif
