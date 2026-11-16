/*
 * Resona UI kit: widgets for module screens.
 *
 * Knobs, faders, level meters, toggles, step tiles and an XY pad, drawn as thin isometric
 * line figures in the Resona Box's own palette and pixel font, plus a Form that holds a few of
 * them, moves a selection between them with SELECTOR, changes the selected one with CUSTOMIZE,
 * and draws the page around them (title, counter, footer hint).
 *
 * They are plain classes with no heap and no virtual functions, meant to sit in your State.
 * The host hands a module zeroed memory without running constructors, so each one is set up
 * with init(), which takes the arguments (position, name, range, unit) and returns the widget
 * so settings can be chained:
 *
 *   struct State {
 *       resona::ui::Knob cutoff, res;
 *       resona::ui::Fader gain;
 *       resona::ui::Form form;
 *   };
 *
 *   void init(void *s, const resona_host *) {
 *       State *st = static_cast<State *>(s);
 *       st->cutoff.init(46, 72, "CUTOFF", 20.0f, 12000.0f, 1000.0f, "HZ").log_scale();
 *       st->res.init(122, 72, "RES", 0.0f, 100.0f, 20.0f, "%");
 *       st->gain.init(20, 104, "GAIN", -24.0f, 12.0f, 0.0f, "DB");
 *       st->form.init("FILTER");
 *       st->form.add(&st->cutoff);
 *       st->form.add(&st->res);
 *       st->form.add(&st->gain);
 *   }
 *   void ui_draw(void *s, const resona_ui *ui) { static_cast<State *>(s)->form.draw(ui); }
 *   void on_event(void *s, const resona_event *e) {
 *       State *st = static_cast<State *>(s);
 *       const int changed = st->form.handle(*e);   // which widget the encoder just changed
 *       if (changed == 0) set_cutoff(st->cutoff.value());
 *   }
 *   void process(...) { ...; st->form.tick(); }     // animates the lift and the pointers
 *
 * Every widget has value(), set(v), norm() (0..1), draw(ui, mode), turn(detents), press() and
 * tick(). Widgets are drawn with a handful of calls each (noted below) so a page of a dozen
 * stays inside the 128 calls a frame may have. On a host without the UI v1 primitives they
 * fall back to a line of text.
 *
 * GPL-3.0 licensed, see LICENSE.
 */
#ifndef RESONA_WIDGETS_H
#define RESONA_WIDGETS_H

#include <stdint.h>

#include "resona/math.h"
#include "resona/module.h"
#include "resona/ui.h"

namespace resona {
namespace ui {

/* The Resona Box's palette (the factory App's renderer). */
namespace col {
constexpr uint16_t BG = RESONA_RGB(7, 9, 12);
constexpr uint16_t INK = RESONA_RGB(232, 227, 211);
constexpr uint16_t MUTED = RESONA_RGB(116, 111, 103);
constexpr uint16_t GRID = RESONA_RGB(64, 62, 58);
constexpr uint16_t CYAN = RESONA_RGB(39, 206, 212);
constexpr uint16_t CYAN_DIM = RESONA_RGB(18, 59, 63);
constexpr uint16_t GREEN = RESONA_RGB(66, 201, 120);
constexpr uint16_t YELLOW = RESONA_RGB(224, 190, 59);
constexpr uint16_t YELLOW_DIM = RESONA_RGB(58, 48, 12);
constexpr uint16_t RED = RESONA_RGB(218, 54, 68);
}  // namespace col

/* How a widget is shown: at rest, under the selection, or being changed. */
enum Mode : uint8_t { Rest = 0, Selected = 1, Editing = 2 };

namespace detail {

static const int16_t kSin[360] = {
#include "resona/ui_trig.inc"
};

inline int32_t sin_deg(int32_t deg)
{
    deg %= 360;
    if (deg < 0) {
        deg += 360;
    }
    return kSin[deg];
}

inline int32_t cos_deg(int32_t deg) { return sin_deg(deg + 90); }

/* round(a * q14 / 16384), half away from zero. */
inline int32_t scale(int32_t a, int32_t q14)
{
    const int32_t p = a * q14;

    return (p >= 0 ? p + 8192 : p - 8192) / 16384;
}

inline int32_t floor_div2(int32_t v) { return v >= 0 ? v / 2 : -((-v + 1) / 2); }

inline uint16_t ink(Mode m, uint16_t rest)
{
    return m == Selected ? col::CYAN : (m == Editing ? col::YELLOW : rest);
}

inline uint16_t dim(Mode m)
{
    return m == Editing ? col::YELLOW_DIM : col::CYAN_DIM;
}

/* One step of an integer ease: a third of the way, snapping when close. */
inline void ease(int16_t &cur, int16_t target)
{
    const int32_t d = (int32_t)target - cur;

    if (d > -3 && d < 3) {
        cur = target;
    } else {
        cur = (int16_t)(cur + d / 3);
    }
}

struct Pt {
    int16_t x, y;
};

/* A 2:1 isometric frame: x runs right and down, y runs left and down, z runs up. */
struct Iso {
    int32_t ox, oy;

    Pt at(int32_t x, int32_t y, int32_t z) const
    {
        return Pt{(int16_t)(ox + x - y), (int16_t)(oy + floor_div2(x + y) - z)};
    }
};

inline void poly(const resona_ui *ui, const Pt *p, uint32_t n, uint16_t c)
{
    resona_ui_poly(ui, &p[0].x, n, c);
}

inline void poly_fill(const resona_ui *ui, const Pt *p, uint32_t n, uint16_t c)
{
    resona_ui_poly_fill(ui, &p[0].x, n, c);
}

/* A face: filled with `fill`, then outlined with `line`. 2 calls. */
inline void face(const resona_ui *ui, const Pt *p, uint32_t n, uint16_t line, uint16_t fill)
{
    poly_fill(ui, p, n, fill);
    poly(ui, p, n, line);
}

/* An axis-aligned box in an Iso frame: its right, front-left and top faces, 6 calls. */
inline void box(const resona_ui *ui, const Iso &f, int32_t x, int32_t y, int32_t z, int32_t w,
                int32_t d, int32_t h, uint16_t line, uint16_t top_fill)
{
    const Pt r[4] = {f.at(x + w, y, z), f.at(x + w, y + d, z), f.at(x + w, y + d, z + h),
                     f.at(x + w, y, z + h)};
    const Pt l[4] = {f.at(x, y + d, z), f.at(x + w, y + d, z), f.at(x + w, y + d, z + h),
                     f.at(x, y + d, z + h)};
    const Pt t[4] = {f.at(x, y, z + h), f.at(x + w, y, z + h), f.at(x + w, y + d, z + h),
                     f.at(x, y + d, z + h)};

    face(ui, r, 4, line, col::BG);
    face(ui, l, 4, line, col::BG);
    face(ui, t, 4, line, top_fill);
}

inline int32_t text_w(uint32_t scale_, const char *s)
{
    return resona_ui_text_width(scale_, s);
}

inline void text_c(const resona_ui *ui, int32_t cx, int32_t y, uint32_t scale_, uint16_t c,
                   const char *s)
{
    ui->text(ui->ctx, cx - text_w(scale_, s) / 2, y, scale_, c, s);
}

inline void text_r(const resona_ui *ui, int32_t right, int32_t y, uint32_t scale_, uint16_t c,
                   const char *s)
{
    ui->text(ui->ctx, right - text_w(scale_, s), y, scale_, c, s);
}

inline void dotted_rule(const resona_ui *ui, int32_t y, uint16_t c)
{
    resona_ui_dotline(ui, 8, y, 311, y, 3, c);
}

}  // namespace detail

/* ---- the value a widget carries ---------------------------------------- */

class Value {
public:
    float value_, min_, max_, def_, step_;
    const char *label_;
    const char *unit_;
    int8_t decimals_;   /* -1: automatic */
    uint8_t log_;

    void setup(const char *label, float min, float max, float def, const char *unit)
    {
        label_ = label ? label : "";
        unit_ = unit ? unit : "";
        min_ = min;
        max_ = max > min ? max : min + 1.0f;
        def_ = def < min_ ? min_ : (def > max_ ? max_ : def);
        value_ = def_;
        step_ = 0.0f;
        decimals_ = -1;
        log_ = 0;
    }
    float value() const { return value_; }
    void set(float v) { value_ = v < min_ ? min_ : (v > max_ ? max_ : v); }
    void reset() { value_ = def_; }
    float min() const { return min_; }
    float max() const { return max_; }
    const char *label() const { return label_; }

    /* 0 .. 1000 along the range (a ratio scale for a log knob). */
    int32_t permille() const
    {
        float t;

        if (log_ && min_ > 0.0f && value_ > 0.0f) {
            t = (resona_log2f(value_) - resona_log2f(min_)) * 1000.0f /
                (resona_log2f(max_) - resona_log2f(min_));
        } else {
            t = (value_ - min_) * 1000.0f / (max_ - min_);
        }
        if (!(t > 0.0f)) {
            return 0;
        }
        return t >= 1000.0f ? 1000 : (int32_t)t;
    }
    float norm() const { return (float)permille() * 0.001f; }

    /* One or more detents: a fiftieth of the range, a twelfth of an octave on a log scale,
     * or `step` when given. */
    void nudge(int32_t detents)
    {
        if (log_ && min_ > 0.0f) {
            float f = 1.0f;
            int32_t n = detents < 0 ? -detents : detents;

            for (int32_t i = 0; i < n && i < 24; ++i) {
                f *= 1.0594631f;
            }
            set(detents < 0 ? value_ / f : value_ * f);
            return;
        }
        set(value_ + (step_ > 0.0f ? step_ : (max_ - min_) * 0.02f) * (float)detents);
    }

    /* "1200 HZ", "-6.0 DB": the number and the unit. Returns the length. */
    int32_t format(char *buf, int32_t cap, bool with_unit = true) const
    {
        const float span = max_ - min_;
        int32_t d = decimals_ >= 0 ? decimals_ : (span <= 10.0f ? 1 : 0);
        int32_t n = resona_fmt_value(buf, cap, value_, d);

        if (with_unit && unit_[0] && n + 2 < cap) {
            buf[n++] = ' ';
            for (int32_t k = 0; unit_[k] && n < cap - 1; ++k) {
                buf[n++] = unit_[k];
            }
            buf[n] = 0;
        }
        return n;
    }
};

/* ---- knob ------------------------------------------------------------------ */

/*
 * A short isometric cylinder with a pointer on its top face, in a ring of dots lit up to the
 * value. Selected, it lifts off its shadow and turns cyan; being changed, yellow. The name
 * and the value sit under it. About 13 calls.
 */
class Knob : public Value {
public:
    int16_t x, y;       /* the centre of its base */
    int16_t lift, shown;

    Knob &init(int32_t cx, int32_t cy, const char *label, float min, float max, float def,
               const char *unit = "")
    {
        setup(label, min, max, def, unit);
        x = (int16_t)cx;
        y = (int16_t)cy;
        lift = 0;
        shown = (int16_t)permille();
        return *this;
    }
    Knob &log_scale() { log_ = 1; shown = (int16_t)permille(); return *this; }
    Knob &step(float s) { step_ = s; return *this; }
    Knob &decimals(int d) { decimals_ = (int8_t)d; return *this; }

    void turn(int32_t detents) { nudge(detents); }
    void press() { reset(); }
    void tick(Mode m)
    {
        detail::ease(lift, m == Rest ? 0 : 5);
        detail::ease(shown, (int16_t)permille());
    }
    void settle(Mode m)
    {
        lift = m == Rest ? 0 : 5;
        shown = (int16_t)permille();
    }

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        char num[20];
        const int32_t r = 16;
        const int32_t h = 10;
        const int32_t ry = r / 2;
        const int32_t cy = y - lift;
        const uint16_t line = detail::ink(m, col::INK);

        format(num, sizeof(num));
        if (!resona_ui_has_v1(ui)) {
            ui->text(ui->ctx, x - 24, y - 6, 1, line, label_);
            ui->text(ui->ctx, x - 24, y + 4, 1, col::INK, num);
            return;
        }
        {
            /* the ring, dots lit up to the shown value */
            const int32_t lit = shown > 0 ? (shown * 10 + 500) / 1000 + 1 : 0;

            resona_ui_dots(ui, x, y, r + 10, ry + 5, 225, 270, 11, 11, 2, col::GRID);
            resona_ui_dots(ui, x, y, r + 10, ry + 5, 225, 270, 11, (uint32_t)lit, 2,
                           detail::ink(m, col::CYAN));
        }
        if (lift > 0) {
            resona_ui_ellipse(ui, x, y, r, ry, col::GRID);   /* where it was */
        }
        resona_ui_ellipse(ui, x, cy, r, ry, line);           /* the base */
        {
            const detail::Pt body[4] = {{(int16_t)(x - r), (int16_t)cy},
                                        {(int16_t)(x - r), (int16_t)(cy - h)},
                                        {(int16_t)(x + r), (int16_t)(cy - h)},
                                        {(int16_t)(x + r), (int16_t)cy}};
            detail::poly_fill(ui, body, 4, col::BG);
        }
        resona_ui_line(ui, x - r, cy, x - r, cy - h, line);
        resona_ui_line(ui, x + r, cy, x + r, cy - h, line);
        resona_ui_ellipse_fill(ui, x, cy - h, r, ry, col::BG);
        resona_ui_ellipse(ui, x, cy - h, r, ry, line);
        {
            const int32_t a = 225 + 270 * (int32_t)shown / 1000;
            const int32_t px = x + detail::scale(r * 4 / 5, detail::sin_deg(a));
            const int32_t py = cy - h - detail::scale(ry * 4 / 5, detail::cos_deg(a));

            resona_ui_line(ui, x, cy - h, px, py, detail::ink(m, col::CYAN));
        }
        {
            /* the name under the knob, the number big under that, the unit small beside it */
            char name[24];
            int32_t n = 0;
            int32_t digits = 0;
            const uint32_t big = 2;

            for (; label_[n] && n < 16; ++n) name[n] = label_[n];
            name[n] = 0;
            while (num[digits] && num[digits] != ' ') ++digits;
            num[digits] = 0;   /* the number alone */
            {
                const int32_t uw = unit_[0] ? detail::text_w(1, unit_) + 3 : 0;
                const bool fits = detail::text_w(big, num) + uw <= 72;
                const uint32_t sc = fits ? big : 1;
                const int32_t nw = detail::text_w(sc, num);
                const int32_t left = x - (nw + uw) / 2;
                const uint16_t c = m == Rest ? col::INK : line;

                detail::text_c(ui, x, y + 22, 1, m == Rest ? col::MUTED : line, name);
                ui->text(ui->ctx, left, y + 34, sc, c, num);
                if (uw) {
                    ui->text(ui->ctx, left + nw + 3, y + 34 + (sc == big ? 7 : 0), 1, col::MUTED, unit_);
                }
            }
        }
    }
};

/* ---- fader --------------------------------------------------------------------- */

/*
 * A slab rail running down and to the right, filled up to the value, with a cube for a
 * handle that sits on it and lifts when selected. The name and the value sit under it.
 * About 15 calls.
 */
class Fader : public Value {
public:
    int16_t x, y;       /* the left end of the rail */
    int16_t length;
    int16_t lift, shown;

    Fader &init(int32_t px, int32_t py, const char *label, float min, float max, float def,
                const char *unit = "", int32_t len = 40)
    {
        setup(label, min, max, def, unit);
        x = (int16_t)px;
        y = (int16_t)py;
        length = (int16_t)len;
        lift = 0;
        shown = (int16_t)permille();
        return *this;
    }
    Fader &log_scale() { log_ = 1; shown = (int16_t)permille(); return *this; }
    Fader &step(float s) { step_ = s; return *this; }
    Fader &decimals(int d) { decimals_ = (int8_t)d; return *this; }

    void turn(int32_t detents) { nudge(detents); }
    void press() { reset(); }
    void tick(Mode m)
    {
        detail::ease(lift, m == Rest ? 0 : 3);
        detail::ease(shown, (int16_t)permille());
    }
    void settle(Mode m)
    {
        lift = m == Rest ? 0 : 3;
        shown = (int16_t)permille();
    }

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        char num[20];
        const detail::Iso f = {x, y};
        const int32_t L = length;
        const int32_t D = 6;
        const int32_t fill = L * shown / 1000;
        const uint16_t line = detail::ink(m, col::INK);

        format(num, sizeof(num));
        if (!resona_ui_has_v1(ui)) {
            ui->text(ui->ctx, x, y, 1, line, label_);
            detail::text_r(ui, x + 60, y, 1, col::INK, num);
            return;
        }
        {   /* the rail */
            const detail::Pt r[4] = {f.at(L, 0, 0), f.at(L, D, 0), f.at(L, D, 3), f.at(L, 0, 3)};
            const detail::Pt l[4] = {f.at(0, D, 0), f.at(L, D, 0), f.at(L, D, 3), f.at(0, D, 3)};
            const detail::Pt t[4] = {f.at(0, 0, 3), f.at(L, 0, 3), f.at(L, D, 3), f.at(0, D, 3)};

            detail::face(ui, r, 4, col::MUTED, col::BG);
            detail::face(ui, l, 4, col::MUTED, col::BG);
            detail::face(ui, t, 4, col::MUTED, col::BG);
        }
        if (fill > 0) {
            const detail::Pt t[4] = {f.at(0, 0, 3), f.at(fill, 0, 3), f.at(fill, D, 3),
                                     f.at(0, D, 3)};

            detail::face(ui, t, 4, detail::ink(m, col::CYAN), detail::dim(m));
        }
        /* the handle, a cube riding on the rail */
        detail::box(ui, f, (L - 8) * shown / 1000, -1, 3 + lift, 8, 8, 8, line, col::BG);
        detail::text_c(ui, x + 14, y + 36, 1, m == Rest ? col::MUTED : line, label_);
        detail::text_c(ui, x + 14, y + 48, 1, m == Rest ? col::INK : line, num);
    }
};

/* ---- level meter --------------------------------------------------------------- */

/*
 * A tower of slabs lit from the bottom: green, yellow near the top, red at the top. `peak`
 * marks a held peak with one bright plate. A display only (no encoder); feed it with set().
 * About 3 + 2 n calls for n plates.
 */
class Meter : public Value {
public:
    int16_t x, y;       /* the front-left corner of the base */
    uint8_t plates;
    int16_t peak;       /* permille, or -1 */

    Meter &init(int32_t px, int32_t py, const char *label, int32_t n = 8)
    {
        setup(label, 0.0f, 1.0f, 0.0f, "");
        x = (int16_t)px;
        y = (int16_t)py;
        plates = (uint8_t)(n < 2 ? 2 : (n > 12 ? 12 : n));
        peak = -1;
        return *this;
    }
    void turn(int32_t) {}
    void press() {}
    void tick(Mode) {}
    void settle(Mode) {}
    void set_peak(float p) { peak = (int16_t)(p <= 0.0f ? -1 : (p >= 1.0f ? 1000 : (int32_t)(p * 1000.0f))); }

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        const detail::Iso f = {x, y};
        const int32_t W = 16;
        const int32_t D = 10;
        const int32_t P = 5;                      /* plate pitch */
        const int32_t H = plates * P;
        const int32_t lit = (permille() * plates + 500) / 1000;
        const uint16_t tone = lit * 100 > plates * 88 ? col::RED
                              : (lit * 100 > plates * 60 ? col::YELLOW : col::GREEN);
        const uint16_t line = detail::ink(m, col::MUTED);

        if (!resona_ui_has_v1(ui)) {
            ui->rect(ui->ctx, x, y, permille() * 30 / 1000 + 1, 4, tone);
            return;
        }
        {
            const detail::Pt r[4] = {f.at(W, 0, 0), f.at(W, D, 0), f.at(W, D, H), f.at(W, 0, H)};
            const detail::Pt l[4] = {f.at(0, D, 0), f.at(W, D, 0), f.at(W, D, H), f.at(0, D, H)};
            const detail::Pt t[4] = {f.at(0, 0, H), f.at(W, 0, H), f.at(W, D, H), f.at(0, D, H)};

            detail::poly_fill(ui, r, 4, col::BG);
            detail::poly_fill(ui, l, 4, col::BG);
            {
                /* lit from the bottom: green, then yellow from 60 %, red from 88 % */
                const int32_t g = (plates * 60 + 99) / 100;
                const int32_t yl = (plates * 88 + 99) / 100;
                int32_t from = 0;

                for (int band = 0; band < 3 && from < lit; ++band) {
                    const int32_t to = band == 0 ? (lit < g ? lit : g)
                                       : (band == 1 ? (lit < yl ? lit : yl) : lit);
                    const uint16_t c = band == 0 ? col::GREEN : (band == 1 ? col::YELLOW : col::RED);
                    const int32_t z0 = from * P;
                    const int32_t z1 = to * P;

                    if (to > from) {
                        const detail::Pt rl[4] = {f.at(W, 0, z0), f.at(W, D, z0), f.at(W, D, z1),
                                                  f.at(W, 0, z1)};
                        const detail::Pt ll[4] = {f.at(0, D, z0), f.at(W, D, z0), f.at(W, D, z1),
                                                  f.at(0, D, z1)};

                        detail::poly_fill(ui, rl, 4, c);
                        detail::poly_fill(ui, ll, 4, c);
                    }
                    from = to;
                }
            }
            detail::poly(ui, r, 4, line);
            detail::poly(ui, l, 4, line);
            detail::face(ui, t, 4, line, col::BG);
        }
        for (int32_t k = 1; k < plates; ++k) {
            const int32_t z = k * P;
            const bool on = k <= lit;
            const bool pk = peak >= 0 && k == (peak * plates + 500) / 1000;
            const uint16_t c = pk ? col::INK : (on ? col::BG : col::GRID);

            resona_ui_line(ui, f.at(W, 0, z).x, f.at(W, 0, z).y, f.at(W, D, z).x, f.at(W, D, z).y, c);
            resona_ui_line(ui, f.at(0, D, z).x, f.at(0, D, z).y, f.at(W, D, z).x, f.at(W, D, z).y, c);
        }
        detail::text_c(ui, x + (W - D) / 2, y + (W + D) / 2 + 5, 1, m == Rest ? col::INK : line, label_);
    }
};

/* ---- toggle --------------------------------------------------------------------- */

/*
 * A small slab with a block that slides from one end to the other; the block's top lights
 * cyan when on. The name and ON / OFF sit under it. About 14 calls.
 */
class Toggle {
public:
    int16_t x, y;
    const char *label_;
    uint8_t on_;
    int16_t lift, pos;      /* pos: 0 .. 12 */

    Toggle &init(int32_t px, int32_t py, const char *label, bool on = false)
    {
        x = (int16_t)px;
        y = (int16_t)py;
        label_ = label ? label : "";
        on_ = on ? 1 : 0;
        lift = 0;
        pos = on ? 12 : 0;
        return *this;
    }
    bool on() const { return on_ != 0; }
    void set(bool v) { on_ = v ? 1 : 0; }
    void turn(int32_t d) { if (d != 0) on_ = d > 0 ? 1 : 0; }
    void press() { on_ = on_ ? 0 : 1; }
    float value() const { return on_ ? 1.0f : 0.0f; }
    void tick(Mode m)
    {
        detail::ease(lift, m == Rest ? 0 : 2);
        detail::ease(pos, on_ ? 12 : 0);
    }
    void settle(Mode m)
    {
        lift = m == Rest ? 0 : 2;
        pos = on_ ? 12 : 0;
    }

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        const detail::Iso f = {x, y};
        const uint16_t line = detail::ink(m, on_ ? col::INK : col::MUTED);

        if (!resona_ui_has_v1(ui)) {
            ui->text(ui->ctx, x, y, 1, line, label_);
            ui->text(ui->ctx, x + 40, y, 1, col::INK, on_ ? "ON" : "OFF");
            return;
        }
        detail::box(ui, f, 0, 0, 0, 22, 10, 3, col::MUTED, col::BG);
        detail::box(ui, f, pos, -1, 3 + lift, 10, 12, 6, line, on_ ? detail::dim(m) : col::BG);
        detail::text_c(ui, x + 4, y + 22, 1, m == Rest ? col::MUTED : line, label_);
        detail::text_c(ui, x + 4, y + 34, 1, on_ ? col::CYAN : col::MUTED, on_ ? "ON" : "OFF");
    }
};

/* ---- step tiles --------------------------------------------------------------- */

/*
 * Up to 32 flat tiles in rows; lit ones rise and turn cyan. A playhead tile is yellow and
 * the cursor tile white. One call to two a tile.
 */
class Steps {
public:
    int16_t x, y;
    uint8_t count, columns;
    int8_t cursor, playhead;
    uint32_t mask;

    Steps &init(int32_t px, int32_t py, int32_t n = 16, int32_t cols = 8)
    {
        x = (int16_t)px;
        y = (int16_t)py;
        count = (uint8_t)(n < 1 ? 1 : (n > 32 ? 32 : n));
        columns = (uint8_t)(cols < 1 ? 1 : cols);
        cursor = 0;
        playhead = -1;
        mask = 0;
        return *this;
    }
    bool lit(int32_t i) const { return (mask >> i) & 1u; }
    void set_step(int32_t i, bool on)
    {
        if (i >= 0 && i < count) {
            mask = on ? (mask | (1u << i)) : (mask & ~(1u << i));
        }
    }
    void set_playhead(int32_t i) { playhead = (int8_t)(i >= 0 && i < count ? i : -1); }
    float value() const { return (float)cursor; }
    void turn(int32_t d)
    {
        int32_t c = cursor + d;

        cursor = (int8_t)(c < 0 ? 0 : (c >= count ? count - 1 : c));
    }
    void press() { mask ^= 1u << cursor; }
    void tick(Mode) {}
    void settle(Mode) {}

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        const detail::Iso f = {x, y};

        if (!resona_ui_has_v1(ui)) {
            char t[8];

            resona_fmt_value(t, 8, (float)(cursor + 1), 0);
            ui->text(ui->ctx, x, y, 1, col::MUTED, "STEP");
            ui->text(ui->ctx, x + 30, y, 1, col::INK, t);
            return;
        }
        for (int32_t i = 0; i < count; ++i) {
            const int32_t a = (i % columns) * 14;
            const int32_t b = (i / columns) * 14;
            const bool on = lit(i);
            const int32_t z = on ? 4 : 0;
            uint16_t c = on ? col::CYAN : col::GRID;
            const detail::Pt top[4] = {f.at(a, b, z), f.at(a + 12, b, z), f.at(a + 12, b + 12, z),
                                       f.at(a, b + 12, z)};

            if (i == playhead) {
                c = col::YELLOW;
            }
            if (m != Rest && i == cursor) {
                c = col::INK;
            }
            if (on) {
                const detail::Pt ground[4] = {f.at(a, b, 0), f.at(a + 12, b, 0),
                                              f.at(a + 12, b + 12, 0), f.at(a, b + 12, 0)};

                detail::poly(ui, ground, 4, col::GRID);
            }
            detail::face(ui, top, 4, c, on ? detail::dim(m) : col::BG);
        }
    }
};

/* ---- XY pad ----------------------------------------------------------------------- */

/* A flat plate with a cross and a puck: two values, X and Y. About 9 calls. */
class XYPad {
public:
    int16_t x, y;
    int16_t size;
    int16_t px, py;      /* permille */
    uint8_t axis;        /* which one the encoder moves: 0 x, 1 y */
    const char *label_;

    XYPad &init(int32_t ox, int32_t oy, const char *label, int32_t s = 40)
    {
        x = (int16_t)ox;
        y = (int16_t)oy;
        size = (int16_t)s;
        px = py = 500;
        axis = 0;
        label_ = label ? label : "";
        return *this;
    }
    float value() const { return (float)(axis ? py : px) * 0.001f; }
    float x_value() const { return (float)px * 0.001f; }
    float y_value() const { return (float)py * 0.001f; }
    void set(float fx, float fy)
    {
        px = (int16_t)(fx <= 0 ? 0 : (fx >= 1 ? 1000 : (int32_t)(fx * 1000.0f)));
        py = (int16_t)(fy <= 0 ? 0 : (fy >= 1 ? 1000 : (int32_t)(fy * 1000.0f)));
    }
    void turn(int32_t d)
    {
        int16_t &v = axis ? py : px;
        int32_t n = v + 20 * d;

        v = (int16_t)(n < 0 ? 0 : (n > 1000 ? 1000 : n));
    }
    void press() { axis = axis ? 0 : 1; }
    void tick(Mode) {}
    void settle(Mode) {}

    void draw(const resona_ui *ui, Mode m = Rest) const
    {
        const detail::Iso f = {x, y};
        const int32_t S = size;
        const uint16_t line = detail::ink(m, col::MUTED);

        if (!resona_ui_has_v1(ui)) {
            ui->text(ui->ctx, x, y, 1, line, label_);
            return;
        }
        const detail::Pt plate[4] = {f.at(0, 0, 0), f.at(S, 0, 0), f.at(S, S, 0), f.at(0, S, 0)};
        const int32_t a = (S - 8) * px / 1000 + 4;
        const int32_t b = (S - 8) * (1000 - py) / 1000 + 4;
        const detail::Pt c = f.at(a, b, 0);

        detail::face(ui, plate, 4, line, col::BG);
        resona_ui_line(ui, f.at(S / 2, 0, 0).x, f.at(S / 2, 0, 0).y, f.at(S / 2, S, 0).x,
                       f.at(S / 2, S, 0).y, col::GRID);
        resona_ui_line(ui, f.at(0, S / 2, 0).x, f.at(0, S / 2, 0).y, f.at(S, S / 2, 0).x,
                       f.at(S, S / 2, 0).y, col::GRID);
        resona_ui_ellipse(ui, c.x, c.y, 6, 3, col::GRID);
        resona_ui_ellipse_fill(ui, c.x, c.y - 3, 6, 3, detail::dim(m));
        resona_ui_ellipse(ui, c.x, c.y - 3, 6, 3, detail::ink(m, col::CYAN));
        detail::text_c(ui, x, y + S + 6, 1, m == Rest ? col::MUTED : line, label_);
    }
};

/* ---- the form that holds them ------------------------------------------------------- */

/*
 * Up to 12 widgets on up to 4 pages, a title, a page counter and a footer hint. SELECTOR
 * turns to move the selection, clicks to change page; CUSTOMIZE turns to change the selected
 * widget (it shows yellow until the selection moves), clicks to press it (a toggle flips, a
 * knob goes back to its default) and a long press resets it. handle() reports which widget
 * changed.
 */
class Form {
public:
    enum Kind : uint8_t { KKnob, KFader, KMeter, KToggle, KSteps, KXY };

    struct Slot {
        void *w;
        uint8_t kind;
        uint8_t page;
    };

    static constexpr int kMax = 12;

    const char *title_;
    Slot slots_[kMax];
    uint8_t n_;
    uint8_t page_, pages_;
    int8_t sel_;
    uint8_t editing_;       /* the last thing done was turning the selected widget */
    uint8_t hint_;

    Form &init(const char *title)
    {
        title_ = title ? title : "";
        n_ = 0;
        page_ = 0;
        pages_ = 1;
        sel_ = 0;
        editing_ = 0;
        hint_ = 1;
        return *this;
    }
    Form &hint(bool on) { hint_ = on ? 1 : 0; return *this; }

    /* Add a widget; `page` is 0-based. Returns its id (the order added), or -1 if full. */
    int add(Knob *w, int page = 0) { return push(w, KKnob, page); }
    int add(Fader *w, int page = 0) { return push(w, KFader, page); }
    int add(Meter *w, int page = 0) { return push(w, KMeter, page); }
    int add(Toggle *w, int page = 0) { return push(w, KToggle, page); }
    int add(Steps *w, int page = 0) { return push(w, KSteps, page); }
    int add(XYPad *w, int page = 0) { return push(w, KXY, page); }

    int count() const { return n_; }
    int page() const { return page_; }
    int selected() const { return sel_; }

    /* The value of widget `id` as a float (a toggle is 0 or 1). */
    float value(int id) const
    {
        if (id < 0 || id >= n_) {
            return 0.0f;
        }
        const Slot &s = slots_[id];
        switch (s.kind) {
        case KKnob: return static_cast<const Knob *>(s.w)->value();
        case KFader: return static_cast<const Fader *>(s.w)->value();
        case KMeter: return static_cast<const Meter *>(s.w)->value();
        case KToggle: return static_cast<const Toggle *>(s.w)->value();
        case KSteps: return static_cast<const Steps *>(s.w)->value();
        default: return static_cast<const XYPad *>(s.w)->value();
        }
    }

    /* The encoder events. Returns the id of the widget whose value changed, or -1. */
    int handle(const resona_event &e)
    {
        if (n_ == 0) {
            return -1;
        }
        if (e.input == RESONA_INPUT_SELECTOR) {
            editing_ = 0;
            if (e.kind == RESONA_EVENT_CLICK && pages_ > 1) {
                page_ = (uint8_t)((page_ + 1) % pages_);
                sel_ = first_on_page();
            } else if (e.kind == RESONA_EVENT_TURN) {
                move(e.value);
            }
            return -1;
        }
        if (e.input != RESONA_INPUT_CUSTOMIZE || sel_ < 0) {
            return -1;
        }
        const Slot &s = slots_[sel_];
        if (e.kind == RESONA_EVENT_TURN) {
            editing_ = 1;
            act(s, 0, e.value);
            return sel_;
        }
        editing_ = 0;
        if (e.kind == RESONA_EVENT_CLICK) {
            act(s, 1, 0);
            return sel_;
        }
        if (e.kind == RESONA_EVENT_LONG) {
            act(s, 2, 0);
            return sel_;
        }
        return -1;
    }

    /* Advance the animation one step. Call it from process(): it is cheap. */
    void tick()
    {
        for (int i = 0; i < n_; ++i) {
            anim(slots_[i], mode_of(i), false);
        }
    }

    /* Jump every animation to where it is going (screenshots, tests). */
    void settle()
    {
        for (int i = 0; i < n_; ++i) {
            anim(slots_[i], mode_of(i), true);
        }
    }

    void draw(const resona_ui *ui) const
    {
        char counter[12];

        ui->clear(ui->ctx, col::BG);
        ui->text(ui->ctx, 8, 6, 2, col::INK, title_);
        if (pages_ > 1) {
            int32_t n = resona_fmt_value(counter, 6, (float)(page_ + 1), 0);
            counter[n++] = ' ';
            counter[n++] = '/';
            counter[n++] = ' ';
            n += resona_fmt_value(counter + n, 6, (float)pages_, 0);
            detail::text_r(ui, 312, 8, 1, col::MUTED, counter);
        }
        detail::dotted_rule(ui, 28, col::GRID);
        for (int i = 0; i < n_; ++i) {
            if (slots_[i].page == page_) {
                paint(ui, slots_[i], mode_of(i));
            }
        }
        if (hint_) {
            detail::dotted_rule(ui, 144, col::GRID);
            ui->text(ui->ctx, 8, 155, 1, col::MUTED, "TURN: SELECT");
            ui->text(ui->ctx, 120, 155, 1, col::MUTED, "PRESS: EDIT");
            detail::text_r(ui, 312, 155, 1, col::MUTED, pages_ > 1 ? "CLICK: PAGE" : "HOLD: RESET");
        }
    }

private:
    int push(void *w, uint8_t kind, int page)
    {
        if (n_ >= kMax || page < 0 || page > 3) {
            return -1;
        }
        slots_[n_] = Slot{w, kind, (uint8_t)page};
        if (page + 1 > pages_) {
            pages_ = (uint8_t)(page + 1);
        }
        return n_++;
    }

    Mode mode_of(int i) const
    {
        if (i != sel_ || slots_[i].page != page_) {
            return Rest;
        }
        return editing_ ? Editing : Selected;
    }

    int first_on_page() const
    {
        for (int i = 0; i < n_; ++i) {
            if (slots_[i].page == page_ && slots_[i].kind != KMeter) {
                return i;
            }
        }
        return 0;
    }

    void move(int32_t d)
    {
        int guard = 0;
        int i = sel_;

        while (d != 0 && guard++ < 64) {
            i = (i + (d > 0 ? 1 : n_ - 1)) % n_;
            if (slots_[i].page == page_ && slots_[i].kind != KMeter) {
                sel_ = (int8_t)i;
                d += d > 0 ? -1 : 1;
            }
        }
    }

    static void act(const Slot &s, int what, int32_t d)
    {
        /* what: 0 turn, 1 press, 2 long press (reset) */
        switch (s.kind) {
        case KKnob: {
            Knob *w = static_cast<Knob *>(s.w);
            what == 0 ? w->turn(d) : w->reset();
            break;
        }
        case KFader: {
            Fader *w = static_cast<Fader *>(s.w);
            what == 0 ? w->turn(d) : w->reset();
            break;
        }
        case KToggle: {
            Toggle *w = static_cast<Toggle *>(s.w);
            if (what == 0) {
                w->turn(d);
            } else if (what == 1) {
                w->press();
            } else {
                w->set(false);
            }
            break;
        }
        case KSteps: {
            Steps *w = static_cast<Steps *>(s.w);
            if (what == 0) {
                w->turn(d);
            } else if (what == 1) {
                w->press();
            } else {
                w->mask = 0;
            }
            break;
        }
        case KXY: {
            XYPad *w = static_cast<XYPad *>(s.w);
            if (what == 0) {
                w->turn(d);
            } else if (what == 1) {
                w->press();
            } else {
                w->set(0.5f, 0.5f);
            }
            break;
        }
        default: break;
        }
    }

    static void anim(const Slot &s, Mode m, bool snap)
    {
        switch (s.kind) {
        case KKnob: {
            Knob *w = static_cast<Knob *>(s.w);
            snap ? w->settle(m) : w->tick(m);
            break;
        }
        case KFader: {
            Fader *w = static_cast<Fader *>(s.w);
            snap ? w->settle(m) : w->tick(m);
            break;
        }
        case KToggle: {
            Toggle *w = static_cast<Toggle *>(s.w);
            snap ? w->settle(m) : w->tick(m);
            break;
        }
        default: break;
        }
    }

    static void paint(const resona_ui *ui, const Slot &s, Mode m)
    {
        switch (s.kind) {
        case KKnob: static_cast<const Knob *>(s.w)->draw(ui, m); break;
        case KFader: static_cast<const Fader *>(s.w)->draw(ui, m); break;
        case KMeter: static_cast<const Meter *>(s.w)->draw(ui, m); break;
        case KToggle: static_cast<const Toggle *>(s.w)->draw(ui, m); break;
        case KSteps: static_cast<const Steps *>(s.w)->draw(ui, m); break;
        default: static_cast<const XYPad *>(s.w)->draw(ui, m); break;
        }
    }
};

}  // namespace ui
}  // namespace resona

#endif /* RESONA_WIDGETS_H */
