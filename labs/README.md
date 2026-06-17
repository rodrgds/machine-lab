# lcom-ng Labs

These labs are the first draft of the student-facing `lcom-ng` track. They keep
the good parts of the old LCOM progression while replacing Minix/LCF details
with a normal C process that talks to `lcom run`.

Suggested order:

1. Lab 1: bitwise helpers and RTC/CMOS
2. Lab 2: i8254 PIT and virtual IRQ events
3. Lab 3: i8042 keyboard and PS/2 scancodes
4. Lab 4: PS/2 mouse packets
5. Lab 5: VBE framebuffer graphics and XPM sprites
6. Lab 6: AC97-lite PCM audio
7. Lab 7: 16550 UART serial ports

Starter headers live in `labs/templates/<lab>/include/`. Complete reference
solutions live in `labs/solutions/<lab>/` and are written only with the public
`lcom-ng` student API: port I/O, IRQ subscription, event waits, framebuffer
mapping, and AC97 buffer mapping.

The reference solutions are tested by CTest targets named `lab1_solution` through
`lab7_solution`. They run as normal student binaries through `lcom run`, with
scripted keyboard/mouse input and deterministic RTC, video, and audio output.
