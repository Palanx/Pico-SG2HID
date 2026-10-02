---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "Makefile"
  - ".claude/workflow/toolchain.manual.json"
  - "src/core/**"
  - "tools/trace_decode.py"
  - "src/hal/ps2_master.pio"
  - "tests/vectors/trace_session.rendered"
---

# Tech debt log

## `SHIFT_US` is a hand copy of the PIO cycle budget (reviewed 2026-10-02)

Files: `tools/trace_decode.py`, `src/hal/ps2_master.pio`, `src/core/ps2_protocol.h`,
`tests/vectors/trace_session.rendered`

`tools/trace_decode.py` subtracts `SHIFT_US = 37` from each byte's elapsed time to get the `ack`
delay. The 37 is copied by hand from two sources: the cycle-budget comment in
`src/hal/ps2_master.pio` ("37 cycles per byte without the ACK wait") and the one cycle per µs
that `kBusClockHz = 250000` in `src/core/ps2_protocol.h` gives at 4 PIO cycles per bit. Nothing
ties these values together. If the PIO program gains or loses an instruction, or the bus clock
changes, every `ack` delay the decoder prints shifts silently, and `make test` stays green.
`trace_session.rendered` is computed from the same 37, so it moves with the copy and cannot
catch the drift.

Nothing breaks today. `ps2_master.pio` has not changed since 24-pio-bus, and 04-trace-mode's
acceptance criteria check it with `git diff --quiet main -- src/hal/ps2_master.pio`. The ATT→ACK
bench capture (firmware 51f1291, 2026-10-01) measured 36–38 µs per byte and `ack` delays of
0–1 µs, so 37 matches the hardware. The point where this stops holding is the first edit to
`ps2_master.pio` or to `kBusClockHz`.

Fixes, cheapest first:
- A host check, in `tests/test_bus_trace.py`, that reads the "So <n> cycles per byte"
  sentence from the `.pio` comment and `kBusClockHz` from the header, and asserts
  `SHIFT_US == n * 1e6 / (4 * kBusClockHz)`. It is a few lines, but it trusts the comment
  to match the instructions.
- Count the instructions on the no-ACK path of the `.pio` program itself. This is exact, but
  it is a small parser for PIO assembly.
- Re-measure on the bench whenever either file changes, reading the 1-byte frame's `us` in an
  ATT→ACK capture (`docs/phases/04-trace-mode/verify.md` explains how). This costs no code, but
  it relies on someone remembering to do it.

What already works: the bench `awk` criterion in `docs/phases/04-trace-mode/spec.md` counts
`ack` delays above 5 µs in a decoded ATT→ACK capture. A non-zero count means `SHIFT_US` is too
small. A count of zero does not prove `SHIFT_US` is not too large, because the delay is clamped
with `max(0, …)`.

Where it was found: 04-trace-mode, validation round 2. The independent reviewer could not
check 37 from the spec and the diff alone.
