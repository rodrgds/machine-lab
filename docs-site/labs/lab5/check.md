# Lab 5 Check

Run the graphics tests before relying on visual inspection. A framebuffer bug
can look like a drawing-design mistake, but the tests can check mode setup,
address calculation, clipping, and dumped frame contents directly.

```sh
machinelab test graphics
machinelab run --headless --dump-frame build/rectangle.ppm -- build/examples/vbe_rectangle
```

## Discussion Prompts

When reviewing the lab, make sure you can explain why pitch is not always
`width * bytes_per_pixel`, what changes when you double-buffer, and which
rendering helpers you would want before starting a game, editor, tracker, or
visualizer.

## External Reading

The LCOM [video card chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab5/pc-video-card.html),
[VBE chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab5/vbe.html),
and [reactive applications chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab5/reactive-graphical-applications.html)
are useful companion readings. For deeper reference, read OSDev on
[VESA video modes](https://wiki.osdev.org/VESA_Video_Modes), the
[VBE 2.0 specification](https://www.phatcode.net/res/221/files/vbe20.pdf), and
the [XPM overview](https://en.wikipedia.org/wiki/X_PixMap).
