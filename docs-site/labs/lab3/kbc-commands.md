# Managing KBC Commands

The controller also accepts commands. A command is not just a byte write; it is
a small protocol. A safe command path checks whether the input buffer is
available, whether the command byte was accepted, whether a response byte should
be read, and whether a retry is appropriate.

The exact command set is small for this lab, but the pattern is important. A
command write is not a store to a magic address; it is one step in a controller
conversation. Command paths are where many otherwise correct input libraries
become brittle. The controller may be busy, the command may produce an
acknowledgement, and the same output path may also carry ordinary scancode
bytes.

The mouse lab uses a similar command/ACK pattern, so the keyboard controller
work prepares you for Lab 4. A robust implementation keeps command responses
separate from normal input instead of assuming every byte from the output buffer
is a key event.

## Common Command Bugs

The usual mistakes are writing while the input buffer is still full, ignoring
timeout or error bits, retrying forever, and mixing command responses with
normal scancode bytes. For the lab, keep command helpers narrow and explicit. A
helper that writes a controller command should not also parse scancodes or
update application state.
