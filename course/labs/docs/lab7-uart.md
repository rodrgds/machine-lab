# Lab 7: UART And Serial Ports

Generated folder: `labs/uart/`

Lab 7 introduces serial communication through a 16550-style UART. A UART turns
bytes into a stream and receives a stream back into bytes. This lab covers
configuration, line control, FIFOs, loopback, byte send/receive, interrupts, and
paired runtimes for protocol experiments.

## Why This Matters

Serial ports are simple enough to understand but rich enough to teach real
systems habits: configuration must match on both sides, status bits matter, a
receive buffer can be empty, transmit hardware can be busy, and protocols need
framing and error handling.

> [!NOTE]
> The `run-pair` command can connect two Machine Lab programs through virtual
> UARTs. That makes final projects possible where two programs negotiate, send
> moves, transfer data, or implement a small protocol.

## Device Model

| Register idea | Role |
| --- | --- |
| line control | word length, stop bits, parity, divisor-latch access |
| divisor latch | baud-rate divisor |
| FIFO control | receive/transmit buffering |
| line status | data-ready and transmitter-ready bits |
| interrupt enable | which UART events raise IRQs |
| loopback | transmit data is routed back to receive path |

The 16550 has shared port addresses where meaning changes depending on control
bits. This makes careful sequencing important.

## Plan

1. Configure baud rate by enabling divisor-latch access.
2. Write divisor low and high bytes.
3. Restore normal line control.
4. Enable FIFO mode.
5. Send a byte only when transmit holding register is ready.
6. Read a byte only when data-ready is set.
7. Use loopback to test send/receive on one program.
8. Subscribe to UART interrupts for event-driven protocols.

> [!IMPORTANT]
> Always restore line-control state after touching the divisor latch. Otherwise
> later reads and writes may go to the wrong logical register.

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

- Validate base port and baud rate.
- Compute divisor from the UART base clock.
- Preserve line-control bits while using divisor-latch access.
- Decide how long send/read should wait before returning failure.
- Keep loopback setup independent from normal configuration.
- Make subscribe/unsubscribe symmetric.
- Design helpers that later protocols can reuse.

> [!TIP]
> Test UART code in loopback first. After loopback works, use `run-pair` to test
> two independent programs and protocol behavior.

## Common Mistakes

- Forgetting to set divisor-latch access before writing the divisor.
- Forgetting to clear divisor-latch access after configuration.
- Sending without checking transmitter-ready.
- Treating "no received byte" as the same thing as "received byte 0".
- Using busy loops with no timeout.

## Discussion Prompts

- What fields would you add to a packet protocol built on top of UART bytes?
- How should a receiver recover if it starts reading in the middle of a packet?
- Why is loopback useful before testing two-machine communication?
- Which final projects become possible with `run-pair`?

## Check

```sh
machinelab test uart
machinelab run --headless -- build/examples/uart_loopback
machinelab run-pair --headless build/examples/uart_peer_sender --right build/examples/uart_peer_receiver
```

## External Reading

- OSDev Serial Ports: <https://wiki.osdev.org/Serial_Ports>
- OSDev UART: <https://wiki.osdev.org/UART>
- 16550 UART overview: <https://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming>
