# Lab 4: PS/2 Mouse Packets

Goal: enable data reporting through the i8042 mouse command path, read IRQ12
bytes, assemble 3-byte packets, and decode movement/buttons.

Reference:

```sh
build/lcom run --headless --script scripts/mouse_move.lcomscript -- build/examples/mouse_packet
```
