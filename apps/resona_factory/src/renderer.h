#ifndef BOX_UI_RENDERER_H
#define BOX_UI_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

#define BOX_UI_WIDTH       320U
#define BOX_UI_HEIGHT      172U
#define BOX_UI_ROLE_COUNT  5U  /* built-in pedal artworks */
#define BOX_UI_MAX_EFFECTS 6U  /* tiles: the five-effect factory chain plus the EFFECT tile */
#define BOX_UI_VISIBLE_EFFECTS 5U
#define BOX_UI_MAX_ROWS    16U

#define BOX_UI_GLOBAL_EQ_BANDS 5U
#define BOX_PRESET_COUNT 50U
#define BOX_PRESET_NAME_LEN 16U

typedef enum {
    BOX_VIEW_MAIN = 0,
    BOX_VIEW_EDITOR,
    BOX_VIEW_SETTINGS,
    BOX_VIEW_GLOBAL_EQ,
    BOX_VIEW_PICKER, /* the preset's effect: rows like Settings, label = name, value = kind */
} box_view_t;

typedef enum {
    BOX_EDITOR_FOCUS_BLOCK = 0,
    BOX_EDITOR_FOCUS_STATE,
    BOX_EDITOR_FOCUS_TYPE,
    BOX_EDITOR_FOCUS_MODEL,
    BOX_EDITOR_FOCUS_PARAMETER,
} box_editor_focus_t;

typedef struct {
    const char *label;
    const char *value;
    const char *counter;
    bool mutable;
    bool mapped;
} box_ui_row_t;

typedef struct {
    box_view_t view;
    uint8_t selected_main_item; /* 0 preset, 1 volume, 2 settings, 3 + effect index */
    uint8_t selected_effect;
    /* The chain: one tile per effect, BOX_UI_VISIBLE_EFFECTS at a time from
     * effect_first. Five effects give the classic five-pedal layout. */
    uint8_t effect_count;
    uint8_t effect_first; /* first tile shown; tiles = effect_count + effect_slot */
    bool effect_slot;     /* an EFFECT tile (choose the effect) after the last one (item 3 + effect_count) */
    bool effect_enabled[BOX_UI_MAX_EFFECTS];
    bool effect_tripped[BOX_UI_MAX_EFFECTS]; /* bypassed by its time guard */
    const char *effect_pack_name[BOX_UI_MAX_EFFECTS]; /* NULL: built-in engine */
    uint8_t effect_art[BOX_UI_MAX_EFFECTS]; /* built-in artwork 0..4, or 0xFF for an RPK */
    bool reorder_mode;
    bool preset_browse;
    bool preset_save;
    bool preset_conflict;
    bool preset_text_visible;
    bool preset_saved;
    uint8_t master_volume;
    uint8_t input_level;
    uint8_t output_level;

    uint8_t preset_index;
    bool preset_valid;
    const char *preset_name;

    const char *page_title;
    uint8_t selected_block_index;
    uint8_t block_count;
    const char *subtype_name;
    uint8_t subtype_index;
    uint8_t subtype_count;
    bool selector_is_model;

    const box_ui_row_t *rows;
    uint8_t row_count;
    uint8_t selected_row;
    box_editor_focus_t editor_focus;
    uint32_t parameter_page;
    uint32_t parameter_page_count;
    uint8_t settings_index;
    uint8_t settings_total;
    bool global_eq_enabled;
    uint8_t global_eq_selected;
    int8_t global_eq_bands[BOX_UI_GLOBAL_EQ_BANDS];
    int8_t global_eq_level_steps;
    bool editing;
    bool effects_bypass;
    bool enc2_popup;
    const char *enc2_popup_title;
    const char *enc2_popup_value;
    bool volume_popup;
} box_ui_render_state_t;

void box_ui_render(uint16_t *framebuffer,
                       const box_ui_render_state_t *state);
/* The PRESET SAVED popup, drawn over whatever is on the framebuffer (a pack's
 * own screen). */
void box_ui_render_saved_overlay(uint16_t *framebuffer);
void box_ui_render_meters(uint16_t *framebuffer,
                              uint8_t input_level,
                              uint8_t output_level);
/* Reserve y=8..18 for the common IN/OUT labels and live meter strip. */
void box_ui_render_meter_header(uint16_t *framebuffer,
                                    uint8_t input_level,
                                    uint8_t output_level);
void box_ui_render_main_controls(
    uint16_t *framebuffer,
    const box_ui_render_state_t *state);

void box_ui_render_main_overlay(uint16_t*framebuffer,const box_ui_render_state_t*state);

/* A short message popup over the lower area (an edit that failed). */
void box_ui_render_toast(uint16_t *framebuffer, const char *title, const char *message);

#define BOX_UI_CONTROLS_PER_PAGE 3U
/* One tile of the RPK main-page controls: label, value, unit and a 0..255 bar. */
typedef struct {
    const char *label;
    const char *value;
    const char *unit;
    uint8_t fill;
    bool mapped; /* Encoder 2 controls this parameter */
    const char *hint, *hint2; /* optional small two-line hint (scale 1) under the value */
} box_ui_control_t;
/* Up to BOX_UI_CONTROLS_PER_PAGE tiles in the lower area (y=61..171).
 * selected is the tile index or -1; first/total label the page when there are more. */
void box_ui_render_controls(uint16_t *framebuffer, const box_ui_control_t *controls,
                                uint8_t count, int selected, bool editing,
                                unsigned first, unsigned total);
#endif /* BOX_UI_RENDERER_H */
