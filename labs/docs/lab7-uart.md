# Lab 7: UART And Serial Ports

Goal: configure a 16550-style UART, use the divisor latch, enable FIFOs and
receive interrupts, and exchange bytes through loopback or the virtual
COM1/COM2 cable.

Required functions:

- `uart_config`
- `uart_enable_fifo`
- `uart_enable_rx_interrupt`
- `uart_set_loopback`
- `uart_send_byte`
- `uart_read_byte`
- `uart_subscribe`
- `uart_unsubscribe`

Try:

```sh
build/lcom run --headless -- build/examples/uart_pair
```
