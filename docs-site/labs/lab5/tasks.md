# Lab 5 Tasks

Work in `labs/graphics/`. The goal is to build a small graphics library that is
safe enough for final projects. A function that draws correctly only for one
hardcoded mode or one on-screen rectangle is not enough.

## Requested Functions

- `int video_set_mode(uint16_t mode)`
- `int video_map_framebuffer(void)`
- `int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)`
- `int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y)`
- `int video_present(void)`

## Guided Gaps

Store mode information for later drawing, decide how repeated mode setup
behaves, clip drawing before writing pixels, keep color packing clear, and parse
enough XPM for course sprites. These decisions should be visible in the code
because final project debugging often begins by asking whether the framebuffer
helper or the application state is wrong.

> [!IMPORTANT]
> Drawing past the framebuffer is memory corruption. Clip rectangles and sprites
> before writing.

## Common Mistakes

Common mistakes include assuming pitch equals visible row width, forgetting to
map before drawing, writing colors in the wrong byte order, and ignoring
negative sprite coordinates. If a rectangle works in the top-left corner but not
near the right edge, inspect pitch and clipping first.

Next: [checks and references](/labs/lab5/check).
