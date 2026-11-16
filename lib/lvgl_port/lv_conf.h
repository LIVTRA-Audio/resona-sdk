/*
 * LVGL 9 configuration for a Resona Box App (lib/lvgl_port).
 *
 * 320 x 172 RGB565, no OS, no C library: LVGL's own allocator, string and sprintf, its heap
 * taken from the App's platform heap (P->alloc) at lv_init. Software rendering only, one
 * font (Montserrat 14), and only the widgets the Box needs: label, button, bar, slider,
 * list (and image, which list uses for icons). Everything not set here takes LVGL's
 * default from lvgl/src/lv_conf_internal.h, so features that default to on are switched off
 * below explicitly. Turn a widget on (LV_USE_xxx 1) and add the .c files of lvgl/src/widgets/xxx/ to
 * your project's sources; each one costs App code.
 *
 * LVGL finds this file because it sits next to the lvgl/ folder.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#if !defined(__ASSEMBLER__) && !defined(__ASSEMBLY__)
/* Provided by lvbox.c: the heap pool from P->alloc, and an assert that logs and stops. */
#ifdef __cplusplus
extern "C" {
#endif
void *lvbox_pool_alloc(unsigned int bytes);
void lvbox_assert_failed(void);
#ifdef __cplusplus
}
#endif
#endif

/* ---- Colour, memory, timing ------------------------------------------------------------ */
#define LV_COLOR_DEPTH 16

#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_SPRINTF_USE_FLOAT 0
/* The App toolchain is freestanding: no <inttypes.h> (LVGL only needs it for log formats). */
#define LV_STDINT_INCLUDE <stdint.h>
#define LV_STDDEF_INCLUDE <stddef.h>
#define LV_STDBOOL_INCLUDE <stdbool.h>
#define LV_INTTYPES_INCLUDE <stdint.h>
#define LV_LIMITS_INCLUDE <limits.h>
#if defined(__GNUC__) && defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 0 && !defined(_LIBC_LIMITS_H_)
/* GCC's own <limits.h> has every limit LVGL needs; this tells it not to chain to a C library's. */
#define _LIBC_LIMITS_H_ 1
#endif
#define LV_STDARG_INCLUDE <stdarg.h>

/* LVGL's heap. The pool comes from the App's platform heap (lvbox_pool_alloc calls P->alloc),
 * so it counts against the App's heap_bytes rather than its .bss. A label/slider/list screen
 * with the default theme peaks under 18 KB (measured in the 64-bit desktop model; less on
 * the Box); raise it for bigger screens. */
#define LV_MEM_SIZE (40 * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR 0
#define LV_MEM_POOL_ALLOC lvbox_pool_alloc

#define LV_DEF_REFR_PERIOD 33 /* the Box calls the App's frame about 30 times a second */
#define LV_DPI_DEF 130

#define LV_USE_OS LV_OS_NONE

/* ---- Rendering --------------------------------------------------------------------------- */
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (8 * 1024U)

#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB565A8 0
#define LV_DRAW_SW_SUPPORT_RGB888 1   /* gradients use it */
#define LV_DRAW_SW_SUPPORT_XRGB8888 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 1 /* layers and transparent images */
#define LV_DRAW_SW_SUPPORT_L8 0
#define LV_DRAW_SW_SUPPORT_AL88 0
#define LV_DRAW_SW_SUPPORT_A8 1
#define LV_DRAW_SW_SUPPORT_I1 0
#define LV_DRAW_SW_DRAW_UNIT_CNT 1
#define LV_USE_DRAW_ARM2D_SYNC 0
#define LV_USE_NATIVE_HELIUM_ASM 0
#define LV_DRAW_SW_COMPLEX 1 /* rounded corners, arcs, slider knobs */
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0

#define LV_USE_DRAW_VGLITE 0
#define LV_USE_PXP 0
#define LV_USE_DRAW_DAVE2D 0
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_VG_LITE_THORVG 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0

/* ---- Diagnostics: off (no printf on the Box; P->log is the App's console) ----------------- */
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0
#define LV_ASSERT_HANDLER lvbox_assert_failed();
#define LV_USE_REFR_DEBUG 0
#define LV_USE_LAYER_DEBUG 0
#define LV_USE_PARALLEL_DRAW_DEBUG 0
#define LV_USE_PROFILER 0
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/* ---- Small caches and features ----------------------------------------------------------- */
#define LV_CACHE_DEF_SIZE 0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_OBJ_STYLE_CACHE 0
#define LV_USE_OBJ_ID 0
#define LV_USE_OBJ_ID_BUILTIN 0
#define LV_USE_OBJ_PROPERTY 0
#define LV_USE_FLOAT 0
#define LV_USE_MATRIX 0
#define LV_USE_PRIVATE_API 0
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ENABLE_GLOBAL_CUSTOM 0

/* ---- One font ------------------------------------------------------------------------------ */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_PLACEHOLDER 1

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/* ---- Widgets: label, button, bar, slider, list, image ------------------------------------- */
#define LV_WIDGETS_HAS_DEFAULT_VALUE 1
#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 0
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_IMAGE 1
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_LABEL_TEXT_SELECTION 0
#define LV_LABEL_LONG_TXT_HINT 0
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_LIST 1
#define LV_USE_LOTTIE 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 1
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_SWITCH 0
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0

/* ---- Theme and layout ---------------------------------------------------------------------- */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 0
#define LV_THEME_DEFAULT_TRANSITION_TIME 80
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0
#define LV_USE_FLEX 1 /* list uses it */
#define LV_USE_GRID 0

/* ---- No file systems, decoders, extras, drivers, examples ---------------------------------- */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_MEMFS 0
#define LV_USE_FS_LITTLEFS 0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD 0
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_TJPGD 0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF 0
#define LV_BIN_DECODER_RAM_LOAD 0
#define LV_USE_RLE 0
#define LV_USE_QRCODE 0
#define LV_USE_BARCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_TINY_TTF 0
#define LV_USE_RLOTTIE 0
#define LV_USE_LZ4_INTERNAL 0
#define LV_USE_LZ4_EXTERNAL 0
#define LV_USE_FFMPEG 0
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY 0
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT 0
#define LV_USE_OBSERVER 0
#define LV_USE_IME_PINYIN 0
#define LV_USE_FILE_EXPLORER 0
#define LV_USE_SDL 0
#define LV_USE_X11 0
#define LV_USE_WAYLAND 0
#define LV_USE_LINUX_FBDEV 0
#define LV_USE_NUTTX 0
#define LV_USE_LINUX_DRM 0
#define LV_USE_TFT_ESPI 0
#define LV_USE_EVDEV 0
#define LV_USE_LIBINPUT 0
#define LV_USE_ST7735 0
#define LV_USE_ST7789 0
#define LV_USE_ST7796 0
#define LV_USE_ILI9341 0
#define LV_USE_RENESAS_GLCDC 0
#define LV_USE_WINDOWS 0
#define LV_USE_OPENGLES 0
#define LV_USE_QNX 0
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_RENDER 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0
#define LV_USE_DEMO_FLEX_LAYOUT 0
#define LV_USE_DEMO_MULTILANG 0
#define LV_USE_DEMO_TRANSFORM 0
#define LV_USE_DEMO_SCROLL 0
#define LV_USE_DEMO_VECTOR_GRAPHIC 0

#endif /* LV_CONF_H */
