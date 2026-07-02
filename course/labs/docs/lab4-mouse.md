# Lab 4: PS/2 Mouse Packets

Generated folder: `labs/mouse/`

Lab 4 extends the i8042 controller idea to the auxiliary PS/2 mouse. Instead of
single key events, the mouse reports packets containing button state, movement
deltas, sign bits, and overflow flags.

You will enable data reporting, process bytes into aligned three-byte packets,
and expose decoded movement to application code.

## Why This Matters

Mouse input is the first lab where synchronization is central. If you lose one
byte, every following byte can be interpreted in the wrong position until you
recover packet alignment. This is the same kind of problem you will meet in
serial protocols, file formats, network packets, and audio buffers.

> [!NOTE]
> The original mouse was historically important because it changed how people
> interacted with computers. In this lab, the important part is lower level: a
> pointing device is a stream of small packets that your program must decode.

## Device Model

| Packet byte | Contains |
| --- | --- |
| byte 0 | button bits, sign bits, overflow bits, synchronization bit |
| byte 1 | X movement low byte |
| byte 2 | Y movement low byte |

The first byte has a bit that should always be set for a normal packet. Use it
to regain synchronization if parsing starts in the middle of a stream.

## Plan

1. Send the command that enables mouse data reporting.
2. Read ACK/NAK-style responses and handle failure.
3. Feed incoming bytes into a packet builder.
4. Use the first-byte synchronization bit to align packets.
5. Sign-extend X and Y deltas.
6. Preserve button state and overflow flags.
7. Disable data reporting when the caller is done.

> [!IMPORTANT]
> Do not assume the first byte you receive is packet byte 0. Real streams can
> start mid-packet, and robust code must resynchronize.

## Requested Functions

- `int mouse_enable_data_reporting(void)`
- `int mouse_disable_data_reporting(void)`
- `int mouse_process_byte(uint8_t byte)`
- `int mouse_get_packet(mouse_packet_t *packet)`

## Guided Gaps

- Decide how many retries command writes should attempt.
- Validate ACK responses without blocking forever.
- Store packet bytes until all three are available.
- Drop bytes until the synchronization bit identifies byte 0.
- Convert 9-bit signed movement into normal C integers.
- Decide whether overflow should invalidate movement or be reported as a flag.

> [!TIP]
> Keep packet assembly and packet interpretation as separate helper steps. That
> makes synchronization bugs easier to isolate.

## Common Mistakes

- Forgetting that Y movement is signed separately from X.
- Treating overflow bits as normal movement bits.
- Returning a packet before all three bytes arrived.
- Never recovering after one bad byte.
- Leaving data reporting enabled after a test.

## Discussion Prompts

- Why does the first packet byte include a constant synchronization bit?
- How would you implement drag detection using only button state and deltas?
- Which mouse helpers would be useful for a drawing program or editor?

## Check

```sh
machinelab test mouse
machinelab run --headless --script scripts/mouse_move.mlabscript -- build/examples/mouse_packet
```

## External Reading

- OSDev Mouse Input: <https://wiki.osdev.org/Mouse_Input>
- OSDev PS/2 Controller: <https://wiki.osdev.org/%228042%22_PS/2_Controller>
- Adam Chapweske PS/2 Mouse Interface archive: <https://web.archive.org/web/20180102104023/http://www.computer-engineering.org/ps2mouse/>
