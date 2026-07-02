# Lab 3 Context: The PC Keyboard

The keyboard is one of the oldest PC input devices, and it still shapes how
systems are controlled. Shortcuts, shells, games, recovery menus, BIOS setup,
and accessibility flows all depend on reliable keyboard input.

At this level, a keyboard does not send "letters." It sends scancode bytes. A
program must decide whether a byte is available, whether the controller reported
an error, whether the byte starts an extended sequence, and whether the key was
pressed or released. Only after those questions are answered should
higher-level code decide whether the event means "type the letter A", "move
left", "open a menu", or "exit the program."

This is why the lab asks you to print scancodes before it asks you to build
interactive applications from them. The byte stream has to be trusted first. If
your low-level parser sometimes drops an extended prefix or mistakes a break
code for a make code, every layer above it will inherit the confusion.

## Why This Matters

Final projects need stable input. If the scancode parser is wrong, every menu,
game control, editor shortcut, or debug console built on top of it becomes
unreliable.

There is also a collaboration reason to keep this layer clean. In a group
project, one student may work on menus while another works on movement or text
entry. Both of them should be able to rely on the same keyboard library without
knowing the details of the i8042 status register.
