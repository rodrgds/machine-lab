# Lab 4: PS/2 Mouse Packets

Generated folder: `labs/mouse/`

Lab 4 extends the i8042 controller idea to the auxiliary PS/2 mouse. The mouse
reports packets containing button state, movement deltas, sign bits, and
overflow flags.

The important change from Lab 3 is that one device event is now naturally a
group of bytes. If you lose synchronization, every following byte can be misread
until the parser finds the beginning of a packet again. That makes the mouse a
compact but serious protocol exercise.

Start with [why pointing devices changed interfaces](/labs/lab4/context), then
study the [mouse controller path](/labs/lab4/mouse-controller) and the rules for
[packet parsing](/labs/lab4/packets). After completing the
[implementation tasks](/labs/lab4/tasks), compare this older PS/2-shaped stream
with [modern pointer input](/labs/lab4/modern).

> [!TIP]
> Mouse input is the first lab where losing one byte can corrupt every following
> interpretation until you resynchronize.
