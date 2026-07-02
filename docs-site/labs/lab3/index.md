# Lab 3: Keyboard And i8042

Generated folder: `labs/kbd/`

Lab 3 moves from periodic events to human input. A key press is not a
character; it is a device event that may arrive as one or more scancode bytes.
This distinction is important. Characters belong to text input, keyboard
layouts, modifiers, and application policy. Scancodes belong to the device
stream.

In this lab you work below the text-input layer so that you can understand how a
system first learns that a physical key changed state. That knowledge is useful
for games, editors, shells, debug consoles, and any final project that needs
responsive controls rather than occasional line-based input.

Read [why keyboards are controller streams](/labs/lab3/context), then study the
[keyboard controller](/labs/lab3/keyboard) and the rules for
[scancode parsing](/labs/lab3/scancodes). After that, compare
[interrupts and polling](/labs/lab3/sync), learn the small set of
[KBC commands](/labs/lab3/kbc-commands) used by the lab, and combine keyboard
input with timer events in [multiple devices](/labs/lab3/multiple-devices).
The [implementation tasks](/labs/lab3/tasks) collect the required functions, and
the final page compares this model with [modern keyboards](/labs/lab3/modern).

> [!TIP]
> Treat the keyboard as a byte stream first. Turn bytes into application actions
> only after the low-level parser is correct.
