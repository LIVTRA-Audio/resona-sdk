# App-owned Unicode fonts

This Box App draws `音量 50%` with a font compiled from its own `assets/` folder.
The two12×12 Chinese demonstration glyphs are original drawings, GPL-3.0 like the rest of the SDK. ASCII
characters use the small built-in font when absent from this font. No OS font,
internal LVGL object or firmware code is used.

Fonts belong to the App or DSP block project. Both can call
`resona_ui_text_font`; block screens retain their per-frame draw-call budget.
The helper emits rectangle runs and needs no heap or font engine on the device.
Scale1..8, glyph metrics and clipping work identically in preview and on Box.
Call `resona_font_valid` once on immutable font data before drawing.

## Use a TTF/OTF you own

Keep the source font and its redistribution notice in your project. On your
computer, install Pillow for the authoring step, then select the glyphs needed:

```sh
python -m pip install Pillow
python /path/to/sdk/tools/font.py assets/fonts/MyFont.ttf \
  --license assets/fonts/LICENSE.txt --text "音量效果预置 0123456789%" \
  --size 16 --name kUiFont --out src/generated/ui_font.h
```

Include that header and draw through your App's `resona_ui`:

```cpp
#include <resona/font.h>
#include "generated/ui_font.h"
// In init: if (!resona_font_valid(&kUiFont)) return RESONA_ERR_ARG;
resona_ui_text_font(ui, 12, 40, 1, color, "音量 50%", &kUiFont);
```

The generated subset and copied `.LICENSE.txt` belong to the project. Verify
that the source font contains your characters, review the preview, and retain
the font's license when distributing the App. The converter does not add
missing glyphs. The resulting App needs no Python, Pillow or source TTF at
runtime. Font data counts toward the App/package's normal resource budgets.

`resona_font_text_advance` measures the longest line's pen advance, including
final spacing. Glyphs support width/height/offset/advance metrics and LF line
breaks. UTF-8 decoding is supported; shaping, bidirectional layout and kerning
require an App-owned layout engine. An App can bundle its own renderer or LVGL
integration using the framebuffer, allocator and ticks, as the SDK's optional
`lib/lvgl_port` does. The OS itself has no LVGL.

Built-in `ui->text` stays a small5×7 font: each unsupported Unicode scalar
draws one replacement glyph. Use this font helper for your own glyph coverage.
Full-circle dot spacing: exact+/-360 arcs
exclude the duplicate endpoint; partial arcs retain both endpoints.
