# VESA BIOS Extensions

VBE is a compatibility interface for discovering and setting graphics modes.
Machine Lab exposes a VBE-shaped API so you can practice the same flow without
needing BIOS calls from a VM.

A typical graphics setup asks for mode information, chooses a supported mode,
requests a linear framebuffer, maps the framebuffer, draws into it, and presents
the result. The exact function names are course-specific, but the order is the
important part. Drawing before a mode is selected or before memory is mapped is
not meaningful.

## Mode Information

Mode information tells you the horizontal resolution, vertical resolution, bits
per pixel, pitch, and framebuffer address. Treat this structure as the source of
truth for drawing helpers. A rectangle function should not silently assume a
hardcoded width just because the first test happens to use a familiar mode.

The linear framebuffer flag also matters. It asks the device to expose the frame
as a flat memory region rather than an older banked model. Banked graphics are
historically interesting, but the linear model is the right teaching target for
this course because it lets the rest of the lab focus on address calculation and
clipping.

> [!IMPORTANT]
> Do not hardcode mode dimensions in drawing helpers. Use the mode information
> returned by the runtime.
