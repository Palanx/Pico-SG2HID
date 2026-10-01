# Phase 04-trace-mode — notes

## Outcome

- `docs/adr/0015-bus-trace-text-lines-cpu-timing.md`: CPU-side per-byte timing, `T1` text lines,
  a pure formatter in `core`, a stdlib-only decoder in `tools/`. Rejected: a PIO cycle counter,
  a binary format.
- `src/core/bus_trace.{h,cpp}` (new): `ps2::WireByte { out, in, elapsed_us }` and
  `format_trace_line( frame, completed, line )`. It writes the `T1` line into a caller's buffer
  through a file-local `LineWriter` (`std::to_chars` for decimals, a digit table for hex), with
  no allocation and no I/O. It returns 0 for `completed > n` or overflow.
- `src/hal/bus_port.h`: `struct ByteExchange { std::optional<std::uint8_t> in; std::uint32_t
  elapsed_us; }`; `exchange_byte` returns it.
- `src/hal/bus_frame.{h,cpp}`: `exchange_frame( std::span<WireByte> )`. It sends `.out`, writes
  `.elapsed_us` for every attempted byte (the failed one included) and `.in` for completed
  bytes. ATT framing, the ACK flag and the single exit and release are unchanged.
- `src/hal/pio_port.cpp`: `exchange_byte` takes `elapsed_us = time_us_32( ) - start` right
  after `rx_within` on both paths, before `recover( )` on a timeout. `ps2_master.pio` is
  unchanged.
- `src/app/main.cpp`: each sequence runs as a `WireByte` array. The `loopback:` line is
  unchanged and is followed by the `T1` line, or by `trace: line too long`.
  `kTraceLineSize = 256`.
- `tools/trace_decode.py` (new, executable, stdlib only): renders `T1` lines as a headline plus
  one row per byte. Row format:
  `  byte <i>: out <XX> in <XX|--> <u> us[ ack <d> us]`, or `… in -- not sent`.
  - `ack` is printed for completed non-last bytes, `d = max(0, u - SHIFT_US)`, `SHIFT_US = 37`.
  - Other lines are echoed. A malformed `T1` line prints `unreadable trace line: …` and makes
    the exit 1.
- `tests/vectors/trace_session.txt` and `.rendered` (new, hand-written): a `probe:` line and
  five well-formed frames. They are complete 5- and 1-byte frames, plus 5-byte aborted at 0,
  9-byte aborted at 4 and 2-byte aborted at its last byte. One malformed line comes last.
- `tests/bus_trace_cases.cpp` (new): the five frames as hand-written `WireByte`s. Fields the
  formatter must not read hold junk values. It also checks the two refusals.
- `tests/test_bus_trace.py` (new, `RULE R-PROTO-08`): the real run compares the formatter and
  the decoder against the vectors. Rejection cases: 4/4.
- `tests/bus_frame_cases.cpp`: the fake returns a distinct `elapsed_us` per exchange. There is
  one new real-run line (no rule id) for the elapsed-time placement.
  `tests/test_bus_frame.py`: two anchors follow `if ( !got.in ) {`; still 5/5.
- `docs/constraints.md`: R-PROTO-08 after R-PROTO-07, with a Scope clause.
- `docs/phases/04-trace-mode/verify.md`.

## Deviations

- **`tests/vectors/README.md` changed; the Plan did not name it.** Its first sentence said every
  vector is a `constexpr std::uint8_t` header, and step 7 adds two text files there. The README
  got one paragraph saying why the trace vectors are text and that only
  `tests/test_bus_trace.py` reads them. Not a missing pointer: the README is in the Context
  pointers ("the house rules for hand-written vectors"); only Plan step 7's *Touches* line
  omitted it. **Spec amended (2026-10-01):** step 7's *Touches* line now names
  `tests/vectors/README.md`, with a sub-bullet saying what the paragraph says. Reconciled:
  checked the Goal, the Context pointers and the acceptance criteria for other statements
  about the vectors' form or step 7's file list — none, so nothing else changed.
- **Session vector has a sixth frame.** It is the 2-byte frame aborted at its last byte. Step 7's
  "at least" list doesn't require it, and it was added so the `did not complete` headline has a
  literal. In scope, no spec change.
- **The formatter's refusal check has its own `ok:` line with no rule id**
  (`format_trace_line returns 0 …`), printed by `bus_trace_cases.cpp` and forwarded. The
  R-PROTO-08 line also fails when that binary exits non-zero. The spec said the cases file
  "asserts" it and did not say how it is reported.
- **Decoder row format chosen here.** The spec fixed the row's content, not its exact
  layout. The layout above is now pinned by `trace_session.rendered` and by the bench `awk`
  (` ack <d> us`).

## Debt

- `SHIFT_US = 37` (`tools/trace_decode.py`) is computed from `ps2_master.pio`'s cycle budget,
  not measured. Ceiling: every `ack` delay carries the CPU's polling overhead (a few µs).
  Upgrade path: re-read it off this phase's ATT→ACK bench capture (`verify.md` explains how),
  and re-check it against the emulator in `06-hil-digital`.
- `tests/test_bus_trace.py`'s `CXXFLAGS` is a hand-copied list, the same as
  `tests/test_bus_frame.py`'s, and is not read from the `Makefile`. Same ceiling and upgrade
  path as the 24-pio-bus entry. Recorded, not scheduled.

## For later phases

- **06-hil-digital**: `ByteExchange.elapsed_us` is available per byte through
  `exchange_frame`'s `WireByte`s. Link polling can trace with `format_trace_line` unchanged.
  The runtime on/off switch (out of scope here) still needs a home.
- **09-guitar-observe**: capture with `head -n … "$PORT" | tail -n +2 > file`, then decode with
  `tools/trace_decode.py file`. A failed byte's `us` is the time the master waited (about 135 µs
  at the 100 µs timeout). `SHIFT_US` should be confirmed from a loopback capture before the
  guitar's `ack` delays are read as absolute numbers.
- **Format changes**: per ADR-0015, a changed line is `T2` with its own vectors, never an edit
  of `T1`. `trace_decode.py` currently echoes a `T2` line as an unknown line.
