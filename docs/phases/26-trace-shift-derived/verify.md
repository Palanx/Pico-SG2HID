# Phase 26-trace-shift-derived — how to check this yourself

No hardware is needed. Nothing on the Picos changes: no firmware is rebuilt or flashed.

## What was built

When you decode a bus trace (`python3 tools/trace_decode.py <port or file>`), each byte
that waited for the guitar's `ACK` shows an `ack` column, for example
`byte 0: out 01 in FF 48 us ack 11 us`. That `ack` value is not measured directly. The
master measures the byte's total time (48 µs), and the decoder subtracts the time the
byte spends being shifted out on the wire, 37 µs. What remains is how long the guitar took
to acknowledge.

Those 37 µs were typed into the decoder by hand. They come from counting the steps
(cycles) of the small program the Pico's PIO block runs to clock a byte out
(`src/hal/ps2_master.pio`), at the speed the bus is clocked. If someone changed that
program or the bus speed, the real shift time would change, the decoder would keep
subtracting 37, and every `ack` value would be silently wrong.

Now the number has two parts in `tools/trace_decode.py`:

- `SHIFT_NOMINAL_US = 37` — the part the program implies. A new test,
  `tests/test_trace_shift.py`, recounts the cycles from the program itself and fails
  `make test` if this number no longer matches (rule R-PROTO-09).
- `SHIFT_CALIBRATION_US = 0` — a correction you may set by hand after measuring on the
  bench. The new test never checks it. It does change every `ack` value the decoder
  prints, though, so after changing it the 9 `ack` rows of the hand-written expected output
  `tests/vectors/trace_session.rendered` must be rewritten to match, or `make test` fails
  on R-PROTO-08.

The decoder subtracts their sum, which is still 37, so every decoded trace reads exactly
as before.

## Check it yourself

1. Open Terminal in the repository folder and run:

   ```
   python3 tests/test_trace_shift.py
   ```

   You should see three lines, each starting with `ok:`:

   ```
     ok:   R-PROTO-09 (SHIFT_NOMINAL_US equals ps2_master.pio's no-ACK cycles at kCyclesPerBit and kBusClockHz)
     ok:   rejection cases: 5/5 (each mutation turns the R-PROTO-09 line to FAIL)
     ok:   accept case: the calibration knob moved, and the check still passes
   ```

   - The first line is the real check: the count from the program matches the decoder.
   - The second line says the test was shown five deliberately broken copies (an extra
     instruction, a longer delay, a slower bus clock, a different clock divider, a wrong
     number in the decoder), and it caught all five.
   - The third line says that moving the hand correction does not trip the test.

2. Run the full suite:

   ```
   make test
   ```

   The last line must be `OK`.

3. Optional, to see the test bite: open `tools/trace_decode.py`, change
   `SHIFT_NOMINAL_US = 37` to `38`, save, and run step 1 again. The first line now starts
   with `FAIL:` and the next line says `37 cycles give 37 us; the decoder says 38`. Change
   it back to `37` and confirm step 1 shows three `ok:` lines again.

## If something looks wrong

- **The first line is `FAIL:` and says the decoder disagrees.** The PIO program, the bus
  clock or the decoder's number was changed. If the change to the program or the clock was
  intended, look at the value the message gives (`… cycles give N us`):
  - If it is a whole number, set `SHIFT_NOMINAL_US` to it. That changes every `ack` value
    the decoder prints, so the 9 `ack` rows of the hand-written expected output
    `tests/vectors/trace_session.rendered` must be rewritten by hand to match, or
    `make test` fails on R-PROTO-08 instead.
  - If it is not a whole number (it prints as a fraction, such as `37/2 us`), stop. The
    decoder subtracts whole microseconds, so how to round is a decision for the phase that
    changed the program or the clock, not something to fix here.
- **The first line is `FAIL:` and says a `jmp` condition is not understood.** The PIO
  program now uses a kind of jump the test does not know. The test refuses to guess. It
  has to be taught the new condition.
- **Decoded `ack` values all sit well above zero on a loopback capture with ATT jumpered
  to ACK.** This is where `SHIFT_CALIBRATION_US` comes in. The procedure for that capture
  is in `docs/phases/04-trace-mode/verify.md`. Rewrite `tests/vectors/trace_session.rendered`
  afterwards (its 9 `ack` rows), as above.
