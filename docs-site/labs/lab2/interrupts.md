# Interrupts

Polling repeatedly checks a status bit. Interrupts let a device notify the
program when something happened.

| Mechanism | Strength | Cost |
| --- | --- | --- |
| Polling | simple, predictable control flow | wastes work when events are rare |
| Interrupts | efficient for sporadic events | requires event-loop structure |

The original PC interrupt path used programmable interrupt controllers and IRQ
lines. Modern systems use more advanced interrupt controllers, but the
programming idea is still recognizable: subscribe, receive event, do minimal
handler work, return to main logic.

Polling is not wrong by itself. It can be the simplest solution when the program
has nothing else to do or when events are expected immediately. The problem is
that it scales poorly when the CPU could be doing useful work while the device
is quiet. A timer interrupt lets the program wait for a meaningful event instead
of repeatedly asking the same question.

## Handler Discipline

An interrupt handler should acknowledge or record the event, update a small
counter or flag, and avoid heavy rendering, parsing, allocation, or blocking
calls. Treat the handler as the place where the program learns that an event
happened, not as the place where the whole application reacts to it.

Your application loop should do the larger work after the event is recorded.
This separation is what keeps the program understandable once multiple devices
are involved. In Lab 3, keyboard events and timer events will arrive through the
same event loop, and each handler must leave enough information for the main
logic to make the right decision.

> [!IMPORTANT]
> A timer interrupt is not a license to run the whole program inside the
> handler. Keep the handler short.
