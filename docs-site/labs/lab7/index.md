# Lab 7: UART And Serial Ports

Generated folder: `labs/uart/`

Lab 7 introduces serial communication through a 16550-style UART. A UART turns
bytes into a stream and receives a stream back into bytes. It is simple enough
to fit in a lab, but it contains the same reliability questions that appear in
larger communication systems.

| Register idea | Role |
| --- | --- |
| line control | word length, stop bits, parity, divisor access |
| divisor latch | baud-rate divisor |
| FIFO control | receive/transmit buffering |
| line status | data-ready and transmitter-ready bits |
| interrupt enable | which events raise IRQs |
| loopback | transmit is routed back to receive |

Begin with [serial protocol thinking](/labs/lab7/protocols), complete the
[implementation tasks](/labs/lab7/tasks), run the [checks](/labs/lab7/check),
and compare this model with [modern serial links](/labs/lab7/modern).

> [!TIP]
> Loopback proves one endpoint before `run-pair` asks you to reason about two
> independent programs.
