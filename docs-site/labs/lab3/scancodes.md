# Reading Scancodes

Scancodes are device-level key events. They are not ASCII. For simple keys, the
break code is often the make code with the high bit set. That pattern is useful,
but do not turn it into a universal rule without checking the scancode set and
the extended-byte cases.

| Byte pattern | Meaning |
| --- | --- |
| `0x1e` | A key make code |
| `0x9e` | A key break code |
| `0xE0 ...` | extended sequence |
| `0x81` | ESC break code in these tests |

Make code means key press. Break code means key release. Some keys arrive as
multi-byte sequences, and your parser must not report a complete scancode until
the sequence is complete.

## Parser State

A correct parser needs state. It may be idle with no pending prefix, it may have
seen `0xE0` and be waiting for the second byte, or it may have a complete
scancode ready for the caller. That state should live in the keyboard library,
not in every application loop that wants input.

This is one of the first examples in the course where a single byte is not
always a complete message. The same idea returns in mouse packets, UART
protocols, and file formats: the parser must know whether it has enough bytes to
describe one complete event.

## Application State

Higher-level code can turn scancodes into "key is currently down" tables, text
input, menu actions, or game movement. Those are application decisions. The
low-level parser should report what happened at the device boundary without
deciding what the program should do about it.
