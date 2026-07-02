# Reading Mouse Packets

A basic PS/2 packet has three bytes:

| Packet byte | Contains |
| --- | --- |
| byte 0 | buttons, sign bits, overflow bits, synchronization bit |
| byte 1 | X movement low byte |
| byte 2 | Y movement low byte |

The first byte includes a bit that should always be set for a normal packet.
Use that bit to recover if parsing starts in the middle of a stream. This is the
same general idea as finding a start marker in a serial protocol: the parser
needs a way to regain trust after it loses its place.

## Signed Movement

X and Y movement are signed values. The sign bits are in byte 0, while the low
movement bytes are byte 1 and byte 2. This split forces you to assemble the
value deliberately.

Do not treat the movement bytes as ordinary unsigned coordinates. A mouse packet
usually reports relative movement since the previous packet, not an absolute
screen position. The application can later add those deltas to a cursor
position, clamp the result to the screen, or use the movement to control a game
camera.

## Packet State

Track how many bytes have been collected, whether byte 0 is synchronized,
whether a complete packet is ready, and whether overflow flags should invalidate
or merely report movement. The parser should be able to receive one byte at a
time and only expose a packet after all three bytes are available.

This state-machine shape will look familiar by the end of the course. UART
framing, sprite parsing, and replay scripts all involve partial input that only
becomes meaningful after enough bytes or tokens arrive.

> [!TIP]
> Keep packet assembly separate from coordinate updates. A drawing program can
> use decoded deltas later; the lab library should only parse packets.
