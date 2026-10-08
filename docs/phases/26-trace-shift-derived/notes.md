# Phase 26-trace-shift-derived — notes

## Outcome

- `tools/trace_decode.py`: `SHIFT_US = 37` is now `SHIFT_NOMINAL_US = 37` plus
  `SHIFT_CALIBRATION_US = 0`, and `SHIFT_US` is their sum. The comment above them says
  which part is checked (R-PROTO-09) and how to set the knob. `render( )` is unchanged, so
  the decoder's output on `tests/vectors/trace_session.txt` is byte-identical.
- `tests/test_trace_shift.py` (new) walks `src/hal/ps2_master.pio` from `.wrap_target` to
  `.wrap` with the ACK flag clear. It gets 37 cycles and converts them at `kCyclesPerBit`
  (4) and `kBusClockHz` (250000) to 37 µs. It then runs five rejection cases (each fails for
  the reason the spec's table gives: 38, 45, 74, 37/2 µs, and the drifted copy 38) and one
  accept case.
- `docs/constraints.md`: R-PROTO-09 added after R-PROTO-08, `test: tests/test_trace_shift.py`.
- `docs/phases/26-trace-shift-derived/verify.md` (new): the operator guide, no hardware.
- No file under `src/` or `tests/vectors/` changed.
- Round 4 (after the re-expansion, 2026-10-08): `mutated( )` now requires each anchor to occur exactly once and prints `the anchor … occurs n times in …, not once` otherwise; case 2's label is `a delay changed`; `verify.md` covers a computed nominal that is not a whole number (stop, the phase that changed the program or clock decides the rounding).
- Round 6 (2026-10-08): `pio_cycles( )` reports as `FAIL` a walk that leaves the program, a `[` that is not a last-token delay, and a missing or non-decimal operand; Plan step 2 lists all seven error cases.
- Round 7 (2026-10-08): `pio_cycles( )` reads `name: <instruction>` on one line as a label plus that instruction.

## Deviations

- None.
- Validation round 1 (2026-10-08), three `undecidable` findings without a settling criterion, recorded for the spec fix: (1) the spec does not say that `make test` picks up `tests/test_*.py` by glob, so "fails `make test`" rests on a runner the spec never names; (2) the Plan table's "fails because" column states a reason per rejection case, and the check counts any `FAIL` as a pass, so the stated reasons are not verified; (3) `verify.md`'s example line `byte 0: out 01 in FF 48 us ack 11 us` has no source in the spec's pointers.
- Round 2 (fixing validation round 1, 2026-10-08): spec amended, no code or `verify.md` change. (1) Context pointers gain `Makefile:35` (`PY_TESTS := $(wildcard tests/test_*.py)`), which founds the Goal's "fails `make test`". (2) The Plan's rejection table loses its "fails because" column; the check counts any `FAIL`, and the spec no longer states a per-case reason. (3) Context pointers gain `docs/phases/06-hil-digital/notes.md` §Bench readings for `verify.md`'s example line. Statements reconciled: the Goal's "fails `make test` when it differs" (now founded, unchanged); Plan step 2's "The rejection cases must each turn the check to `FAIL`" (consistent: that is what the code checks); Plan step 4's guide bullets (unchanged; the example is now sourced). A grep of `spec.md` for the deleted values (38, 45, 74, 18.5 µs) finds none left.
- Round 3 (fixing validation round 2, 2026-10-08). (1) Code-side: `pio_cycles( )` loops `MAX_STEPS + 1` times, and the last pass only checks for `.wrap`, so a walk of exactly 10 000 instructions passes and one of 10 001 fails, as Plan step 2 says. Checked at the boundary with `MAX_STEPS = 4`: 4 instructions pass, 5 fail. (2) Spec-side: Plan step 2's `check( texts )` bullet now says the label is printed on the real run only. The cases call `check( )` silently, and the spec's own `grep -c '^  ok:'` → 3 criterion requires it. Statements reconciled: the Cases bullet's "run `check( )` on mutated in-memory texts" (consistent); the three printed lines in the Cases bullet and the Acceptance criteria (consistent). (3) `verify.md`'s "set `SHIFT_NOMINAL_US` to the value the message gives" now adds that `tests/vectors/trace_session.rendered` must be rewritten by hand, or R-PROTO-08 fails. Found by the round-2 reviewer as `unstated`. Following the old text alone breaks `make test`, so it is fixed and not deferred. Property checked, generalising (3): every place `verify.md` tells the operator to change either shift constant must say that `trace_session.rendered` (9 `ack` rows) moves with it. Enumerated every mention of `SHIFT_NOMINAL_US`/`SHIFT_CALIBRATION_US` in `verify.md`:
  - L25–26 "adjusting it never breaks the build": false, because R-PROTO-08 fails. It now says the new test never checks it, and that the vector must be rewritten.
  - The troubleshooting `SHIFT_NOMINAL_US` entry: fixed, as above.
  - The loopback-capture entry: it now points to the same rewrite.
  - Step 3's temporary 38: it is reverted in the same step, so no rewrite is needed.
  - L51 "does not trip the test": true of R-PROTO-09, kept.
  - The spec's Goal "Changing it never fails the check" names R-PROTO-09's check: true, unchanged.
- Validation round 3 (2026-10-08), two `undecidable` findings without a settling criterion, recorded for the re-expansion: (1) rejection case 2's label "a delay changed in the bit loop" states where the mutation lands, and `str.replace` changes every `side 0 [1]`; the spec does not say where or how often the anchor occurs, and round 2 removed this kind of unchecked per-case reason; (2) `verify.md`'s "set `SHIFT_NOMINAL_US` to the value the message gives" cannot be followed when the computed nominal is not whole (the message prints e.g. `37/2 us`); the spec does not say what the operator does then.
- Round 4 (implementing the re-expanded spec, 2026-10-08): no deviation from the spec; the three owed edits its Plan names landed. Property checked, generalising round 3's (1): every mutation's anchor occurs exactly once in its file — all six (`set x, 7`, `side 0 [1]`, `kBusClockHz = 250000`, `kCyclesPerBit  = 4`, `SHIFT_NOMINAL_US = 37`, `SHIFT_CALIBRATION_US = 0`) counted with `grep -cF` → 1 each, and now asserted by `mutated( )`. Property checked, generalising round 3's (2): every place `verify.md` tells the operator to change and keep `SHIFT_NOMINAL_US` or `SHIFT_CALIBRATION_US` names the 9 `ack` rows of `trace_session.rendered` — the calibration bullet (row count added), the whole-number remedy (new), the loopback-capture entry (row count added); step 3's temporary 38 is reverted in-step and needs no rewrite.
- Round 5 (fixing validation round 4, 2026-10-08): spec only, no code or `verify.md` change. Plan step 4's open "Every place the guide tells the operator to change … and keep the change says …" is replaced by the closed list of the three places in `verify.md` (the `SHIFT_CALIBRATION_US` bullet under What was built, the whole-number remedy, the loopback-capture entry), with step 3's reverted 38 named as the exception. Statements reconciled: Plan step 4's whole-number bullet (does not restate the rewrite; unchanged); the Goal and Out of scope (do not mention the rewrite; unchanged); no other spec sentence quantifies over `verify.md`. The three places were checked in `verify.md` by grep for `9 \`ack\` rows`. Scanning the spec for every/each/all found one more open quantifier: Plan step 1's check "(output unchanged, all four R-PROTO-08 mutations still bite)", whose mutations the spec never names; deleted, leaving `python3 tests/test_bus_trace.py` → exit 0. The other hits close over named sets (the three constants, the rejection table, the anchors below it) or state a rule (`[n]` cost, the `tests/test_*.py` glob).
- Validation round 5 (2026-10-08), three `undecidable` findings without a settling criterion, recorded for the spec fix: (1) input outside Plan step 2's four error cases (the walk's `pc` leaving the program, a non-decimal `set x` operand) ends in a Python traceback, not a `FAIL` line, and the spec does not say whether that is acceptable; (2) the unknown-label case does not say whether it applies to `jmp`s the walk reaches or to every `jmp` in the program; (3) `DELAY` reads `[n]` only as the line's last token, and the spec does not say where the delay sits relative to `side`, so `… [n] side 0` would be silently undercounted.
- Round 6 (fixing validation round 5, 2026-10-08, operator's choice: code + spec). Code: `pio_cycles( )` raises `Unreadable` when the walk leaves the program, when a `[` is not a last-token delay, and when a needed operand is missing or `set x`'s is not decimal. Spec: Plan step 2's four error cases become a closed list of seven (a drafted "in no case does it crash or count silently" was deleted before validation: it quantified over all inputs and rested only on this fix), with the unknown-label check stated as on reached `jmp`s only (what the code does), and the `[n]` bullet says the delay is read only as the last token. Generalising the three findings into one property, "no input on the walked path ends in a traceback or a silent miscount": enumerated every point in `pio_cycles( )` that reads input — `program[pc]` (case 2), `DELAY`/`[` (case 3), `words[0..2]` and `int( words[2] )` (case 4, `words[0]` is always present because blank lines are dropped), the `jmp` condition (5), `labels[target]` (6), the step bound (7), the markers (1) — and probed cases 2–6 with malformed texts: each raises `Unreadable`, and the real program still gives 37. `constant( )` and the `SHIFT_NOMINAL_US` read already raise `Unreadable` on a missing line. Statements reconciled: the Plan's opening paragraph (now names the round-5 edit); Out of scope's "A new condition fails the check" (consistent with case 5); R-PROTO-09's Scope clause in `docs/constraints.md` ("only the instructions and `jmp` conditions `ps2_master.pio` uses": still true, unchanged); `verify.md`'s "`jmp` condition is not understood" entry (still true, unchanged).
- Round 7 (at the round-6 cap, 2026-10-08, operator's choice: fix finding (2) in this session; the cap's close/re-expand/re-cut choice is still open for findings (1), (3), (4)). Code: `LABEL` matches `name:` followed by an optional instruction, which is appended at the label's position. Spec: Plan step 2's "Ignored: … labels (`name:`)" becomes a bullet saying a label marks the next instruction's position and a same-line instruction is that instruction; one Acceptance criterion added (the real program with `bitloop:` and its `out` joined on one line still counts 37; it exits 1 on the previous code, where `bitloop` was left undefined). Statements reconciled: the seven error cases (case 6, unknown label, unchanged: a same-line label now defines its name); R-PROTO-09's Scope clause in `docs/constraints.md` and its list of understood constructs (checked: it does not describe label syntax, unchanged); `verify.md` (does not mention labels, unchanged). This closes the residue that kept the tech-debt entry "`SHIFT_US` is a hand copy of the PIO cycle budget" from being fully paid: a same-line label on a `set x`, `out y` or `jmp` miscounted silently.
- operator closed with open finding: §Plan tests/test_trace_shift.py — (1) a bare `out y` (no bit count) raises nothing; it costs 1 cycle either way, so the count is unaffected.
- operator closed with open finding: §Plan tests/test_trace_shift.py — (3) `int( )` accepts `+7`, `1_0`, `-1` as a `set x` operand; `+7` gives the same value, and whether pioasm accepts the other two was not checked (inferred to be rejected, `set` takes a 5-bit immediate).
- operator closed with open finding: §Plan tests/test_trace_shift.py — (4) a zero `kCyclesPerBit` or `kBusClockHz` ends in `ZeroDivisionError` instead of a `FAIL` line; the exit is non-zero, so `make test` still fails.

## Debt

- `pio_cycles( )` understands only the `jmp` conditions `ps2_master.pio` uses
  (unconditional, `x--`, `!y`). A program that adds another one on the no-ACK path fails the
  check with "jmp condition … is not understood". Upgrade: teach the new condition when a
  phase changes the program. This is recorded as R-PROTO-09's Scope clause, not a hidden
  ceiling.

## For later phases

- **Validation 2026-10-08 (round 6), reviewer taste (not blocking):** `check( )`'s docstring (again); `%r` quoting in `mutated( )` (again); module-level `failures` (again).
- **Validation 2026-10-08 (round 5), reviewer taste (not blocking):** `%r` quoting of the anchor in `mutated( )`'s FAIL line; module-level mutable `failures`; `read( )` without an explicit encoding; `LABEL.match( line )` evaluated twice per label line.
- **Validation 2026-10-08 (round 4, after the re-expansion), reviewer taste (not blocking):** `check( )`'s docstring says it prints the R-PROTO-09 line "with the reason when `is_verbose`", but it prints nothing unless `is_verbose`; `constant( )` has no `\b` before the name; R-PROTO-09's Scope clause has no "recorded <date>" stamp (again); `LABEL` does not recognise `public name:` or a label sharing a line with an instruction. Also a known ceiling the round-4 reviewer raised and the passing criterion settled for today's tree only: `DELAY` reads `[n]` at line end, so a delay written before `side` would be silently undercounted. Owner: the next phase that edits `ps2_master.pio`.
- **Validation 2026-10-08 (round 3), reviewer taste (not blocking):** R-PROTO-09's Scope heading style (again); `US_PER_SECOND` and `MAX_STEPS` as module constants.
- **Validation 2026-10-08 (round 2), reviewer taste (not blocking):** R-PROTO-09's Scope clause has no "recorded <date>" stamp, unlike R-PROTO-06/07/08; Deviations' "- None." precedes real entries.
- **Validation 2026-10-08 (round 1), reviewer taste (not blocking):** R-PROTO-09's Scope clause reads "**Scope — what the check reads:**" where its neighbours say "Scope, recorded <date> — …"; `%`-formatting where f-strings would do.
- **Tech-debt log**: the entry "`SHIFT_US` is a hand copy of the PIO cycle budget" in
  `.claude/rules/tech-debt.md` is paid by this phase. Removed after validation on the
  operator's approval (2026-10-08), with its frontmatter paths; the "Every mutant runs on
  every `make test`" entry was corrected in the same edit to describe `test_trace_shift.py`'s
  cases as in-memory.
- **09-guitar-observe**: if the guitar's decoded `ack` delays show a constant bias,
  `SHIFT_CALIBRATION_US` is where the correction goes. The check never reads it.

## Validation — 2026-10-08
- criteria: 14 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules; set: `tools/trace_decode.py`, `tests/test_trace_shift.py`, docs)
- index: stale (`tests/test_trace_shift.py`, `tools/trace_decode.py`), rebuilt
- independent review: undecidable: (1) whether `make test` runs `tests/test_trace_shift.py` — no runner named in the spec; (2) the rejection table's "fails because" reasons are not checked by the code, any `FAIL` counts; (3) `verify.md`'s `byte 0: out 01 in FF 48 us ack 11 us` example has no pointer. No settling criterion for any. (settled: none) (unstated: 7 — `check( )`'s `is_verbose` parameter; the "jmp to unknown label" error; `.wrap_target`/`.wrap` as position markers; per-case `FAIL:` lines; a missing anchor failing the accept case; `verify.md`'s "If something looks wrong" section; the PHASES.md status move)
- closure test: fail: three `undecidable` findings without a settling criterion = missing pointers. Notes has all four sections, none blank; every file in the set is named by the spec's Plan or Context pointers.
- findings: 3
- finding keys: §Goal tests/test_trace_shift.py; §Plan tests/test_trace_shift.py; §Plan docs/phases/26-trace-shift-derived/verify.md
- spec size: 10762 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec. Round 1, so no escape check applies. Fixes, deleting first, each with a Deviations entry listing what it reconciled: (1) add one Context pointer, `Makefile:35` — `PY_TESTS := $(wildcard tests/test_*.py)`, so `make test` runs the new file with no runner edit; (2) delete the Plan table's "fails because" column (the reasons are not checked; the values belong in notes, where they are already recorded under Outcome); (3) add one Context pointer, `docs/phases/06-hil-digital/notes.md` §Bench readings, the decoded line `verify.md` quotes. No code or `verify.md` change is owed.

## Validation — 2026-10-08 (round 2)
- criteria: 14 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: fresh (623186d → 15cf471 touched no source files)
- independent review: contradicts (spec-side): Plan step 2 says `check( texts )` prints the R-PROTO-09 line on every call; the code adds `is_verbose` and the cases call `check( work, False )` — evidence: the spec's own criterion `python3 tests/test_trace_shift.py | grep -c '^  ok:'  # expect: 3` is unsatisfiable if the accept case prints the label too, so the sentence is what is wrong; no Deviations entry recorded it. contradicts (code-side): Plan step 2 "raises … when the walk exceeds 10 000 instructions"; `pio_cycles( )` returns only when `pc == end` is checked before an instruction, so a walk that reaches `.wrap` after exactly 10 000 instructions is rejected — the spec states the intended bound and the hunk departs from it. (settled: 2 — `python3 tests/test_trace_shift.py                                # expect: exit 0` for the `constexpr std::uint32_t` declarations `constant( )` reads; `python3 tests/test_trace_shift.py | grep -c '^  ok:'             # expect: 3` for `pio_cycles( )` reaching 37 on the real program. Settled by string and exit code, the belay-debt case.) (unstated: 7 — the "jmp to unknown label" error; non-`Unreadable` errors ending in a traceback; per-case `FAIL:` lines; `str.replace` hitting every occurrence; R-PROTO-09's "does not read" sentence; `verify.md` step 3 and its troubleshooting section, including that changing `SHIFT_NOMINAL_US` also changes `tests/vectors/trace_session.rendered`; the PHASES.md status move)
- closure test: pass. Notes has all four sections, none blank; every file in the set is named by the spec; no unsettled `undecidable`.
- findings: 2
- finding keys: §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py
- spec size: 10865 (+103 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to implementation. Round 2, so no escape applies. `§Plan tests/test_trace_shift.py` repeats round 1's key; if it comes back in round 3, the phase goes back to `/expand-phase`. Code first: in `pio_cycles( )`, check for `.wrap` once more after the last allowed instruction (`range( MAX_STEPS + 1 )`), so exactly 10 000 instructions pass and the 10 001st fails. Then the spec, deleting first: cut Plan step 2's `check( texts )` sentence so the label is printed by the real run only (the cases call it silently), with a Deviations entry listing what it reconciled.

## Validation — 2026-10-08 (round 3)
- criteria: 14 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: stale (`tests/test_trace_shift.py`), rebuilt
- independent review: undecidable: (1) rejection case 2's label "in the bit loop" is an unchecked claim about where `side 0 [1]` occurs, and `str.replace` hits every occurrence — missing: the anchor's occurrences in `ps2_master.pio`; (2) `verify.md`'s "set `SHIFT_NOMINAL_US` to the value the message gives" cannot be followed for a non-whole nominal — missing: what the operator does then. No settling criterion for either. (settled: none) (unstated: 6 — "jmp to unknown label"; `.wrap_target`/`.wrap` as markers and other directives counted as instructions; `check( )`'s `is_verbose`; missing-anchor cases; `verify.md`'s third heading and the vector-rewrite advice; the PHASES.md status move)
- closure test: fail: two `undecidable` findings without a settling criterion = missing pointers. Notes has all four sections, none blank; every file in the set is named by the spec.
- findings: 2
- finding keys: §Plan tests/test_trace_shift.py; §Plan docs/phases/26-trace-shift-derived/verify.md
- spec size: 10894 (+29 since the previous validation)
- upstream: none
- not-ours: none
- verdict: escaped to /expand-phase: §Plan tests/test_trace_shift.py (round 3, recurring from rounds 1 and 2; `§Plan docs/phases/26-trace-shift-derived/verify.md` also recurs from round 1). Status set to `pending`. This is the phase's first escape. Every criterion, gate and the sweep passed in all three rounds, and no `contradicts` is open. For the re-expansion: (a) the rejection table names each mutation by its anchor only, and the labels in the code carry no claim about where it lands (or the spec gives the anchor's occurrence count in `ps2_master.pio` and the check asserts it); (b) `verify.md`'s remedy for a nominal mismatch covers the non-whole case, or is cut to "the PIO program or bus clock changed; the shift time has to be re-derived", and the spec states the remedy; (c) fold the round-2 and round-3 Deviations (the `check( )` printing, the `MAX_STEPS` bound, the vector rewrite on either constant) into the Plan.

## Validation — 2026-10-08 (round 4, first after the re-expansion)
- criteria: 17 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- gates tree: 56e22bb68b62d8475e13cebfa33d6babc7c1979f
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: stale (`tests/test_trace_shift.py`), rebuilt
- independent review: clean (settled: 1 — `python3 tests/test_trace_shift.py                                # expect: exit 0` for `DELAY` reading `[n]` only at line end, today's program; settled by string and exit code, the belay-debt case) (unstated: 6 — per-case `FAIL: rejection case` line; unnamed helpers `read( )`, `Unreadable`, `run_cases( )`, `ROOT`, `FILES`; malformed input ending in a traceback rather than `FAIL:`; `%r` quoting of the anchor; R-PROTO-09 Scope's "does not read the comment / hardware" sentence; `verify.md`'s troubleshooting section and step 3 exercise)
- closure test: fail: Plan step 4's "Every place the guide tells the operator to change `SHIFT_NOMINAL_US` or `SHIFT_CALIBRATION_US` and keep the change says …" quantifies over `verify.md`'s sentences and names no members (written by the re-expansion). Notes has all four sections, none blank; every file in the set is named by the spec.
- findings: 1
- finding keys: §Plan docs/phases/26-trace-shift-derived/verify.md
- spec size: 13375 (+2481 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec. Round 1 after the escape, so no escape check applies. Fix, deleting first: replace the open sentence in Plan step 4 with the closed list of the three keep-a-change places in `verify.md` (the `SHIFT_CALIBRATION_US` bullet under What was built, the whole-number remedy, the loopback-capture entry) plus step 3's reverted 38 as the one exception, with a Deviations entry naming what it reconciled. No code or `verify.md` change is owed.

## Validation — 2026-10-08 (round 5)
- criteria: 17 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- gates tree: 22788f7b4ccefa64bc33dbb1a7122b0bd973b2f6
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: fresh (d550ddc)
- independent review: undecidable: (1) inputs outside the four error cases end in a traceback, not `FAIL` — missing: what the walk does when `pc` leaves the program, and what a valid `<n>` is; (2) unknown-label error on reached `jmp`s only or on every `jmp` — missing: which; (3) `[n]` read only at line end, `… [n] side 0` silently undercounted — missing: the token order the walk accepts, or that another order is an error. No settling criterion for any. (settled: none) (unstated: 5 — per-case `FAIL: rejection case` line; R-PROTO-09's "Added 2026-10-08" and "does not read the comment" sentences; `verify.md`'s troubleshooting section and step 3; the `jmp` condition troubleshooting entry; x/y start at 0 and `x--` goes negative)
- closure test: fail: three `undecidable` findings without a settling criterion = missing pointers. Notes has all four sections, none blank; every file in the set is named by the spec; no unenumerated quantified claim left (round 5's scan).
- findings: 3
- finding keys: §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py
- spec size: 13348 (-27 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec. Round 2 since the escape, so no escape check applies; round 4's key (§Plan verify.md) does not recur. If `§Plan tests/test_trace_shift.py` comes back in round 6, this is the round cap after an earlier escape: operator's choice (close / re-expand / re-cut), no automatic route.

## Validation — 2026-10-08 (round 6)
- criteria: 17 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- gates tree: 1ab6f49e3a10018a5b12da8089a7bdd4bb1b96b8
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: stale (`tests/test_trace_shift.py`), rebuilt
- independent review: undecidable: (1) a bare `out y` raises nothing — missing: whether case 4's needed operand includes the bit count; (2) `name: <instruction>` on one line is read as mnemonic `name:` and the label is never defined — missing: whether that form is in scope or fails; (3) `int( )` accepts `+7`, `1_0`, `-1` — missing: what "decimal number" covers; (4) a zero `kCyclesPerBit` or `kBusClockHz` ends in `ZeroDivisionError`, not `FAIL` — missing: what a zero divisor does. No settling criterion for any. (settled: none) (unstated: 6 — per-case `FAIL: rejection case` line; R-PROTO-09 Scope's extra sentences; `verify.md`'s step 3 and troubleshooting; helpers and module constants; unknown label checked before unknown condition; `startswith` skip of `.program`/`.side_set`)
- closure test: fail: four `undecidable` findings without a settling criterion = missing pointers. Notes has all four sections, none blank; every file in the set is named by the spec.
- findings: 4
- finding keys: §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py
- spec size: 13933 (+585 since the previous validation)
- upstream: `commands/validate-phase.md` — the cap fired on a recurrence after an earlier escape, each round's reviewer sampling new malformed inputs from an open set; /belay-feedback recommended with rounds 4–6's `- independent review:` lines
- not-ours: none
- verdict: at round cap: awaiting operator. Round 3 since the escape; `§Plan tests/test_trace_shift.py` recurs from round 5, and notes already holds an escape, so no automatic route. Steps 1–4 passed and no code-side `contradicts` is open, so all three options stand: (a) close with the 4 open findings, (b) re-expand, (c) re-cut.

## Validation — 2026-10-08 (round 7, operator at the round cap)
- criteria: 18 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- gates tree: 0b96f22eaa108958bcb74b9a0fc2465c4a668359
- boundary sweep: not swept: no file in the set is under a declared layer (13 deny rules)
- index: stale (`tests/test_trace_shift.py`), rebuilt
- independent review: skipped: operator chose at the round-6 cap to fix finding (2) and close with (1), (3), (4) open; no new reviewer was dispatched over the round-7 change (same-line labels, one new criterion). Stated, not silent: this round's closure is self-declared. (settled: none) (unstated: none)
- closure test: pass with 3 operator-closed findings recorded in Deviations. Notes has all four sections, none blank; every file in the set is named by the spec.
- findings: 3 (operator-closed)
- finding keys: §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py; §Plan tests/test_trace_shift.py
- spec size: 14298 (+365 since the previous validation)
- upstream: `commands/validate-phase.md` — the cap fired on a recurrence after an earlier escape; /belay-feedback recommended with rounds 4–6's `- independent review:` lines (carried from round 6)
- not-ours: none
- verdict: done (operator-closed at round cap: 3 open)
