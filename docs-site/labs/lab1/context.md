# Lab 1 Context: Why I/O Looked This Way

Early PC hardware was built around simple buses and small controller chips. A
device did not expose an object, a file descriptor, or a driver API. It exposed
registers. Software selected a register, read a byte, checked a status bit, and
repeated the protocol until the device was ready.

The original LCOM/Minix path used privileged services because ordinary programs
should not freely touch hardware ports. Machine Lab keeps the same controller
shape, but the runtime safely virtualizes the port reads and writes.

## The Transferable Idea

The important lesson is not that the RTC happens to use `0x70` and `0x71`. Those
addresses are historical details. The deeper lesson is that a controller has a
small public contract, and the programmer is responsible for following it
precisely. A status bit may say whether data is valid. A configuration bit may
change how the next byte should be interpreted. A write to one register may
only affect the meaning of a later read from another register.

This kind of interface can feel primitive compared with a modern library call,
but it is also refreshingly honest. There is no hidden object model. There is a
device state machine, a few bytes of visible state, and a specification that
tells you how to move from one state to the next. That pattern reappears in
every later lab, even when the device becomes a timer, keyboard, mouse, video
card, audio buffer, or serial port.

## What Machine Lab Removes

Machine Lab does not ask you to configure VirtualBox, register a Minix service,
request I/O privileges from the microkernel, or restore the VM's own device
drivers after experiments. Those are real operating-system topics, and they are
worth studying in the right setting. They are not necessary before you learn how
bit fields, status registers, and controller protocols work.

For this reason, the first lab should be read as a small device-driver exercise
with the dangerous parts removed. You still need to be careful, but the
consequences are test failures and trace output rather than a broken guest
system.
