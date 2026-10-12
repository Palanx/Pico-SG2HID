# ADR-0020: The master waits after ATT falls and before every byte but the first

- Status: accepted
- Date: 2026-10-10

## Context

First contact with the real SG (`09-guitar-observe`) stopped at G1: the guitar never ACKs byte 0
of any frame, in digital or analog mode. Power is ruled out (socket pin 5 at 3.25 V with the
guitar powered), so is the wiring (Part 2 proved all five signals through the socket's own
contacts), and the guitar plays on a PS2. The same master firmware passes against the emulator.

The master clocks the first bit 1–2 µs after ATT falls, and shifts each byte as soon as the
previous one's ACK arrives. The emulator is a Pico polling ATT in a tight loop and tolerates
both; a controller whose microcontroller wakes on ATT may not. A library that works with real
controllers, Arduino PS2X (`madsci1016/Arduino-PS2X`, `PS2X_lib.{h,cpp}`), holds ATT low for
`CTRL_BYTE_DELAY` before the first clock and waits it again after every byte: 3 µs on AVR, 4 µs
on PIC32, 18 µs on ESP8266. No source found gives the console's own measured gaps.

## Decision

`exchange_frame( )` waits `kAttSetupUs` after `att_assert( )` before the first byte, and
`kByteGapUs` before every later byte, both 20 µs, declared in `src/core/ps2_protocol.h` next to
`kBusClockHz`. The wait is a new port call, `bus_wait_us( )` in `src/hal/bus_port.h`,
implemented in its own file, `src/hal/bus_wait.cpp`, so `src/hal/pio_port.cpp` and the timed
byte window stay unchanged since `51f1291` and G11's calibration holds. R-PROTO-11 binds the
order of the waits to `tests/test_bus_frame.py`. No `boundaries.rules` edit: `hal` already
depends on `core` only.

## Consequences

- **Easier.** Both values are calibration knobs in one header; the frame loop's host test pins
  where the waits happen, not how long they are.
- **Harder.** A 9-byte frame grows from ~430 µs to ~610 µs of the 1000 µs poll period. A larger
  value has to be checked against that period.
- **Debt taken knowingly.** 20 µs is chosen, not measured: above every PS2X branch, with no
  console capture to compare against. If the guitar answers, `09-guitar-observe` records it as
  sufficient, not as necessary.
- **Rejected: also slowing `kBusClockHz`.** The PS2 drives controllers at 250 kHz, the rate the
  master already uses, and a different clock moves the trace decoder's shift budget
  (`26-trace-shift-derived`) and G11's calibration. It stays the next knob if this one fails.
- **Rejected: runtime-tunable waits through the observe firmware's serial port.** It would save
  reflashes during a sweep, but nothing measured asks for a sweep yet.
- **Rejected: the wait inside `exchange_byte( )`.** It would change `pio_port.cpp` and the
  timed window the trace's `us=` comes from.
