# Lab 3 Tasks

Work in `labs/kbd/`. The goal is to build a reliable keyboard layer before you
build application behavior on top of it. Keep the controller reads, parser
state, and test-facing output separate enough that each part can be checked on
its own.

## Requested Functions

- `int kbc_read_status(uint8_t *status)`
- `int kbc_read_output(uint8_t *byte)`
- `int kbc_write_command(uint8_t command)`
- `int kbd_process_byte(uint8_t byte)`
- `int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make)`

## Guided Gaps

Check output pointer arguments, ignore reads when no byte is available, detect
controller error bits, store a pending `0xE0` prefix, and report make/break
state without losing the original bytes. Command/ACK handling should stay
separate from scancode parsing, because a command response is not a key event.

You also need to decide how callers observe "no complete scancode yet." That is
not necessarily an error; it may simply mean the parser has seen the first byte
of an extended sequence and is waiting for the next one.

> [!IMPORTANT]
> Do not treat every byte as a complete scancode. Extended scancodes must be
> assembled before the API reports an event.

## Common Mistakes

Common mistakes include clearing parser state too early, reporting a scancode
before all bytes arrived, treating break codes as normal key presses, and
reading output without checking status. Most of these are ownership mistakes:
the program loses track of who owns the next byte, the current parser state, or
the subscribed IRQ.

Next: [checks and references](/labs/lab3/check).
