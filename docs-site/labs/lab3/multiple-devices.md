# Handling Multiple I/O Devices

Real reactive programs wait for more than one event source. Lab 3 combines
keyboard input with timer events. The event loop should answer whether the
keyboard produced a byte, whether the timer ticked, whether the program should
stop because ESC was released, and whether it should stop because no key arrived
for too long.

That sounds simple, but it is the first place where the program has to combine
independent device events into one policy. Keyboard events reset the idle
counter. Timer events advance it. The loop terminates only when one of the
stated conditions becomes true.

## Why The Timer Matters

Keyboard input alone cannot measure idle time. Timer interrupts give the loop a
heartbeat so it can terminate after a quiet interval.

This is the same pattern used by games and editors. Input changes state, time
advances state, and rendering presents state. Even though this lab may only
print scancodes, the structure is already close to the loop you will use for a
final project.

This is also where reusable lab libraries start to pay off. The timer code
should not know about scancodes, and the keyboard code should not know about
idle seconds. The application loop is the place where those two pieces of
information become a policy.
