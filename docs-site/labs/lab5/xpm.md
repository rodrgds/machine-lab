# Pixmaps And XPMs

XPM is a text-based image format that can be embedded directly in C programs.
It is not a modern asset pipeline, but it is excellent for a teaching lab
because students can inspect the image data without a separate binary tool.

For the course sprites, you need enough support for width and height, number of
colors, characters per pixel, the color table, transparent pixels, and pixel
rows. You do not need a full image library. In fact, a small parser is more
useful here because every part of the format remains visible.

## Drawing Sprites

Sprites are rectangles with per-pixel decisions. A transparent pixel should not
overwrite the destination. A sprite that begins partly off-screen should be
clipped rather than rejected entirely. Color values should be converted into the
current mode's pixel format before they are written.

This is the bridge from rectangles to games and tools. A game character, mouse
cursor, icon, tile, note block, or document caret can all be represented as
small images drawn at positions controlled by application state.

> [!TIP]
> Keep XPM parsing separate from sprite drawing. Loading an image and drawing an
> already-loaded pixmap are different responsibilities.
