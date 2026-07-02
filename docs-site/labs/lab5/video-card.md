# The Video Card

A video card is a device that turns memory and commands into display output.
Modern GPUs are large parallel processors, but older educational interfaces keep
the visible model simple:

| Concept | Role |
| --- | --- |
| mode | resolution and pixel format |
| framebuffer | memory backing the visible image |
| pitch | bytes from one row start to the next |
| color format | how color channels are packed |
| present | make the prepared frame visible |

The mode describes the dimensions and pixel format. The framebuffer is the
memory region that stores pixels. The pitch tells you how many bytes separate
the beginning of one row from the beginning of the next. These values must come
from mode information, not from guesses in drawing code.

## Text Mode vs Graphics Mode

Text mode stores character cells. It is small and efficient for terminals
because the hardware or firmware interprets bytes as glyphs and attributes.
Graphics mode stores pixels. It gives applications control over every point on
the screen, which makes sprites, games, image viewers, and visual tools
possible.

## Why Pitch Exists

Rows may include padding for alignment or device constraints. Therefore:

```text
offset = y * pitch + x * bytes_per_pixel
```

not:

```text
offset = (y * width + x) * bytes_per_pixel
```

This is one of the most important formulas in the lab. If it is wrong,
everything built on top of it will look random: rectangles tear, sprites drift,
and writes near the right edge corrupt the next row.
