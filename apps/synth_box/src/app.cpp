// Synth Box: a box app that turns the Resona Box into a small synthesizer.
//
//   - finds the Tiny Synth pack in the box's storage and runs it in the first block;
//     the factory blocks are switched off, so the synth goes straight to the output
//   - an 8-step sequencer (platform timers) plays it; the pattern is kept in app storage
//   - MIDI notes from USB play it too
//
// Encoder 1: turn = choose a step, click = step on/off, long = waveform
// Encoder 2: turn = the step's note, click = play/stop, long = filter cutoff up an octave
#include <resona/platform.h>
#include <resona/ui.h>
#include <resona/effects.hpp>

namespace {

const resona_platform *P;
resona::Effects g_fx;
resona_effect_handle g_synth;
int g_owned;

constexpr uint32_t kSteps = 8;
constexpr uint32_t kTimerStep = 1;
constexpr uint32_t kTimerGateOff = 2;
constexpr uint32_t kStepMs = 150;      // 100 BPM in 16ths
constexpr uint32_t kStoreMagic = 0x53594E31;  // 'SYN1'

struct Pattern {
    uint32_t magic;
    uint8_t note[kSteps];
    uint8_t on[kSteps];
};

struct State {
    Pattern pat;
    int ready;          // Tiny Synth is running in the App's effect graph
    int playing;
    uint32_t step;      // the step that sounds
    uint32_t cursor;    // the step being edited
    int note_param, gate_param, wave_param, cutoff_param;  // indices, found by name
    int wave;
    float cutoff;
    int midi_note;      // a note held from MIDI, or -1
    uint8_t out_level;
    int dirty;
};

const uint16_t kBg = RESONA_RGB(6, 8, 20);
const uint16_t kInk = RESONA_RGB(226, 232, 240);
const uint16_t kDim = RESONA_RGB(70, 78, 100);
const uint16_t kHot = RESONA_RGB(255, 92, 140);
const uint16_t kCyan = RESONA_RGB(60, 210, 230);

bool streq(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

// Tiny Synth runs in an App-owned effect graph: the App owns audio while it runs.
int find_param(const char *name)
{
    for (uint32_t i = 0; i < 32; ++i) {
        resona_effect_param pi;
        if (P->effect_param_info(g_synth, i, &pi) != RESONA_OK) break;
        if (streq(pi.name, name)) return (int)i;
    }
    return -1;
}

void setup_synth(State *s)
{
    s->ready = 0;
    if (!g_synth) {
        if (!g_owned) { if (g_fx.begin(P)) return; g_owned = 1; }
        g_synth = g_fx.pack("org.resona.example.tiny-synth");
        if (!g_synth) return;
        g_fx.clear_routes();
        g_fx.output(g_synth);
        g_fx.connect(g_fx.Usb, g_fx.Output, 1.0f, 0, 0); g_fx.connect(g_fx.Usb, g_fx.Output, 1.0f, 1, 1);
        g_fx.connect(g_fx.Bluetooth, g_fx.Output, 1.0f, 0, 0); g_fx.connect(g_fx.Bluetooth, g_fx.Output, 1.0f, 1, 1);
        g_fx.capture(g_synth);
        if (g_fx.commit()) { g_synth = 0; return; }
    }
    s->note_param = find_param("NOTE");
    s->gate_param = find_param("GATE");
    s->wave_param = find_param("WAVE");
    s->cutoff_param = find_param("CUTOFF");
    s->ready = s->note_param >= 0 && s->gate_param >= 0;
    if (s->cutoff_param >= 0) {
        resona_effect_param pi;
        if (P->effect_param_info(g_synth, (uint32_t)s->cutoff_param, &pi) == RESONA_OK) s->cutoff = pi.def;
    }
}

void gate(State *s, int note, int on)
{
    if (!s->ready) return;
    if (on) P->effect_param_set(g_synth, (uint32_t)s->note_param, (float)note);
    P->effect_param_set(g_synth, (uint32_t)s->gate_param, on ? 1.0f : 0.0f);
}

void save_pattern(State *s)
{
    s->pat.magic = kStoreMagic;
    P->app_store_write(0, &s->pat, sizeof s->pat);
}

const char *note_name(int n, char *buf)
{
    static const char *const names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    P->format(buf, 8, "%s%d", names[n % 12], n / 12 - 1);
    return buf;
}

// Two octaves from C3 (48); the sounding note is lit.
void draw_keyboard(resona_ui *ui, int lit)
{
    const int x0 = 12, y0 = 104, ww = 19, wh = 56, bw = 12, bh = 34;
    static const int white[7] = {0, 2, 4, 5, 7, 9, 11};
    static const int black[5] = {1, 3, 6, 8, 10};
    static const int black_x[5] = {1, 2, 4, 5, 6};
    for (int o = 0; o < 2; ++o) {
        for (int k = 0; k < 7; ++k) {
            int note = 48 + o * 12 + white[k];
            int x = x0 + (o * 7 + k) * (ww - 1) + o * 0;
            ui->rect(ui->ctx, x, y0, ww - 2, wh, note == lit ? kHot : kInk);
        }
        for (int k = 0; k < 5; ++k) {
            int note = 48 + o * 12 + black[k];
            int x = x0 + (o * 7 + black_x[k]) * (ww - 1) - bw / 2 - 1;
            ui->rect(ui->ctx, x, y0, bw, bh, note == lit ? kHot : RESONA_RGB(20, 22, 34));
        }
    }
}

void draw(State *s)
{
    uint16_t *px;
    uint32_t stride;
    resona_ui *ui = P->screen_begin(&px, &stride);
    char buf[40], nb[8];

    ui->clear(ui->ctx, kBg);
    ui->text(ui->ctx, 12, 8, 2, kHot, "SYNTH BOX");
    if (!s->ready) {
        ui->text(ui->ctx, 12, 50, 2, kInk, "INSTALL TINY SYNTH");
        ui->text(ui->ctx, 12, 74, 1, kDim, "RESONA-LAB PACK TRY / INSTALL");
        P->screen_present(0, 0, 0, 0);
        s->dirty = 0;
        return;
    }
    static const char *const waves[3] = {"SAW", "SQUARE", "SINE"};
    P->format(buf, sizeof buf, "%s  %s  %d HZ", s->playing ? "PLAY" : "STOP", waves[s->wave % 3], (int)s->cutoff);
    ui->text(ui->ctx, 130, 12, 1, s->playing ? kCyan : kDim, buf);

    // Steps: dots that light when they sound; the edited one carries a ring.
    for (uint32_t i = 0; i < kSteps; ++i) {
        int cx = 30 + (int)i * 37, cy = 50;
        uint16_t c = !s->pat.on[i] ? kDim : (s->playing && i == s->step ? kHot : kCyan);
        ui->ellipse_fill(ui->ctx, cx, cy, 11, 11, c);
        if (i == s->cursor) ui->ellipse(ui->ctx, cx, cy, 15, 15, kInk);
        ui->text(ui->ctx, cx - 9, 72, 1, s->pat.on[i] ? kInk : kDim, note_name(s->pat.note[i], nb));
    }
    // Output level, straight into the frame buffer: a thin bar under the steps.
    uint32_t w = (uint32_t)s->out_level * 32u;
    for (uint32_t x = 0; x < 296; ++x)
        for (uint32_t y = 90; y < 93; ++y)
            px[y * stride + 12 + x] = x < w ? kHot : RESONA_RGB(24, 28, 44);
    int lit = s->midi_note >= 0 ? s->midi_note : (s->playing && s->pat.on[s->step] ? s->pat.note[s->step] : -1);
    draw_keyboard(ui, lit);
    P->screen_present(0, 0, 0, 0);
    s->dirty = 0;
}

int init(void *state, const resona_platform *platform)
{
    State *s = static_cast<State *>(state);
    P = platform;
    if (P->app_store_read(0, &s->pat, sizeof s->pat) != RESONA_OK || s->pat.magic != kStoreMagic) {
        static const uint8_t riff[kSteps] = {57, 60, 64, 69, 64, 60, 55, 59};
        for (uint32_t i = 0; i < kSteps; ++i) { s->pat.note[i] = riff[i]; s->pat.on[i] = 1; }
    }
    s->midi_note = -1;
    P->input_config(RESONA_INPUT_SELECTOR, 0);
    P->input_config(RESONA_INPUT_CUSTOMIZE, 0);
    setup_synth(s);
    s->playing = s->ready;  // start playing straight away
    if (s->playing) P->timer_start(kTimerStep, kStepMs, 0);
    P->log(s->ready ? "synth box: tiny synth in block 1, playing" : "synth box: tiny synth not installed");
    s->dirty = 1;
    return 0;
}

void step(State *s)
{
    s->step = (s->step + 1) % kSteps;
    if (s->pat.on[s->step]) {
        gate(s, s->pat.note[s->step], 1);
        P->timer_start(kTimerGateOff, kStepMs * 2 / 3, 1);
    }
    s->dirty = 1;
}

void event(void *state, const resona_app_event *ev)
{
    State *s = static_cast<State *>(state);

    switch (ev->kind) {
    case RESONA_APP_EV_TIMER:
        if (ev->u.timer.id == kTimerStep) step(s);
        else if (ev->u.timer.id == kTimerGateOff && s->midi_note < 0) gate(s, 0, 0);
        break;
    case RESONA_APP_EV_MIDI: {
        const uint8_t *m = ev->u.midi.data;
        if (ev->u.midi.len >= 3 && (m[0] & 0xE0) == 0x80) {
            int on = (m[0] & 0xF0) == 0x90 && m[2] > 0;
            if (on) { s->midi_note = m[1]; gate(s, m[1], 1); }
            else if (m[1] == s->midi_note) { s->midi_note = -1; gate(s, 0, 0); }
            s->dirty = 1;
        }
        break;
    }
    case RESONA_APP_EV_INPUT: {
        const resona_event &in = ev->u.input;
        if (in.input == RESONA_INPUT_SELECTOR) {
            if (in.kind == RESONA_EVENT_TURN) s->cursor = (s->cursor + kSteps + (uint32_t)(in.value % (int)kSteps)) % kSteps;
            else if (in.kind == RESONA_EVENT_CLICK) { s->pat.on[s->cursor] ^= 1; save_pattern(s); }
            else if (in.kind == RESONA_EVENT_LONG && s->wave_param >= 0) {
                s->wave = (s->wave + 1) % 3;
                P->effect_param_set(g_synth, (uint32_t)s->wave_param, (float)s->wave);
            }
        } else {
            if (in.kind == RESONA_EVENT_TURN) {
                int n = s->pat.note[s->cursor] + in.value;
                s->pat.note[s->cursor] = (uint8_t)(n < 36 ? 36 : (n > 84 ? 84 : n));
                save_pattern(s);
            } else if (in.kind == RESONA_EVENT_CLICK) {
                s->playing = !s->playing && s->ready;
                if (s->playing) P->timer_start(kTimerStep, kStepMs, 0);
                else { P->timer_stop(kTimerStep); gate(s, 0, 0); }
            } else if (in.kind == RESONA_EVENT_LONG && s->cutoff_param >= 0) {
                s->cutoff = s->cutoff * 2.0f > 12000.0f ? 200.0f : s->cutoff * 2.0f;
                P->effect_param_set(g_synth, (uint32_t)s->cutoff_param, s->cutoff);
            }
        }
        s->dirty = 1;
        break;
    }
    case RESONA_APP_EV_CHANGED:
        if (ev->u.changed.what == RESONA_CHANGED_MODELS && !s->ready) setup_synth(s);
        s->dirty = 1;
        break;
    default:
        break;
    }
}

void frame(void *state, uint32_t)
{
    State *s = static_cast<State *>(state);
    uint8_t in = 0, out = 0;
    P->meters(&in, &out, nullptr);
    if (out != s->out_level) { s->out_level = out; s->dirty = 1; }
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
