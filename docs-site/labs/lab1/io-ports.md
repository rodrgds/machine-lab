# I/O Ports And Privileged Operations

The RTC uses two ports. One port selects which internal register should be
visible, and the other port acts as the data window for the selected register.

| Port | Role |
| --- | --- |
| `RTC_ADDR_REG` (`0x70`) | select the RTC register |
| `RTC_DATA_REG` (`0x71`) | read or write the selected register |

The protocol is a two-step conversation:

```c
lcom_port_write8(RTC_ADDR_REG, RTC_REG_A);
lcom_port_read8(RTC_DATA_REG, &value);
```

This split looks odd at first. It exists because the controller has more
internal registers than it has external port addresses. One port acts as an
address selector; the other acts as the data window.

The sequence also illustrates why device programming is not only about reading
and writing bytes. If another piece of code selected a different register
between the address write and the data read, the second operation would no
longer mean what you think it means. Real drivers protect these critical
sections with ownership rules, locks, interrupt discipline, or a single service
responsible for the device. In Machine Lab the runtime keeps the environment
small, but you should still learn to think in ordered protocol steps.

## Read Before You Trust

The RTC has status registers as well as data registers. Before reading date or
time fields, you must check whether an update is in progress. If you ignore the
status bit, you can read a date while the clock is changing and assemble fields
from two different moments.

That kind of bug is frustrating because every individual byte may look valid.
The day can be in range, the month can be in range, and the year can be in
range, while the combination still represents a moment that never existed. This
is why low-level code often treats "read succeeded" and "value is meaningful" as
separate questions.

> [!IMPORTANT]
> A successful port read only means a byte was read. It does not mean the byte is
> semantically safe to use. Device protocols define when data is valid.
