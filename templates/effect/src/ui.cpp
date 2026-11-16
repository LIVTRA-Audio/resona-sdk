// ui.cpp: the screen. One knob per parameter, four to a page.
//
//   SELECTOR  turn: move the selection       click: next page
//   CUSTOMIZE turn: change the selected      click: press it     long press: reset it
//
// To draw something of your own, add to screen_draw(): the draw calls on `ui` are in
// <resona/module.h>, the ready-made widgets (Knob, Fader, Meter, Toggle, Steps, XYPad) in
// <resona/widgets.h>.
//
// When params.h says where each knob sits (P_SLOTS and its kSlot table, which a Flow writes),
// knob i goes to kSlot[i] (index 0-11 = slot 1-12, four to a page) instead of the next place in
// order.
#include "ui.h"

// Knob `i` at place `at`: four places to a page, side by side.
static void place_knob(Screen &s, uint32_t i, uint32_t at)
{
    s.knob[i].init(46 + 76 * (int32_t)(at % 4u), 84, kParams[i].name, kParams[i].min,
                   kParams[i].max, kParams[i].def, kParams[i].unit);
    if (kParams[i].min > 0.0f && kParams[i].max >= 20.0f * kParams[i].min) {
        s.knob[i].log_scale();    // a wide positive range reads better on a log scale
    }
    s.form.add(&s.knob[i], (int)(at / 4u));
}

#ifdef P_SLOTS
// The knobs join the form in slot order, so the form's widget `id` is the parameter in the
// id-th lowest slot.
static int slot_param(int id)
{
    for (uint32_t i = 0; i < P_COUNT; ++i) {
        int below = 0;
        for (uint32_t j = 0; j < P_COUNT; ++j) {
            below += kSlot[j] < kSlot[i];
        }
        if (below == id) {
            return (int)i;
        }
    }
    return -1;
}

// Whether a knob sits on `page`. A page can be empty when slots are chosen, and the form keeps
// its selection on another page's knob there, which CUSTOMIZE must not change unseen.
static bool page_has_knob(int page)
{
    for (uint32_t i = 0; i < P_COUNT; ++i) {
        if ((int)(kSlot[i] / 4u) == page) {
            return true;
        }
    }
    return false;
}
#endif

void screen_init(Screen &s, const char *title)
{
    s.form.init(title);
#ifdef P_SLOTS
    for (uint32_t slot = 0; slot < 12u; ++slot) {
        for (uint32_t i = 0; i < P_COUNT; ++i) {
            if (kSlot[i] == slot) {
                place_knob(s, i, slot);
            }
        }
    }
#else
    for (uint32_t i = 0; i < P_COUNT; ++i) {
        place_knob(s, i, i);
    }
#endif
    s.form.settle();
}

void screen_set(Screen &s, uint32_t index, float value)
{
    if (index < P_COUNT) {
        s.knob[index].set(value);
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
#ifdef P_SLOTS
    if (e.input == RESONA_INPUT_CUSTOMIZE && !page_has_knob(s.form.page())) {
        return -1;
    }
    const int changed = s.form.handle(e);
    return changed < 0 ? changed : slot_param(changed);
#else
    return s.form.handle(e);
#endif
}

float screen_value(const Screen &s, uint32_t index)
{
    return s.knob[index].value();
}
