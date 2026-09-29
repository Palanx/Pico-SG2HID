# Phase 02-wiring — notes

## Outcome

Plan steps 1–7 landed. Step 7, the operator gate, closed on 2026-09-28: all 18 bench items
pass (readings below), and `requirements.md` §Open questions is rewritten.

- base: main — the phase's work is committed on `feat/02-wiring` (operator, 2026-09-28).
- not-ours: .claude/rules/tech-debt.md — the operator's debt log entry "No typecheck gate
  configured", found in validation round 1 and deferred to a `/plan-feature` after 02-wiring
  (operator confirmed, 2026-09-29).

- `docs/adr/0013-power-only-current-draw-measurement.md`: accepted. It allows the power-only
  current-draw measurement in this phase (3V3 and GND only, signal resistors out, Pico in
  BOOTSEL). ADR-0006's Status line records the amendment.
- `docs/constraints.md`:
  - R-SAFETY-02 is cut to its table clauses plus the wiring-doc match.
  - R-SAFETY-09 is new: no pin-configuring SDK call outside `src/hal/`, with the widened list.
  - R-SAFETY-10 is new: `src/hal/` iterates the table, `planned: 03-pio-bus`.
  - R-SAFETY-08 names the signal lines and ADR-0013.
  - R-SAFETY-01..03 are bound to `tests/test_pin_table.py` and R-SAFETY-09 to
    `tests/test_repo_shape.sh`, each with a Scope clause.
- `CLAUDE.md` §Hardware safety names R-SAFETY-09/10 and the ADR-0013 exception.
- `src/core/pins.h`: `ps2::Signal`, `Direction`, `DriveMode`, `PinAssignment` and the plain C
  array `kMasterPins` (GP2 DATA, GP3 CMD, GP4 ATT, GP5 CLK, GP6 ACK).
- `docs/wiring.md`: the pin table, parts, breadboard layout, one paragraph per resistor, how
  to insulate pin 3, and sources by URL.
- `tests/pin_table_cases.cpp`:
  - one check function per rule, and the aggregate `report_rules( )` writing to a `FILE*`;
  - the real run prints `pin:` lines and three rule lines;
  - 11 in-process rejection cases and 3 wiring cases, all through the aggregate with its
    output captured via `tmpfile( )`.
- `tests/test_pin_table.py`: the driver. It compiles the cases file with `make test`'s flags,
  compares the `pin:` lines with `docs/wiring.md` as `R-SAFETY-02 (wiring doc)`, and runs 4
  copied-tree mutations, each asserting that its anchor matched.
- `tests/test_repo_shape.sh`:
  - `find_safety09( )`, as a one-line finder, and its `run_all` line;
  - 18 rejects: one per alternative, three starred forms, a space before `(`, and one each
    in `src/emu/`, `src/usb/` and `src/core/`;
  - 3 accepts (a hal call, `gpio_put`, `gpio_get`), with the floor raised to 28;
  - wiring cases 11/11.
- `docs/phases/02-wiring/verify.md`: the laptop check, a multimeter primer and the three-part
  checklist, with a reading column per item.

Acceptance status on 2026-09-25:
- `make test` exits 0 with `OK`, including `accounting: test_pin_table.py 3 rule(s), 2 live
  label(s)`, `neutered: 36/36` and `alternations: 87/87`.
- `make lint` exits 0.
- Every grep criterion matches its expected count, except the three over
  `docs/product/requirements.md`, which wait on step 7. Those three matched on 2026-09-28
  (2, 1, 1).

Acceptance status on 2026-09-28, after the round-3 fix: all 19 criteria pass. `make test` and
`make lint` exit 0.

## Bench readings (step 7, items 1–18)

Reported by the operator in chat on 2026-09-28. All items pass, recorded as read.

Board: a Pico-compatible RP2040 clone with USB-C and one button (marked "V1590"; model not
identified). Its edge pinout matched `docs/wiring.md` in every reading below. The meter's
probe-to-probe baseline is 0.5 Ω.

| # | Reading | Result |
|---|---|---|
| 1–5 | 325–360 Ω across the five series resistors; item 5 re-measured after the ACK fix below | pass (one resistor read ~360 Ω, above the 310–350 Ω band; operator judged it acceptable) |
| 6 | 0.5 Ω | pass |
| 7 | 0.5 Ω after replacing a faulty jumper (it read ~5 Ω and drifted) | pass |
| 8 | ~9 kΩ, stable | pass |
| 9–10 | `OL` every time | pass |
| 11 | Pico 39 (VSYS) ~50 kΩ, Pico 40 (VBUS) ~25 kΩ, no beep | pass |
| 12 | 1 ↔ 9 (DATA ↔ ACK) ≈ 20.7 kΩ; the other nine pairs `OL`. Extra, outside the item: socket 1 ↔ socket 4 (GND) and socket 1 ↔ socket 5 (3V3) gave drifting values, the capacitor-charging pattern, not a short | pass |
| 13 | 3.3 V (at Pico pin 36) | pass |
| 14 | 0 V | pass |
| 15 | 2.7 V | pass |
| 16 | ~2.7 V after the ACK fix below | pass |
| 17 | 851.9 µA at power-up, settling to 360–500 µA (µA range; Pico in BOOTSEL, no bus traffic) | pass |
| 18 | Drifts down with up-and-down steps (790, 840, 680, 780, 600, 640, 500 µA …), then wanders in a 360–500 µA band | pass |

Also measured while diagnosing, USB plugged in: VSYS 4.9 V, VBUS 5.2 V. Both 10 kΩ
pull-ups measured 10.0 kΩ and 9.7 kΩ in circuit (Pico pin ↔ 3V3 rail, unpowered).

**ACK was first wired to Pico pin 8 (GND) instead of pin 9 (GP6).** Every unpowered reading
passed anyway. Item 16 read ~6 mV and exposed it. It was moved to pin 9 and re-measured.
Nothing was damaged: 3V3 → 10 kΩ → GND is 0.33 mA.

Item 17 first read `0.0` on the 200 mA range: the draw is below that range's resolution. It
was read on the µA range, same jack. The connector is a PS2 female socket with all nine pins
broken out (C4).

**`verify.md` expectations the bench contradicted.** Applied to `verify.md` on 2026-09-28 with
the operator's approval (item 12a is the new check; no item was renumbered):
- Item 8: the reading can settle at a stable few kΩ, not only climb toward `OL`.
- Item 11: a real Pico reads tens of kΩ from 3V3 to VSYS and VBUS through its own circuitry.
  The pass criterion is "no beep", not "`OL`".
- Items 15–16: expect ~2.7 V, not ~3.3 V. The RP2040 pads reset with the pull-down enabled
  and the pull-up disabled (`PADS_BANK0_GPIO2_PDE_RESET = 0x1`, `PUE_RESET = 0x0` in
  pico-sdk `pads_bank0.h`), and BOOTSEL leaves them that way. With the external 10 kΩ this
  forms a divider to ~2.7 V.
- Missing item: each signal ↔ GND must not beep. Without it, a jumper one row off onto a GND
  pin passes all of C1, as ACK did.
- Item 17: the SG draws under 1 mA idle, not "tens of mA", so `0.0` on the 200 mA range is
  expected. Step 6 now says to drop to the µA range, and step 8 says to turn the meter off
  only after the USB is out.

## Deviations

- **Step 1 stopped half-way at first.** Claude Code's auto-mode classifier denied the edit
  to the R-SAFETY lines in `docs/constraints.md` as a "security weaken" action. The spec,
  and the operator decisions it records, order exactly that rewrite. The session did not
  work around the denial.
  - Resolved 2026-09-25: the operator authorized one retry in chat, with no settings change,
    and it landed.
  - R-SAFETY-10's wording is the spec's only. The first attempt added a clause the operator
    never decided ("no pin number written into a configuration call directly"); it was
    dropped.
- **`pins.h` rows are not one line per entry.** The spec asks for a plain C array with
  designated initializers so that `AlignArrayOfStructures` aligns the columns. A designated
  row is longer than 100 columns, so clang-format (enforced by the post-edit hook) wraps each
  entry over four lines, with the `=` signs aligned.
  - It is still a plain C array and still readable.
  - Checked: no other spec statement asserts one-line rows.
  - The copied-tree mutation anchors in `tests/test_pin_table.py` follow this wrapped layout.
- **The PS2 pinout source does not state its viewing side.** The spec asks for it. The cited
  source (Curious Inventor) does not state it, so `docs/wiring.md` says so. It relies instead
  on the breakout socket's printed numbering plus continuity, per the operator decision in the
  spec's table.
- **A Context pointer is wrong.** The spec says `00-scaffold/notes.md` §For later phases
  explains "why scratch copies use `cp -a`". It does not. That rationale is in
  `01-ps2-codec/notes.md` (around line 2292), and it concerns manual mutation copies that
  need `.git`. The driver needs no `.git`, so it copies only `src/`, `tests/` and `docs/`
  with `shutil.copytree`, the same as `tests/test_ps2_codec.py`.
- **`verify.md` is titled "how to verify it on the bench", not "how to check this
  yourself".** A title containing "check it" would make the heading grep count 3 instead of
  2.

- **Validation round 1 (2026-09-28) returned the phase to the spec.** Amended in `spec.md`:
  - Step 6 Powered: DATA and ACK read ≈ 2.7 V, not ≈ 3.3 V (spec-side `contradicts`; evidence:
    the bench readings above and the pad reset values they cite).
  - Step 3 and the Context pointer: the "viewing side stated" demand is deleted (spec-side
    `contradicts`; evidence: the viewing-side Deviation above, and the Connector decision in
    the spec's own table).
  - Step 4 and the `test_ps2_codec.py` Context pointer: "with `make test`'s flags" is deleted
    (`undecidable`). The driver's list was checked equal to the Makefile's `CXXFLAGS`
    (`-std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc`) on 2026-09-28.
  - Step 6 Current draw: "on its mA range" is deleted (`undecidable`; the reading needed the µA
    range).
  - Step 6 Unpowered: "no signal line reads ≈ 330 Ω or less to GND" is added. It is item 12a,
    which `verify.md` gained with the bench corrections and the spec never covered.
  - Reconciliation checked: the Goal, the decisions table's Connector row, the primer bullets
    and the step 7 requirements lines. None asserts the changed facts.

- **Validation round 2 (2026-09-28) returned the phase to the spec.** Three `undecidable`
  findings, amended in `spec.md`:
  - Goal: the power item's re-ownership now points to step 7's OPEN line, names the expand
    commit (`545fe21`) as where this row's PHASES.md exit criterion was narrowed, and says the
    `09-guitar-observe` row is not edited.
  - The `Makefile` Context pointer: `tests/test_*.py` run by the `PY_TESTS` wildcard (checked
    in the Makefile on 2026-09-28), so no Makefile hunk is owed.
  - Step 3: "one paragraph per resistor" becomes one per role, and the three roles are named.
  - Reconciliation checked: step 7's requirements lines and the Out of scope bullet on
    `09-guitar-observe` (consistent). Step 4's `make test` claims needed no change.

- **Validation round 3 (2026-09-28): one code-side and one spec-side finding.**
  - Code-side `contradicts`, filed by the reviewer as `undecidable`. Spec step 4 says the
    wiring cases "require it to fail and name the rule". `tests/pin_table_cases.cpp:309` tests
    `std::strstr( capture.text, rejection.rule )`, but `report_rules` always prints all three
    rule lines, so that half can never be false. Evidence that the code is the wrong side: its
    own FAIL message ("did not fail naming %s"), and `says_fail` doing exactly that for the
    rejection cases at line 282. Owed to `/implement-phase`: use `says_fail`.
  - Spec-side: step 1 named only the 09/10 update to `CLAUDE.md`. The R-SAFETY-08 line naming
    the ADR-0013 exception is recorded in Outcome above. Step 1 now names it. Reconciliation
    checked: the Context pointer on `CLAUDE.md` §Hardware safety (consistent, no change).
  - Fixed 2026-09-28 by `/implement-phase`: `wiring_cases( )` now tests
    `!capture.is_ok && says_fail( capture, rejection.rule )`. Mutation check: relabelling the
    "DATA push-pull" wiring case to R-SAFETY-03 in a scratch copy now reports `wiring cases:
    2/3`. Before the fix it passed.
  - Generalised. The property: every "a named rule failed" assertion reads that rule's own
    line for `FAIL:`, not only its presence in the output. Checked:
    - `rejection_cases( )`: `says_fail`, holds.
    - `wiring_cases( )`: was the instance, fixed.
    - `tests/test_pin_table.py` `reject( )`: `is_ok` plus `line_says_fail`, holds.
    - `tests/test_repo_shape.sh` R-SAFETY-09 `reject` / `accept`: these call the finder
      directly, with no aggregate output to misread. The property does not apply.

- **Validation round 4 (2026-09-29) returned the phase to the spec.** One `undecidable`
  finding: the `00-scaffold/notes.md` Context pointer's "why scratch copies use `cp -a`"
  clause, already recorded above as false. The clause is deleted; nothing is added.
  Reconciliation checked: the `test_ps2_codec.py` Context pointer and step 4's copied-tree
  bullets say nothing about how the copy is made, and no other spec statement mentions
  `cp -a`.

## Debt

- **R-SAFETY-09 is a grep, not a parse.** The ceiling is measured and recorded in its
  §Scope clause. Not reported: a call whose `(` is on the next line, a macro, a function
  pointer, direct register writes (`sio_hw`, `iobank0_hw`, `padsbank0_hw`), `.pio` files and
  `.c` files. Falsely reported: calls inside `/* … */` or a string.
  - Upgrade path: the `clang-query` upgrade already owned by `03-pio-bus` (the file's
    existing `belay-debt:` header), plus adding `.c` and `.pio` to what the check scans once
    those files exist.
- **The copied-tree mutation anchors are pinned to clang-format's current layout of
  `pins.h`.** A reformat moves them. The per-mutation anchor assert turns that into a loud
  FAIL rather than a false pass, so the ceiling is maintenance, not correctness.
- **`tests/test_pin_table.py` is a `.py` check held to the accounting property only.** This
  is the released harness debt named in the spec's §Out of scope. The 4 copied-tree cases
  stand in for mutating its internals.

## For later phases

- **`03-pio-bus`**:
  - Check R-SAFETY-09's function names against the installed SDK headers.
  - `src/hal/` is where R-SAFETY-10's check lands. `find_safety09( )` already excludes
    `src/hal/` by path prefix.
  - If that phase adds `.c` or `.pio` files under `src/`, R-SAFETY-09 does not see them until
    `src_files( )` gains those extensions. A `.pio` `set pindirs` is the realistic case.
- **`03-pio-bus`**: the GPIOs are consecutive, GP2..GP6 in the order DATA, CMD, ATT, CLK, ACK.
  The output group (CMD, ATT, CLK) is GP3..GP5, and the inputs sit at either end.
- **`05-emulator`**: its pin table is a second `PinAssignment` array. R-SAFETY-06 compares it
  against `kMasterPins`, and `pin_table_cases.cpp`'s check functions take any
  `std::span<const PinAssignment>`, so they can be reused for it.
- **`09-guitar-observe`**: step 7 moves "does the SG answer the bus correctly on 3V3 alone?"
  to that phase as an OPEN line in `requirements.md`.
- **`09-guitar-observe`**: the 0.36–0.50 mA idle figure is without bus traffic. Under
  polling the draw rises, but the margin to the ~250 mA budget is several hundred times.

- Taste from the round-1 review, non-blocking: the ANSWERED current line leads with the 0.85 mA
  peak, and the idle band may be the better headline for a power budget; the 02-wiring row
  in PHASES.md was edited in place by the expand step rather than superseded.
- Taste from the round-4 review, non-blocking:
  - `tests/test_pin_table.py` hardcodes a copy of the Makefile's `CXXFLAGS`, which can drift
    silently.
  - The R-SAFETY-09 false-positive floor was raised to 28. The diff alone does not show that
    this equals the accepted count.
  - `verify.md` item 17's expected column carries the measured "under 1 mA".
  - `find_safety09( )` adds a leading `\b` that the spec's matching description omits.

## Validation — 2026-09-28
- criteria: 19 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap
- boundary sweep: clean
- independent review: contradicts (spec-side): verify.md items 15–16 read ≈ 2.7 V against spec step 6's ≈ 3.3 V — bench readings in notes.md; contradicts (spec-side): wiring.md does not state the viewing side, spec step 3 asks for it — notes.md viewing-side Deviation; undecidable: step 4 does not say whether copied flags equal the Makefile's; undecidable: step 6 fixes the mA range, the procedure goes to µA
- closure test: fail: the four review findings, plus verify.md item 12a absent from the spec
- findings: 5
- spec size: 18503 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-09-28
- criteria: 19 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap
- boundary sweep: clean
- independent review: undecidable: the PHASES.md exit criterion edited in place, with no spec pointer; undecidable: "one paragraph per resistor" (per part or per role); undecidable: how `make test` finds `tests/test_pin_table.py` with no Makefile hunk
- closure test: fail: the three undecidable findings
- findings: 3
- spec size:    18403 (-100 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-09-28
- criteria: 19 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap
- boundary sweep: clean
- independent review: contradicts (code-side): the wiring cases' "name the rule" check is always true (tests/pin_table_cases.cpp:309) — the case's own FAIL message and `says_fail` at line 282; undecidable: CLAUDE.md's R-SAFETY-08 ADR-0013 sentence is not named in step 1
- closure test: fail: the step 1 pointer
- findings: 2
- spec size: 18787 (+384 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to implementation (escape not taken: converging 5 → 3 → 2)

## Validation — 2026-09-29
- criteria: 19 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`; logged debt, deferred by the operator)
- boundary sweep: clean
- independent review: undecidable: the `00-scaffold/notes.md` Context pointer (spec line 95) claims to explain "why scratch copies use `cp -a`"; the spec never states what property a scratch copy must preserve, so the reviewer cannot judge `shutil.copytree` in `tests/test_pin_table.py` `reject( )`. The Deviations entry "A Context pointer is wrong" already records that the claim is false; it was never removed from the spec.
- closure test: fail: the undecidable `cp -a` pointer
- findings: 1
- spec size: 18849 (+62 since the previous validation)
- upstream: none
- not-ours: .claude/rules/tech-debt.md subtracted
- verdict: returned to spec (escape not taken: converging 5 → 3 → 2 → 1)

## Validation — 2026-09-29
- criteria: 19 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`; logged debt, deferred by the operator)
- boundary sweep: clean
- independent review: clean (taste only: `\b` in `find_safety09( )` is not POSIX ERE; `verify.md` item 17 carries the measured value as its expectation; the wiring cases reuse the `Rejection` struct name; item 11 wording "no beep" vs the spec's "no continuity")
- closure test: pass
- findings: 0
- spec size: 18814 (-35 since the previous validation)
- upstream: none
- not-ours: .claude/rules/tech-debt.md subtracted
- verdict: done
