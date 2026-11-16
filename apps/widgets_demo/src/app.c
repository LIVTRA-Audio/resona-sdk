/*
 * Widgets Demo: the Box UI widget kit (sdk/lib/widgets) in a two-page App.
 *
 *   Page 1, AMP: three value tiles. E1 turns move the focus, E2 turns change the focused
 *                value, a click resets it (with a toast).
 *   Page 2, PRESETS: a list. E1 or E2 turns move the selection, a click "loads" it.
 *   E1 long press switches pages.
 *
 * GPL-3.0 licensed, see the SDK's LICENSE.
 */
#include <resona/platform.h>

#include "box_ui.h"

#define PAGES 2
#define TILES 3

static const resona_platform *P;

static const char *const kPresets[] = {
    "CLEAN ROOM", "EDGE OF BREAKUP", "PLEXI CRUNCH", "MODERN HIGH GAIN", "AMBIENT SWELL",
    "SLAPBACK", "FUZZ FACE", "BASS DI",
};
static const char *const kSlots[] = {"01", "02", "03", "04", "05", "06", "07", "08"};

typedef struct {
    const char *label, *unit;
    int32_t min, max, def;
} tile_spec;

static const tile_spec kTiles[TILES] = {
    {"GAIN", "DB", -24, 24, 6},
    {"TONE", "%", 0, 100, 55},
    {"MIX", "%", 0, 100, 100},
};

typedef struct {
    int32_t page;
    int32_t value[TILES];
    boxui_focus focus;
    boxui_list list;
    boxui_toast toast;
    int32_t dirty;
} State;

static void draw(State *s)
{
    boxui_canvas c;
    char text[24];

    if (boxui_begin(&c, P) != 0) return;
    boxui_clear(&c, BOXUI_BG);
    P->format(text, sizeof text, "%d/%d", (int)s->page + 1, PAGES);
    if (s->page == 0) {
        boxui_header(&c, "AMP", text);
        for (int32_t i = 0; i < TILES; ++i) {
            const tile_spec *t = &kTiles[i];
            char value[12];
            int32_t permille = (s->value[i] - t->min) * 1000 / (t->max - t->min);

            P->format(value, sizeof value, t->min < 0 ? "%+d" : "%d", (int)s->value[i]);
            boxui_tile(&c, 8 + i * 103, 40, 98, 104, t->label, value, t->unit, permille,
                       boxui_focus_mode(&s->focus, i));
        }
    } else {
        boxui_header(&c, "PRESETS", text);
        boxui_list_draw(&c, &s->list, 8, 34, 304, 28, BOXUI_SELECTED);
    }
    boxui_page_dots(&c, 160, 160, s->page, PAGES);
    boxui_toast_draw(&c, &s->toast);
    boxui_present(&c, P);
    s->dirty = 0;
}

static int init(void *state, const resona_platform *platform)
{
    State *s = (State *)state;

    P = platform;
    for (int32_t i = 0; i < TILES; ++i) s->value[i] = kTiles[i].def;
    boxui_focus_init(&s->focus, TILES, 0);
    boxui_list_init(&s->list, kPresets, kSlots, (int32_t)(sizeof kPresets / sizeof kPresets[0]), 4);
    s->dirty = 1;
    return 0;
}

static void event(void *state, const resona_app_event *ev)
{
    State *s = (State *)state;
    int32_t delta = 0;
    boxui_action a = boxui_focus_handle(&s->focus, ev, &delta);
    char text[48];

    if (a == BOXUI_NONE) return;
    if (a == BOXUI_BACK) {
        s->page = (s->page + 1) % PAGES;
        /* the focus walks the tiles on page 1 and the list rows on page 2 */
        boxui_focus_init(&s->focus, s->page == 0 ? TILES : s->list.count, 0);
        if (s->page == 1) s->focus.index = s->list.selected;
    } else if (s->page == 0) {
        const tile_spec *t = &kTiles[s->focus.index];
        int32_t *v = &s->value[s->focus.index];

        if (a == BOXUI_ADJUST) {
            *v += delta;
            if (*v < t->min) *v = t->min;
            if (*v > t->max) *v = t->max;
        } else if (a == BOXUI_ACTIVATE) {
            *v = t->def;
            P->format(text, sizeof text, "%s RESET", t->label);
            boxui_toast_show(&s->toast, text, BOXUI_YELLOW, 1200);
        }
    } else {
        if (a == BOXUI_MOVED || a == BOXUI_ADJUST) { /* both encoders scroll the list */
            boxui_list_move(&s->list, a == BOXUI_ADJUST ? delta : s->focus.index - s->list.selected);
            s->focus.index = s->list.selected;
        } else if (a == BOXUI_ACTIVATE) {
            P->format(text, sizeof text, "LOADED %s", kPresets[s->list.selected]);
            boxui_toast_show(&s->toast, text, BOXUI_GREEN, 1500);
        }
    }
    s->dirty = 1;
}

static void frame(void *state, uint32_t ms)
{
    State *s = (State *)state;

    if (boxui_toast_tick(&s->toast, ms)) s->dirty = 1;
    if (s->dirty) draw(s);
}

static const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    RESONA_APP_ID, RESONA_APP_NAME, RESONA_APP_VERSION, /* from resona.project.json */
    sizeof(State), 0,
    init, event, frame,
};

RESONA_EXPORT const resona_app *resona_app_get(void) { return &kApp; }
