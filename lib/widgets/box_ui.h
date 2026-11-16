/*
 * Box UI: the Resona Box's own screen style (the factory App's renderer), factored into a
 * small C widget kit for Box Apps.
 *
 * Everything draws straight into the App's RGB565 back buffer (P->screen_begin), with the
 * Box's 5x7 pixel font at scales 1..3 and the factory palette. Nothing allocates, nothing is
 * global: the widgets that keep state (list, toast, focus) are plain structs that live in the
 * App's own State. The OS has no UI toolkit; this one is compiled into the App and costs
 * about 4 KB of App code.
 *
 *   boxui_canvas c;
 *   boxui_begin(&c, P);                         // screen_begin: back buffer + size
 *   boxui_clear(&c, BOXUI_BG);
 *   boxui_header(&c, "AMP", "1/2");
 *   boxui_tile(&c, 8, 40, 98, 96, "GAIN", "12", "DB", 600, BOXUI_SELECTED);
 *   boxui_page_dots(&c, 160, 164, 0, 2);
 *   boxui_present(&c, P);                       // screen_present of the whole frame
 *
 * Input: boxui_focus_handle() turns RESONA_APP_EV_INPUT events into actions: E1 (selector)
 * turn moves the focus, E1/E2 click activates, E1 long goes back, E2 (customize) turn adjusts
 * the focused item.
 *
 * Text is ASCII; lower case is drawn as upper case and other characters as '?'.
 *
 * GPL-3.0 licensed, see the SDK's LICENSE.
 */
#ifndef RESONA_BOX_UI_H
#define RESONA_BOX_UI_H

#include <stdint.h>

#include <resona/platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Palette (the factory renderer's colours, RGB565) ---------------------------------- */
#define BOXUI_BG RESONA_RGB(0x07, 0x09, 0x0C)
#define BOXUI_INK RESONA_RGB(0xE8, 0xE3, 0xD3)
#define BOXUI_MUTED RESONA_RGB(0x74, 0x6F, 0x67)
#define BOXUI_LABEL RESONA_RGB(0xB4, 0xAE, 0xA2)
#define BOXUI_GRID RESONA_RGB(0x40, 0x3E, 0x3A)
#define BOXUI_CYAN RESONA_RGB(0x27, 0xCE, 0xD4)
#define BOXUI_CYAN_DIM RESONA_RGB(0x12, 0x3B, 0x3F)
#define BOXUI_GREEN RESONA_RGB(0x42, 0xC9, 0x78)
#define BOXUI_YELLOW RESONA_RGB(0xE0, 0xBE, 0x3B)
#define BOXUI_RED RESONA_RGB(0xDA, 0x36, 0x44)

/* How a widget is shown. */
#define BOXUI_REST 0
#define BOXUI_SELECTED 1 /* under the focus: cyan */
#define BOXUI_EDITING 2  /* being changed: yellow */

/* ---- Canvas ---------------------------------------------------------------------------- */

typedef struct boxui_canvas {
    uint16_t *px;    /* the back buffer */
    int32_t stride;  /* pixels per row */
    int32_t w, h;    /* 320 x 172 on the Resona Box */
    resona_ui *ui;   /* the platform's drawing table, if you want its primitives too */
} boxui_canvas;

/* screen_begin: fills `c` from the platform. Returns 0, or -1 without a back buffer. */
int boxui_begin(boxui_canvas *c, const resona_platform *P);
/* Wrap any RGB565 buffer (an off-screen sprite, a test buffer). */
void boxui_canvas_init(boxui_canvas *c, uint16_t *px, int32_t stride, int32_t w, int32_t h);
/* screen_present of the whole canvas. */
void boxui_present(const boxui_canvas *c, const resona_platform *P);

void boxui_clear(const boxui_canvas *c, uint16_t color);
void boxui_fill(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                uint16_t color);
/* The factory's dotted rule: one pixel every 3, from x0 to x1. */
void boxui_dotted_rule(const boxui_canvas *c, int32_t x0, int32_t x1, int32_t y, uint16_t color);
/* The factory's selection: a dim cyan bar with a cyan (selected) or yellow (editing) edge. */
void boxui_selection(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                     int mode);

/* ---- Text: the 5x7 pixel font at scale 1..3 (larger scales work too) ------------------- */

/* Default letter spacing for a scale: 1, 1, 2 (as the factory screens use). */
int32_t boxui_spacing(int scale);
int32_t boxui_text_width(const char *s, int scale);
int32_t boxui_text_width_ex(const char *s, int scale, int32_t spacing);
/* Returns the x after the text. */
int32_t boxui_text(const boxui_canvas *c, int32_t x, int32_t y, const char *s, uint16_t color,
                   int scale);
int32_t boxui_text_ex(const boxui_canvas *c, int32_t x, int32_t y, const char *s,
                      uint16_t color, int scale, int32_t spacing);
void boxui_text_right(const boxui_canvas *c, int32_t right, int32_t y, const char *s,
                      uint16_t color, int scale);
void boxui_text_center(const boxui_canvas *c, int32_t center, int32_t y, const char *s,
                       uint16_t color, int scale);
/* The largest scale <= max_scale at which `s` fits in `width` (at least 1). */
int boxui_fit_scale(const char *s, int32_t width, int max_scale);

/* ---- Widgets ----------------------------------------------------------------------------- */

/* Page title (scale 2) on the left, `right` (counter, state) on the right, rule at y = 31. */
void boxui_header(const boxui_canvas *c, const char *title, const char *right);

/* A value tile: label (scale 1) on top, the value as large as fits (<= 3) with its unit,
 * and a bar at the bottom filled to `permille` (0..1000; < 0 hides the bar). */
void boxui_tile(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
                const char *label, const char *value, const char *unit, int32_t permille,
                int mode);

/* A horizontal bar: track in GRID, fill in CYAN (YELLOW while editing). */
void boxui_bar(const boxui_canvas *c, int32_t x, int32_t y, int32_t w, int32_t h,
               int32_t permille, int mode);

/* A ten-segment level meter as on the main page (level 0..8; green, yellow, red). */
void boxui_meter(const boxui_canvas *c, int32_t x, int32_t y, int32_t level);

/* Page dots centred on cx: the current page is a cyan bar, the others grey squares. */
void boxui_page_dots(const boxui_canvas *c, int32_t cx, int32_t y, int32_t index, int32_t count);

/* A scrolling list with a selection. Keep it in your State; `items` (and the optional
 * right-aligned `values`) are arrays of `count` strings that outlive the list. */
typedef struct boxui_list {
    const char *const *items;
    const char *const *values; /* may be NULL */
    int32_t count;
    int32_t selected;
    int32_t top;  /* first visible row; kept in range by boxui_list_move/_draw */
    int32_t rows; /* visible rows */
} boxui_list;

void boxui_list_init(boxui_list *l, const char *const *items, const char *const *values,
                     int32_t count, int32_t rows);
/* Move the selection by `delta` (clamped). Returns 1 if it moved. */
int boxui_list_move(boxui_list *l, int32_t delta);
/* Rows of `row_h` pixels (28 matches the factory editor); a scroll mark on the right. */
void boxui_list_draw(const boxui_canvas *c, boxui_list *l, int32_t x, int32_t y, int32_t w,
                     int32_t row_h, int mode);

/* A short message over the screen's lower half that goes away by itself. */
typedef struct boxui_toast {
    char text[40];
    uint32_t remaining_ms;
    uint16_t accent;
} boxui_toast;

void boxui_toast_show(boxui_toast *t, const char *text, uint16_t accent, uint32_t ms);
/* Advance by `ms`; returns 1 when the toast just disappeared (redraw). */
int boxui_toast_tick(boxui_toast *t, uint32_t ms);
int boxui_toast_visible(const boxui_toast *t);
void boxui_toast_draw(const boxui_canvas *c, const boxui_toast *t);

/* ---- Encoder focus ----------------------------------------------------------------------- */

typedef enum boxui_action {
    BOXUI_NONE = 0,
    BOXUI_MOVED,    /* E1 turned: `index` changed */
    BOXUI_ACTIVATE, /* E1 or E2 clicked on `index` */
    BOXUI_BACK,     /* E1 long press */
    BOXUI_ADJUST,   /* E2 turned (or E1 while `editing`): *delta detents for `index` */
    BOXUI_DOUBLE    /* E1 or E2 double click (if enabled with input_config) */
} boxui_action;

typedef struct boxui_focus {
    int32_t index;
    int32_t count;
    int32_t wrap;    /* 1: turning past the last item comes back to the first */
    int32_t editing; /* 1: E1 turns adjust instead of moving (set it on ACTIVATE) */
} boxui_focus;

void boxui_focus_init(boxui_focus *f, int32_t count, int32_t wrap);
/* Feed every App event; returns what to do. `delta` (may be NULL) gets the detents of an
 * ADJUST. Events other than encoder input return BOXUI_NONE. */
boxui_action boxui_focus_handle(boxui_focus *f, const resona_app_event *ev, int32_t *delta);
/* The mode to draw item `i` with. */
int boxui_focus_mode(const boxui_focus *f, int32_t i);

#ifdef __cplusplus
}
#endif

#endif /* RESONA_BOX_UI_H */
