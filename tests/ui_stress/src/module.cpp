// UI Stress - a Resona module. See the SDK's include/resona/module.h for the rules.
#include <resona/math.h>
#include <resona/module.h>
#include <resona/ui.h>

namespace {

struct State {
    float gain;
    float value[1];
};

const resona_param kParams[] = {
    {"GAIN", -24.0f, 24.0f, 0.0f, "dB"},
};

void init(void *s, const resona_host *) {
    State *st = static_cast<State *>(s);
    st->gain = 1.0f;
    st->value[0] = kParams[0].def;
}

void reset(void *) {}

void set_param(void *s, uint32_t index, float value) {
    State *st = static_cast<State *>(s);
    if (index == 0) {
        st->value[0] = value;
        st->gain = resona_db_to_lin(value);
    }
}

void process(void *s, const float *const *inputs, float *const *outputs, uint32_t frames) {
    const float *in = inputs[0];   // mono: one channel
    float *out = outputs[0];
    const float g = static_cast<State *>(s)->gain;
    for (uint32_t i = 0; i < frames; ++i) out[i] = in[i] * g;
}

void ui_draw(void *s, const resona_ui *ui) {
    const State *st = static_cast<State *>(s);
    const uint16_t K = RESONA_RGB(255, 160, 0);
    ui->clear(ui->ctx, RESONA_RGB(0, 0, 24));
    resona_ui_title(ui, "UI Stress", K);
    resona_ui_param_row(ui, 44, &kParams[0], st->value[0], K);

    /* Extreme coordinates and sizes: clipping must never wrap. */
    ui->rect(ui->ctx, INT32_MIN, 100, 400, 6, RESONA_RGB(255, 0, 0));
    ui->rect(ui->ctx, 100, 100, INT32_MAX, 2, RESONA_RGB(0, 255, 0));
    ui->rect(ui->ctx, INT32_MAX - 3, 100, 100, 6, RESONA_RGB(0, 0, 255));
    ui->rect(ui->ctx, -10, -10, 30, 30, RESONA_RGB(255, 255, 0));
    ui->rect(ui->ctx, 300, 150, 100, 100, RESONA_RGB(0, 255, 255));
    ui->rect(ui->ctx, 50, 50, -20, 10, RESONA_RGB(255, 0, 255));
    ui->rect(ui->ctx, 60, 60, 0, 10, RESONA_RGB(255, 0, 255));
    ui->rect(ui->ctx, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX, RESONA_RGB(9, 9, 9));
    ui->rect(ui->ctx, 10, 120, 1, 1, RESONA_RGB(255, 255, 255));

    /* Text: clamped scales, clipped edges, unknown characters, case. */
    ui->text(ui->ctx, -9, 130, 0, K, "SCALE ZERO");
    ui->text(ui->ctx, 200, 112, 100, K, "BIG");
    ui->text(ui->ctx, 4, 140, 1, RESONA_RGB(255, 255, 255), "lower Case ~@^|{} \200\377 ?");
    ui->text(ui->ctx, 4, 150, 1, RESONA_RGB(0, 255, 0),
             "0123456789012345678901234567890123456789012345678901234567890123456789ZZZ");
    ui->text(ui->ctx, 300, 160, 2, K, "CLIPPED RIGHT");
    ui->text(ui->ctx, 0, 165, 1, K, "");

    /* Flood past the command budget: the tail must not appear. */
    for (int i = 0; i < 200; ++i) {
        ui->rect(ui->ctx, 4 + (i % 100) * 3, 70 + (i / 100) * 4, 2, 3,
                 RESONA_RGB(40 + i, 200 - i, 90));
    }
    ui->rect(ui->ctx, 0, 0, 320, 1, RESONA_RGB(255, 255, 255)); /* dropped */
}

const resona_module kModule = {
    RESONA_MODULE_ABI, sizeof(resona_module),
    "org.resona.test.ui-stress", "UI Stress", "0.1.0",
    RESONA_LAYOUT_MONO, RESONA_RATE_44100 | RESONA_RATE_48000,
    sizeof(State), 1, kParams, nullptr,
    init, reset, set_param, process, ui_draw, nullptr,
};

}  // namespace

extern "C" RESONA_EXPORT const resona_module *resona_module_get(void) { return &kModule; }
