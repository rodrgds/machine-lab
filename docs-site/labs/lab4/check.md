# Lab 4 Check

Run the mouse tests with deterministic scripts before trying to debug movement
visually. Packet parsers can fail in ways that look like random cursor motion,
so it is better to confirm byte assembly, synchronization, and sign conversion
with fixed input first.

```sh
machinelab test mouse
machinelab run --headless --script scripts/mouse_move.mlabscript -- build/examples/mouse_packet
```

## Discussion Prompts

When reviewing the lab, discuss why packet byte 0 includes a constant
synchronization bit, how you would implement drag detection from button state
and movement deltas, and which helpers a drawing program would need before it
could use the mouse library comfortably.

## External Reading

The LCOM [mouse introduction](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab4/intro.html)
and [mouse packet chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab4/reading-mouse-packets.html)
are useful companion readings. For broader protocol details, read OSDev on
[mouse input](https://wiki.osdev.org/Mouse_Input) and the
[PS/2 controller](https://wiki.osdev.org/%228042%22_PS/2_Controller), plus Adam
Chapweske's archived
[PS/2 Mouse Interface](https://web.archive.org/web/20180102104023/http://www.computer-engineering.org/ps2mouse/).
