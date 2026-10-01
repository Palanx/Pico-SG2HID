# Phase 24-pio-bus — notes

## Outcome

- base: 946bb0f (the expansion commit; this phase's work is committed on top of it in 9e777ec)

- **`src/core/`**: `ps2_protocol.h` gains `kWireBitsPerByte` (8), `kBusClockHz` (250 kHz) and
  `kAckTimeoutUs` (100, `belay-debt:`). `pins.h` gains `consteval gpio_of( Signal )`, and its
  comment naming `03-pio-bus` now names `src/hal/ps2_master.pio`.
- **`src/hal/`** (new layer):
  - `bus_port.h` is the seam: `bus_init`, `att_assert`, `att_release`, `exchange_byte`,
    `probe_wire_bits`. It includes no SDK header.
  - `bus_frame.{h,cpp}` holds `exchange_frame( std::span<std::uint8_t> )`. It has one exit and
    one `att_release`, stops at the first `std::nullopt` and returns the completed count. No
    SDK header.
  - `ps2_master.pio`: one program with 9 instructions and its cycle budget as a comment. CLK
    is an optional side-set, there are 4 cycles per bit, and bit 8 of the TX word is the
    ACK-wait flag. The ACK wait is a `jmp pin` loop. It has no `% c-sdk` block and no
    `pindirs` instruction.
  - `pio_port.cpp` sets up the pins in one `for ( const auto& pin : kMasterPins )` loop.
    Inputs (by `DriveMode::OpenDrainInputOnly`) get `gpio_init` and a pull-up. `ATT` is
    written high before it becomes an output. `CMD` and `CLK` are set high through the SM,
    then `pio_sm_set_consecutive_pindirs`, then `pio_gpio_init`. SM config uses `gpio_of`.
    Divider is `clk_sys / ( 4 * kBusClockHz )`. `exchange_byte` puts one word and waits for RX
    for at most 32 µs + `kAckTimeoutUs`. On timeout it disables, clears, restarts, jumps to
    offset and re-enables. `probe_wire_bits` switches to divider 65535, samples the `CMD` pad
    on each rising `CLK` edge from the CPU, drains the RX word or recovers, restores the
    divider, and gives up after 1 s.
- **`src/app/main.cpp`** is now the loopback program: three hand-written sequences and a
  probe, printed once a second in the formats the spec's bench criteria parse.
- **`CMakeLists.txt`**: `pico_generate_pio_header` and `hardware_pio`.
- **Tests**:
  - `tests/bus_frame_cases.cpp` + `tests/test_bus_frame.py` bind R-SAFETY-07 and R-PROTO-06.
    The fake port covers lengths 1/2/5/9 × every failure position, and 5 copied-tree
    mutations each flip their own rule.
  - `tests/test_repo_shape.sh`: `src_files( )` lists `.pio`, and `find_safety09( )` also
    matches `(set|out|mov) pindirs`. New `find_safety10( )` excludes on `pin\.gpio`. It gets
    18 reject cases, 2 accept cases (floor 28 → 30) and 1 wiring case (11 → 12).
    New `find_proto07( )` (R-PROTO-07, SPI mode 3 on the `.pio` side-sets): 4 rejects, 4
    accepts (floor → 34), 1 wiring case (→ 13). See the R-PROTO-07 Deviation.
  - `tests/pin_table_cases.cpp` asserts `gpio_of` at compile time.
- **`docs/constraints.md`**:
  - Bindings: R-SAFETY-07 and R-PROTO-06 → `test: tests/test_bus_frame.py`, R-SAFETY-10 →
    `test: tests/test_repo_shape.sh`, each with a dated Scope clause. R-PROTO-01 → `manual:`
    (the probe), narrowed to LSB-first. New R-PROTO-07 (mode 3) → `test:
    tests/test_repo_shape.sh`.
  - R-PROTO-01's text no longer says the loopback can observe bit order, and says why it
    cannot. R-SAFETY-10's "does not exist before `03-pio-bus`" sentence is corrected.
  - R-SAFETY-09's Scope names `.pio` and the `pindirs` forms. R-CLEAN-03/05 File scope list
    `.pio`.
- **`docs/phases/24-pio-bus/verify.md`**: host checks, the bench procedure and the reasoning
  behind the probe.

Acceptance, host part: every command passes. `make test` → `OK` (`alternations: 109/109`,
`neutered: 39/39`); the no-SDK, bash-5-only `make test` → `OK`; `make firmware` produces the
UF2; `make lint`, `make typecheck`, `test_firmware_flags.sh`, `test_rule_traceability.py`,
`test_boundaries.sh` and `test_phase_docs.sh` → rc 0; every grep criterion returns its
expected count. Lint reach into `src/hal/pio_port.cpp` was confirmed by planting a `24`
literal: `make lint` reported it as `readability-magic-numbers`. The file was restored after.
**Bench part: not run.** It needs the operator, the flashed Pico and the jumpers (`verify.md`
§2).

## Deviations

- **`gpio_of` check in `tests/pin_table_cases.cpp`.** The spec asked for a `static_assert`
  on each of the five rows. The first version indexed `kMasterPins[ 0..4 ]`, and that broke
  `tests/test_pin_table.py`'s existing copied-tree case "ACK entry deleted". With a row
  deleted, index 4 no longer compiles, so the case saw a build failure, not its
  R-SAFETY-02 line. It was replaced with one `static_assert` over a `consteval` lambda that
  loops over the rows. Each row that exists is still checked. A deleted row now reaches
  R-SAFETY-02 at run time, which is where that rule is reported. Only this statement
  changes; nothing else in the spec says the same thing.
- **New constant `kWireBitsPerByte`** in `src/core/ps2_protocol.h`, not named by the spec.
  `bus_port.h`'s `std::array<std::uint8_t, 8>` is a magic number under R-CLEAN-04 (`make
  lint` reported it). `kBitsPerByte` already exists in `src/core/hid_report.h` in the same
  namespace, so reusing that name was a redefinition. `hid_report.h` was read to find this,
  and the spec's Context pointers do not name it. `bus_port.h` therefore includes
  `core/ps2_protocol.h` (hal → core, allowed).
- **R-SAFETY-10 cases.** The spec asked for one reject per alternative plus one accept. On
  top of that, the rejects mirror all of R-SAFETY-09's starred and whitespace forms
  (`gpio_init_mask`, `gpio_set_dir_out_masked`, `…_mask64`, `gpio_init ( 2 )`), 18 in total.
  A second accept (`gpio_init( 2 );` in `src/app/x.cpp`) proves that the `src/hal/` path
  filter does not widen. So the accept floor is 30, not 29.
- **R-PROTO-01's `manual:` text** says what the probe observes: bit order, read off the
  wire. It also says what it does not observe: the sampling edge. Mode 3 is taken from the
  PIO program, where `in` sits on the `side 1` instruction. The spec binds the whole rule
  to the probe. The text now says that half of the rule is read from the program, not
  measured.
- **`std::unreachable( )`** is written `std::unreachable()`: `.clang-format` has no space in
  empty parentheses, and R-STYLE-01 enforces that. Formatting only.

- **Validation 2026-09-30, spec findings — amended in `spec.md` the same day.**
  - Missing pointer: the Deviation above on `kWireBitsPerByte` needed `src/core/hid_report.h`
    (`kBitsPerByte` in the same namespace), and the Context pointers do not name it.
  - Reviewer, undecidable: Plan step 2 says `test_bus_frame.py` compiles "using `make test`'s
    flags" but never says where they come from. They are a hard-coded copy of the Makefile's
    `CXXFLAGS`, and today they match it.
  - Reviewer, undecidable: Plan step 3's "Every counted floor rises by the number of cases
    added" does not list the floors. The actual floors are accept 28 → 30 and wiring
    11 → 12; the reject section has no floor.
  - Reviewer, undecidable: `probe_wire_bits( )` calls `recover( )` on timeout. The spec gives
    its failure path only "`std::nullopt`, restores the divider".
  - Reviewer, contradicts (spec-side): Plan step 4 orders "pins … Then it loads the
    program". `pio_port.cpp` claims the SM and adds the program before the pin loop,
    because `pio_sm_set_pins_with_mask` and `pio_sm_set_consecutive_pindirs` take a
    claimed SM.
  - Reviewer, contradicts (spec-side): Plan step 1 says ps2_protocol.h gains "two"
    constants. The third, `kWireBitsPerByte`, is recorded in the Deviation above.
  - Amendment (operator-ordered):
    - Context gains `src/core/hid_report.h`. "the two new ones" became "the new ones".
    - Step 1 lists `kWireBitsPerByte`.
    - Step 2's `probe_wire_bits` type uses `kWireBitsPerByte`.
    - Step 2's flags are "a copy of the Makefile's `CXXFLAGS`".
    - Step 3 names the second accept case and both floors.
    - Step 4: `bus_init` claims the SM and loads the program before the pin loop.
    - Step 4's constexpr list drops "bits per byte".
    - The probe recovers the SM on timeout.
    - Reconciliation checked: Goal's part table, step 2's "None of the three `src/hal/` files
      includes an SDK header" (still true: `bus_port.h` includes only `core/`), step 4's SM
      config sentence (now begins "Then it configures"), and the Acceptance greps
      (`for ( const auto& pin : kMasterPins )` still occurs once). None needed a further
      change.

- **Validation 2026-09-30 round 2, spec findings — amended in `spec.md` the same day.**
  - Step 4 says "inputs (`DATA`, `ACK`)" but never says the code picks inputs by
    `DriveMode::OpenDrainInputOnly`. It also does not say that both rows carry that mode
    (`src/core/pins.h:40,56`, checked; R-SAFETY-01 holds).
  - Step 4's "every pin-configuring call on a line naming `pin.gpio`": the
    `pio_sm_set_pins_with_mask( …, mask, mask )` line does not name it, and the spec does not
    define "pin-configuring".
  - Step 2's "a copy of the Makefile's `CXXFLAGS`" was added by round 1's amendment and rests
    only on that fix. The reviewer cannot check it.
  - Amendment (operator-ordered):
    - Step 2's flags clause was deleted.
    - Step 4's "every pin-configuring call on a line naming `pin.gpio`" was deleted. Step 3
      already defines what R-SAFETY-10 checks.
    - Step 4's inputs are now "the `DriveMode::OpenDrainInputOnly` rows (`DATA`, `ACK`)".
    - The `pins.h` Context pointer now names those two rows.
    - Reconciliation checked:
      - The `test_ps2_codec.py` Context pointer ("the `make test` flags") points at the
        model file, not at this phase's code, and is left as is.
      - Step 6's R-SAFETY-10 Scope clause describes the check, not `bus_init`, and is still
        consistent with step 3.
      - Goal makes no claim about inputs or flags.
      - None needed a further change.

- **Validation 2026-09-30 round 3, spec findings — amended in `spec.md` the same day, after
  the operator overrode the escape.**
  - The `test_ps2_codec.py` Context pointer still says "the `make test` flags". Round 2's
    amendment left that sentence in place on purpose, and it regenerated the same flags
    finding. Round 2's own advice applied: delete the sentence, don't keep it.
  - Step 7's "the two R-PROC-02 headings" does not name them. The diff uses `## What was
    built` and `## Check it yourself`, and `sh tests/test_phase_docs.sh` accepts them.
  - `tests/pin_table_cases.cpp`'s comment cites `tests/test_pin_table.py`, which the spec
    never names (see the first Deviation above).
  - Operator override (2026-09-30): round 3 escaped to `/expand-phase` on 6 → 3 → 3, and
    the operator chose to amend here instead. The reasons:
    - The three rounds' findings were distinct and shallow, not one recurring disagreement.
    - A re-expanded 21 KB spec would bring new claims to audit for code that already
      exists.
    - The count looks like a reviewer noise floor. Recorded as belay feedback against
      `commands/validate-phase.md`.
    Round 4 is the last patch round. Status went back from `pending` to `in-progress`.
  - Amendment:
    - The `test_ps2_codec.py` pointer drops "the `make test` flags".
    - Step 7 names the headings `## What was built` and `## Check it yourself`.
    - Context gains `tests/test_pin_table.py`, with why the check cannot index rows.
    - Step 1's `static_assert` is now "one `static_assert` over a loop on the table rows",
      which reconciles the first Deviation above.
    - Reconciliation checked:
      - No other sentence mentions `make test`'s flags (step 2's was deleted in round 2).
      - The Acceptance `test_phase_docs.sh` line is consistent with the named headings.
      - No other statement claims "each of the five rows".

- **Validation 2026-09-30 round 4, findings (open).**
  - Code-side contradicts: `docs/constraints.md`'s R-SAFETY-07 Scope says "Five copied-tree
    mutations of the frame loop prove the line bites". Plan step 2 assigns only two of the
    five to R-SAFETY-07, and `reject()` (`tests/test_bus_frame.py:122`) checks only the
    mutation's own rule. The clause overstates its check by three.
  - Spec, undecidable: the rules table binds all of R-PROTO-01 ("LSB-first, SPI mode 3") to
    the probe. The probe observes only bit order; mode 3 is read off the PIO program. This is
    the same gap the fourth Deviation above records.
  - Spec, undecidable: the R-SAFETY-10 Scope sentence on a call wrapped by `clang-format`
    (a false positive when `pin.gpio` lands on the next line) is not in step 6's list.
  - **Resolution agreed for round 5** (operator, 2026-09-30). Apply exactly this, then run
    `/validate-phase 24-pio-bus`:
    1. Code: in `docs/constraints.md`, R-SAFETY-07's Scope clause, replace "Five copied-tree
       mutations of the frame loop prove the line bites." with "Two copied-tree mutations of
       the frame loop (no `att_release`; the failure path returns before releasing) prove the
       line bites."
    2. Spec: in the rules table row for R-PROTO-01 (`spec.md:27`), make the To column
       "`manual:` — the slow-clock probe for bit order; mode 3 read off the PIO program (`in`
       on the `side 1` instruction)". In step 6's R-PROTO-01 bullet, add the same split.
    3. Spec: keep the clang-format sentence in `constraints.md`; don't delete it. Add to step
       6's R-SAFETY-10 bullet: "a call whose arguments `clang-format` wraps onto the next line
       is reported (a false positive)".
    - Each spec amendment gets its Deviations entry, as usual.
    - Closing rule: if round 5 has only spec-side `undecidable` nits, with no `contradicts`
      and no failed gate, the operator closes the phase by override. Record it in the round 5
      verdict and set the status to `done`. Do not iterate a round 6.

- **Round 5 implementation, 2026-09-30 — the resolution above, applied.**
  - Code: `docs/constraints.md` R-SAFETY-07 Scope now claims two mutations, naming them. Text
    exactly as item 1.
  - Spec amendment: the rules table's R-PROTO-01 To column (`spec.md:27`) and step 6's
    R-PROTO-01 bullet now split the binding: the probe for bit order, mode 3 read off the PIO
    program (`in pins, 1` on the `side 1` instruction, `src/hal/ps2_master.pio:33`, checked).
  - Spec amendment: step 6's R-SAFETY-10 bullet now lists the `clang-format` wrap false
    positive, matching the sentence kept in `docs/constraints.md`.
  - Generalised (step 5):
    - Property 1: a Scope clause claims only the mutations that check its own rule.
      Checked every mutation-count claim in `docs/constraints.md`,
      `docs/phases/24-pio-bus/verify.md`, `src/hal/` and `tests/test_bus_frame.py`. Only
      R-SAFETY-07 counted mutations. R-PROTO-06's Scope makes no count claim. `Outcome`'s "5
      copied-tree mutations each flip their own rule" is correct as written.
    - Property 2: every statement of R-PROTO-01's binding separates bit order (probe) from
      mode 3 (program). Checked `spec.md`, `verify.md`, `docs/constraints.md`,
      `ps2_master.pio`, `docs/index/`. `constraints.md` R-PROTO-01 already split it. One
      more instance found and fixed: `verify.md` §"Why the probe line means LSB-first" said
      the probe catches "a mistake in the program" and that R-PROTO-01 is bound to it whole.
      It now says bit-order mistake, binds only that half, and explains why the probe cannot
      see the sampling edge. `ps2_master.pio:1` names both properties as a description of
      the program, not a binding — left as is.
  - Reconciliation checked: Goal's "LSB-first in SPI mode 3" describes behaviour, not the
    binding, unchanged; step 7's "LSB-first and mode 3 in one picture" is still true of
    `verify.md`; the Acceptance grep `R-PROTO-01 .* — manual: ` still matches.
  - Re-run: `test_rule_traceability.py`, `test_phase_docs.sh`, `test_bus_frame.py` → rc 0;
    `make test` → `OK`. No source file changed this round.

- **R-PROTO-01 split; new R-PROTO-07 (operator-ordered, 2026-09-30, after the round-5
  amendment above).** The round-5 wording left mode 3 with no check, only a description of
  how it was reviewed. A rule carries exactly one binding (ADR-0005), so the rule was split,
  following the R-PROTO-06 / R-SAFETY-10 / R-ERR-05 precedent.
  - `docs/constraints.md`: R-PROTO-01 narrowed to "LSB-first", still `manual:` (the probe),
    with a "Narrowed" note. New R-PROTO-07, "the bus master runs SPI mode 3", →
    `test: tests/test_repo_shape.sh`, with a Split note and a Scope clause. The clause says
    the check reads program text, not the wire. It names what is not checked: the side-set
    pin being `CLK`, the settling delays, and an instruction after a label on the same line.
    It also says the emulator evidence in `06-hil-digital` is inferred, not observed.
  - `tests/test_repo_shape.sh`:
    - New `find_proto07( )` over `.pio` files under `src/hal/`. A line starting with
      `pull` / `out pins` / `in pins` must carry `side 1` / `side 0` / `side 1` before any
      `;` comment.
    - Header `# RULE R-PROTO-07` and a `run_all( )` line.
    - Cases: 4 rejects (one per pattern alternative, plus a `side 1` hidden in a comment),
      4 accepts (one per exclusion alternative, plus a comment line naming `pull`) and
      1 wiring case.
    - Accept floor 30 → 34, wiring 12 → 13.
    - Checked on the real program: each of the three side-sets flipped in
      `src/hal/ps2_master.pio` turns `R-PROTO-07` to `FAIL` naming the line. The file was
      restored after.
  - `src/hal/ps2_master.pio:1`: the comment names R-PROTO-07 for mode 3 and R-PROTO-01 for
    LSB-first. Comment only.
  - Spec amendment:
    - Rules table: R-PROTO-01 narrowed, plus a new R-PROTO-07 row.
    - Context pointer §Invariants: adds 07.
    - Step 3: the finder, its cases and floors 28 → 34 / 11 → 13.
    - Step 6: R-PROTO-01 narrowed, plus a new R-PROTO-07 bullet.
    - Acceptance: the `test_repo_shape.sh` line expects R-PROTO-07, plus a new binding grep.
    - This replaces the round-5 "mode 3 read off the PIO program" wording in the table and in
      step 6.
  - `verify.md`:
    - §"LSB first, mode 3" tags mode 3 with R-PROTO-07.
    - The file table paragraph names the new check.
    - §1 gains a `grep PROTO-07` row.
    - The probe section binds R-PROTO-01 to bit order only and points to R-PROTO-07.
  - Reconciliation checked:
    - Goal's "LSB-first in SPI mode 3" describes behaviour and is still true.
    - Goal's "which is why R-PROTO-01 needs the probe" is about bit order and is still true.
    - Step 4's PIO bullet already specifies `out` with `CLK` low and `in` as it rises.
    - Step 7's "LSB-first and mode 3 in one picture" still holds.
    - `src/core/ps2_protocol.h:40` and `src/hal/bus_port.h:32` cite R-PROTO-01 for bit order
      only, so they are correct as they are.
  - Not changed, on purpose:
    - PHASES.md row 24's acceptance text ("R-PROTO-01 to `manual:`") stays as is. That is
      still true, and rows are never edited.
    - Done phases' notes that quote "LSB-first, SPI mode 3" are history.

- **Round 5's undecidable, closed in the spec (operator-ordered, 2026-10-01).** The operator
  reopened the phase after the round-5 override (status `done` → `in-progress`, commit
  9dbd4f3) to close the gap rather than defer it.
  - Spec amendment: Plan step 3's `find_proto07( )` bullet now says that `hits( )` applies the
    exclusion to `grep -n` output (`N:text` lines), and that the exclusion is anchored on that
    prefix so `[^;]*` cannot start after a `;`.
  - Founded on the code, not on the fix: `tests/test_repo_shape.sh` `hits( )` pipes
    `grep -nE "$1"` output into `grep -vE "$2"`.
  - The code is unchanged. The anchor stays because without it a `;` comment that itself
    reads `in pins … side 1` would excuse the line.
  - Reconciliation checked:
    - The `test_repo_shape.sh` Context pointer ("its second argument is an exclusion
      pattern") is still true. It is the general pointer, and step 3 now carries the
      specifics.
    - `find_safety10( )`'s unanchored `pin\.gpio` exclusion doesn't depend on the prefix,
      so nothing else needs it.
    - The For later phases entry "Round 5's open undecidable, closed by override" is now
      stale and was deleted in this edit.

- **Operator override of round 6's escape, 2026-10-01.** Round 6 escaped to `/expand-phase`
  (findings 3 → 1 → 2 since round 3's escape). The operator overrode it: status `pending` →
  `in-progress`, after commit 9f186e8.
  - The operator's reason: the belay feedback filed in round 3 against
    `commands/validate-phase.md` (a starved reviewer turns up a different shallow gap on each
    pass, so the findings count never reaches zero and the escape keeps firing) is a
    package defect. It stops this phase from completing on its own terms.
  - Closing exception, operator-granted: round 7 closes the phase by override if no gate
    fails and the review has no `contradicts`. That holds even with spec-side
    `undecidable`s, which then go to For later phases. No round 8.
  - Cut 1, spec amendment: Plan step 4's "the slowest clock divider" becomes "the clock
    divider to 65535", and the constexpr list's "the slowest divider" becomes "the probe's
    divider". Founded on `src/hal/pio_port.cpp:26` (`kSlowestDivInt = 65535`).
    - Reconciled: `docs/constraints.md` R-PROTO-01 said "its slowest divider" and now says
      "divider 65535".
    - Checked, no change needed: `verify.md`'s "about 500 times" (65535 against a bus divider
      of 125 at 125 MHz, about 524 times) and the Goal, which does not mention the divider.
  - Cut 2, a claim deleted rather than added to the spec: `tests/test_bus_frame.py`'s
    docstring "with the flags `make test` uses" and its comment "The flags make test uses
    (the Makefile's CXXFLAGS)" were removed. Round 2 had already deleted the spec sentence
    behind them.
    - The list is unchanged, and it matches `Makefile:20` as of 2026-10-01. Nothing ties the
      two together; that is recorded under Debt.
    - Checked: no other file in the phase claims those flags (grep over `test_bus_frame.py`,
      `bus_frame_cases.cpp`, `spec.md` and `verify.md`).

## Debt

- `kAckTimeoutUs` (`src/core/ps2_protocol.h`, `belay-debt:`) is a 100 µs budget, not a
  measurement. `09-guitar-observe` measures the real ACK delay.
- `kWireBitsPerByte` (`ps2_protocol.h`) and `kBitsPerByte` (`hid_report.h`) are the same
  number under two names. Ceiling: none functional. Upgrade path: `hid_report.h` uses the
  protocol one, if a phase ever touches both. No owner row. Recorded, not scheduled.
- `find_safety10( )` excuses any line containing `pin.gpio`, so it does not check that `pin`
  is the table loop variable (stated in R-SAFETY-10's Scope clause). Upgrade path: the
  `clang-query` + compile database upgrade, which has no row yet.

- `tests/test_bus_frame.py`'s `CXXFLAGS` is a hand-copied list, not read from the
  `Makefile`. Ceiling: a flag added to `make test` doesn't reach this test. Upgrade path:
  have the driver ask `make` for `CXXFLAGS`, if the two ever drift. No owner row; recorded,
  not scheduled.

## For later phases

- **06-hil-digital**:
  - `exchange_byte`'s budget runs from `pio_sm_put` and counts 8 bit periods (32 µs) plus
    `kAckTimeoutUs`. The program really spends 37 PIO cycles (37 µs) per byte before the ACK
    poll (pull, set, 8×4, out y, jmp), so the ACK slack is about 95 µs, not 100. Account
    for this if the timeout is tuned against the emulator.
  - Between frames `CMD` holds the last bit shifted out instead of being forced high. If the
    emulator or the guitar cares about `CMD`'s idle level, `recover()` and the end of a
    frame need a `set pins` or an SM pin write.
  - The recovery path (disable → clear FIFOs → restart → `jmp offset` → enable) has only
    been compiled. The first time it runs is the ACK-open bench step in `verify.md`.
- **04-trace-mode**: the loopback program prints with `std::printf` over USB stdio from the
  main loop. `exchange_frame` returns only a count, so per-byte ACK timing is not available
  to trace without extending the port.
- **R-SAFETY-09 / R-SAFETY-10 on `.pio`**: `hits( )` strips only `//`, and pioasm also takes
  `;` comments, so a comment mentioning `set pindirs` is a false positive. That is recorded
  in the Scope clauses. `ps2_master.pio`'s comments avoid the phrase.
- **Reviewer taste (not blocking):** the comment on `kSeqPattern` in `main.cpp` says
  "every bit pattern that would show a stuck or swapped line", which nine bytes cannot
  cover. The `recover( )` comment says it leaves "CLK high"; that holds only because the
  `jmp` lands on `pull … side 1`.

- **Validation round 5 (2026-10-01), taste, not blocking:**
  - `kWireBitsPerByte` is a `std::size_t`, but `pio_port.cpp` uses it in `std::uint32_t`
    constexpr arithmetic (`kRxByteShift`, `kByteBudgetUs`).
  - `verify.md` §1 expects "three `ok:` lines" from `test_bus_frame.py`. That count includes
    the `rejection cases` line alongside the two rule lines.

- **Validation round 7 (2026-10-01), taste, not blocking:**
  - `pio_port.cpp` `bus_init( )` computes `mask` for every row, but only the CMD/CLK branch
    uses it.
  - `find_safety10( )`'s comment says "no .pio program sets pindirs at all". A `.pio` line
    that contains `pin.gpio` would still be excused by the shared `pin\.gpio` exclusion.

## Validation — 2026-09-30
- criteria: 29 passed / 0 failed (24 host commands + 5 bench, run on the operator's build/loopback.log and build/ackopen.log: probe 10/10 LSB-first, all loopback frames match with ATT high and 1x–2x clock time, ACK-open frames abort 0/5 and 0/9 with ATT high, and 1/1 completes 10 times)
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean
- independent review: contradicts (spec-side): Plan step 4 orders the pin loop before loading the program — pio_port.cpp:74-88 must claim the SM first, because the SDK's set_pins/pindirs calls take it; contradicts (spec-side): Plan step 1 "two" constants — kWireBitsPerByte, recorded in Deviations; undecidable: where test_bus_frame.py's flags come from; undecidable: which counted floors exist (step 3); undecidable: probe_wire_bits' recovery on timeout
- closure test: fail: Deviations reports a missing pointer (src/core/hid_report.h), plus the three undecidables
- findings: 6
- spec size: 21202 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-09-30 (round 2)
- criteria: 29 passed / 0 failed. The phase code is unchanged since round 1; only spec.md, notes.md and docs/index changed. Re-run this round: scripts/check.sh and the six test scripts among the criteria. Carried over from round 1: make firmware, the no-SDK make test, the greps, and the 5 bench criteria on the same logs.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean
- independent review: undecidable: step 4 "inputs (DATA, ACK)" does not say the branch is by DriveMode or that both rows are OpenDrainInputOnly; undecidable: step 4 "every pin-configuring call on a line naming pin.gpio" is not defined, and the set_pins_with_mask line names `mask`; undecidable: step 2 "a copy of the Makefile's CXXFLAGS", whose values the spec does not give
- closure test: fail: three undecidables (missing pointers)
- findings: 3
- spec size: 21618 (+416 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-09-30 (round 3)
- criteria: 29 passed / 0 failed. All 24 host criteria were re-run this round, the final `make test` through scripts/check.sh. The 5 bench criteria were re-read from the operator's unchanged build/loopback.log and build/ackopen.log.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean
- independent review: undecidable: the Context pointer "the `make test` flags" does not list them, and test_bus_frame.py hard-codes a list; undecidable: step 7's "the two R-PROC-02 headings" are not named; undecidable: the pin_table_cases.cpp comment cites tests/test_pin_table.py, which the spec does not name
- closure test: fail: three undecidables (missing pointers)
- findings: 3
- spec size: 21598 (-20 since the previous validation)
- upstream: none
- not-ours: none
- verdict: escaped to /expand-phase: spec re-expanded (findings 6 → 3 → 3 are not strictly falling; status set to pending)

## Validation — 2026-09-30 (round 4)
- criteria: 29 passed / 0 failed. The phase code is unchanged since round 3, where all 24 host criteria ran in full. Re-run this round: scripts/check.sh, test_phase_docs, test_rule_traceability, test_bus_frame and test_repo_shape. The bench criteria are unchanged.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean
- independent review: contradicts (code-side): R-SAFETY-07's Scope clause in docs/constraints.md claims five mutations prove its line bites. Spec Plan step 2 assigns two, and tests/test_bus_frame.py:122 checks only each mutation's own rule. undecidable: R-PROTO-01's mode-3 half is bound to the probe, which cannot see it. undecidable: the R-SAFETY-10 clang-format-wrap sentence is not in step 6's list.
- closure test: fail: two undecidables (missing pointers)
- findings: 3
- spec size: 21792 (+194 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to implementation. Escape not taken: the operator overrode round 3's escape and declared round 4 the last patch round. The count after round 3's escape verdict is 1 round. Lifetime findings: 6 → 3 → 3 → 3.

## Validation — 2026-10-01 (round 5)
- criteria: 28 passed / 0 failed. All 23 host criteria were run this round, exactly as
  written; criterion 3's `ok: R-SAFETY-10` and `ok: R-PROTO-07` lines were confirmed
  separately. The 5 bench criteria were re-run on the operator's build/loopback.log and
  build/ackopen.log (2026-09-30). Since those logs, the firmware source has changed only in
  `ps2_master.pio`'s first-line comment.
- project gates: test pass, lint pass, typecheck pass (scripts/check.sh rc 0, no
  `workflow gap:` line)
- boundary sweep: clean (`scripts/check.sh --files` over the 10 `src/` files in the set:
  all gates passed)
- independent review: undecidable: Plan step 3 doesn't say that `hits( )` gives the
  exclusion `N:`-prefixed lines, and `find_proto07( )`'s `^[0-9]+:` exclusion anchor
  relies on it. No contradicts. Taste: 2 items, moved to For later phases.
- closure test: fail: one undecidable (missing pointer). Overridden by the operator's
  closing rule (round 4 resolution, 2026-09-30): round 5 has only spec-side undecidable
  nits, no contradicts and no failed gate, so the phase closes by override. The spec was not
  amended for it, and no round 6 was run. The finding and its fix are recorded under For
  later phases.
- findings: 1
- spec size: 23173 (+1381 since the previous validation)
- upstream: none (manifest present; no file in the set is package-owned)
- not-ours: none
- verdict: done (operator override per the round-4 closing rule; closure test failed on one
  undecidable). Lifetime findings: 6 → 3 → 3 → 3 → 1.

## Validation — 2026-10-01 (round 6)
- criteria: 28 passed / 0 failed. All 23 host criteria were re-run this round, exactly as
  written; criterion 3's `ok: R-SAFETY-10` and `ok: R-PROTO-07` lines were confirmed. The 5
  bench criteria carry over from round 5 (same logs). The firmware source has not changed
  since then.
- project gates: test pass, lint pass, typecheck pass (scripts/check.sh rc 0, no
  `workflow gap:` line)
- boundary sweep: clean
- independent review:
  - undecidable: Plan step 4's "the slowest clock divider" is not defined. `pio_port.cpp`
    uses `kSlowestDivInt = 65535`, the largest integer divider. The hardware also takes a
    fractional part, and 0 means 65536.
  - undecidable: `tests/test_bus_frame.py`'s docstring and the comment above its hard-coded
    `CXXFLAGS` claim "the flags `make test` uses". Round 2 deleted the spec sentence behind
    that claim. The list matches `Makefile:20` today, checked 2026-10-01; the spec just does
    not say it must.
  - Round 5's `hits( )` `N:` prefix finding was not raised again.
  - Taste: `kWireBitsPerByte` as `std::size_t` (repeat); the `verify.md` greps
    `| grep SAFETY-10` and `| grep PROTO-07` can match more than the one `ok:` line.
- closure test: fail: two undecidables (missing pointers)
- findings: 2
- spec size: 23360 (+187 since the previous validation)
- upstream: none
- not-ours: none
- verdict: escaped to /expand-phase: spec re-expanded. Findings since round 3's escape verdict
  are 3 → 1 → 2, not strictly falling, so the status is set to pending. The operator overrode
  round 3's escape; whether to override this one is theirs.

## Validation — 2026-10-01 (round 7)
- criteria: 28 passed / 0 failed. All 23 host criteria were re-run this round, exactly as
  written; criterion 1's and 3's `ok:` rule lines were confirmed. The 5 bench criteria carry
  over from round 5 (same logs). The firmware source has not changed since; this round
  touched only `tests/test_bus_frame.py`'s comments and docs.
- project gates: test pass, lint pass, typecheck pass (scripts/check.sh rc 0, no
  `workflow gap:` line)
- boundary sweep: clean
- independent review: clean. Taste: 2 items, moved to For later phases.
- closure test: pass. All four sections have entries. The reviewer found every non-workflow
  file in the diff named in the Plan. No Deviations entry reports a missing pointer since the
  round-6 cuts.
- findings: 0
- spec size: 23361 (+1 since the previous validation)
- upstream: none in this round. The round-3 belay feedback against
  `commands/validate-phase.md` still stands.
- not-ours: none
- verdict: done. A clean pass, so the operator's round-7 closing exception was not needed.

