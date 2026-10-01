# ADR-0015: Trace the bus as one `T1` text line per frame, timed by the CPU

- Status: accepted
- Date: 2026-10-01

## Context

Requirement R5 makes the firmware the operator's logic analyzer: before the real guitar is
polled in full (`09-guitar-observe`), every frame must be readable from the Mac, bytes and
per-byte `ACK` timing included. After `24-pio-bus`, `exchange_frame` returns only a count, and
the PIO program (`src/hal/ps2_master.pio`) measures nothing. Two choices are due before code
lands: where a byte's time is measured, and what crosses the USB serial port.

## Decision

Measure each byte on the CPU: `exchange_byte` in `src/hal/pio_port.cpp` takes `time_us_32( )`
before handing the byte to the PIO and again when the received word is seen, or when the byte
is given up on, and returns the difference beside the received byte. The frame loop stores it
per byte. Print each frame as one ASCII line in the `T1` format (fixed in
`docs/phases/04-trace-mode/spec.md` §Goal): a version prefix, the frame length, the bytes
completed, the bytes sent, the bytes received and the elapsed microseconds. A pure formatter in
`src/core/bus_trace.cpp` writes the line into a caller's buffer with no allocation and no I/O;
`tools/trace_decode.py`, standard library only, renders it, deriving the `ACK` delay as elapsed
minus a fixed shift time. `ps2_master.pio` does not change.

## Consequences

- Easier: the trace is readable with `cat` before any decoder exists, the PIO program and
  R-PROTO-07's check are untouched, and `24-pio-bus`'s bench logs stay valid. Both ends are
  checked against the same hand-written lines (R-PROTO-08).
- Harder: the timing carries CPU overhead (the polling loop around the RX FIFO, a few µs), so
  the `ACK` delay is an estimate whose zero point, `SHIFT_US` in the decoder, is a calibration
  knob read off a loopback trace, not a cycle-exact count.
- A format change is a new version prefix (`T2`), never an edit of `T1`: old captures stay
  decodable.
- Rejected: a PIO cycle counter inside `ps2_master.pio`. Exact, but it rewrites the program,
  touches R-PROTO-07's check and voids `24-pio-bus`'s bench logs, for a precision the
  guitar's ACK timing (tens of µs) does not need.
- Rejected: a binary framed format. Unreadable without the decoder, and the bandwidth it saves
  is not needed at three frames a second.
