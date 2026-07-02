# Serial Protocols

A UART sends and receives bytes. A protocol gives those bytes meaning. Without a
protocol, the receiver only sees a stream: there is no message boundary, no
payload length, no checksum, and no rule for what to do if a byte is missing.

## UART Configuration

Both sides must agree on baud rate, word length, stop bits, parity, and whether
FIFOs and interrupts are enabled. A mismatch can produce data that looks
random, or worse, data that is only occasionally wrong.

Configuration is therefore part of the protocol. Before two programs can
exchange application messages, they must agree on how bytes are represented and
when the transmitter and receiver are ready.

## From Bytes To Messages

A robust message protocol usually needs a start marker or length, payload bytes,
a checksum or sequence number, acknowledgement or retry behavior, and timeout
rules. Lab 7 starts with raw UART helpers. Final projects can build higher-level
protocols on top.

## `run-pair`

`run-pair` creates two independent runtime machines and bridges their serial
ports. This is different from loopback inside one machine. It is closer to two
programs connected by a cable, which means each side has its own state,
timeouts, and failure modes.
