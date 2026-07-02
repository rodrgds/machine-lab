# The Keyboard Controller

The i8042-style controller sits between the CPU and the keyboard device. In a
real PC, the details are tangled with history and compatibility. For this lab,
the useful view is that the controller exposes a small register interface and
raises an event when a scancode byte is waiting.

| Register | Role |
| --- | --- |
| status register | output-buffer state and error bits |
| output buffer | next scancode byte |
| command register | controller commands |
| IRQ line | keyboard event notification |

The safe read sequence is to read status, confirm that the output buffer is
full, check the error bits, read the output byte, and feed that byte to the
scancode parser. Each step has a reason. The status read prevents empty-buffer
reads. The error checks prevent corrupted bytes from becoming trusted input. The
parser call keeps device I/O separate from scancode assembly.

It is tempting to collapse all of this into a single "get key" helper. Avoid
that too early. At this layer you are not yet getting a key; you are getting one
byte from a controller stream, and the byte may be a prefix, a command response,
or part of a larger event.

> [!IMPORTANT]
> Do not read output data just because your program wants input. Read only when
> the status register says data is available.
