/*
 * Small helpers for drawing module screens (see resona/module.h).
 * Freestanding: no C library, no heap.
 */
#ifndef RESONA_UI_H
#define RESONA_UI_H

#include <stdint.h>

#include "resona/module.h"
#include "resona/utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void resona_ui_frame(const resona_ui *ui, int32_t x, int32_t y, int32_t w,
                                   int32_t h, int32_t t, uint16_t color);

/* True when the host provides the UI v1 primitives (line, ellipse, dots, poly). */
static inline int resona_ui_has_v1(const resona_ui *ui)
{
    return ui->ext == RESONA_UI_EXT_MAGIC;
}

/*
 * The primitives, each with a fallback of plain rectangles for a host that has not got them,
 * so a module still draws something sensible on an older firmware: axis-aligned shapes are
 * exact, anything else becomes a box or a short run of dots.
 */
static inline void resona_ui_line(const resona_ui *ui, int32_t x0, int32_t y0, int32_t x1,
                                  int32_t y1, uint16_t color)
{
    int32_t i;
    int32_t n;
    int32_t dx;
    int32_t dy;

    if (resona_ui_has_v1(ui) && ui->line) {
        ui->line(ui->ctx, x0, y0, x1, y1, color);
        return;
    }
    if (x0 == x1) {
        ui->rect(ui->ctx, x0, y0 < y1 ? y0 : y1, 1, (y0 < y1 ? y1 - y0 : y0 - y1) + 1, color);
        return;
    }
    if (y0 == y1) {
        ui->rect(ui->ctx, x0 < x1 ? x0 : x1, y0, (x0 < x1 ? x1 - x0 : x0 - x1) + 1, 1, color);
        return;
    }
    dx = x1 - x0;
    dy = y1 - y0;
    n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    if (n > 15) {
        n = 15;
    }
    for (i = 0; i <= n; ++i) {
        ui->rect(ui->ctx, x0 + dx * i / n, y0 + dy * i / n, 1, 1, color);
    }
}

static inline void resona_ui_ellipse(const resona_ui *ui, int32_t cx, int32_t cy, int32_t rx,
                                     int32_t ry, uint16_t color)
{
    if (resona_ui_has_v1(ui) && ui->ellipse) {
        ui->ellipse(ui->ctx, cx, cy, rx, ry, color);
        return;
    }
    resona_ui_frame(ui, cx - rx, cy - ry, 2 * rx + 1, 2 * ry + 1, 1, color);
}

static inline void resona_ui_ellipse_fill(const resona_ui *ui, int32_t cx, int32_t cy, int32_t rx,
                                          int32_t ry, uint16_t color)
{
    if (resona_ui_has_v1(ui) && ui->ellipse_fill) {
        ui->ellipse_fill(ui->ctx, cx, cy, rx, ry, color);
        return;
    }
    ui->rect(ui->ctx, cx - rx, cy - ry, 2 * rx + 1, 2 * ry + 1, color);
}

/* `show` of `count` square dots of `size` along an elliptical arc; 0 degrees is
 * 12 o'clock. Exact +/-360-degree circles exclude the repeated endpoint;
 * partial arcs include both endpoints. Small radii/large dots may still overlap. */
static inline void resona_ui_dots(const resona_ui *ui, int32_t cx, int32_t cy, int32_t rx,
                                  int32_t ry, int32_t start_deg, int32_t sweep_deg,
                                  uint32_t count, uint32_t show, uint32_t size, uint16_t color)
{
    if (resona_ui_has_v1(ui) && ui->dots) {
        ui->dots(ui->ctx, cx, cy, rx, ry, start_deg, sweep_deg, count, show, size, color);
        return;
    }
    /* Without it, one bar the length of the lit part stands in for the ring. */
    if (show > 0 && count > 0) {
        ui->rect(ui->ctx, cx - rx, cy + ry, (int32_t)(2 * (uint32_t)rx * show / count) + 1, 1,
                 color);
    }
}

static inline void resona_ui_poly(const resona_ui *ui, const int16_t *xy, uint32_t n,
                                  uint16_t color)
{
    uint32_t i;

    if (resona_ui_has_v1(ui) && ui->poly) {
        ui->poly(ui->ctx, xy, n, color);
        return;
    }
    for (i = 0; i < n; ++i) {
        const uint32_t j = (i + 1) % n;

        resona_ui_line(ui, xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1], color);
    }
}

static inline void resona_ui_poly_fill(const resona_ui *ui, const int16_t *xy, uint32_t n,
                                       uint16_t color)
{
    int32_t x0 = xy[0];
    int32_t y0 = xy[1];
    int32_t x1 = xy[0];
    int32_t y1 = xy[1];
    uint32_t i;

    if (resona_ui_has_v1(ui) && ui->poly_fill) {
        ui->poly_fill(ui->ctx, xy, n, color);
        return;
    }
    for (i = 1; i < n; ++i) {
        if (xy[2 * i] < x0) x0 = xy[2 * i];
        if (xy[2 * i] > x1) x1 = xy[2 * i];
        if (xy[2 * i + 1] < y0) y0 = xy[2 * i + 1];
        if (xy[2 * i + 1] > y1) y1 = xy[2 * i + 1];
    }
    ui->rect(ui->ctx, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
}

/* A line with every `step`th pixel drawn: the box's dotted rule. */
static inline void resona_ui_dotline(const resona_ui *ui, int32_t x0, int32_t y0, int32_t x1,
                                     int32_t y1, uint32_t step, uint16_t color)
{
    int32_t x;

    if (resona_ui_has_v1(ui) && ui->dotline) {
        ui->dotline(ui->ctx, x0, y0, x1, y1, step, color);
        return;
    }
    /* A host without it: short dashes, which is as close as rectangles get in few calls. */
    if (y0 == y1) {
        for (x = x0; x <= x1; x += 12) {
            ui->rect(ui->ctx, x, y0, 6, 1, color);
        }
    } else {
        resona_ui_line(ui, x0, y0, x1, y1, color);
    }
}

/* Width in pixels of `s` at `scale`, matching the host's text advance. */
static inline int32_t resona_ui_text_width(uint32_t scale, const char *s)
{
    int32_t n = 0;
    uint32_t bytes=0, at=0;
    while (s[bytes]) ++bytes;
    while (at<bytes) { uint32_t used; (void)resona_utf8_decode((const uint8_t*)s+at,bytes-at,&used);at+=used;++n; }
    return n == 0 ? 0 : n * 6 * (int32_t)scale - (int32_t)scale;
}

/* Rectangle outline `t` pixels thick. */
static inline void resona_ui_frame(const resona_ui *ui, int32_t x, int32_t y, int32_t w,
                                   int32_t h, int32_t t, uint16_t color)
{
    ui->rect(ui->ctx, x, y, w, t, color);
    ui->rect(ui->ctx, x, y + h - t, w, t, color);
    ui->rect(ui->ctx, x, y, t, h, color);
    ui->rect(ui->ctx, x + w - t, y, t, h, color);
}

/* Horizontal bar: `track` background with `fill` over the left `frac` (0..1). */
static inline void resona_ui_bar(const resona_ui *ui, int32_t x, int32_t y, int32_t w,
                                 int32_t h, float frac, uint16_t fill, uint16_t track)
{
    int32_t n;

    if (!(frac > 0.0f)) {
        frac = 0.0f;
    }
    if (frac > 1.0f) {
        frac = 1.0f;
    }
    n = (int32_t)(frac * (float)w + 0.5f);
    ui->rect(ui->ctx, x, y, w, h, track);
    if (n > 0) {
        ui->rect(ui->ctx, x, y, n, h, fill);
    }
}

/*
 * Format `v` with `decimals` (0 to 3) digits after the point into `buf`.
 * Returns the length. Handles negative values; clamps to +-999999.
 */
static inline int32_t resona_fmt_value(char *buf, int32_t cap, float v, int32_t decimals)
{
    int32_t pos = 0;
    int32_t neg = 0;
    uint32_t scale = 1u;
    uint32_t whole;
    uint32_t frac;
    char tmp[12];
    int32_t n = 0;
    int32_t i;

    if (cap < 2) {
        return 0;
    }
    if (decimals < 0) {
        decimals = 0;
    }
    if (decimals > 3) {
        decimals = 3;
    }
    for (i = 0; i < decimals; ++i) {
        scale *= 10u;
    }
    if (v < 0.0f) {
        neg = 1;
        v = -v;
    }
    if (v > 999999.0f) {
        v = 999999.0f;
    }
    {
        uint32_t scaled = (uint32_t)(v * (float)scale + 0.5f);

        whole = scaled / scale;
        frac = scaled % scale;
        if (scaled == 0u) {
            neg = 0;
        }
    }
    if (neg && pos < cap - 1) {
        buf[pos++] = '-';
    }
    do {
        tmp[n++] = (char)('0' + (whole % 10u));
        whole /= 10u;
    } while (whole && n < 11);
    while (n && pos < cap - 1) {
        buf[pos++] = tmp[--n];
    }
    if (decimals > 0 && pos < cap - 1) {
        buf[pos++] = '.';
        for (i = decimals - 1; i >= 0 && pos < cap - 1; --i) {
            uint32_t d = 1u;
            int32_t k;

            for (k = 0; k < i; ++k) {
                d *= 10u;
            }
            buf[pos++] = (char)('0' + ((frac / d) % 10u));
        }
    }
    buf[pos] = 0;
    return pos;
}

/*
 * Standard page: a title strip and up to three parameter rows. Rows are 40
 * pixels apart starting at y = 44, which fills the 172-pixel canvas.
 */
static inline void resona_ui_title(const resona_ui *ui, const char *name, uint16_t accent)
{
    ui->rect(ui->ctx, 0, 0, (int32_t)ui->width, 3, accent);
    ui->text(ui->ctx, 8, 12, 3, accent, name);
}

static inline void resona_ui_param_row(const resona_ui *ui, int32_t y, const resona_param *p,
                                       float value, uint16_t accent)
{
    char buf[32];
    int32_t n;
    int32_t k;
    float span = p->max - p->min;
    float frac = span > 0.0f ? (value - p->min) / span : 0.0f;

    n = resona_fmt_value(buf, 24, value, span <= 10.0f ? 1 : 0);
    if (p->unit && p->unit[0] && n < 30) {
        buf[n++] = ' ';
        for (k = 0; p->unit[k] && n < 31; ++k) {
            buf[n++] = p->unit[k];
        }
        buf[n] = 0;
    }
    ui->text(ui->ctx, 8, y, 2, RESONA_RGB(150, 150, 150), p->name);
    ui->text(ui->ctx, 312 - resona_ui_text_width(2, buf), y, 2, RESONA_RGB(255, 255, 255), buf);
    resona_ui_bar(ui, 8, y + 20, 304, 8, frac, accent, RESONA_RGB(40, 40, 40));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RESONA_UI_H */
