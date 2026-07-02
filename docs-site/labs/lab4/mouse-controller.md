# The i8042 Mouse Controller

The PS/2 mouse is the auxiliary device behind the same controller family used
for the keyboard. The shared controller is convenient for the lab because the
status-checking and command-writing habits from Lab 3 remain useful, but the
mouse adds a new problem: command responses and movement packets travel through
closely related paths.

## Command Pattern

Before the mouse produces movement packets, software must enable data reporting.
That command flow should wait until the controller can accept a command, send
the mouse command path, write the mouse command, read and validate the
acknowledgement, and retry or fail cleanly. The exact byte values are less
important than the discipline: command writes are a protocol, not a single
unconditional store.

The acknowledgement matters because the output stream is shared. If the program
mistakes an ACK byte for movement data, packet assembly begins one byte off and
the parser may remain confused until it finds a valid synchronization bit.

## Why Disable Reporting?

Tests and projects should leave devices in a predictable state. If data
reporting stays enabled after a program exits, later input streams can start
with leftover bytes.

This is a general systems habit. A lab function should clean up the device state
it changed, even when an error or early exit occurs. That makes automated tests
reliable and prevents one experiment from contaminating the next one.

> [!IMPORTANT]
> Command responses and movement data share the byte stream. Do not let an ACK
> byte become packet byte 0.
