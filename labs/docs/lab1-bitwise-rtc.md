# Lab 1: Bitwise Helpers And RTC

Goal: learn bit operations and read date/time from the virtual CMOS RTC through
ports `0x70` and `0x71`.

Run reference examples:

```sh
build/lcom run --headless --rtc 2026-06-16T12:34:56 -- build/examples/rtc_date
```

Requested functions are listed in `labs/function-requests.json`.
