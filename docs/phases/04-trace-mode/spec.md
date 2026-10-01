# Phase 04-trace-mode — the bus trace over USB CDC and its host decoder

## Goal

After this phase every frame the loopback firmware runs is also printed over USB stdio (the
Pico's USB CDC serial port) as one **trace line**: the bytes the master sent, the bytes it
received, how many bytes completed, and for every byte it attempted the microseconds from
handing the byte to the PIO until the byte finished or was given up on. A host script,
`tools/trace_decode.py`, reads a captured session (a file, or the serial device directly) and
renders each trace line as a readable frame: one headline saying whether the frame completed
or where it aborted, and one row per byte with the sent and received byte, the elapsed time,
and the derived `ACK` delay (elapsed minus the fixed shift time; the last byte waits for no
`ACK`, so it has none). Lines that are not trace lines are echoed unchanged; a line that starts
like a trace line but does not parse is reported as unreadable.

Decisions taken at expansion (operator, 2026-10-01), recorded in a new ADR-0015 before code
lands:

- Timing is measured by the CPU (`time_us_32( )` around each byte in `src/hal/pio_port.cpp`),
  not counted inside the PIO program. `ps2_master.pio` does not change.
- The format is text: one ASCII line per frame, prefixed with a format version (`T1`).

The trace line format, `T1`, is the contract between the two ends:

```
T1 n=<n> k=<k> out=<b0>,…,<bn-1> in=<b0>,…,<bn-1> us=<u0>,…,<un-1>
```

| Field | Content |
|---|---|
| `n` | frame length, decimal |
| `k` | bytes completed, decimal, `0 <= k <= n` |
| `out` | every byte sent, two uppercase hex digits each |
| `in` | byte `i` received for `i < k`; `--` for `i >= k` |
| `us` | elapsed µs, decimal, for every attempted byte (`i < k`, plus byte `k` when `k < n`); `-` for a byte never sent |

Fields are separated by one space, list items by `,`, and nothing follows the last item.

What this phase moves:

| Rule | From | To |
|---|---|---|
| R-PROTO-08 (new): the firmware's trace formatter and the host decoder are both checked against the same hand-written `T1` lines | new | `test: tests/test_bus_trace.py` |

## Context pointers

- `CLAUDE.md` §Hardware safety: R-SAFETY-08 — the guitar stays unplugged; this phase's bench
  is the same one-Pico loopback as 24-pio-bus.
- `docs/product/requirements.md` R5: what trace mode is for (the operator's logic analyzer).
- `docs/adr/0007-error-model.md`: no status beside a separate out-parameter; a value that may be
  absent carries its absence (`std::optional`). Shapes `ByteExchange` below.
- `docs/templates/adr.md` and the newest ADR, `docs/adr/0014-pico-sdk-external-pinned.md`: the
  shape and numbering of ADR-0015.
- `docs/constraints.md`:
  - §Layering: `tools/` is outside the layering; `core` performs no I/O and allocates nothing.
  - §Invariants: R-PROTO-05 (hand-written literals under `tests/vectors/`; no `src/` file
    references that path), R-PROTO-06, R-SAFETY-07, R-ARCH-01, R-ARCH-03, R-CLEAN-04, R-PROC-04
    (tests need only a C++23 compiler and `python3`: the decoder uses the standard library only).
  - §PS2 protocol: where R-PROTO-08 goes, after R-PROTO-07.
- `docs/phases/24-pio-bus/notes.md` §For later phases, `04-trace-mode` and `06-hil-digital`
  bullets: `exchange_frame` returns only a count; a byte without `ACK` wait spends a fixed
  number of PIO cycles before `push` (the decoder's shift constant).
- `src/hal/bus_port.h`: the seam; `exchange_byte` changes its return type here.
- `src/hal/bus_frame.{h,cpp}`: the frame loop; its signature changes here.
- `src/hal/pio_port.cpp`: `exchange_byte` gains the elapsed time.
- `src/hal/ps2_master.pio`: its cycle budget comment is where the decoder's shift constant
  comes from. Read only.
- `src/core/ps2_protocol.h`: the model for a `core` header (comment density, `namespace ps2`).
- `src/app/main.cpp`: the loopback program; it gains the trace line. Its `probe:` and
  `loopback:` lines keep their exact format (24-pio-bus's bench criteria parse them).
- `tests/bus_frame_cases.cpp`, `tests/test_bus_frame.py`: the fake port and the five mutations
  whose anchors move with the frame loop.
- `tests/test_ps2_codec.py`: the model for the new driver — compile a `*_cases.cpp` (not
  `test_*.cpp`) with sources from `src/`, forward its `ok:`/`FAIL:` lines, copied-tree mutations
  that assert their anchor matched.
- `tests/test_checks_are_live.py` §`property_accounting`: a `.py` check declares `RULE <id>` in
  its docstring, its real run prints an `ok:`/`FAIL:` line naming each declared id, and case
  lines say "rejection cases".
- `tests/test_rule_traceability.py`: a `test:` binding needs the file to carry `RULE <id>`.
- `tests/vectors/README.md`: the house rules for hand-written vectors.
- `CMakeLists.txt`: the `SG2HID_SOURCES` glob already picks up a new `src/core/*.cpp`.
- `docs/phases/24-pio-bus/verify.md`: the jumpers, flashing and port-reading steps this
  phase's `verify.md` reuses, and the house shape of `verify.md`.

## Plan

1. **ADR-0015.** Touches `docs/adr/0015-bus-trace-text-lines-cpu-timing.md`.
   - Status accepted. Decision: CPU-side per-byte timing, `T1` text lines over USB stdio, a pure
     formatter in `core`, a stdlib-only decoder in `tools/`. Alternatives recorded: a PIO cycle
     counter (exact, but rewrites `ps2_master.pio`, touches R-PROTO-07 and voids 24's bench
     logs), and a binary framed format (unreadable without the decoder; the bandwidth is not
     needed).
   - Check: `test -f docs/adr/0015-bus-trace-text-lines-cpu-timing.md` → exit 0.

2. **The trace formatter in `core`.** Touches `src/core/bus_trace.h`, `src/core/bus_trace.cpp`.
   - `bus_trace.h`, `namespace ps2`:
     - `struct WireByte { std::uint8_t out; std::uint8_t in; std::uint32_t elapsed_us; };` —
       one byte on the wire as the frame loop records it.
     - `[[nodiscard]] std::size_t format_trace_line( std::span<const WireByte> frame,
       std::size_t completed, std::span<char> line );` — writes the `T1` line (§Goal), with no
       newline and no terminating NUL, and returns its length. Returns 0 and leaves the line's
       content unspecified when `completed > frame.size( )` or the line does not fit.
   - No allocation, no I/O, no clock; every number is a named `constexpr` (R-CLEAN-04).
   - Check: `make typecheck` → exit 0; `make test` → last line `OK`.

3. **The seam and the frame loop carry the timing.** Touches `src/hal/bus_port.h`,
   `src/hal/bus_frame.h`, `src/hal/bus_frame.cpp`, `tests/bus_frame_cases.cpp`,
   `tests/test_bus_frame.py`.
   - `bus_port.h`: `struct ByteExchange { std::optional<std::uint8_t> in; std::uint32_t
     elapsed_us; };` and `[[nodiscard]] ByteExchange exchange_byte( std::uint8_t out, bool
     should_wait_ack );`. `in` is `std::nullopt` exactly when the old return was; `elapsed_us`
     is filled on both paths.
   - `bus_frame.h`/`.cpp`: `[[nodiscard]] std::size_t exchange_frame( std::span<WireByte>
     frame );` (includes `core/bus_trace.h`). Byte `i` sends `frame[ i ].out`; the elapsed time
     goes to `frame[ i ].elapsed_us` for every attempted byte, the failed one included; the
     received byte goes to `frame[ i ].in` only when it completed. ATT framing, the `ACK` flag,
     the stop at the first failure, one exit and one release are unchanged.
   - `bus_frame_cases.cpp`: the fake's `exchange_byte` returns a distinct `elapsed_us` per
     exchange. The existing R-SAFETY-07 and R-PROTO-06 assertions are unchanged in meaning.
     One more real-run line, with no rule id, says every attempted byte holds the fake's
     elapsed time and every byte never sent still holds its initial value.
   - `test_bus_frame.py`: the five mutations keep their rule and their behaviour; only their
     anchors follow the new code. Still `rejection cases: 5/5`.
   - Check: `python3 tests/test_bus_frame.py` → exit 0, `ok:` lines naming R-SAFETY-07 and
     R-PROTO-06, `rejection cases: 5/5`.

4. **The PIO port measures.** Touches `src/hal/pio_port.cpp`.
   - `exchange_byte` returns `elapsed_us = time_us_32( ) - start` taken when the RX word is
     seen, or when the budget is given up on (before `recover( )`). `start` is taken before
     `pio_sm_put`, as today. Nothing else in the file changes.
   - Check: `make firmware && test -f build/pico/sg2hid.uf2` → exit 0; `make lint` → exit 0.

5. **The loopback program prints the trace.** Touches `src/app/main.cpp`.
   - Each sequence is copied into a `WireByte` array (`out` from the sequence), run through
     `exchange_frame`, and judged as today from the `in` fields. The `loopback:` line is
     printed exactly as before, then the frame's trace line from `format_trace_line`, then
     `\n`. A zero length prints `trace: line too long` instead. The line buffer's size is a
     named `constexpr`.
   - Check: `make firmware` → exit 0; `make lint` → exit 0.

6. **The host decoder.** Touches `tools/trace_decode.py`.
   - Python 3, standard library only, executable, `#!/usr/bin/env python3`. Reads the files
     named on the command line, or standard input when none, line by line (so a serial device
     path streams live).
   - A line starting with `T1 ` is parsed against §Goal's table; anything else is echoed
     unchanged. Each parsed line renders as:
     - a headline: `frame: <k>/<n> bytes, complete` when `k == n`;
       `frame: <k>/<n> bytes, aborted at byte <k>: no ACK after <u> us` when `k < n − 1`;
       `frame: <k>/<n> bytes, aborted at byte <k>: did not complete after <u> us` when
       `k == n − 1` (the last byte waits for no `ACK`), where `<u>` is byte `k`'s elapsed time;
     - one row per byte: index, `out` byte, `in` byte (or `--`), elapsed µs (or `not sent`),
       and `ack <d> us` for every completed byte but the last, where `d = max( 0, elapsed −
       SHIFT_US )`. `SHIFT_US` is a named constant derived from `ps2_master.pio`'s cycle
       budget, with a comment saying it is a calibration knob and how to read it off a
       loopback trace.
   - Everything goes to standard output. A `T1` line that does not parse prints
     `unreadable trace line: <line>`; the run goes on
     and exits 1 at the end. Otherwise exit 0.
   - Check: `printf 'T1 n=1 k=1 out=80 in=80 us=36\n' | python3 tools/trace_decode.py` → exit
     0, output has `frame: 1/1 bytes, complete`.

7. **The shared vectors and R-PROTO-08's check.** Touches `tests/vectors/trace_session.txt`,
   `tests/vectors/trace_session.rendered`, `tests/bus_trace_cases.cpp`,
   `tests/test_bus_trace.py`, `tests/vectors/README.md`.
   - `README.md`: one paragraph saying the two `trace_session.*` files are the one exception to
     its header form — the `T1` contract's literals are text — and that only
     `tests/test_bus_trace.py` reads them.
   - `trace_session.txt`, hand-written: at least a complete 5-byte frame, a complete 1-byte
     frame, a 5-byte frame aborted at byte 0, a 9-byte frame aborted mid-frame, one `probe:`
     line, and one malformed `T1` line, which is last.
   - `trace_session.rendered`, hand-written: the decoder's exact expected output for it.
   - `bus_trace_cases.cpp` (not `test_*.cpp`, as `ps2_codec_cases.cpp`): hand-written
     `WireByte` inputs, one per well-formed `T1` line of the session, in order; prints each
     formatted line. It also asserts that `completed > n` and a too-small buffer return 0.
   - `test_bus_trace.py`, docstring `RULE R-PROTO-08`. Real run:
     - formatter: compiles `bus_trace_cases.cpp` with `src/core/bus_trace.cpp`, using the same
       `CXXFLAGS` list `tests/test_bus_frame.py` defines, runs it, and requires its output lines to equal the
       session's well-formed `T1` lines in order;
     - decoder: runs `tools/trace_decode.py` on `trace_session.txt` and requires exit 1 (the
       malformed line) and stdout equal to `trace_session.rendered`;
     - one real-run line, `ok:`/`FAIL:` naming R-PROTO-08, covering both.
   - Rejection cases, copied tree, each asserting its anchor matched and requiring the
     R-PROTO-08 line to turn to `FAIL`:
     - formatter prints `-` for the failed byte's elapsed time;
     - formatter separates list items with something other than `,`;
     - decoder computes the `ACK` delay without subtracting `SHIFT_US`;
     - decoder treats the last byte as waiting for `ACK`.
     It prints `rejection cases: 4/4` and fails below that.
   - Check: `python3 tests/test_bus_trace.py` → exit 0, an `ok:` line naming R-PROTO-08,
     `rejection cases: 4/4`; `python3 tests/test_checks_are_live.py` → exit 0.

8. **The rule.** Touches `docs/constraints.md`.
   - New R-PROTO-08 after R-PROTO-07: the bus trace has one format, `T1`; the firmware's
     formatter and the host decoder are both checked against the same hand-written trace
     lines under `tests/vectors/`. Binding `test: tests/test_bus_trace.py`. Its Scope clause:
     the check reads the formatter and the decoder, not the USB transport and not the timing
     values' accuracy, which only the bench shows.
   - Check: `python3 tests/test_rule_traceability.py` → exit 0.

9. **`docs/phases/04-trace-mode/verify.md`**, for a non-specialist, with the headings
   `## What was built` and `## Check it yourself`.
   - What a trace line is, field by field; what an `ACK` delay is and why it is elapsed minus
     the shift time; what `SHIFT_US` is and how to tell from a loopback trace if it is off.
   - The host checks: `python3 tests/test_bus_trace.py` and what its lines mean.
   - The bench, **guitar unplugged** (R-SAFETY-08), same jumpers and flashing as
     24-pio-bus's `verify.md`: capture with jumpers, decode, read one frame aloud; then with
     the `ATT`–`ACK` jumper removed, capture, decode, and say from the trace which byte
     aborted and after how long.
   - Decoding live: `python3 tools/trace_decode.py /dev/cu.usbmodem*`.
   - Check: `sh tests/test_phase_docs.sh` → exit 0.

## Acceptance criteria

Host part: from the repository root with the SDK installed, `PICO_SDK_PATH` exported and the
cask's ARM `bin` on `PATH`, in order.

```
test -f docs/adr/0015-bus-trace-text-lines-cpu-timing.md                  # expect: exit 0
python3 tests/test_bus_trace.py                                          # expect: exit 0; an `ok:` line naming R-PROTO-08; `rejection cases: 4/4`
python3 tests/test_bus_frame.py                                          # expect: exit 0; `ok:` lines naming R-SAFETY-07 and R-PROTO-06; `rejection cases: 5/5`
printf 'T1 n=1 k=1 out=80 in=80 us=36\n' | python3 tools/trace_decode.py | grep -c 'frame: 1/1 bytes, complete'   # expect: 1
printf 'T1 n=2 k=0 out=01,42 in=--,-- us=140,-\n' | python3 tools/trace_decode.py | grep -c 'aborted at byte 0: no ACK after 140 us'   # expect: 1
grep -lE '#[[:space:]]*include[[:space:]]*[<"](pico|hardware)/' src/core/bus_trace.h src/core/bus_trace.cpp src/hal/bus_port.h src/hal/bus_frame.h src/hal/bus_frame.cpp   # expect: no output
git diff --quiet main -- src/hal/ps2_master.pio                           # expect: exit 0
grep -cE '^import (serial|numpy)|^from (serial|numpy)' tools/trace_decode.py   # expect: 0
grep -cE '^- \*\*R-PROTO-08\*\* .* — test: `tests/test_bus_trace\.py`$' docs/constraints.md   # expect: 1
make firmware && test -f build/pico/sg2hid.uf2                           # expect: exit 0
make lint                                                                # expect: exit 0
make typecheck                                                           # expect: exit 0
python3 tests/test_rule_traceability.py                                  # expect: exit 0
python3 tests/test_checks_are_live.py                                    # expect: exit 0
sh tests/test_repo_shape.sh                                              # expect: exit 0
sh tests/test_boundaries.sh                                              # expect: exit 0
sh tests/test_phase_docs.sh                                              # expect: exit 0
mv build/pico build/pico.off; env -u PICO_SDK_PATH PATH=/opt/homebrew/opt/bash/bin:/usr/bin:/bin make test | tail -1; mv build/pico.off build/pico   # expect: OK
make test                                                                # expect: last line OK
```

Bench part: the operator runs these with the guitar unplugged, this phase's firmware flashed
and the jumpers as `verify.md` says. `/validate-phase` records the operator's output; it cannot
run them itself. The first line read from the port may be partial, hence `tail -n +2`.

```
# Jumpers CMD→DATA and ATT→ACK in place:
PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 51 "$PORT" | tail -n +2 > build/trace.log
python3 tools/trace_decode.py build/trace.log > build/trace.txt; echo $?                     # expect: 0
grep -c '^T1 ' build/trace.log                                                               # expect: >= 15 — three frames per second, seven lines per second
grep '^frame:' build/trace.txt | grep -vc 'complete$'                                        # expect: 0
awk '/ ack [0-9]+ us/ { for (i = 1; i <= NF; i++) if ($i == "ack" && $(i+1) > 5) bad++ } END { print bad + 0 }' build/trace.txt   # expect: 0 — with ATT jumpered to ACK, ACK is already low, so every delay is near zero
# USB unplugged, ATT→ACK jumper removed, USB back in:
PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 51 "$PORT" | tail -n +2 > build/trace-ackopen.log
python3 tools/trace_decode.py build/trace-ackopen.log > build/trace-ackopen.txt; echo $?     # expect: 0
grep '^frame:' build/trace-ackopen.txt | grep -vcE 'frame: 0/(5|9) bytes, aborted at byte 0: no ACK after [0-9]+ us|frame: 1/1 bytes, complete'   # expect: 0
grep -c 'frame: 1/1 bytes, complete' build/trace-ackopen.txt                                 # expect: >= 5
```

## Out of scope

- Counting the `ACK` wait inside the PIO program, and any change to `ps2_master.pio`. Rejected
  in ADR-0015.
- A runtime switch that turns tracing on or off, and tracing frames the link layer polls.
  Owners: `06-hil-digital` (polling) and `09-guitar-observe` (the trace procedure against the
  real guitar).
- Printing `Link::last_fault`: nothing calls `step` yet. Owner: `06-hil-digital`.
- Tracing the bit-order probe; changing the `probe:` or `loopback:` lines.
- A serial-port library (`pyserial`) or any dependency for the decoder; Windows serial paths.
- Python style or lint for `tools/`: no tool is configured for it, and none is added here.
