# Lab 7 Tasks

Work in `labs/uart/`. The goal is to configure the UART, send and receive bytes
reliably, and build enough confidence in the endpoint that `run-pair` tests can
connect two programs together.

## Requested Functions

- `int uart_config(uint16_t base, uint32_t baud, uint8_t line_control)`
- `int uart_enable_fifo(uint16_t base)`
- `int uart_enable_rx_interrupt(uint16_t base)`
- `int uart_set_loopback(uint16_t base, int enabled)`
- `int uart_send_byte(uint16_t base, uint8_t byte)`
- `int uart_read_byte(uint16_t base, uint8_t *byte)`
- `int uart_subscribe(uint8_t irq, lcom_irq_t *out)`
- `int uart_unsubscribe(lcom_irq_t *irq)`

## Guided Gaps

Validate the base port and baud rate, compute the divisor from the UART base
clock, preserve line-control bits around divisor-latch access, add timeouts for
send/read waits, and keep loopback setup independent from normal configuration.
The divisor-latch access bit is especially important because it changes which
logical register later reads and writes refer to.

> [!IMPORTANT]
> Always restore line-control state after touching the divisor latch. Otherwise
> later reads and writes may go to the wrong logical register.

## Common Mistakes

Common mistakes include forgetting divisor-latch access before writing the
divisor, forgetting to clear divisor-latch access after configuration, sending
without checking transmitter-ready, and treating "no byte" as "received byte
0". These are small mistakes, but in a byte stream they quickly become protocol
bugs.

Next: [checks and references](/labs/lab7/check).
