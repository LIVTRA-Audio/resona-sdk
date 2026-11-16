// ui.cpp: every widget class of <resona/widgets.h> on three pages.
//
//   SELECTOR  turn: move the selection      click: next page
//   CUSTOMIZE turn: change the selected     click: press it (toggle, step, default)
//                                           long press: reset it
//
// How little code the widgets take: set each one up in screen_init(), hand the events to
// the form.
#include "ui.h"

using namespace resona::ui;

void screen_init(Screen &s, const char *title)
{
    // Page 0: four knobs. The cutoff is on a log scale; its pointer and the dots follow.
    s.cutoff.init(46, 72, "CUTOFF", 20.0f, 12000.0f, 1000.0f, "HZ").log_scale();
    s.res.init(122, 72, "RES", 0.0f, 100.0f, 20.0f, "%");
    s.level.init(198, 72, "LEVEL", -24.0f, 12.0f, 0.0f, "DB").decimals(1);
    s.mix.init(274, 72, "MIX", 0.0f, 100.0f, 50.0f, "%");

    // Page 1: three faders, two level towers and a switch.
    s.attack.init(14, 60, "ATTACK", 1.0f, 200.0f, 20.0f, "MS").log_scale();
    s.release.init(78, 60, "RELEASE", 10.0f, 2000.0f, 340.0f, "MS").log_scale();
    s.gain_fader.init(142, 60, "GAIN", -24.0f, 12.0f, 0.0f, "DB").decimals(1);
    s.in_meter.init(214, 114, "IN");
    s.out_meter.init(250, 114, "OUT");
    s.gate.init(282, 46, "GATE", true);

    // Page 2: sixteen steps and an XY pad.
    s.steps.init(150, 50, 16, 8);
    s.steps.mask = 0x1111u | 0x0404u | 0x8000u;
    s.steps.playhead = 5;
    s.pad.init(64, 52, "CUTOFF / RES", 40);
    s.pad.set(0.30f, 0.65f);

    s.form.init(title);
    s.form.add(&s.cutoff, 0);
    s.form.add(&s.res, 0);
    s.form.add(&s.level, 0);
    s.form.add(&s.mix, 0);
    s.form.add(&s.attack, 1);
    s.form.add(&s.release, 1);
    s.form.add(&s.gain_fader, 1);
    s.form.add(&s.gate, 1);
    s.form.add(&s.in_meter, 1);
    s.form.add(&s.out_meter, 1);
    s.form.add(&s.steps, 2);
    s.form.add(&s.pad, 2);

    s.in_meter.set(0.62f);
    s.out_meter.set(0.82f);
    s.out_meter.set_peak(0.9f);
    s.form.settle();
}

void screen_set(Screen &s, uint32_t index, float value)
{
    // The host's parameters and the first four knobs are the same four numbers.
    Knob *k[P_COUNT] = {&s.cutoff, &s.res, &s.level, &s.mix};
    if (index < P_COUNT) {
        k[index]->set(value);
    }
}

void screen_tick(Screen &s)
{
    s.form.tick();
}

void screen_draw(const Screen &s, const resona_ui *ui)
{
    s.form.draw(ui);
}

int screen_event(Screen &s, const resona_event &e)
{
    return s.form.handle(e);
}

float screen_value(const Screen &s, uint32_t index)
{
    return s.form.value((int)index);
}
