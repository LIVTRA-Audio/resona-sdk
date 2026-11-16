/*
 * lvbox: LVGL 9 display, tick and encoder port for Resona Box Apps. See lvbox.h.
 * GPL-3.0 licensed, see the SDK's LICENSE.
 */
#include "lvbox.h"

#define LVBOX_W 320
#define LVBOX_H 172

typedef struct {
    lv_indev_t *indev;
    lv_group_t *group;
    int32_t diff;  /* detents not yet read */
    uint8_t press; /* 1 while a click's press is being read */
} lvbox_encoder;

static const resona_platform *g_platform;
static lv_display_t *g_display;
static lvbox_encoder g_enc[2];
/* The rectangle flushed this frame, presented once at the last band. */
static int32_t g_x1, g_y1, g_x2, g_y2;
static int g_dirty;

/* ---- Hooks lv_conf.h names ----------------------------------------------------------------- */

void *lvbox_pool_alloc(unsigned int bytes)
{
    return g_platform ? g_platform->alloc(bytes, 8) : NULL;
}

void lvbox_assert_failed(void)
{
    if (g_platform) g_platform->log("lvgl: assertion failed");
    for (;;) {
    }
}

/* ---- Display --------------------------------------------------------------------------------- */

static void flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t *fb = NULL;
    uint32_t stride = 0;
    const int32_t w = lv_area_get_width(area);
    const uint16_t *src = (const uint16_t *)px_map;

    g_platform->screen_begin(&fb, &stride);
    if (fb && stride) {
        for (int32_t y = area->y1; y <= area->y2; ++y) {
            if (y < 0 || y >= LVBOX_H) {
                src += w;
                continue;
            }
            uint16_t *dst = fb + (uint32_t)y * stride;
            for (int32_t x = 0; x < w; ++x) {
                const int32_t dx = area->x1 + x;
                if (dx >= 0 && dx < LVBOX_W) dst[dx] = src[x];
            }
            src += w;
        }
        if (!g_dirty) {
            g_x1 = area->x1;
            g_y1 = area->y1;
            g_x2 = area->x2;
            g_y2 = area->y2;
            g_dirty = 1;
        } else {
            if (area->x1 < g_x1) g_x1 = area->x1;
            if (area->y1 < g_y1) g_y1 = area->y1;
            if (area->x2 > g_x2) g_x2 = area->x2;
            if (area->y2 > g_y2) g_y2 = area->y2;
        }
    }
    if (lv_display_flush_is_last(disp) && g_dirty) {
        g_platform->screen_present(g_x1, g_y1, g_x2 - g_x1 + 1, g_y2 - g_y1 + 1);
        g_dirty = 0;
    }
    lv_display_flush_ready(disp);
}

/* ---- Encoders ---------------------------------------------------------------------------------- */

static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    lvbox_encoder *e = (lvbox_encoder *)lv_indev_get_user_data(indev);

    data->enc_diff = (int16_t)e->diff;
    e->diff = 0;
    data->state = e->press ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static int encoder_setup(lvbox_encoder *e, int make_default)
{
    e->group = lv_group_create();
    e->indev = lv_indev_create();
    if (!e->group || !e->indev) return -1;
    lv_indev_set_type(e->indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(e->indev, encoder_read);
    lv_indev_set_user_data(e->indev, e);
    lv_indev_set_display(e->indev, g_display);
    lv_indev_set_group(e->indev, e->group);
    /* Gestures arrive as events, so read on demand rather than polling on a timer. */
    lv_indev_set_mode(e->indev, LV_INDEV_MODE_EVENT);
    if (make_default) lv_group_set_default(e->group);
    return 0;
}

/* ---- Public ------------------------------------------------------------------------------------ */

uint32_t lvbox_heap_bytes(uint32_t rows)
{
    if (rows == 0) rows = LVBOX_DEFAULT_ROWS;
    if (rows > LVBOX_H) rows = LVBOX_H;
    return LV_MEM_SIZE + LVBOX_W * rows * 2u + 64u;
}

int lvbox_init(const resona_platform *P, uint32_t rows)
{
    void *band;
    uint32_t band_bytes;

    if (!P) return RESONA_ERR_ARG;
    if (rows == 0) rows = LVBOX_DEFAULT_ROWS;
    if (rows > LVBOX_H) rows = LVBOX_H;
    g_platform = P;
    lv_init();
    if (!lv_is_initialized()) return RESONA_ERR_NO_MEMORY;
    lv_tick_set_cb(P->millis);

    g_display = lv_display_create(LVBOX_W, LVBOX_H);
    band_bytes = LVBOX_W * rows * 2u;
    band = P->alloc(band_bytes, LV_DRAW_BUF_ALIGN < 8 ? 8 : LV_DRAW_BUF_ALIGN);
    if (!g_display || !band) return RESONA_ERR_NO_MEMORY;
    lv_display_set_color_format(g_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(g_display, band, NULL, band_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(g_display, flush);

    if (encoder_setup(&g_enc[0], 1) != 0 || encoder_setup(&g_enc[1], 0) != 0)
        return RESONA_ERR_NO_MEMORY;
    return RESONA_OK;
}

int lvbox_event(const resona_app_event *ev)
{
    lvbox_encoder *e;
    const resona_event *in;

    if (!g_display || !ev || ev->kind != RESONA_APP_EV_INPUT) return 0;
    in = &ev->u.input;
    if (in->input > RESONA_INPUT_CUSTOMIZE) return 0;
    e = &g_enc[in->input];
    switch (in->kind) {
    case RESONA_EVENT_TURN:
        /* E2's group stays in edit mode: its turns change the object, not the focus. */
        if (in->input == RESONA_INPUT_CUSTOMIZE) lv_group_set_editing(e->group, true);
        e->diff += in->value;
        lv_indev_read(e->indev);
        return 1;
    case RESONA_EVENT_CLICK:
    case RESONA_EVENT_DOUBLE:
        e->press = 1; /* a click is one gesture: read it as a press and a release */
        lv_indev_read(e->indev);
        e->press = 0;
        lv_indev_read(e->indev);
        return 1;
    case RESONA_EVENT_LONG:
        lv_group_send_data(e->group, LV_KEY_ESC);
        lv_group_set_editing(e->group, false);
        return 1;
    default:
        return 0;
    }
}

void lvbox_frame(void)
{
    if (g_display) lv_timer_handler();
}

void lvbox_refresh(void)
{
    if (g_display) lv_refr_now(g_display);
}

lv_group_t *lvbox_group(uint32_t input)
{
    return input <= RESONA_INPUT_CUSTOMIZE ? g_enc[input].group : NULL;
}

lv_indev_t *lvbox_indev(uint32_t input)
{
    return input <= RESONA_INPUT_CUSTOMIZE ? g_enc[input].indev : NULL;
}

lv_display_t *lvbox_display(void)
{
    return g_display;
}
