# Lab 5: VBE Framebuffer And XPM

Generated folder: `labs/graphics/`

Lab 5 turns the screen into memory. You select a VBE-style graphics mode, map a
linear framebuffer, compute pixel addresses, draw rectangles, parse small XPM
sprites, and present frames to the runtime.

This is the point where many final projects become visible: games, editors,
trackers, visualizers, and UI tools all need a reliable drawing layer.

## Why This Matters

High-level graphics libraries hide the framebuffer. This lab shows the core
idea underneath: a screen can be represented as rows of pixels in memory, and a
program can draw by writing bytes at the right offsets.

> [!NOTE]
> Machine Lab does not ask you to write a GPU driver. It gives you a VBE-shaped
> interface so you can practice mode setup, mapping, pitch calculation, clipping,
> and pixel writes in portable C.

## Device Model

| Concept | Meaning |
| --- | --- |
| mode info | width, height, pitch, bits per pixel, framebuffer address |
| linear framebuffer | contiguous memory where pixel data is written |
| pitch | bytes between the start of one row and the next |
| color format | how a `uint32_t` color becomes bytes in memory |
| present | tells the runtime to display the frame you prepared |

Pitch is not always `width * bytes_per_pixel`, so address calculation must use
the pitch reported by mode information.

## Plan

1. Query mode information.
2. Set the requested graphics mode.
3. Map the framebuffer exactly once.
4. Convert `(x, y)` into `y * pitch + x * bytes_per_pixel`.
5. Fill clipped rectangles.
6. Parse a simple XPM and skip transparent pixels.
7. Present only after drawing.

> [!IMPORTANT]
> Drawing past the framebuffer is memory corruption. Clip rectangles and sprites
> before writing pixels.

## Requested Functions

- `int video_set_mode(uint16_t mode)`
- `int video_map_framebuffer(void)`
- `int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)`
- `int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y)`
- `int video_present(void)`

## Guided Gaps

- Store mode info so later functions know pitch and bytes per pixel.
- Decide when repeated mode setup should succeed or fail.
- Clip drawing instead of writing past the framebuffer.
- Keep color packing clear for 8, 15, 16, 24, and 32-bit modes if supported.
- Parse enough XPM for course sprites without writing a full image library.
- Separate drawing functions from application state machines.

> [!TIP]
> Implement one-pixel address calculation first, then build rectangles and
> sprites on top of it. Most graphics bugs start as one wrong offset.

## Common Mistakes

- Assuming pitch equals visible row width.
- Forgetting to map the framebuffer before drawing.
- Writing colors in the wrong byte order.
- Ignoring negative sprite coordinates.
- Presenting before drawing or forgetting to present after drawing.

## Discussion Prompts

- Why is pitch not always `width * bytes_per_pixel`?
- What changes when you double-buffer?
- Which rendering helpers would you want before starting a final game?
- How should an editor or music tracker divide application state from drawing?

## Check

```sh
machinelab test graphics
machinelab run --headless --dump-frame build/rectangle.ppm -- build/examples/vbe_rectangle
```

## External Reading

- OSDev VESA Video Modes: <https://wiki.osdev.org/VESA_Video_Modes>
- VESA BIOS Extensions 2.0 PDF: <https://www.phatcode.net/res/221/files/vbe20.pdf>
- XPM format overview: <https://en.wikipedia.org/wiki/X_PixMap>
