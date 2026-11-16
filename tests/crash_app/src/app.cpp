// Crash Test: writes through a null pointer in init(). The platform must recover: the box
// reboots, sees that the app never reached a stable run, and starts in safe mode.
#include <resona/platform.h>

namespace {

int init(void *, const resona_platform *p)
{
    p->log("crash test: about to fault");
    volatile uint32_t *bad = reinterpret_cast<volatile uint32_t *>(0x4u);
    *bad = 0xDEADBEEFu;
    return 0;
}
void event(void *, const resona_app_event *) {}
void frame(void *, uint32_t) {}

const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app), "org.resona.test.crash", "Crash Test", "0.1.0",
    0, 0, init, event, frame,
};

}  // namespace

extern "C" const resona_app *resona_app_get(void) { return &kApp; }
