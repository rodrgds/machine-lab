# Drawing Rectangles

Rectangles are the first useful drawing primitive. They can represent
backgrounds, panels, buttons, health bars, cursors, tile blocks, selection
regions, and debug overlays.

The algorithm is straightforward: clip the rectangle to the screen, compute the
row start once for each row, write pixels from left to right, and present after
the draw operation or after a batch of operations. The important part is not the
loop itself; it is making the bounds and address calculation correct before the
loop writes memory.

## Building Up Slowly

Start by drawing one pixel. Then draw a horizontal row. Then draw a small
rectangle that is completely inside the screen. Only after that should you test
rectangles that touch or cross the edges. This sequence makes bugs easier to
localize: one pixel tests address calculation, one row tests horizontal
iteration, and a rectangle tests row stepping through pitch.

## Common Bugs

The usual errors are off-by-one width/height loops, writing past the right edge,
using `width * bytes_per_pixel` instead of pitch, and presenting inside every
inner loop. The last bug may still produce the correct image, but it teaches the
wrong rendering model. Draw a frame, then present it.

> [!TIP]
> Draw a one-pixel rectangle, then a row, then a full rectangle. Each step tests
> one level of address calculation.
