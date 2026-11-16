// Open Serial monitor and enable Serial output before loading this example.
#include <resona/module.h>

namespace {

struct State {
    float gain;
};

void init(void *state, const resona_host *host)
{
    static_cast<State *>(state)->gain = 1.0f;

    // Like println: one message per call; the host adds the newline.
    // Logging is optional. Keep each message within 128 UTF-8 bytes.
    // This SDK callback is for init() only, never the audio process().
    if (host && host->log) {
        host->log("Debug Print: Hello from my custom algorithm!");
        host->log("Debug Print: audio ready (48 kHz, mono).");
    }
}

void reset(void *)
{
    // No signal history to clear; preserve the gain parameter.
}

void set_param(void *state, uint32_t index, float value)
{
    if (index == 0) {
        static_cast<State *>(state)->gain = value;
    }
}

void process(void *state, const float *const *inputs, float *const *outputs,
             uint32_t frames)
{
    const float gain = static_cast<const State *>(state)->gain;
    for (uint32_t i = 0; i < frames; ++i) {
        outputs[0][i] = inputs[0][i] * gain;
    }
    // No logging here: process() must always meet its audio deadline.
}

void draw(void *, const resona_ui *ui)
{
    ui->clear(ui->ctx, 0x0000);
    ui->text(ui->ctx, 12, 12, 2, 0xffff, "DEBUG PRINT");
    ui->text(ui->ctx, 12, 44, 1, 0x07ff, "Check Lab Serial Monitor");
}

const resona_param kParams[] = {
    {"GAIN", 0.0f, 1.0f, 1.0f, ""},
};

const resona_module kModule = {
    RESONA_MODULE_ABI, sizeof(resona_module),
    "org.resona.example.debug_print", "Debug Print", "0.1.0",
    RESONA_LAYOUT_MONO, RESONA_RATE_48000,
    sizeof(State), 1, kParams, nullptr,
    init, reset, set_param, process, draw, nullptr,
};

}  // namespace

extern "C" RESONA_EXPORT const resona_module *resona_module_get(void)
{
    return &kModule;
}
