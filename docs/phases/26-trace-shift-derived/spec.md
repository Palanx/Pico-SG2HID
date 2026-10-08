# Phase 26-trace-shift-derived — derive SHIFT_US's nominal part from the PIO program

<!-- Written by /expand-phase, immediately before implementation, never earlier
     (P4). Amendable during implementation ONLY together with a Deviations
     entry in notes.md. -->

## Goal

`tools/trace_decode.py` subtracts a shift time from each byte's elapsed time to print its
`ack` delay. Today that shift time, `SHIFT_US = 37`, is a number copied by hand from
`src/hal/ps2_master.pio`'s cycle-budget comment, so an edit to the PIO program or to the bus
clock changes the true shift time while `make test` stays green.

After this phase:

- The decoder's shift time is the sum of two named constants: `SHIFT_NOMINAL_US` (37), the
  shift time the PIO program implies, and `SHIFT_CALIBRATION_US` (0), the calibration knob
  ADR-0015 calls for, set by hand from a loopback trace. `SHIFT_US` is their sum. The
  decoder's output does not change.
- A new host check, `tests/test_trace_shift.py`, bound to a new rule R-PROTO-09, computes
  the nominal shift time from the sources and fails `make test` when it differs from
  `SHIFT_NOMINAL_US`. The sources are the instructions `ps2_master.pio` executes for a byte
  that waits for no `ACK`, `kCyclesPerBit` in `src/hal/pio_port.cpp`, and `kBusClockHz` in
  `src/core/ps2_protocol.h`. The formula is
  nominal µs = cycles × 1 000 000 / (`kCyclesPerBit` × `kBusClockHz`), and it must come out
  as a whole number equal to `SHIFT_NOMINAL_US`.
- `SHIFT_CALIBRATION_US` is not checked. Changing it never fails the check.

## Context pointers

- `.claude/rules/tech-debt.md` — the debt entry "`SHIFT_US` is a hand copy of the PIO cycle
  budget" that this phase pays: what drifts, and the fix options it lists.
- `docs/adr/0015-bus-trace-text-lines-cpu-timing.md` — `SHIFT_US` is "a calibration knob
  read off a loopback trace, not a cycle-exact count". The calibration constant keeps that
  knob (operator's decision 2026-10-08, recorded on the PHASES.md row).
- `tools/trace_decode.py` — lines 26–36 hold the `SHIFT_US` comment and constant. The
  constant is used once, in `render( )`.
- `src/hal/ps2_master.pio` — the program the check walks (read only). The no-ACK path is
  `pull`, `set x, 7`, eight passes through `out`/`in`/`jmp x--`, `out y, 1`, `jmp !y done`,
  then `push`.
- `src/hal/pio_port.cpp:22` — `kCyclesPerBit = 4` (read only).
- `src/core/ps2_protocol.h:44` — `kBusClockHz = 250000` (read only).
- `tests/test_bus_trace.py` — the house shape for a Python check: a `RULE` marker in the
  docstring, one `ok:`/`FAIL:` line for the real run, mutations that assert their anchor
  matched, and a `rejection cases: n/n` line. Its R-PROTO-08 check keeps the decoder's
  output byte-identical to `tests/vectors/trace_session.rendered`.
- `Makefile:35` — `PY_TESTS := $(wildcard tests/test_*.py)`: `make test` runs every
  `tests/test_*.py`, so the new check needs no runner edit.
- `tests/test_checks_are_live.py` — holds every `tests/test_*.py` to its accounting
  property. A declared `RULE` id must appear on a real-run result line. A line containing
  `rejection`, `accept` or `false-positive` followed by `case(s)` counts as a case line,
  not a real-run line.
- `docs/constraints.md` §Invariants — where R-PROTO-09 is added. R-PROTO-08's entry shows
  the format.
- `tests/test_rule_traceability.py` — fails on any drift between a rule's `test:` binding
  and the file's `RULE` marker.
- `docs/phases/04-trace-mode/notes.md` — the bench measured 36–38 µs per byte, so 37 holds
  on the hardware.
- `docs/phases/06-hil-digital/notes.md` §Bench readings — the decoded line
  `byte 0: out 01 in FF 48 us ack 11 us` that `verify.md` quotes as its example.
- `docs/phases/25-scaffold-bash-floor/verify.md` — the shape of a `verify.md` for a
  host-only phase (R-PROC-02 headings).

## Plan

Re-expansion after the round-3 escape (`notes.md` §Validation, round 3). Steps 1 and 3
landed and stand. Step 2 owes two edits to `tests/test_trace_shift.py`: the exactly-once
anchor check in `mutated( )`, and case 2's label losing "in the bit loop". Step 4 owes
`verify.md`'s non-whole remedy. Validation round 5 added one more owed edit to step 2:
`pio_cycles( )`'s error cases 2–4. Everything else in steps 2 and 4 already matches the
code.

1. **Split the constant.** In `tools/trace_decode.py`, replace `SHIFT_US = 37` with
   `SHIFT_NOMINAL_US = 37`, `SHIFT_CALIBRATION_US = 0` and
   `SHIFT_US = SHIFT_NOMINAL_US + SHIFT_CALIBRATION_US`. Each constant sits on its own line
   with no trailing comment. Rewrite the comment above them so that it says:
   - the nominal part is the PIO cycle count and is checked by `tests/test_trace_shift.py`
     (R-PROTO-09);
   - the calibration part is the hand-set knob, and the existing loopback-trace procedure
     says how to set it.

   `render( )` is unchanged.
   Touches `tools/trace_decode.py`.
   Check: `python3 tests/test_bus_trace.py` → exit 0.

2. **Write the check.** Create `tests/test_trace_shift.py`, standard library only, in the
   shape of `tests/test_bus_trace.py`. Its docstring carries
   `# RULE R-PROTO-09 — docs/constraints.md §Invariants — …`. It contains:
   - `pio_cycles( text )`. It walks the `.pio` program from `.wrap_target` to `.wrap`, with
     the `ACK` flag read by `out y, …` as 0, and returns the cycles executed. Lines are
     handled as follows:
     - Ignored: comments (from `;`), blank lines, `.program` and `.side_set`.
     - A label `name:` marks the position of the next instruction. An instruction after it
       on the same line (`name: out pins, 1`) is that instruction.
     - Every instruction executed costs 1 cycle plus its `[n]` delay, read only as the
       line's last token. A `side` operand costs nothing.
     - `set x, <n>` sets x. `out y, <n>` sets y to 0.
     - `jmp <label>` always jumps. `jmp x-- <label>` jumps while x is non-zero, then
       decrements x. `jmp !y <label>` jumps when y is 0.
     - Any other mnemonic only costs its cycles.

     - `.wrap_target` and `.wrap` are position markers, not instructions. Any other
       directive on the path counts as an instruction.

     It raises an error, which the check reports as a `FAIL`, in these seven cases:
     1. `.wrap_target` or `.wrap` is missing;
     2. the walk leaves the program (its next instruction is past the last one) before
        reaching `.wrap`;
     3. an instruction it reaches holds a `[` that is not a last-token `[n]` delay;
     4. an instruction it reaches lacks an operand its mnemonic needs (`set x`, `out y`,
        `jmp`), or its `set x` operand is not a decimal number;
     5. a `jmp` it reaches has a condition other than none, `x--` or `!y`;
     6. a `jmp` it reaches names a label the program does not define (a `jmp` the walk
        never reaches is not read);
     7. the walk does not reach `.wrap` within `MAX_STEPS` = 10 000 instructions. A walk that reaches `.wrap` after exactly 10 000
     instructions passes; one that needs 10 001 fails.
   - `constant( text, name )`. It reads `constexpr std::uint32_t <name> = <n>;` (any spaces
     around `=`) and returns n, or raises the same error when no such line exists.
   - `check( texts, is_verbose )`. It takes the four file texts (`.pio`, `pio_port.cpp`,
     `ps2_protocol.h`, decoder) and computes nominal µs as a `fractions.Fraction`. It reads
     `^SHIFT_NOMINAL_US = (\d+)$` from the decoder and returns whether they match. When
     `is_verbose` (the real run; the cases pass `False`), it prints
     `  ok:   R-PROTO-09 (SHIFT_NOMINAL_US equals ps2_master.pio's no-ACK cycles at kCyclesPerBit and kBusClockHz)`
     or the same label after `FAIL:`, with the reason (computed value or error) on an
     indented line after a `FAIL`.
   - `mutated( texts, mutation )`. It applies one mutation and requires its anchor to occur
     exactly once in its file. Otherwise it prints
     `  FAIL: <label>: the anchor <anchor> occurs <n> times in <file>, not once` and returns
     `None`, and the case counts as failed. Every anchor below occurs exactly once in
     today's tree.
   - Cases, which run `check( texts, False )` on mutated in-memory texts. The labels name
     the change only, never where in the file it lands. The rejection cases must each turn
     the check to `FAIL`:

     | # | file | label | anchor → replacement |
     |---|---|---|---|
     | 1 | `.pio` | one instruction added | `set x, 7` → `set x, 7\n    nop` |
     | 2 | `.pio` | a delay changed | `side 0 [1]` → `side 0 [2]` |
     | 3 | `ps2_protocol.h` | the bus clock changed | `kBusClockHz = 250000` → `kBusClockHz = 125000` |
     | 4 | `pio_port.cpp` | the divider changed | `kCyclesPerBit  = 4` → `kCyclesPerBit  = 8` |
     | 5 | decoder | the copy drifted | `SHIFT_NOMINAL_US = 37` → `SHIFT_NOMINAL_US = 38` |

     It prints `  ok:   rejection cases: 5/5 …` or `  FAIL: rejection cases: n/5`. There
     is one accept case, labelled `the calibration knob moved`, which must stay `ok`: the decoder's `SHIFT_CALIBRATION_US = 0` →
     `SHIFT_CALIBRATION_US = 2`. It prints `  ok:   accept case: …` or `  FAIL: accept case: …`.
   - It exits 1 if anything failed, 0 otherwise.

   Touches `tests/test_trace_shift.py`.
   Check: `python3 tests/test_trace_shift.py` → exit 0, three `ok:` lines.

3. **Add the rule.** In `docs/constraints.md` §Invariants, add R-PROTO-09 after
   R-PROTO-08. Its text: the decoder's `SHIFT_NOMINAL_US` equals the cycles `ps2_master.pio`
   executes for a byte that waits for no `ACK`, converted to µs at `kCyclesPerBit` and
   `kBusClockHz`; `SHIFT_CALIBRATION_US` is the hand-set calibration knob of ADR-0015 and
   is not checked. Add one Scope clause saying what the check reads: only the instructions
   and `jmp` conditions `ps2_master.pio` uses, as listed in step 2. Binding:
   `test: tests/test_trace_shift.py`.
   Touches `docs/constraints.md`.
   Check: `python3 tests/test_rule_traceability.py` → exit 0.

4. **Write the operator guide.** Create `docs/phases/26-trace-shift-derived/verify.md` with
   a `## What was built` and a `## Check it yourself` heading. No hardware is needed. It
   covers:
   - why the 37 µs matters (the `ack` column of a decoded trace);
   - running `python3 tests/test_trace_shift.py` and reading its three `ok:` lines;
   - where to put a bench-measured correction (`SHIFT_CALIBRATION_US`);
   - what to do when the R-PROTO-09 line is `FAIL` because the program or the clock
     changed on purpose. If the message's computed value is a whole number, set
     `SHIFT_NOMINAL_US` to it. If it is not a whole number (the message prints a fraction,
     e.g. `37/2 us`), stop: the decoder subtracts whole microseconds, so how to round is a
     decision for the phase that changed the program or the clock. The guide uses the
     phrase `not a whole number` for this case.

   Three places in the guide say that the 9 `ack` rows of
   `tests/vectors/trace_session.rendered` must then be rewritten by hand, or `make test`
   fails on R-PROTO-08: the `SHIFT_CALIBRATION_US` bullet under What was built, the
   whole-number remedy, and the loopback-capture entry. Step 3's temporary 38 is reverted
   in the same step and needs no rewrite.

   Touches `docs/phases/26-trace-shift-derived/verify.md`.
   Check: `grep -cE '^#+ (What was built|.*[Cc]heck it)' docs/phases/26-trace-shift-derived/verify.md` → 2.

5. **Gates.** Touches nothing. Check: `make test 2>&1 | tail -n 1` → `OK`;
   `python3 tests/test_checks_are_live.py` → exit 0.

## Acceptance criteria

```
make test 2>&1 | tail -n 1                                       # expect: OK
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
python3 tests/test_trace_shift.py                                # expect: exit 0
python3 tests/test_trace_shift.py | grep -c '^  ok:'             # expect: 3
grep -c '^SHIFT_NOMINAL_US = 37$' tools/trace_decode.py          # expect: 1
grep -c '^SHIFT_CALIBRATION_US = 0$' tools/trace_decode.py       # expect: 1
grep -c '^SHIFT_US = SHIFT_NOMINAL_US + SHIFT_CALIBRATION_US$' tools/trace_decode.py   # expect: 1
python3 tools/trace_decode.py tests/vectors/trace_session.txt | cmp - tests/vectors/trace_session.rendered   # expect: exit 0
grep -c 'R-PROTO-09.*test: `tests/test_trace_shift.py`' docs/constraints.md   # expect: 1
git diff --quiet main -- src tests/vectors                       # expect: exit 0
python3 tests/test_rule_traceability.py                          # expect: exit 0
python3 tests/test_checks_are_live.py                            # expect: exit 0
sh tests/test_phase_docs.sh                                      # expect: exit 0
grep -c 'bit loop' tests/test_trace_shift.py                     # expect: 0
python3 -c "import sys; sys.path.insert(0, 'tests'); import test_trace_shift as t; sys.exit(t.mutated({t.PIO: 'ab ab'}, t.Mutation(t.PIO, 'twice', 'ab', 'cd')) is not None)"   # expect: exit 0, one FAIL line naming 2 occurrences
grep -c 'not a whole number' docs/phases/26-trace-shift-derived/verify.md   # expect: 1
python3 -c "import sys; sys.path.insert(0, 'tests'); import test_trace_shift as t; p = t.read(t.PIO); q = p.replace('bitloop:\n    out pins, 1', 'bitloop: out pins, 1'); sys.exit(q == p or t.pio_cycles(q) != 37)"   # expect: exit 0
```

## Out of scope

- Any edit to `src/hal/ps2_master.pio`, `src/hal/pio_port.cpp` or `kBusClockHz`. The
  check reads them and never changes them.
- A general PIO assembler parser. Only the mnemonics and `jmp` conditions in step 2 are
  understood. A new condition fails the check and is extended when a phase needs it.
- Deriving `kCyclesPerBit` from the program's bit loop. It is read from `pio_port.cpp`.
- Re-measuring the shift time on the bench, or setting `SHIFT_CALIBRATION_US` to anything
  but 0. Owner: 09-guitar-observe, if the guitar's traces show a bias.
- The `T1` format and the decoder's output (R-PROTO-08).
- Editing `.claude/rules/tech-debt.md`. The entry's removal is proposed to the operator
  after validation and waits for their answer.
- `docs/phases/06-hil-digital/verify.md`'s "37 µs to shift the byte out". It stays true.
