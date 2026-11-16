/*
 * LVGL Demo: the optional LVGL 9 port (sdk/lib/lvgl_port) in a Box App.
 *
 *   Encoder 2 turns the LEVEL slider (its own group, always in edit mode).
 *   Encoder 1 moves the focus through the button list; a click picks the focused sound.
 *   Encoder 1 long press is LV_KEY_ESC.
 *
 * LVGL and its widgets are compiled into this App (see resona.project.json), which is what
 * makes it much larger than an App drawn with lib/widgets.
 *
 * GPL-3.0 licensed, see the SDK's LICENSE.
 */
#include <resona/platform.h>

#include "lvbox.h"

static const resona_platform *P;

typedef struct {
    lv_obj_t *level;  /* "LEVEL 40" */
    lv_obj_t *status; /* what the list picked */
    lv_obj_t *slider;
} State; /* LVGL callbacks reach it through their user data */

static void slider_changed(lv_event_t *e)
{
    State *s = (State *)lv_event_get_user_data(e);
    lv_label_set_text_fmt(s->level, "LEVEL %d", (int)lv_slider_get_value(s->slider));
}

static void sound_clicked(lv_event_t *e)
{
    State *s = (State *)lv_event_get_user_data(e);
    lv_obj_t *button = lv_event_get_target_obj(e);
    lv_obj_t *list = lv_obj_get_parent(button);

    lv_label_set_text_fmt(s->status, "%s", lv_list_get_button_text(list, button));
}

static void build_screen(State *s)
{
    static const struct {
        const char *icon, *name;
    } kSounds[] = {
        {LV_SYMBOL_AUDIO, "Clean"},   {LV_SYMBOL_CHARGE, "Crunch"}, {LV_SYMBOL_BELL, "Lead"},
        {LV_SYMBOL_LOOP, "Ambient"},  {LV_SYMBOL_MUTE, "Bypass"},
    };
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *title, *list;

    lv_theme_default_init(lvbox_display(), lv_palette_main(LV_PALETTE_CYAN),
                          lv_palette_main(LV_PALETTE_AMBER), true, LV_FONT_DEFAULT);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x07090C), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(scr);
    lv_label_set_text(title, "LVGL " LV_SYMBOL_RIGHT " RESONA BOX");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8E3D3), 0);
    lv_obj_set_pos(title, 10, 6);

    s->level = lv_label_create(scr);
    lv_obj_set_style_text_color(s->level, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(s->level, LV_ALIGN_TOP_RIGHT, -10, 6);

    /* E2: the slider lives in Encoder 2's group, which the port keeps in edit mode. */
    s->slider = lv_slider_create(scr);
    lv_obj_set_size(s->slider, 290, 8);
    lv_obj_align(s->slider, LV_ALIGN_TOP_MID, 0, 34);
    lv_slider_set_range(s->slider, 0, 100);
    lv_slider_set_value(s->slider, 40, LV_ANIM_OFF);
    lv_obj_add_event_cb(s->slider, slider_changed, LV_EVENT_VALUE_CHANGED, s);
    lv_group_add_obj(lvbox_group(RESONA_INPUT_CUSTOMIZE), s->slider);
    lv_label_set_text_fmt(s->level, "LEVEL %d", 40);

    /* E1: list buttons join the default group (Encoder 1) as they are created. */
    list = lv_list_create(scr);
    lv_obj_set_size(list, 196, 112);
    lv_obj_align(list, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    lv_obj_set_style_pad_row(list, 0, 0);
    for (unsigned i = 0; i < sizeof kSounds / sizeof kSounds[0]; ++i) {
        lv_obj_t *b = lv_list_add_button(list, kSounds[i].icon, kSounds[i].name);
        lv_obj_add_event_cb(b, sound_clicked, LV_EVENT_CLICKED, s);
    }

    s->status = lv_label_create(scr);
    lv_obj_set_width(s->status, 104);
    lv_label_set_long_mode(s->status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s->status, lv_color_hex(0xE0BE3B), 0);
    lv_label_set_text(s->status, "E1: pick\nE2: level");
    lv_obj_align(s->status, LV_ALIGN_BOTTOM_RIGHT, -6, -40);
}

static int init(void *state, const resona_platform *platform)
{
    State *s = (State *)state;

    P = platform;
    if (lvbox_init(P, 0) != RESONA_OK) {
        P->log("lvgl demo: not enough heap for LVGL");
        return -1;
    }
    build_screen(s);
    lvbox_refresh();
    return 0;
}

static void event(void *state, const resona_app_event *ev)
{
    (void)state;
    lvbox_event(ev);
}

static void frame(void *state, uint32_t ms)
{
    (void)state;
    (void)ms;
    lvbox_frame();
}

static const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    RESONA_APP_ID, RESONA_APP_NAME, RESONA_APP_VERSION, /* from resona.project.json */
    sizeof(State), 64u * 1024u, /* lvbox_heap_bytes(0): LVGL's heap and the render band */
    init, event, frame,
};

RESONA_EXPORT const resona_app *resona_app_get(void) { return &kApp; }
