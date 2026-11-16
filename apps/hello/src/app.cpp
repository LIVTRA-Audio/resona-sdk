// Hello Box: the smallest box app. It shows what an app can do that a pack cannot: own the
// whole screen and both encoders, keep globals, run a C++ constructor before init, and store
// its own data across power cycles.
#include <resona/platform.h>
#include <resona/ui.h>

namespace {

const resona_platform *P;

// A global with a constructor: it runs before init(), proving static construction works.
struct Greeting {
    const char *text;
    int built;
    Greeting() : text("HELLO BOX"), built(1) {}
};
Greeting g_greeting;

struct State {
    int turns[2];
    int clicks;
    int boots;     // read from and written to the app's own storage
    uint32_t ms;   // time since start
    int dirty;
};

void draw(State *s)
{
    uint16_t *px;
    uint32_t stride;
    resona_ui *ui = P->screen_begin(&px, &stride);
    char line[48];

    ui->clear(ui->ctx, RESONA_RGB(10, 12, 18));
    ui->text(ui->ctx, 12, 10, 3, RESONA_RGB(255, 160, 0), g_greeting.built ? g_greeting.text : "NO CTOR");
    P->format(line, sizeof line, "BOOT %d", s->boots);
    ui->text(ui->ctx, 12, 44, 2, RESONA_RGB(232, 227, 211), line);
    P->format(line, sizeof line, "ENC1 %+d  ENC2 %+d", s->turns[0], s->turns[1]);
    ui->text(ui->ctx, 12, 72, 2, RESONA_RGB(39, 206, 212), line);
    P->format(line, sizeof line, "CLICKS %d   T %u S", s->clicks, (unsigned)(s->ms / 1000u));
    ui->text(ui->ctx, 12, 100, 2, RESONA_RGB(232, 227, 211), line);
    // A pixel-level touch: a gradient bar drawn straight into the frame buffer.
    for (uint32_t x = 0; x < 296; ++x) {
        uint16_t c = RESONA_RGB(x * 255u / 296u, 80, 255 - x * 255u / 296u);
        for (uint32_t y = 140; y < 150; ++y) px[y * stride + 12 + x] = c;
    }
    P->screen_present(0, 0, 0, 0);
    s->dirty = 0;
}

int init(void *state, const resona_platform *platform)
{
    State *s = static_cast<State *>(state);
    int boots = 0;

    P = platform;
    if (P->app_store_read(0, &boots, sizeof boots) != RESONA_OK || boots < 0 || boots > 100000) boots = 0;
    s->boots = boots + 1;
    P->app_store_write(0, &s->boots, sizeof s->boots);
    P->log("hello box: started");
    P->timer_start(1, 1000, 0);
    s->dirty = 1;
    return 0;
}

void event(void *state, const resona_app_event *ev)
{
    State *s = static_cast<State *>(state);

    if (ev->kind == RESONA_APP_EV_INPUT) {
        if (ev->u.input.kind == RESONA_EVENT_TURN) s->turns[ev->u.input.input & 1] += ev->u.input.value;
        else s->clicks++;
        s->dirty = 1;
    } else if (ev->kind == RESONA_APP_EV_TIMER) {
        s->dirty = 1;
    }
}

void frame(void *state, uint32_t ms)
{
    State *s = static_cast<State *>(state);

    s->ms += ms;
    if (s->dirty) draw(s);
}

const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    RESONA_APP_ID, RESONA_APP_NAME, RESONA_APP_VERSION, /* from resona.project.json */
    sizeof(State), 0,
    init, event, frame,
};

}  // namespace

extern "C" const resona_app *resona_app_get(void) { return &kApp; }
