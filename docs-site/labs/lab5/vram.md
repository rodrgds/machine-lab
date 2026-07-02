# Accessing Video Memory

The linear framebuffer model stores rows sequentially in memory. The top-left
pixel is near the beginning of the mapped region. Later rows come after earlier
rows.

```c
uint8_t *pixel = framebuffer + y * pitch + x * bytes_per_pixel;
```

Before writing through that pointer, confirm the framebuffer is mapped, confirm
`x` and `y` are inside the visible area, and confirm color packing matches the
current mode. The formula is short, but it sits on top of several assumptions.
If any of those assumptions are false, the write can become memory corruption
instead of drawing.

## Color Packing

A pixel is not always a `uint32_t` in the layout you would like. Some modes use
15, 16, 24, or 32 bits per pixel. Some store color channels in different
positions. In Machine Lab the supported modes are intentionally limited, but the
habit should be the same as in a real driver: read the mode information and pack
colors according to that format.

## Clipping

Clipping means trimming a draw operation to the visible screen. It is safer than
trying to write and hoping coordinates are valid. A rectangle that begins partly
off-screen can still have a visible portion, and a sprite with negative
coordinates may still need to draw the pixels that overlap the screen.

Every final project needs clipping, even if the first rectangle test does not
look like it. Menus slide, sprites move, cursors reach edges, and camera systems
show only part of a larger world.
