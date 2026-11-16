/*
 * lvbox: an optional LVGL 9 port for Resona Box Apps.
 *
 * The Resona OS has no UI toolkit. An App that wants LVGL compiles it in (lvgl/ next to this
 * file, configured by lv_conf.h) and pays for it in App size: about 150-200 KB of code for
 * the widget set enabled here, against 512 KB allowed per App. For Box-style screens with
 * no toolkit, see lib/widgets (about 4 KB).
 *
 * What the port does:
 *   - lv_init, with LVGL's heap (LV_MEM_SIZE) taken from P->alloc;
 *   - the tick from P->millis;
 *   - a 320x172 RGB565 display rendering in partial bands; the flush copies each band into the
 *     App's back buffer (P->screen_begin) and presents the changed rectangle once per frame;
 *   - two LVGL encoder input devices, fed from RESONA_APP_EV_INPUT:
 *       E1 (selector)  -> group lvbox_group(RESONA_INPUT_SELECTOR), the default group:
 *                         turn moves the focus, click presses/toggles edit, long = LV_KEY_ESC;
 *       E2 (customize) -> group lvbox_group(RESONA_INPUT_CUSTOMIZE), kept in edit mode, so
 *                         its turns change the focused object (slider, bar, ...) directly.
 *
 *   static int init(void *s, const resona_platform *P) {
 *       if (lvbox_init(P, 0) != 0) return -1;
 *       lv_obj_t *slider = lv_slider_create(lv_screen_active());
 *       lv_group_add_obj(lvbox_group(RESONA_INPUT_CUSTOMIZE), slider);   // E2 drives it
 *       ...                                       // objects created later join the E1 group
 *       return 0;
 *   }
 *   static void event(void *s, const resona_app_event *ev) { lvbox_event(ev); }
 *   static void frame(void *s, uint32_t ms) { lvbox_frame(); }
 *
 * Set the App's heap_bytes to at least lvbox_heap_bytes(rows) (LV_MEM_SIZE + the band).
 *
 * GPL-3.0 licensed, see the SDK's LICENSE. LVGL is GPL-3.0 licensed, see lvgl/LICENCE.txt.
 */
#ifndef RESONA_LVBOX_H
#define RESONA_LVBOX_H

#include <stdint.h>

#include <resona/platform.h>

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default render band: 320 x 24 RGB565 = 15 KB. */
#define LVBOX_DEFAULT_ROWS 24u

/* Bytes of P->alloc the port takes with `rows` render rows (0 = default). */
uint32_t lvbox_heap_bytes(uint32_t rows);

/* Start LVGL. `rows` is the render band height (0 = LVBOX_DEFAULT_ROWS; 172 renders whole
 * frames). Returns 0, or RESONA_ERR_NO_MEMORY. Call once, from the App's init. */
int lvbox_init(const resona_platform *P, uint32_t rows);

/* Feed every App event. Returns 1 if it was an encoder event LVGL took. */
int lvbox_event(const resona_app_event *ev);

/* Run LVGL's timers and redraw what changed; call from the App's frame. */
void lvbox_frame(void);

/* Redraw now (e.g. at the end of init, so the first frame is already on screen). */
void lvbox_refresh(void);

/* The group an encoder drives (RESONA_INPUT_SELECTOR or RESONA_INPUT_CUSTOMIZE). */
lv_group_t *lvbox_group(uint32_t input);
lv_indev_t *lvbox_indev(uint32_t input);
lv_display_t *lvbox_display(void);

#ifdef __cplusplus
}
#endif

#endif /* RESONA_LVBOX_H */
