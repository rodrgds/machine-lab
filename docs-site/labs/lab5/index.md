# Lab 5: Video Card In Graphics Mode

Generated folder: `labs/graphics/`

Lab 5 turns the screen into memory. You select a VBE-style graphics mode, map a
linear framebuffer, compute pixel addresses, draw rectangles, parse small XPM
sprites, and present frames.

This is the lab where the earlier device work starts to look like an
application platform. Timers provide rhythm, keyboard and mouse input provide
events, and the video device provides pixels. A final project is mostly the
careful composition of those pieces with a state model of your own.

Begin with [why video cards look like memory devices](/labs/lab5/context), then
study [the video card model](/labs/lab5/video-card), [VBE mode setup](/labs/lab5/vbe),
and [video memory](/labs/lab5/vram). Once the addressing model is clear, draw
[rectangles](/labs/lab5/rectangles), draw [XPM sprites](/labs/lab5/xpm), and
use those pieces to build [reactive graphical applications](/labs/lab5/reactive-apps).
The [implementation tasks](/labs/lab5/tasks) collect the required functions, and
the last page compares this model with [modern graphics stacks](/labs/lab5/modern).

> [!TIP]
> Start with one pixel address. Rectangles, sprites, and UI widgets are all built
> from that one calculation.
