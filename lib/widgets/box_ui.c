/*
 * Box UI widget kit, see box_ui.h. The font and the look come from the factory App's
 * renderer (apps/resona_factory/src/renderer.c, ui_pixel_font.h). GPL-3.0 licensed.
 */
#include "box_ui.h"

#include <stddef.h>

/* ---- The 5x7 pixel font (same glyphs as the factory App) --------------------------------- */

typedef struct {
    char ch;
    uint8_t rows[7];
} boxui_glyph;

static const boxui_glyph kGlyphs[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}},
    {'/', {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'%', {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}},
    {'#', {0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00}},
    {':', {0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00}},
    {'<', {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}},
    {'>', {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'=', {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'_', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}},
    {'?', {0x0E, 0x11, 0x02, 0x04, 0x04, 0x00, 0x04}},
};

static const uint8_t *glyph_rows(char ch)
{
    size_t i;
    const size_t n = sizeof(kGlyphs) / sizeof(kGlyphs[0]);

    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    for (i = 0; i < n; ++i)
        if (kGlyphs[i].ch == ch) return kGlyphs[i].rows;
    return kGlyphs[n - 1].rows; /* '?' */
}

static size_t text_len(const char *s)
{
    size_t n = 0;
    if (s)
        while (s[n]) ++n;
    return n;
}

static int32_t clamp_i(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ---- Canvas ------------------------------------------------------------------------------ */

void boxui_canvas_init(boxui_canvas *c, uint16_t *px, int32_t stride, int32_t w, int32_t h)
{
    c->px = px;
    c->stride = stride;
    c->w = w;
    c->h = h;
    c->ui = NULL;
}

int boxui_begin(boxui_canvas *c, const resona_platform *P)
{
    uint16_t *px = NULL;
    uint32_t stride = 0;
    resona_ui *ui = P->screen_begin(&px, &stride);

    boxui_canvas_init(c, px, (int32_t)stride, ui && ui->width ? ui->width : 320,
                      ui && ui->height ? ui->height : 172);
    c->ui = ui;
    if (!px || stride == 0) {
        c->w = c->h = 0;
        return -1;
    }
    return 0;
}

void boxui_present(const boxui_canvas *c, const resona_platform *P)
{
    P->screen_present(0, 0, c->w, c->h);
}

void boxui_fill(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                uint16_t color)
{
    int32_t x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int32_t x1 = x + w > c->w ? c->w : x + w, y1 = y + h > c->h ? c->h : y + h;

    if (!c->px || w <= 0 || h <= 0 || x0 >= x1 || y0 >= y1) return;
    for (int32_t yy = y0; yy < y1; ++yy) {
        uint16_t *row = c->px + (size_t)yy * (size_t)c->stride;
        for (int32_t xx = x0; xx < x1; ++xx) row[xx] = color;
    }
}

void boxui_clear(const boxui_canvas *c, uint16_t color)
{
    boxui_fill(c, 0, 0, c->w, c->h, color);
}

void boxui_dotted_rule(const boxui_canvas *c, int32_t x0, int32_t x1, int32_t y, uint16_t color)
{
    for (int32_t x = x0; x < x1; x += 3) boxui_fill(c, x, y, 1, 1, color);
}

void boxui_selection(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h, int mode)
{
    if (mode == BOXUI_REST) return;
    boxui_fill(c, x, y, w, h, BOXUI_CYAN_DIM);
    boxui_fill(c, x, y, 3, h, mode == BOXUI_EDITING ? BOXUI_YELLOW : BOXUI_CYAN);
}

/* ---- Text -------------------------------------------------------------------------------- */

int32_t boxui_spacing(int scale)
{
    return scale >= 3 ? 2 : 1;
}

int32_t boxui_text_width_ex(const char *s, int scale, int32_t spacing)
{
    size_t n = text_len(s);
    return n == 0 || scale <= 0 ? 0 : (int32_t)n * (5 * scale + spacing) - spacing;
}

int32_t boxui_text_width(const char *s, int scale)
{
    return boxui_text_width_ex(s, scale, boxui_spacing(scale));
}

int32_t boxui_text_ex(const boxui_canvas *c, int32_t x, int32_t y, const char *s,
                      uint16_t color, int scale, int32_t spacing)
{
    const int32_t start = x;

    if (!s || scale <= 0) return x;
    for (; *s; ++s) {
        const uint8_t *rows = glyph_rows(*s);
        if (x >= c->w) break;
        if (x + 5 * scale > 0 && *s != ' ')
            for (int gy = 0; gy < 7; ++gy)
                for (int gx = 0; gx < 5; ++gx)
                    if (rows[gy] & (uint8_t)(1u << (4 - gx)))
                        boxui_fill(c, x + gx * scale, y + gy * scale, scale, scale, color);
        x += 5 * scale + spacing;
    }
    return x == start ? x : x - spacing;
}

int32_t boxui_text(const boxui_canvas *c, int32_t x, int32_t y, const char *s, uint16_t color,
                   int scale)
{
    return boxui_text_ex(c, x, y, s, color, scale, boxui_spacing(scale));
}

void boxui_text_right(const boxui_canvas *c, int32_t right, int32_t y, const char *s,
                      uint16_t color, int scale)
{
    boxui_text(c, right - boxui_text_width(s, scale), y, s, color, scale);
}

void boxui_text_center(const boxui_canvas *c, int32_t center, int32_t y, const char *s,
                       uint16_t color, int scale)
{
    boxui_text(c, center - boxui_text_width(s, scale) / 2, y, s, color, scale);
}

int boxui_fit_scale(const char *s, int32_t width, int max_scale)
{
    int scale = max_scale < 1 ? 1 : max_scale;
    while (scale > 1 && boxui_text_width(s, scale) > width) --scale;
    return scale;
}

/* ---- Widgets ----------------------------------------------------------------------------- */

void boxui_header(const boxui_canvas *c, const char *title, const char *right)
{
    boxui_text(c, 12, 10, title ? title : "", BOXUI_INK, 2);
    if (right && *right) boxui_text_right(c, c->w - 12, 13, right, BOXUI_MUTED, 1);
    boxui_dotted_rule(c, 8, c->w - 8, 31, BOXUI_GRID);
}

void boxui_bar(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
               int32_t permille, int mode)
{
    int32_t fill = w * clamp_i(permille, 0, 1000) / 1000;
    boxui_fill(c, x, y, w, h, BOXUI_GRID);
    boxui_fill(c, x, y, fill, h,
               mode == BOXUI_EDITING ? BOXUI_YELLOW : (mode == BOXUI_SELECTED ? BOXUI_CYAN : BOXUI_INK));
}

void boxui_tile(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                const char *label, const char *value, const char *unit, int32_t permille,
                int mode)
{
    const uint16_t accent = mode == BOXUI_EDITING ? BOXUI_YELLOW : BOXUI_CYAN;
    const int32_t inner = w - 20;
    int32_t unit_w = unit && *unit ? boxui_text_width(unit, 1) + 4 : 0;
    int scale = boxui_fit_scale(value, inner - unit_w, 3);
    int32_t vy = y + 22 + (21 - 7 * scale) / 2;
    int32_t vx;

    if (mode == BOXUI_REST) {
        boxui_fill(c, x, y, w, 1, BOXUI_GRID);
        boxui_fill(c, x, y + h - 1, w, 1, BOXUI_GRID);
        boxui_fill(c, x, y, 1, h, BOXUI_GRID);
        boxui_fill(c, x + w - 1, y, 1, h, BOXUI_GRID);
    } else {
        boxui_selection(c, x, y, w, h, mode);
    }
    boxui_text(c, x + 10, y + 8, label ? label : "", mode == BOXUI_REST ? BOXUI_LABEL : accent, 1);
    vx = boxui_text(c, x + 10, vy, value ? value : "--",
                    mode == BOXUI_EDITING ? BOXUI_YELLOW : BOXUI_INK, scale);
    if (unit_w) boxui_text(c, vx + 4, vy + 7 * scale - 7, unit, BOXUI_MUTED, 1);
    if (permille >= 0) boxui_bar(c, x + 10, y + h - 14, inner, 4, permille, mode);
}

void boxui_meter(const boxui_canvas *c, int32_t x, int32_t y, int32_t level)
{
    int32_t lit = (clamp_i(level, 0, 8) * 10 + 4) / 8;
    for (int32_t i = 0; i < 10; ++i) {
        uint16_t color = BOXUI_GRID;
        if (i < lit) color = i < 6 ? BOXUI_GREEN : (i < 9 ? BOXUI_YELLOW : BOXUI_RED);
        boxui_fill(c, x + i * 11, y, 8, 8, color);
    }
}

void boxui_page_dots(const boxui_canvas *c, int32_t cx, int32_t y, int32_t index, int32_t count)
{
    const int32_t gap = 6, dot = 4, current = 12;
    int32_t width, x;

    if (count <= 1) return;
    width = (count - 1) * (dot + gap) + current;
    x = cx - width / 2;
    for (int32_t i = 0; i < count; ++i) {
        if (i == index) {
            boxui_fill(c, x, y, current, dot, BOXUI_CYAN);
            x += current + gap;
        } else {
            boxui_fill(c, x, y, dot, dot, BOXUI_GRID);
            x += dot + gap;
        }
    }
}

/* ---- List -------------------------------------------------------------------------------- */

static void list_keep_visible(boxui_list *l)
{
    if (l->rows < 1) l->rows = 1;
    l->selected = l->count > 0 ? clamp_i(l->selected, 0, l->count - 1) : 0;
    if (l->selected < l->top) l->top = l->selected;
    if (l->selected >= l->top + l->rows) l->top = l->selected - l->rows + 1;
    l->top = clamp_i(l->top, 0, l->count > l->rows ? l->count - l->rows : 0);
}

void boxui_list_init(boxui_list *l, const char *const *items, const char *const *values,
                     int32_t count, int32_t rows)
{
    l->items = items;
    l->values = values;
    l->count = count < 0 ? 0 : count;
    l->selected = 0;
    l->top = 0;
    l->rows = rows;
    list_keep_visible(l);
}

int boxui_list_move(boxui_list *l, int32_t delta)
{
    int32_t before = l->selected;
    l->selected += delta;
    list_keep_visible(l);
    return l->selected != before;
}

void boxui_list_draw(const boxui_canvas *c, boxui_list *l, int32_t x, int32_t y, int32_t w,
                     int32_t row_h, int mode)
{
    const int scale = row_h >= 20 ? 2 : 1;
    const int32_t ty = (row_h - 7 * scale) / 2;

    list_keep_visible(l);
    for (int32_t r = 0; r < l->rows && l->top + r < l->count; ++r) {
        const int32_t i = l->top + r, ry = y + r * row_h;
        const int sel = i == l->selected ? mode : BOXUI_REST;
        const uint16_t fg = sel == BOXUI_EDITING ? BOXUI_YELLOW : (sel == BOXUI_SELECTED ? BOXUI_CYAN : BOXUI_INK);
        const char *value = l->values ? l->values[i] : NULL;

        if (i == l->selected && mode == BOXUI_REST)
            boxui_fill(c, x, ry, 3, row_h, BOXUI_MUTED); /* where the selection is, unfocused */
        boxui_selection(c, x, ry, w - 6, row_h, sel);
        boxui_text(c, x + 10, ry + ty, l->items[i], sel ? fg : BOXUI_INK, scale);
        if (value) boxui_text_right(c, x + w - 16, ry + ty, value, sel ? fg : BOXUI_MUTED, scale);
        boxui_dotted_rule(c, x, x + w - 6, ry + row_h, BOXUI_GRID);
    }
    if (l->count > l->rows) { /* scroll mark */
        const int32_t track = l->rows * row_h;
        const int32_t thumb = track * l->rows / l->count;
        const int32_t at = (track - thumb) * l->top / (l->count - l->rows);
        boxui_fill(c, x + w - 3, y, 2, track, BOXUI_GRID);
        boxui_fill(c, x + w - 3, y + at, 2, thumb < 6 ? 6 : thumb, BOXUI_CYAN);
    }
}

/* ---- Toast ------------------------------------------------------------------------------- */

void boxui_toast_show(boxui_toast *t, const char *text, uint16_t accent, uint32_t ms)
{
    size_t i = 0;
    if (text)
        for (; text[i] && i < sizeof t->text - 1; ++i) t->text[i] = text[i];
    t->text[i] = 0;
    t->accent = accent;
    t->remaining_ms = ms ? ms : 1;
}

int boxui_toast_tick(boxui_toast *t, uint32_t ms)
{
    if (!t->remaining_ms) return 0;
    t->remaining_ms = ms >= t->remaining_ms ? 0 : t->remaining_ms - ms;
    return t->remaining_ms == 0;
}

int boxui_toast_visible(const boxui_toast *t)
{
    return t->remaining_ms != 0;
}

void boxui_toast_draw(const boxui_canvas *c, const boxui_toast *t)
{
    const int32_t x = 8, w = c->w - 16, h = 40, y = c->h - h - 8;
    int scale;

    if (!t->remaining_ms) return;
    scale = boxui_fit_scale(t->text, w - 30, 2);
    boxui_fill(c, x, y, w, h, BOXUI_CYAN_DIM);
    boxui_fill(c, x, y, 3, h, t->accent);
    boxui_dotted_rule(c, x, x + w, y - 1, BOXUI_GRID);
    boxui_dotted_rule(c, x, x + w, y + h, BOXUI_GRID);
    boxui_text(c, x + 14, y + (h - 7 * scale) / 2, t->text, BOXUI_INK, scale);
}

/* ---- Focus ------------------------------------------------------------------------------- */

void boxui_focus_init(boxui_focus *f, int32_t count, int32_t wrap)
{
    f->index = 0;
    f->count = count;
    f->wrap = wrap;
    f->editing = 0;
}

boxui_action boxui_focus_handle(boxui_focus *f, const resona_app_event *ev, int32_t *delta)
{
    const resona_event *in;

    if (!ev || ev->kind != RESONA_APP_EV_INPUT) return BOXUI_NONE;
    in = &ev->u.input;
    if (in->kind == RESONA_EVENT_TURN) {
        if (in->input == RESONA_INPUT_CUSTOMIZE || f->editing) {
            if (delta) *delta = in->value;
            return in->value ? BOXUI_ADJUST : BOXUI_NONE;
        }
        if (f->count <= 0) return BOXUI_NONE;
        {
            int32_t before = f->index, next = f->index + in->value;
            if (f->wrap) next = ((next % f->count) + f->count) % f->count;
            else next = clamp_i(next, 0, f->count - 1);
            f->index = next;
            return next != before ? BOXUI_MOVED : BOXUI_NONE;
        }
    }
    if (in->kind == RESONA_EVENT_CLICK) return BOXUI_ACTIVATE;
    if (in->kind == RESONA_EVENT_DOUBLE) return BOXUI_DOUBLE;
    if (in->kind == RESONA_EVENT_LONG && in->input == RESONA_INPUT_SELECTOR) return BOXUI_BACK;
    return BOXUI_NONE;
}

int boxui_focus_mode(const boxui_focus *f, int32_t i)
{
    if (i != f->index) return BOXUI_REST;
    return f->editing ? BOXUI_EDITING : BOXUI_SELECTED;
}
