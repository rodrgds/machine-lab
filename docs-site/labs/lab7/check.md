# Lab 7 Check

Start with loopback before running two programs. Loopback proves that one UART
endpoint can configure itself, send a byte, and receive that byte back through
the runtime. After that, `run-pair` checks whether two independent machines can
communicate through a bridged serial link.

```sh
machinelab test uart
machinelab run --headless -- build/examples/uart_loopback
machinelab run-pair --headless build/examples/uart_peer_sender --right build/examples/uart_peer_receiver
```

## Discussion Prompts

When reviewing the lab, discuss what fields you would add to a packet protocol
built on UART bytes, how a receiver should recover if it starts mid-packet, and
why loopback is useful before two-machine communication.

## External Reading

For additional background, read OSDev on [serial ports](https://wiki.osdev.org/Serial_Ports)
and [UARTs](https://wiki.osdev.org/UART), plus the Wikibooks guide to
[8250/16550 UART programming](https://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming).
