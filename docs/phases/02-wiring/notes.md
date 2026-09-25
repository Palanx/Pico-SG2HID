# Phase 02-wiring — notes

## Outcome

Plan steps 1–6 landed. Step 7, the operator gate, is owed: the operator runs the bench
checklist and reports the readings.

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
  `docs/product/requirements.md`, which wait on step 7.

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
- **Step 7 (still owed)**: once the operator reports the readings, rewrite `requirements.md`
  §Open questions as the spec's step 7 says, and copy the full readings here.
