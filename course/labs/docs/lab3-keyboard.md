# Lab 3: Keyboard And i8042

Generated folder: `labs/kbd/`

Lab 3 moves from periodic events to human input. The keyboard is one of the
oldest and most important PC input devices: it can drive command interfaces,
games, editors, shortcuts, debugging tools, and recovery flows.

In this lab you interact with the i8042-style keyboard controller. You read its
status register, decide whether output data is available, process scancode
bytes, and return complete make/break events to the application.

## Why This Matters

Keyboard input is not "a character appears." At this level, a key press and a
key release are device events. Some keys produce multi-byte sequences. The
program must avoid reading stale data, must handle partial sequences, and must
not confuse make codes with break codes.

> [!NOTE]
> Later projects use this lab indirectly. A game, word processor, tracker, menu,
> or protocol console all depend on the same low-level habit: read status first,
> then consume bytes carefully.

## Device Model

| Port/register | Role |
| --- | --- |
| status register | reports output-buffer state and error bits |
| output buffer | contains the next scancode byte when ready |
| command register | accepts controller commands |
| IRQ line | notifies the program that keyboard data arrived |

Scancodes can be one byte or two bytes. In the common set used here, `0xE0`
starts an extended sequence, and break codes indicate key release.

## Polling And Interrupts

Polling checks the controller repeatedly. Interrupts let the device notify the
program. Both matter:

| Mechanism | Strength | Cost |
| --- | --- | --- |
| Polling | simple control flow | wastes cycles when no key arrived |
| Interrupts | responsive and efficient for sporadic input | requires event-loop structure |

Machine Lab tests focus on correct byte handling. Your final projects should
prefer interrupt/event-loop style where practical.

> [!IMPORTANT]
> Do not treat every byte as a complete scancode. Extended scancodes must be
> assembled before the API reports an event.

## Requested Functions

- `int kbc_read_status(uint8_t *status)`
- `int kbc_read_output(uint8_t *byte)`
- `int kbc_write_command(uint8_t command)`
- `int kbd_process_byte(uint8_t byte)`
- `int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make)`

## Guided Gaps

- Check output pointer arguments.
- Ignore or reject reads when the status register reports no available byte.
- Detect controller error bits before trusting output data.
- Store a pending `0xE0` prefix until the second byte arrives.
- Distinguish make and break events without losing the original bytes.
- Keep byte parsing separate from printing or application controls.

> [!TIP]
> Write down the byte sequence you expect before running a test. Then compare
> that expected sequence with the actual printed hexadecimal bytes.

## Common Mistakes

- Clearing parser state too early after `0xE0`.
- Reporting a scancode before all bytes arrived.
- Treating break codes as normal key presses.
- Reading the output buffer without checking status.
- Mixing keyboard library code with final-project menu logic.

## Discussion Prompts

- Why do input libraries report both key press and key release?
- What should happen if the controller reports a parity or timeout error?
- How would you build a higher-level "key is currently down" table from this
  low-level API?

## Check

```sh
machinelab test kbd
machinelab run --headless --script scripts/type_a_esc.mlabscript -- build/examples/keyboard_scan
```

## External Reading

- OSDev PS/2 Keyboard: <https://wiki.osdev.org/PS/2_Keyboard>
- OSDev PS/2 Controller: <https://wiki.osdev.org/%228042%22_PS/2_Controller>
- IBM PC keyboard scancode notes: <https://aeb.win.tue.nl/linux/kbd/scancodes.html>
