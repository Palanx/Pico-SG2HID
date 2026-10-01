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
  - `tests/pin_table_cases.cpp` asserts `gpio_of` at compile time.
- **`docs/constraints.md`**:
  - Bindings: R-SAFETY-07 and R-PROTO-06 → `test: tests/test_bus_frame.py`, R-SAFETY-10 →
    `test: tests/test_repo_shape.sh`, each with a dated Scope clause. R-PROTO-01 → `manual:`
    (the probe).
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

## Debt

- `kAckTimeoutUs` (`src/core/ps2_protocol.h`, `belay-debt:`) is a 100 µs budget, not a
  measurement. `09-guitar-observe` measures the real ACK delay.
- `kWireBitsPerByte` (`ps2_protocol.h`) and `kBitsPerByte` (`hid_report.h`) are the same
  number under two names. Ceiling: none functional. Upgrade path: `hid_report.h` uses the
  protocol one, if a phase ever touches both. No owner row. Recorded, not scheduled.
- `find_safety10( )` excuses any line containing `pin.gpio`, so it does not check that `pin`
  is the table loop variable (stated in R-SAFETY-10's Scope clause). Upgrade path: the
  `clang-query` + compile database upgrade, which has no row yet.

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

