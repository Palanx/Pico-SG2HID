# Phase 04-trace-mode — notes

## Outcome

- base: b57a88f (the expansion commit; operator's choice at validation, 2026-10-01 — the work is committed in 7c100ad and 51f1291)
- `docs/adr/0015-bus-trace-text-lines-cpu-timing.md`: CPU-side per-byte timing, `T1` text lines,
  a pure formatter in `core`, a stdlib-only decoder in `tools/`. Rejected: a PIO cycle counter,
  a binary format.
- `src/core/bus_trace.{h,cpp}` (new): `ps2::WireByte { out, in, elapsed_us }` and
  `format_trace_line( frame, completed, line )`. It writes the `T1` line into a caller's buffer
  through a file-local `LineWriter` (`std::to_chars` for decimals, a digit table for hex), with
  no allocation and no I/O. It returns 0 for an empty frame, `completed > n` or overflow.
- `src/hal/bus_port.h`: `struct ByteExchange { std::optional<std::uint8_t> in; std::uint32_t
  elapsed_us; }`; `exchange_byte` returns it.
- `src/hal/bus_frame.{h,cpp}`: `exchange_frame( std::span<WireByte> )`. It sends `.out`, writes
  `.elapsed_us` for every attempted byte (the failed one included) and `.in` for completed
  bytes. ATT framing, the ACK flag and the single exit and release are unchanged.
- `src/hal/pio_port.cpp`: `exchange_byte` takes `elapsed_us = time_us_32( ) - start` right
  after `rx_within` on both paths, before `recover( )` on a timeout, with interrupts disabled
  for that timed window (at most `kByteBudgetUs`, ~132 µs). `ps2_master.pio` is
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

- **Interrupts disabled around the timed window in `exchange_byte` (2026-10-01, operator's
  choice after the first bench capture).** The spec's step 4 said nothing else in
  `pio_port.cpp` changes. The first capture (`build/trace.log`, ATT→ACK jumpered, firmware
  `7c100ad`) failed the bench criterion "every `ack` ≤ 5 µs" with 6 values of 6–14 µs.
  - Steady-state bytes read 36–38 µs, so `SHIFT_US = 37` holds.
  - The outliers were mostly byte 0 of the 9-byte frame (43–44, once 51 on byte 1), plus
    byte 4 once (45). The 1-byte frame, which waits for no ACK, read 42–46.
  - So the excess was not ACK. Inferred cause, not proven: USB stdio interrupts after the
    preceding `printf` landing inside the timed window.
  - Fix: `save_and_disable_interrupts( )` before `start`, `restore_interrupts( )` after
    `elapsed_us`, before `recover( )`; `#include "hardware/sync.h"`.
  - Reconciled: spec step 4 amended in this edit. `verify.md` now says interrupts are off and
    expects 36–40 µs per byte, not 37–45. ADR-0015's "a few µs" overhead statement is still
    true and was left as is (ADRs are immutable). Checked: the Goal and the acceptance
    criteria say nothing about interrupts.
  - The ACK-open capture (`build/trace-ackopen.log`) passed all its criteria before the fix.
  - Both captures need re-taking on the new firmware before `/validate-phase`.

- **Spec amended at validation round 1 (2026-10-01): three undecidables from the independent review.**
  - `T1`'s `n` had no lower bound. `format_trace_line` writes `T1 n=0 k=0 out= in= us=`, and
    `trace_decode.py` rejects it as unreadable. Operator's decision: `n >= 1`, and the formatter
    refuses an empty frame. The Goal table's `n` row and Plan 7's `bus_trace_cases.cpp` bullet
    now say so. The code still has to follow (returned to implementation).
  - Plan 7's "using the same `CXXFLAGS` list `tests/test_bus_frame.py` defines" was deleted: equal
    values or one shared definition could not be told apart. The two lists are equal today; the
    copy is already in Debt.
  - Plan 1's "Alternatives recorded:" now says they go one line each under Consequences, as
    `docs/templates/adr.md` asks. ADR-0015 already does that.
  - Reconciled. Checked: the Goal table's `k` row (`0 <= k <= n`, still true); Plan 7's other
    bullets; the acceptance criteria (none state `n`'s range, the flags or the ADR's sections).
    Two statements outside the spec name the formatter's refusals and become incomplete once the
    code changes: R-PROTO-08's Scope clause in `docs/constraints.md` ("its refusal of
    `completed > n` and of a short buffer") and `verify.md`'s `format_trace_line returns 0`
    bullet. They are the implementation round's to update, in the same edit as the code.

- **Implementation round 2 (2026-10-02): the formatter refuses an empty frame.**
  - Code: `format_trace_line` returns 0 when `frame.empty( )`. Its header comment says `n` is
    at least 1 and lists the refusal. `bus_trace_cases.cpp` asserts it in the existing refusal
    line, now "returns 0 for an empty frame, completed > n and a short buffer". Mutation
    checked by hand: with the guard removed, that line prints `FAIL`.
  - Generalised. The property is "every line the formatter emits, the decoder parses". Each
    rejection in `trace_decode.py` `parse( )` was checked against the formatter:
    - the empty `\S+` lists and `n == 0`: the gap fixed here;
    - `k > n`: already refused;
    - list length `n`: each list loops over `frame.size( )`;
    - `out` two uppercase hex digits: `kHexDigits`;
    - `in` hex below `k` and `--` from `k`: same split;
    - `us` decimal up to `k` and `-` after: `is_attempted = i <= completed`, `uint32_t`.
    No other gap.
  - The caller in `src/app/main.cpp` prints `trace: line too long` on any 0. That is unreachable
    for an empty frame, because its sequences are non-empty constants. Left as is.
  - Reconciled, in the same edit: R-PROTO-08's Scope clause in `docs/constraints.md` and
    `verify.md`'s `format_trace_line returns 0` bullet. Plan 3's signature bullet (`spec.md`,
    "Returns 0 … when `completed > frame.size( )` or the line does not fit") was missed by
    round 1's reconciliation and now names the empty frame too. A grep for
    `completed > (n|frame)` over `docs`, `src`, `tests` and `tools` finds no other statement
    of the refusals.
  - Bench: the firmware binary changed, but only for a frame `main.cpp` never builds. The
    trace lines are byte-identical, so the round-1 captures still stand.

## Debt

- `SHIFT_US = 37` (`tools/trace_decode.py`) is computed from `ps2_master.pio`'s cycle budget.
  The ATT→ACK bench capture (firmware 51f1291) confirms it: 36–38 µs per byte, `ack` 0–1 µs.
  It is still a hand copy that nothing checks against the `.pio` file. That is logged in
  `.claude/rules/tech-debt.md`, "`SHIFT_US` is a hand copy of the PIO cycle budget"
  (2026-10-02). Re-check it against the emulator in `06-hil-digital`.
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

- **Review taste (validation round 1), not blocking:**
  - R-PROTO-08 says `T1` is fixed in `src/core/bus_trace.h`; ADR-0015 points at the spec's Goal.
    Two homes for one format.
  - `tests/test_bus_trace.py` passes a bare `timeout=300` to the build step, next to the named
    `RUN_TIMEOUT_S`.
  - `tests/bus_trace_cases.cpp` mixes `{.out = …}` and `{ .out = … }`.
  - `bus_trace.cpp` leaves the two hex digits that `put_hex` writes implicit.
  - `verify.md` gives 37 µs for the shift and 32 µs of shifting in the timeout budget. Both are
    true (8 bits × 4 µs, plus about 5 µs of bookkeeping), but a reader may stumble.
  - The vectors README and R-PROTO-08's Scope count five frames. The session has six.
- **Review taste (validation round 2), not blocking:** `trace_decode.py` strips `\r\n` before
  echoing a non-trace line, so the Pico's CRLF lines come out as LF; the spec says "echoed
  unchanged" (the visible text is identical). The other two taste items repeat round 1's.

## Validation — 2026-10-01
- criteria: 27 passed / 0 failed. All 19 host criteria were run exactly as written. The 8 bench
  criteria were run on the operator's build/trace.log and build/trace-ackopen.log, captured at
  18:19–18:20 on firmware 51f1291 (committed 18:18; `make firmware` rebuilt nothing). Measured:
  21 `T1` lines, ack 0–1 µs, bytes 36–38 µs; ACK open: 14 aborts at 133 µs, seven 1/1 frames
  complete.
- project gates: test pass, lint pass, typecheck pass (scripts/check.sh rc 0, no
  `workflow gap:` line)
- boundary sweep: clean (`scripts/check.sh --files` over the 12 source and test files in the set)
- independent review: undecidable: `T1` does not bound `n`, so the formatter writes an `n=0` line
  the decoder rejects — undecidable: Plan 7's "the same `CXXFLAGS` list" (equal values or one
  definition) — undecidable: Plan 1's "Alternatives recorded" vs the ADR, which puts them under
  Consequences. No contradicts. Taste: 6 items, moved to For later phases. (settled: 1 — the bench `awk … ack …`
  criterion, `SHIFT_US = 37`)
- closure test: fail: 3 undecidables not settled by a passing criterion (spec amended, see
  Deviations)
- findings: 3
- spec size: 18829 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation. Code side: `format_trace_line` returns 0 for an empty
  frame, plus a `bus_trace_cases.cpp` assertion; R-PROTO-08's Scope clause and `verify.md`'s
  refusal bullet are updated in the same edit.

## Validation — 2026-10-02
- criteria: 27 passed / 0 failed. All 19 host criteria were run exactly as written. The 8 bench
  criteria were run verbatim on the operator's existing captures, `build/trace.log` and
  `build/trace-ackopen.log`, taken on firmware 51f1291. They were not re-captured: round 2
  changed only the formatter's empty-frame refusal, and `main.cpp` never builds an empty frame
  (notes §Deviations, implementation round 2). Measured: 21 `T1` lines, 0 incomplete, 0
  `ack` > 5 µs; ACK open: 0 unexpected headlines, seven 1/1 frames complete.
- project gates: test pass, lint pass, typecheck pass (scripts/check.sh rc 0, no
  `workflow gap:` line)
- boundary sweep: clean (`scripts/check.sh --files` over the 12 source, test and tool files in
  the set; 13 active deny rules; files under src/core/, src/hal/, src/app/)
- independent review: clean. No contradicts. (settled: 2 — the bench `awk … ack …` criterion,
  for `SHIFT_US = 37`; `python3 tests/test_checks_are_live.py`, for the unnamed refusal `ok:`
  line in `test_bus_trace.py`). Taste: 3 items, one new, under For later phases.
- closure test: pass
- findings: 0
- spec size: 18859 (+30 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
