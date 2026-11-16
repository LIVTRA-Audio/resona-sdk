/* The Resona Box factory UI renderer, moved from the firmware into the factory app (GPL-3.0). */
#include "ui_pixel_font.h"
#include "renderer.h"

#include <stddef.h>
#include <resona/libc.h>

#define UI_BG        0x07090CU
#define UI_INK       0xE8E3D3U
#define UI_MUTED     0x746F67U
#define UI_LABEL     0xB4AEA2U
#define UI_GRID      0x403E3AU
#define UI_CYAN      0x27CED4U
#define UI_CYAN_DIM  0x123B3FU
#define UI_GREEN     0x42C978U
#define UI_YELLOW    0xE0BE3BU
#define UI_RED       0xDA3644U

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#include "generated/box_ui_pedal_assets.inc"

static uint16_t rgb565(uint32_t rgb)
{
    uint8_t r = (uint8_t)(rgb >> 16);
    uint8_t g = (uint8_t)(rgb >> 8);
    uint8_t b = (uint8_t)rgb;

    return (uint16_t)(((uint16_t)(r >> 3) << 11) |
                      ((uint16_t)(g >> 2) << 5) |
                      (uint16_t)(b >> 3));
}

static void fill(uint16_t *fb, int x, int y, int w, int h, uint32_t rgb)
{
    uint16_t pixel = rgb565(rgb);
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)BOX_UI_WIDTH ? (int)BOX_UI_WIDTH : x + w;
    int y1 = y + h > (int)BOX_UI_HEIGHT ? (int)BOX_UI_HEIGHT : y + h;
    int xx;
    int yy;

    if (!fb || w <= 0 || h <= 0 || x0 >= x1 || y0 >= y1) {
        return;
    }
    for (yy = y0; yy < y1; ++yy) {
        uint16_t *row = fb + (size_t)yy * BOX_UI_WIDTH;
        for (xx = x0; xx < x1; ++xx) {
            row[xx] = pixel;
        }
    }
}

static uint16_t dim_rgb565(uint16_t pixel)
{
    uint16_t bg = rgb565(UI_BG);
    unsigned r = (pixel >> 11) & 0x1FU;
    unsigned g = (pixel >> 5) & 0x3FU;
    unsigned b = pixel & 0x1FU;
    unsigned bg_r = (bg >> 11) & 0x1FU;
    unsigned bg_g = (bg >> 5) & 0x3FU;
    unsigned bg_b = bg & 0x1FU;

    r = (r * 2U + bg_r * 3U) / 5U;
    g = (g * 2U + bg_g * 3U) / 5U;
    b = (b * 2U + bg_b * 3U) / 5U;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void draw_pedal_art(uint16_t *fb, uint8_t role, int x, int y,
                           bool enabled)
{
    unsigned yy;

    if (!fb || role >= BOX_UI_ROLE_COUNT) {
        return;
    }
    for (yy = 0U; yy < BOX_PEDAL_ART_HEIGHT; ++yy) {
        unsigned xx;
        uint16_t *dst = fb + (size_t)(y + (int)yy) * BOX_UI_WIDTH + x;

        for (xx = 0U; xx < BOX_PEDAL_ART_WIDTH; ++xx) {
            uint16_t pixel = g_box_pedal_art[role][yy][xx];

            dst[xx] = enabled ? pixel : dim_rgb565(pixel);
        }
    }
}

static int text_width(const char *value, int scale, int spacing);
static void text_center(uint16_t *fb, int center, int y, const char *value,
                        uint32_t rgb, int scale, int spacing);

/* RPK effects share the original pedals' pixel-art scale/materials. */
static void draw_pedal_knob(uint16_t *fb, int x, int y, bool enabled)
{
    fill(fb, x + 1, y + 1, 8, 8, 0x11191BU);
    fill(fb, x + 2, y, 4, 8, enabled ? 0xA3AAA6U : 0x6B7473U);
    fill(fb, x + 1, y + 1, 6, 6, enabled ? 0xA3AAA6U : 0x6B7473U);
    fill(fb, x, y + 2, 8, 4, enabled ? 0x89928EU : 0x596362U);
    fill(fb, x + 2, y + 1, 4, 6, 0x25302FU);
    fill(fb, x + 1, y + 2, 6, 4, 0x25302FU);
    fill(fb, x + 3, y + 1, 1, 3, enabled ? UI_INK : UI_MUTED);
}

static void draw_user_pedal(uint16_t *fb, int x, int y, bool enabled, bool empty)
{
    uint32_t face = empty ? 0x414C4DU : enabled ? 0x31596BU : 0x35464DU;
    uint32_t edge = empty ? 0x778382U : enabled ? 0xA4B5B7U : 0x677777U;
    uint32_t plate = empty ? 0x313C3DU : enabled ? 0xE2D9BDU : 0xA2A393U;

    /* Clipped metal corners, a lit upper/left edge and a dark lower edge. */
    fill(fb, x + 5, y + 3, 49, 60, 0x101919U);
    fill(fb, x + 4, y + 1, 48, 62, 0x172425U);
    fill(fb, x + 2, y + 3, 52, 58, 0x172425U);
    fill(fb, x + 4, y + 2, 48, 59, face);
    fill(fb, x + 3, y + 4, 50, 55, face);
    fill(fb, x + 5, y + 2, 46, 1, edge);
    fill(fb, x + 3, y + 4, 1, 54, edge);
    fill(fb, x + 5, y + 60, 46, 1, 0x233333U);
    fill(fb, x + 52, y + 4, 1, 55, 0x203031U);
    draw_pedal_knob(fb, x + 8, y + 6, enabled && !empty);
    draw_pedal_knob(fb, x + 24, y + 6, enabled && !empty);
    draw_pedal_knob(fb, x + 40, y + 6, enabled && !empty);
    fill(fb, x + 3, y + 18, 50, 31, 0x172425U);
    fill(fb, x + 4, y + 19, 48, 29, plate);
    /* LED and a small stepped circular footswitch, matching the pixel artwork. */
    fill(fb, x + 11, y + 54, 4, 4, 0x172425U);
    fill(fb, x + 12, y + 55, 2, 2, enabled && !empty ? UI_GREEN : 0x69716BU);
    fill(fb, x + 25, y + 51, 6, 1, 0x142223U);
    fill(fb, x + 23, y + 53, 10, 6, 0x142223U);
    fill(fb, x + 25, y + 59, 6, 1, 0x142223U);
    fill(fb, x + 25, y + 52, 6, 7, 0x89938CU);
    fill(fb, x + 24, y + 53, 8, 5, 0xBCC3B5U);
    fill(fb, x + 26, y + 53, 4, 4, 0xDCE0D0U);
    fill(fb, x + 26, y + 57, 4, 1, 0x65766EU);
}

static void draw_pack_art(uint16_t *fb, int x, int y, const char *name,
                          bool enabled)
{
    const char *next = name;
    uint32_t ink = enabled ? 0x172425U : 0x414C46U;
    char lines[3][9];
    int rows = 0;

    draw_user_pedal(fb, x, y, enabled, false);
    for (int row = 0; row < 3 && *next; ++row) {
        size_t remaining, take;

        while (*next == ' ') ++next;
        remaining = strlen(next);
        take = remaining > 8U ? 8U : remaining;
        if (remaining > take && next[take] != ' ') {
            for (size_t i = take; i > 0U; --i) {
                if (next[i] == ' ') { take = i; break; }
            }
        }
        if (!take) break;
        memcpy(lines[rows], next, take);
        lines[rows++][take] = '\0';
        next += take;
    }
    if (rows == 1 && text_width(lines[0], 2, 1) <= 48) {
        text_center(fb, x + 28, y + 26, lines[0], ink, 2, 1);
    } else {
        for (int row = 0; row < rows; ++row) {
            text_center(fb, x + 28, y + 20 + (3 - rows) * 5 + row * 10,
                        lines[row], ink, 1, 1);
        }
    }
}

static void draw_effect_art(uint16_t *fb, uint8_t role, int x, int y,
                            bool selected, bool enabled, const char *pack)
{
    if (pack) {
        draw_pack_art(fb, x + 1, y, pack, enabled);
    } else {
        draw_pedal_art(fb, role, x + 1, y, enabled);
    }
    if (selected) {
        fill(fb, x + 8, y + 68, 42, 2, UI_CYAN);
    }
}

static void dotted_rule_color(uint16_t *fb, int y, uint32_t color)
{
    int x;

    for (x = 8; x < 312; x += 3) {
        fill(fb, x, y, 1, 1, color);
    }
}

static void dotted_rule(uint16_t *fb, int y)
{
    dotted_rule_color(fb, y, UI_GRID);
}

static int text_width(const char *text, int scale, int spacing)
{
    size_t len = text ? strlen(text) : 0U;

    return len == 0U ? 0 : (int)len * (5 * scale + spacing) - spacing;
}

static void text(uint16_t *fb, int x, int y, const char *value,
                 uint32_t rgb, int scale, int spacing)
{
    const char *p = value;

    if (!value || scale <= 0) {
        return;
    }
    while (*p) {
        const uint8_t *rows = ui_pixel_glyph_rows(*p++);
        int gy;
        int gx;

        for (gy = 0; gy < 7; ++gy) {
            for (gx = 0; gx < 5; ++gx) {
                if ((rows[gy] & (uint8_t)(1U << (4 - gx))) != 0U) {
                    fill(fb, x + gx * scale, y + gy * scale,
                         scale, scale, rgb);
                }
            }
        }
        x += 5 * scale + spacing;
    }
}

static void text_right(uint16_t *fb, int right, int y, const char *value,
                       uint32_t rgb, int scale, int spacing)
{
    text(fb, right - text_width(value, scale, spacing), y,
         value, rgb, scale, spacing);
}

static void text_center(uint16_t *fb, int center, int y, const char *value,
                        uint32_t rgb, int scale, int spacing)
{
    text(fb, center - text_width(value, scale, spacing) / 2, y,
         value, rgb, scale, spacing);
}

static void format_counter(char out[12], uint8_t index, uint8_t count)
{
    unsigned shown = count == 0U ? 0U : (unsigned)index + 1U;

    out[0] = (char)('0' + (shown / 10U) % 10U);
    out[1] = (char)('0' + shown % 10U);
    out[2] = ' ';
    out[3] = '/';
    out[4] = ' ';
    out[5] = (char)('0' + ((unsigned)count / 10U) % 10U);
    out[6] = (char)('0' + (unsigned)count % 10U);
    out[7] = '\0';
}

static void format_compact_counter(char out[8], uint8_t index, uint8_t count)
{
    unsigned shown = (unsigned)index + 1U;

    if (count == 0U || index >= count) {
        out[0] = '\0';
        return;
    }
    out[0] = (char)('0' + (shown / 10U) % 10U);
    out[1] = (char)('0' + shown % 10U);
    out[2] = '/';
    out[3] = (char)('0' + ((unsigned)count / 10U) % 10U);
    out[4] = (char)('0' + (unsigned)count % 10U);
    out[5] = '\0';
}

static void format_preset_index(char out[4], uint8_t index, bool valid)
{
    unsigned shown;

    if (!valid || index >= BOX_PRESET_COUNT) {
        out[0] = '\0';
        return;
    }
    shown = (unsigned)index + 1U;
    if (shown >= 10U) {
        out[0] = (char)('0' + (shown / 10U) % 10U);
        out[1] = (char)('0' + shown % 10U);
        out[2] = '\0';
    } else {
        out[0] = (char)('0' + shown);
        out[1] = '\0';
    }
}

static void format_volume(char out[4], uint8_t volume)
{
    unsigned value = volume > 100U ? 100U : volume;

    if (value == 100U) {
        memcpy(out, "100", 4U);
    } else {
        out[0] = value >= 10U ? (char)('0' + value / 10U) : ' ';
        out[1] = (char)('0' + value % 10U);
        out[2] = '\0';
    }
}

static void meter(uint16_t *fb, int x, int y, uint8_t level)
{
    unsigned clamped = level > 8U ? 8U : level;
    unsigned lit = (clamped * 10U + 4U) / 8U;
    unsigned i;

    for (i = 0U; i < 10U; ++i) {
        uint32_t color = UI_GRID;

        if (i < lit) {
            color = i < 6U ? UI_GREEN : (i < 9U ? UI_YELLOW : UI_RED);
        }
        fill(fb, x + (int)i * 11, y, 8, 8, color);
    }
}

void box_ui_render_meters(uint16_t *framebuffer,
                              uint8_t input_level,
                              uint8_t output_level)
{
    if (!framebuffer) {
        return;
    }

    /* These are the only changing pixels on the performance page. Keeping
     * both meters in one shallow top-row strip prevents a full-frame flush. */
    fill(framebuffer, 38, 8, 110, 11, UI_BG);
    fill(framebuffer, 198, 8, 110, 11, UI_BG);
    meter(framebuffer, 38, 11, input_level);
    meter(framebuffer, 198, 11, output_level);
}

void box_ui_render_meter_header(uint16_t *fb,
                                    uint8_t input_level,
                                    uint8_t output_level)
{
    if (!fb) return;
    text_center(fb, 22, 12, "IN", UI_INK, 1, 1);
    text_center(fb, 182, 12, "OUT", UI_INK, 1, 1);
    box_ui_render_meters(fb, input_level, output_level);
}

static void draw_settings_icon(uint16_t *fb, int x, int y)
{
    uint32_t color = UI_MUTED;

    fill(fb, x, y + 2, 18, 1, color);
    fill(fb, x + 4, y, 3, 5, color);
    fill(fb, x, y + 9, 18, 1, color);
    fill(fb, x + 11, y + 7, 3, 5, color);
    fill(fb, x, y + 16, 18, 1, color);
    fill(fb, x + 7, y + 14, 3, 5, color);
}

static void draw_pixel_check(uint16_t *fb, int x, int y, uint32_t color)
{
    fill(fb, x, y + 4, 3, 3, color);
    fill(fb, x + 3, y + 7, 3, 3, color);
    fill(fb, x + 6, y + 4, 3, 3, color);
    fill(fb, x + 9, y + 1, 3, 3, color);
}

static void draw_popup_panel(uint16_t *fb, uint32_t bar)
{
    fill(fb, 8, 68, 304, 92, UI_CYAN_DIM);
    fill(fb, 8, 68, 3, 92, bar);
    dotted_rule(fb, 67);
    dotted_rule(fb, 160);
}

static void draw_saved_overlay(uint16_t *fb)
{
    draw_popup_panel(fb, UI_YELLOW);
    text(fb, 20, 82, "PRESET", UI_MUTED, 2, 1);
    text(fb, 20, 112, "SAVED", UI_YELLOW, 3, 2);
    draw_pixel_check(fb, 278, 116, UI_GREEN);
}

void box_ui_render_saved_overlay(uint16_t *framebuffer)
{
    if (framebuffer) {
        draw_saved_overlay(framebuffer);
    }
}

static void draw_volume_overlay(uint16_t *fb, uint8_t volume)
{
    char value[4];
    unsigned lit = ((unsigned)volume * 10U + 50U) / 100U;
    unsigned i;

    format_volume(value, volume);
    draw_popup_panel(fb, UI_YELLOW);
    text(fb, 20, 80, "VOLUME", UI_MUTED, 2, 1);
    text_right(fb, 302, 80, value, UI_YELLOW, 3, 2);
    for (i = 0U; i < 10U; ++i) {
        uint32_t color = UI_GRID;

        if (i < lit) {
            color = i < 6U ? UI_GREEN : (i < 9U ? UI_YELLOW : UI_RED);
        }
        fill(fb, 20 + (int)i * 28, 130, 24, 8, color);
    }
}

static void draw_bypass_overlay(uint16_t *fb)
{
    draw_popup_panel(fb, UI_RED);
    text(fb, 20, 80, "EFFECTS", UI_MUTED, 2, 1);
    text(fb, 20, 112, "BYPASS", UI_RED, 3, 2);
}

static void draw_param_overlay(uint16_t *fb, const char *title,
                               const char *value)
{
    int title_scale = 2;

    draw_popup_panel(fb, UI_RED);
    if (text_width(title, 2, 1) > 270) {
        title_scale = 1;
    }
    text(fb, 20, title_scale == 2 ? 82 : 86, title ? title : "",
         UI_INK, title_scale, 1);
    text_right(fb, 302, 118, value ? value : "", UI_YELLOW, 3, 2);
}

static void render_main_controls(uint16_t *fb,
                                 const box_ui_render_state_t *state)
{
    char index[4];
    char volume[4];
    char preset_name[13];
    int preset_name_x;
    bool preset_active = state->preset_browse || state->preset_save;
    bool volume_active = state->selected_main_item == 1U && state->editing;
    uint32_t preset_color;
    uint32_t volume_color;

    format_preset_index(index, state->preset_index, state->preset_valid);
    format_volume(volume, state->master_volume);
    memset(preset_name, 0, sizeof(preset_name));
    if (state->preset_name) {
        strncpy(preset_name, state->preset_name, sizeof(preset_name) - 1U);
    }
    if (state->preset_save && state->preset_conflict) {
        memcpy(preset_name, "REPLACE", sizeof("REPLACE"));
    }
    preset_color = preset_active ? UI_YELLOW :
                   (state->selected_main_item == 0U ? UI_CYAN : UI_INK);
    volume_color = volume_active ? UI_YELLOW :
                   (state->selected_main_item == 1U ? UI_CYAN : UI_INK);
    if (state->selected_main_item == 0U) {
        fill(fb, 8, 32, 190, 27, UI_CYAN_DIM);
        fill(fb, 8, 32, 3, 27,
             preset_active ? UI_YELLOW : UI_CYAN);
    } else if (state->selected_main_item == 1U) {
        fill(fb, 201, 32, 43, 27, UI_CYAN_DIM);
        fill(fb, 201, 32, 3, 27,
             volume_active ? UI_YELLOW : UI_CYAN);
    } else if (state->selected_main_item == 2U) {
        fill(fb, 259, 32, 53, 27, UI_CYAN_DIM);
        fill(fb, 259, 32, 3, 27, UI_CYAN);
    }
    if (!state->preset_save || state->preset_text_visible) {
        text(fb, 20, 37, index, preset_color, 2, 1);
        preset_name_x = index[0] ? 28 + text_width(index, 2, 1) : 20;
        text(fb, preset_name_x, 37,
             preset_name[0] ? preset_name : "INIT PRESET",
             preset_color, 2, 1);
    }
    text_center(fb, 222, 37, volume, volume_color, 2, 1);
    draw_settings_icon(fb, 278, 36);
    dotted_rule_color(fb, 60,
                      state->global_eq_enabled ? UI_CYAN : UI_GRID);
}

void box_ui_render_main_controls(
    uint16_t *framebuffer,
    const box_ui_render_state_t *state)
{
    if (!framebuffer || !state) {
        return;
    }
    fill(framebuffer, 0, 28, BOX_UI_WIDTH, 33, UI_BG);
    render_main_controls(framebuffer, state);
}

void box_ui_render_main_overlay(uint16_t*fb,const box_ui_render_state_t*state){
    if (state->enc2_popup) {
        draw_param_overlay(fb, state->enc2_popup_title,
                           state->enc2_popup_value);
    } else if (state->preset_saved) {
        draw_saved_overlay(fb);
    } else if (state->volume_popup) {
        draw_volume_overlay(fb, state->master_volume);
    } else if (state->effects_bypass) {
        draw_bypass_overlay(fb);
    }
}

/* The EFFECT tile after the pedals: a dashed outline the size of a pedal around a swap glyph
 * (two opposed arrows). It opens the preset's EFFECT picker. */
static void draw_swap_arrow(uint16_t *fb, int cx, int yc, bool right, uint32_t color)
{
    int tip = right ? cx + 14 : cx - 14;
    int d;

    fill(fb, right ? cx - 13 : tip + 5, yc - 1, 22, 3, color);
    for (d = 0; d <= 5; ++d) {
        fill(fb, right ? tip - d : tip + d, yc - d, 1, 2 * d + 1, color);
    }
}

static void draw_effect_slot(uint16_t *fb, int x, int y, bool selected)
{
    uint32_t edge = selected ? UI_CYAN : UI_GRID;
    uint32_t glyph = selected ? UI_CYAN : UI_LABEL;
    int i;

    for (i = 0; i < 52; i += 4) {
        fill(fb, x + 3 + i, y + 2, 2, 1, edge);
        fill(fb, x + 3 + i, y + 61, 2, 1, edge);
    }
    for (i = 0; i < 60; i += 4) {
        fill(fb, x + 3, y + 2 + i, 1, 2, edge);
        fill(fb, x + 54, y + 2 + i, 1, 2, edge);
    }
    draw_swap_arrow(fb, x + 29, y + 20, true, glyph);
    draw_swap_arrow(fb, x + 29, y + 34, false, glyph);
    text_center(fb, x + 29, y + 50, "EFFECT", selected ? UI_CYAN : UI_MUTED, 1, 1);
    if (selected) {
        fill(fb, x + 8, y + 68, 42, 2, UI_CYAN);
    }
}

static void render_main(uint16_t *fb, const box_ui_render_state_t *state)
{
    uint8_t count = state->effect_count > BOX_UI_MAX_EFFECTS ?
                    BOX_UI_MAX_EFFECTS : state->effect_count;
    uint8_t tiles = (uint8_t)(count + (state->effect_slot && count < BOX_UI_MAX_EFFECTS ? 1U : 0U));
    uint8_t first = state->effect_first;
    uint8_t shown;
    int first_x;
    uint8_t i;

    if (tiles <= BOX_UI_VISIBLE_EFFECTS) {
        first = 0U;
    } else if (first > tiles - BOX_UI_VISIBLE_EFFECTS) {
        first = (uint8_t)(tiles - BOX_UI_VISIBLE_EFFECTS);
    }
    shown = (uint8_t)(tiles - first);
    if (shown > BOX_UI_VISIBLE_EFFECTS) {
        shown = BOX_UI_VISIBLE_EFFECTS;
    }
    /* Five tiles sit at x = 5 (the original layout); fewer are centred at the same pitch. */
    first_x = shown ? ((int)BOX_UI_WIDTH - ((int)(shown - 1U) * 63 + 56)) / 2 - 1 : 0;

    box_ui_render_meter_header(fb, state->input_level, state->output_level);
    dotted_rule_color(fb, 27,
                      state->global_eq_enabled ? UI_CYAN : UI_GRID);
    render_main_controls(fb, state);

    if (count == 0U) {
        text_center(fb, 160, 68, "EMPTY PRESET", UI_MUTED, 1, 1);
    }
    for (i = 0U; i < shown; ++i) {
        uint8_t e = (uint8_t)(first + i);
        int x = first_x + (int)i * 63;
        bool selected = state->selected_main_item == (uint8_t)(3U + e);
        bool enabled;
        uint8_t art;
        const char *pack = NULL;

        if (e >= count) {
            draw_effect_slot(fb, x + 1, 80, selected);
            continue;
        }
        enabled = state->effect_enabled[e] && !state->effects_bypass;
        art = state->effect_art[e];
        if (art >= BOX_UI_ROLE_COUNT || state->effect_pack_name[e]) {
            pack = state->effect_pack_name[e] && state->effect_pack_name[e][0] ?
                   state->effect_pack_name[e] : "RPK";
            art = 0U;
        }
        draw_effect_art(fb, art, x, 80, selected, enabled, pack);
        if (state->effect_tripped[e]) {
            /* Bypassed by its time guard: a red mark in the corner. */
            fill(fb, x + 47, 81, 6, 6, UI_RED);
        }
        if (selected && state->reorder_mode) {
            fill(fb, x + 8, 148, 42, 2, UI_YELLOW);
        }
    }
    if (state->reorder_mode) {
        text_center(fb, 160, 162, "TURN MOVE   PUSH DONE", UI_YELLOW, 1, 1);
    } else if (tiles > BOX_UI_VISIBLE_EFFECTS) {
        /* One dot per tile; the visible ones are lit, the selected one is cyan. */
        int x = (int)BOX_UI_WIDTH / 2 - ((int)tiles * 10 - 4) / 2;

        for (i = 0U; i < tiles; ++i) {
            bool visible = i >= first && i < first + shown;
            uint32_t color = state->selected_main_item == (uint8_t)(3U + i) ? UI_CYAN :
                             (visible ? UI_LABEL : UI_GRID);

            fill(fb, x + (int)i * 10, 158, 6, 4, color);
        }
    }
    box_ui_render_main_overlay(fb,state);
}

static void editor_selection(uint16_t *fb, int x, int y, int width,
                             int height, bool editing)
{
    fill(fb, x, y, width, height, UI_CYAN_DIM);
    fill(fb, x, y, 3, height, editing ? UI_YELLOW : UI_CYAN);
}

static void editor_page_indicator(uint16_t *fb,uint32_t page,uint32_t count){
 if(count<=1U)return;
 char label[24];if(page>=count)page=count-1U;
 snprintf(label,sizeof label,"%u / %u",(unsigned)page+1U,(unsigned)count);text_center(fb,160,156,label,UI_CYAN,1,1);
}

static void render_editor(uint16_t *fb, const box_ui_render_state_t *state)
{
    char block_counter[12];
    char subtype_counter[12];
    char selector_value[48];
    const char *block_name = state->page_title ? state->page_title : "BLOCK";
    bool block_selected =
        state->editor_focus == BOX_EDITOR_FOCUS_BLOCK;
    bool state_selected =
        state->editor_focus == BOX_EDITOR_FOCUS_STATE;
    bool type_selected =
        state->editor_focus == BOX_EDITOR_FOCUS_TYPE ||
        state->editor_focus == BOX_EDITOR_FOCUS_MODEL;
    bool enabled = state->selected_effect < BOX_UI_MAX_EFFECTS &&
                   state->effect_enabled[state->selected_effect];
    uint8_t count = state->row_count > 3U ? 3U : state->row_count;
    int block_name_x = 34;
    int block_name_w;
    int subtype_counter_w;
    int selector_scale;
    uint8_t i;

    format_counter(block_counter, state->selected_block_index,
                   state->block_count);
    if (block_selected) {
        editor_selection(fb, 8, 3, 216, 28, state->editing);
    }
    if (state_selected) {
        editor_selection(fb, 226, 3, 86, 28, state->editing);
    }
    text(fb, 17, 10, "<", block_selected ? UI_CYAN : UI_MUTED, 2, 2);
    text(fb, block_name_x, 10, block_name,
         block_selected && state->editing ? UI_YELLOW : UI_INK, 2, 2);
    block_name_w = text_width(block_name, 2, 2);
    text(fb, block_name_x + block_name_w + 7, 10, ">",
         block_selected ? UI_CYAN : UI_MUTED, 2, 2);
    text_right(fb, 217, 13, block_counter,
               block_selected ? UI_CYAN : UI_MUTED, 1, 1);
    fill(fb, 225, 8, 1, 18, UI_GRID);
    text_right(fb, 302, 10, enabled ? "ON" : "OFF",
               state_selected ? (state->editing ? UI_YELLOW : UI_CYAN) :
               (enabled ? UI_INK : UI_MUTED), 2, 2);
    dotted_rule(fb, 31);

    if (type_selected) {
        editor_selection(fb, 8, 32, 304, 28, state->editing);
    }
    if (state->selector_is_model) {
        format_compact_counter(subtype_counter, state->subtype_index,
                               state->subtype_count);
        memset(selector_value, 0, sizeof(selector_value));
        if (subtype_counter[0] != '\0') {
            strncpy(selector_value, subtype_counter,
                    sizeof(selector_value) - 1U);
            strncat(selector_value, " ",
                    sizeof(selector_value) - strlen(selector_value) - 1U);
        }
        strncat(selector_value,
                state->subtype_name ? state->subtype_name : "--",
                sizeof(selector_value) - strlen(selector_value) - 1U);
        text(fb, 18, 39, selector_value,
             type_selected ?
                 (state->editing ? UI_YELLOW : UI_CYAN) : UI_INK,
             2, 1);
    } else {
        text(fb, 18, 39, "TYPE", UI_INK, 2, 1);
        format_counter(subtype_counter, state->subtype_index,
                       state->subtype_count);
        subtype_counter_w = text_width(subtype_counter, 1, 1);
        selector_scale = text_width(state->subtype_name, 2, 1) > 165 ? 1 : 2;
        text_right(fb, 302, 42, subtype_counter,
                   type_selected ? UI_CYAN : UI_MUTED, 1, 1);
        text_right(fb, 292 - subtype_counter_w,
                   selector_scale == 2 ? 39 : 42,
                   state->subtype_name ? state->subtype_name : "--",
                   type_selected && state->editing ? UI_YELLOW : UI_INK,
                   selector_scale, 1);
    }
    dotted_rule(fb, 60);

    for (i = 0U; i < count; ++i) {
        int y = 61 + (int)i * 29;
        bool selected =
            state->editor_focus == BOX_EDITOR_FOCUS_PARAMETER &&
            state->selected_row == i;
        bool mutable = state->rows[i].mutable;
        bool mapped = state->rows[i].mapped;
        uint32_t fg = mapped ? UI_RED : (mutable ? UI_INK : UI_MUTED);
        uint32_t value_fg = fg;
        int value_scale = 2;
        int value_right = 302;

        if (selected) {
            editor_selection(fb, 8, y, 304, 28, state->editing);
            if (mutable && state->editing) {
                value_fg = UI_YELLOW;
            } else if (mapped) {
                value_fg = UI_RED;
            } else {
                value_fg = mutable ? UI_CYAN : UI_MUTED;
            }
        }
        if (state->rows[i].counter) {
            int counter_width = text_width(state->rows[i].counter, 1, 1);

            text_right(fb, 302, y + 10, state->rows[i].counter,
                       selected ? UI_CYAN : UI_MUTED, 1, 1);
            value_right -= counter_width + 10;
        }
        if (text_width(state->rows[i].value, 2, 1) > value_right - 100) {
            value_scale = 1;
        }
        text(fb, 18, y + 7,
             state->rows[i].label, fg, 2, 1);
        text_right(fb, value_right, y + (28 - 7 * value_scale) / 2,
                   state->rows[i].value, value_fg, value_scale, 1);
        dotted_rule(fb, y + 28);
    }
    editor_page_indicator(fb, state->parameter_page,
                          state->parameter_page_count);
    if (state->preset_saved) {
        draw_saved_overlay(fb);
    }
}

static void format_eq_steps_compact(char out[12], int steps)
{
    unsigned magnitude = (unsigned)(steps < 0 ? -steps : steps);

    snprintf(out, 12U, "%s%u.%u", steps < 0 ? "-" : "+",
             magnitude / 2U, (magnitude & 1U) ? 5U : 0U);
}

static int global_eq_slider_y(int steps, int limit)
{
    if (steps < -limit) {
        steps = -limit;
    } else if (steps > limit) {
        steps = limit;
    }
    return 87 - steps * 35 / limit;
}

static void render_global_eq(uint16_t *fb,
                             const box_ui_render_state_t *state)
{
    static const int slider_x[BOX_UI_GLOBAL_EQ_BANDS + 1U] = {
        31, 82, 133, 184, 235, 288
    };
    static const char *const slider_names[BOX_UI_GLOBAL_EQ_BANDS + 1U] = {
        "100", "250", "630", "1.6K", "4K", "LEVEL"
    };
    uint8_t selected = state->global_eq_selected;
    uint8_t control;

    if (selected > BOX_UI_GLOBAL_EQ_BANDS + 1U) {
        selected = 0U;
    }
    if (selected == 0U) {
        editor_selection(fb, 8, 3, 304, 28, state->editing);
    } else {
        fill(fb, 8, 3, 3, 28, state->editing ? UI_YELLOW : UI_CYAN);
    }
    text(fb, 18, 10, "GLOBAL EQ", UI_INK, 2, 2);
    fill(fb, 263, 8, 1, 18, UI_GRID);
    text_right(fb, 302, 10, state->global_eq_enabled ? "ON" : "OFF",
               selected == 0U ? UI_CYAN :
               (state->global_eq_enabled ? UI_INK : UI_MUTED), 2, 2);
    dotted_rule(fb, 31);

    fill(fb, 263, 35, 1, 131, UI_GRID);
    for (control = 0U;
         control < BOX_UI_GLOBAL_EQ_BANDS + 1U; ++control) {
        bool is_level = control == BOX_UI_GLOBAL_EQ_BANDS;
        bool is_selected = selected == (uint8_t)(control + 1U);
        int steps = is_level ? state->global_eq_level_steps :
                    state->global_eq_bands[control];
        int limit = is_level ? 24 : 30;
        int handle_y = global_eq_slider_y(steps, limit);
        int x = slider_x[control];
        uint32_t handle_color = is_selected ?
            (state->editing ? UI_YELLOW : UI_CYAN) :
            (state->global_eq_enabled ? UI_CYAN : UI_MUTED);
        uint32_t label_color = is_selected ?
            (state->editing ? UI_YELLOW : UI_CYAN) : UI_MUTED;
        char compact[12];

        if (is_selected) {
            editor_selection(fb, x - 22, 35, is_level ? 47 : 44, 131,
                             state->editing);
        }
        fill(fb, x - 1, 52, 3, 72, UI_GRID);
        fill(fb, x - 5, 52, 11, 1, UI_GRID);
        fill(fb, x - 5, 87, 11, 1, UI_MUTED);
        fill(fb, x - 5, 123, 11, 1, UI_GRID);
        fill(fb, x - 9, handle_y - 2, 19, 5, handle_color);
        text_center(fb, x, 134, slider_names[control], label_color, 1, 1);
        format_eq_steps_compact(compact, steps);
        text_center(fb, x, 149, compact,
                    is_selected ? UI_CYAN : UI_INK, 1, 1);
    }

}

static void render_settings(uint16_t *fb, const box_ui_render_state_t *state)
{
    char counter[12];
    uint8_t count = state->row_count > BOX_UI_MAX_ROWS ?
                    BOX_UI_MAX_ROWS : state->row_count;
    int first_y;
    int row_step;
    int row_h;
    int label_scale;
    int value_scale;
    uint8_t i;

    first_y = 32;
    row_step = 24;
    row_h = 22;
    label_scale = 2;
    value_scale = 2;

    format_counter(counter,
                   state->settings_total ? state->settings_index :
                                           state->selected_row,
                   state->settings_total ? state->settings_total : count);
    text(fb, 8, 6, state->page_title ? state->page_title : "SETTINGS",
         UI_INK, 2, 2);
    text_right(fb, 312, 8, counter, state->editing ? UI_CYAN : UI_MUTED,
               1, 1);
    dotted_rule(fb, 28);
    for (i = 0U; i < count; ++i) {
        int y = first_y + (int)i * row_step;
        bool selected = state->selected_row == i;
        bool mutable = state->rows[i].mutable;
        uint32_t fg = UI_INK;
        uint32_t value_fg = mutable ? UI_INK : UI_YELLOW;
        int row_value_scale = value_scale;

        if (selected) {
            fill(fb, 8, y, 304, row_h, UI_CYAN_DIM);
            fill(fb, 8, y, 3, row_h,
                 state->editing ? UI_YELLOW : UI_CYAN);
            fg = UI_INK;
            value_fg = mutable ?
                       (state->editing ? UI_YELLOW : UI_CYAN) : UI_YELLOW;
        }
        if (text_width(state->rows[i].value, row_value_scale, 1) > 130) {
            row_value_scale = 1;
        }
        text(fb, 18, y + (row_h - 7 * label_scale) / 2,
             state->rows[i].label, fg, label_scale, 1);
        if (strcmp(state->rows[i].label, "GLOBAL EQ") == 0) {
            int dot_x = 18 + text_width(state->rows[i].label,
                                        label_scale, 1) + 6;

            /* A custom dot stays vertically centered; the font period sits
             * on the baseline and does not produce the requested mark. */
            fill(fb, dot_x, y + row_h / 2 - 5, 6, 6, UI_CYAN);
        }
        text_right(fb, 302,
                   y + (row_h - 7 * row_value_scale) / 2,
                   state->rows[i].value, value_fg, row_value_scale, 1);
    }
}

void box_ui_render(uint16_t *framebuffer,
                       const box_ui_render_state_t *state)
{
    if (!framebuffer || !state) {
        return;
    }
    fill(framebuffer, 0, 0, BOX_UI_WIDTH, BOX_UI_HEIGHT, UI_BG);
    switch (state->view) {
    case BOX_VIEW_EDITOR:
        render_editor(framebuffer, state);
        break;
    case BOX_VIEW_SETTINGS:
        render_settings(framebuffer, state);
        break;
    case BOX_VIEW_GLOBAL_EQ:
        render_global_eq(framebuffer, state);
        break;
    case BOX_VIEW_PICKER:
        render_settings(framebuffer, state);
        dotted_rule(framebuffer, 154);
        text_center(framebuffer, 160, 160, "PUSH CHOOSE   HOLD BACK", UI_MUTED, 1, 1);
        break;
    case BOX_VIEW_MAIN:
    default:
        render_main(framebuffer, state);
        break;
    }
    if (state->preset_saved && state->view != BOX_VIEW_MAIN &&
        state->view != BOX_VIEW_EDITOR) {
        draw_saved_overlay(framebuffer);
    }
}

void box_ui_render_toast(uint16_t *fb, const char *title, const char *message)
{
    int scale;

    if (!fb) {
        return;
    }
    draw_popup_panel(fb, UI_RED);
    text(fb, 20, 82, title ? title : "", UI_MUTED, 2, 1);
    scale = text_width(message, 3, 2) <= 284 ? 3 : 2;
    text(fb, 20, scale == 3 ? 112 : 116, message ? message : "", UI_RED, scale, scale == 3 ? 2 : 1);
}

void box_ui_render_controls(uint16_t *fb, const box_ui_control_t *controls,
                                uint8_t count, int selected, bool editing,
                                unsigned first, unsigned total)
{
    /* Three large tiles: names and units at scale 2, values at scale 3. Names use
     * a lighter grey than the header so they stay readable at arm's length. */
    enum { TILE_X = 5, TILE_Y = 65, TILE_W = 100, TILE_H = 92, TILE_STEP = 105, INNER = 88 };
    char label[9];
    uint8_t i;

    if (!fb || !controls) {
        return;
    }
    fill(fb, 0, 61, BOX_UI_WIDTH, 111, UI_BG);
    for (i = 0U; i < count && i < BOX_UI_CONTROLS_PER_PAGE; ++i) {
        const box_ui_control_t *c = &controls[i];
        int x = TILE_X + (int)i * TILE_STEP;
        bool active = selected == (int)i;
        bool edit = active && editing;
        int scale;

        if (edit) {
            fill(fb, x, TILE_Y, TILE_W, TILE_H, UI_CYAN_DIM);
        }
        if (active) {
            fill(fb, x, TILE_Y, TILE_W, 2, UI_CYAN);
            fill(fb, x, TILE_Y + TILE_H - 2, TILE_W, 2, UI_CYAN);
            fill(fb, x, TILE_Y, 2, TILE_H, UI_CYAN);
            fill(fb, x + TILE_W - 2, TILE_Y, 2, TILE_H, UI_CYAN);
        } else if (i > 0U) {
            fill(fb, x - 3, TILE_Y + 8, 1, TILE_H - 16, UI_GRID);
        }
        snprintf(label, sizeof(label), "%s", c->label ? c->label : "");
        text(fb, x + 6, TILE_Y + 7, label, c->mapped ? UI_CYAN : UI_LABEL, 2, 1);
        scale = text_width(c->value, 3, 3) <= INNER ? 3 : 2;
        text(fb, x + 6, TILE_Y + (scale == 3 ? 29 : 32), c->value,
             edit ? UI_CYAN : UI_INK, scale, scale);
        text(fb, x + 6, TILE_Y + 56, c->unit, UI_LABEL, 2, 2);
        if (c->hint) {
            text(fb, x + 6, TILE_Y + 56, c->hint, UI_MUTED, 1, 1);
        }
        if (c->hint2) {
            text(fb, x + 6, TILE_Y + 66, c->hint2, UI_MUTED, 1, 1);
        }
        if (c->mapped) {
            text_right(fb, x + TILE_W - 6, TILE_Y + 56, "E2", UI_CYAN, 2, 2);
        }
        fill(fb, x + 6, TILE_Y + 78, INNER, 6, UI_GRID);
        fill(fb, x + 6, TILE_Y + 78, (INNER * (int)c->fill + 127) / 255, 6,
             active ? UI_CYAN : UI_INK);
    }
    if (total > BOX_UI_CONTROLS_PER_PAGE) {
        /* Page dots centred under the tiles; very long lists show "page / pages". */
        unsigned pages = (total + BOX_UI_CONTROLS_PER_PAGE - 1U) / BOX_UI_CONTROLS_PER_PAGE;
        unsigned page = first / BOX_UI_CONTROLS_PER_PAGE;

        if (pages <= 16U) {
            int x = (int)BOX_UI_WIDTH / 2 - ((int)pages * 12 - 6) / 2;
            unsigned k;

            for (k = 0U; k < pages; ++k) {
                fill(fb, x + (int)k * 12, 162, 6, 6, k == page ? UI_CYAN : UI_GRID);
            }
        } else {
            char label[16];

            snprintf(label, sizeof(label), "%u / %u", page + 1U, pages);
            text_center(fb, (int)BOX_UI_WIDTH / 2, 161, label, UI_LABEL, 1, 1);
        }
    }
}
