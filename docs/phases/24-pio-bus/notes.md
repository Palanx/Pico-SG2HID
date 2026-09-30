# Phase 24-pio-bus — notes

## Outcome

- base: 946bb0f (the expansion commit; this phase's work is uncommitted in the working tree)

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
