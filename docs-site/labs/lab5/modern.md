# How It Works Today

Modern graphics stacks are much larger than VBE and linear framebuffer writes.
The CPU usually does not draw every pixel directly into visible display memory.
It submits commands, buffers, textures, and shaders to a GPU through an
operating-system driver and a graphics API.

## What Changed

Modern systems use GPU drivers, command buffers, shaders, compositors, window
systems, and APIs such as Vulkan, Metal, Direct3D, OpenGL, or WebGPU. The final
image may pass through multiple buffers before it reaches the display. A desktop
compositor may combine several application windows, apply scaling, and present
the result according to display timing.

That stack exists because modern graphics hardware is powerful and highly
parallel. It would be wasteful for the CPU to draw every pixel of every frame by
hand when the GPU can transform vertices, shade pixels, sample textures, and
blend layers much faster.

## What Stayed Useful

Framebuffer thinking still matters. Images are arrays of pixels, pitch and
stride exist in modern textures and buffers, clipping and coordinate systems
still matter, double-buffering and presentation are still core ideas, and
rendering should be separated from application state.

Machine Lab uses VBE-shaped graphics because it exposes those ideas without
requiring a full GPU stack. Once you understand this smaller model, modern
graphics APIs become less mysterious: they are larger systems for producing,
transforming, and presenting image buffers.
