# Lab 5 Context: Why Video Hardware Looks This Way

Text mode is convenient for shells, but graphical applications need pixels. A
graphics mode turns the screen into a two-dimensional array, and the program's
job becomes writing the right bytes for the right pixels.

Older PC graphics APIs exposed mode numbers and framebuffers. The VBE interface
standardized enough of that behavior for software to select higher-resolution
modes and access linear video memory. Machine Lab keeps that shape because it is
small enough to understand directly: choose a mode, map memory, compute
addresses, write colors, and present the result.

This lab is also a useful correction to the idea that graphics begins with a
large engine. At the lowest useful level, graphics begins with memory layout.
Sprites, text rendering, UI panels, tile maps, particle systems, and games all
eventually become writes into an image buffer or commands that produce one.

## What This Lab Teaches

You will practice graphics mode selection, framebuffer mapping, pitch and pixel
address calculation, clipping, simple sprite formats, and event-driven rendering
loops. These are the building blocks for games, editors, trackers, drawing
tools, and visualizers.
