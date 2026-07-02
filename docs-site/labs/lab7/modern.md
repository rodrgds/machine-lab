# How It Works Today

Traditional UARTs are less visible on consumer laptops, but serial thinking is
still common. Firmware consoles, embedded boards, USB-to-serial adapters, debug
ports, bootloaders, industrial devices, and virtual serial ports in VMs all keep
the model alive.

USB CDC and other transport layers may hide the physical UART, but the software
often still sees a byte stream. That means the same problems remain: both sides
must agree on configuration, receivers must handle partial messages, and
protocols need a way to recover when synchronization is lost.

## What Stayed Useful

Lab 7 teaches configuration agreement, status-before-read/write, buffering,
loopback testing, framing and resynchronization, timeout and retry thinking.
Those lessons transfer to network protocols and file formats as much as to
physical serial ports.

The old UART is valuable because it is small enough to understand. Once you can
build a reliable byte-stream helper here, larger communication systems become
less mysterious: they are built from the same concerns, with more layers and
more failure modes.
