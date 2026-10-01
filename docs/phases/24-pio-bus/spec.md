# Phase 24-pio-bus — the PIO bus master and a loopback firmware

## Goal

After this phase the firmware can run one PS2 frame on the bus: it asserts `ATT`, clocks each
byte out on `CMD` and in on `DATA` LSB-first in SPI mode 3 at `kBusClockHz` (250 kHz), waits for
`ACK` after every byte but the last, gives up on a byte whose `ACK` does not come within
`kAckTimeoutUs`, and releases `ATT` whatever happened. `src/app/main.cpp` becomes a loopback
program that exercises this on one Pico and prints what it saw over USB.

The work splits along one seam:

| Part | File | Host-testable | What it owns |
|---|---|---|---|
| Frame loop | `src/hal/bus_frame.{h,cpp}` | yes — no SDK include | `ATT` framing, which bytes wait for `ACK`, stopping at the first failed byte |
| Port seam | `src/hal/bus_port.h` | declarations only | the functions the frame loop calls; the firmware links the PIO port, the host test links a fake |
| PIO port | `src/hal/pio_port.cpp`, `src/hal/ps2_master.pio` | no | pin setup from `kMasterPins`, the PIO program, the per-byte timeout and recovery, the bit-order probe |
| Loopback | `src/app/main.cpp` | no | the known sequences, the printed results |

Rules this phase moves:

| Rule | From | To |
|---|---|---|
| R-SAFETY-07 (`ATT` released on every path) | `planned: 24-pio-bus` | `test: tests/test_bus_frame.py` |
| R-PROTO-06 (`ACK` waited after every byte but the last) | `planned: 24-pio-bus` | `test: tests/test_bus_frame.py` |
| R-SAFETY-10 (`src/hal/` configures pins only by iterating the table) | `planned: 24-pio-bus` | `test: tests/test_repo_shape.sh` |
| R-PROTO-01 (LSB-first, SPI mode 3) | `planned: 24-pio-bus` | `manual:` — the slow-clock bit-order probe |

R-SAFETY-09 also widens: `.pio` joins `src_files( )`, and a `set`, `out` or `mov` to `pindirs`
in a `.pio` file outside `src/hal/` is reported.

What the loopback proves and what it does not: with `CMD` jumpered to `DATA`, every byte
shifted in equals the byte shifted out, so a round-trip shows the clock, the shift and the
sampling edge work together — but it reads the same whichever bit goes first, which is why
R-PROTO-01 needs the probe. With `ATT` jumpered to `ACK`, `ACK` is low for the whole frame, so
the wait is satisfied at once: on hardware, only the timeout path (ACK jumper removed) is
really exercised. That the wait actually waits is first observable against the emulator
(`06-hil-digital`).

## Context pointers

- `CLAUDE.md` §Hardware safety: R-SAFETY-01 (never drive `DATA`/`ACK`), R-SAFETY-08 (the
  guitar is unplugged whenever firmware is flashed; this phase never meets the guitar).
- `docs/adr/0001-cpp17-pico-sdk-pio-bus.md` §Decision: PIO generates `CLK`, shifts both ways,
  and the `ACK` wait is a PIO `wait` with a CPU-side timeout.
- `docs/adr/0006-fail-safe-hardware-policy.md`: one pin table; `src/hal/` configures pins by
  iterating it.
- `docs/adr/0007-error-model.md`, `docs/adr/0011-pure-link-step.md`: a missing `ACK` is not a
  failed call. This phase only reports how many bytes completed; turning that into a
  `DecodeOutcome` and a `step` is `06-hil-digital`'s.
- `docs/constraints.md`:
  - §Invariants: R-SAFETY-01, 07, 09, 10; R-PROTO-01, 06; R-CLEAN-03 and R-CLEAN-05 (their
    "File scope" sentences list `src_files( )`' extensions); R-PROC-04.
  - §Observed conventions: "One PIO program per bus role … with the cycle budget written above
    it as a comment"; the SDK-layer clang-tidy entry (why `make lint` on `src/hal/` needs
    `make firmware` first).
  - §Error handling: the firmware never stops; every error path ends with the bus idle.
- `docs/wiring.md`: the pin table and the breadboard (330 Ω series resistors on all five
  lines, 10 kΩ pull-ups on `DATA` and `ACK`, socket pins 1, 2, 6, 9 for DATA, CMD, ATT, ACK).
  The loopback jumpers go between socket rows.
- `src/core/pins.h`: `kMasterPins`, `Signal`, `Direction`, `DriveMode`; `DATA` and `ACK` are
  the `OpenDrainInputOnly` rows. GP2..GP6 in the order
  DATA, CMD, ATT, CLK, ACK.
- `src/core/ps2_protocol.h`: where wire-level constants live; the new ones go here.
- `src/core/hid_report.h`: already defines `kBitsPerByte` in the same namespace, so the
  wire's bit count needs its own name.
- `src/app/main.cpp`: replaced by the loopback program.
- `CMakeLists.txt`: the `SG2HID_SOURCES` glob and per-source `-Wall -Wextra -Werror`; the PIO
  header and `hardware_pio` are added here.
- `Makefile`: `make test` runs every `tests/test_*.py`; nothing here changes.
- `tests/test_ps2_codec.py` and `tests/ps2_codec_cases.cpp`: the model for the new driver and
  cases file — `*_cases.cpp` not named `test_*.cpp`, forwarded `ok:`/
  `FAIL:` lines, copied-tree mutations that assert their anchor matched.
- `tests/pin_table_cases.cpp`: gains the `gpio_of` check (Plan step 1).
- `tests/test_pin_table.py`: its copied-tree case "ACK entry deleted" compiles
  `pin_table_cases.cpp` with a row missing, so that check cannot index rows by position.
- `tests/test_repo_shape.sh`: `src_files( )`, `hits( )` (its second argument is an exclusion
  pattern), `find_safety09( )`, `run_all( )`, and the `reject`/`accept`/`wiring` sections with
  their counted floors.
- `tests/test_checks_are_live.py`: a finder's pattern is a single-quoted string in a one-line
  body, and removing any one alternative must fail the file — so every alternative needs its
  own reject case. A `.py` check is held to the accounting property: every declared rule id
  gets a real-run `ok:`/`FAIL:` line, and a case line says "rejection case".
- `tests/test_rule_traceability.py`: a `test:` binding needs the file to carry `RULE <id>`.
- `tests/test_style.sh` `tidy_sdk( )`: how `src/hal/` is linted.
- `docs/phases/23-firmware-build/notes.md` §For later phases: `-Werror` applies per source;
  `pico_generate_pio_header( )` is not wired; a `% c-sdk` init helper would configure pins
  where R-SAFETY-09 cannot see it.
- `docs/phases/23-firmware-build/verify.md`: the house shape of `verify.md` and the flashing
  procedure (BOOTSEL, `RPI-RP2`, `/dev/cu.usbmodem*`).
- `$PICO_SDK_PATH/src/rp2_common/hardware_pio/include/hardware/pio.h`: the `pio_sm_*` and
  `sm_config_*` calls; the PIO instruction set is RP2040 datasheet §3.4.

## Plan

1. **Constants and one lookup in `core`.**
   - `src/core/ps2_protocol.h` gains `kWireBitsPerByte = 8`, `kBusClockHz = 250000` and
     `kAckTimeoutUs = 100`. The last carries a `belay-debt:` comment: a budget, not a measurement; `09-guitar-observe`
     owns measuring it.
   - `src/core/pins.h` gains `consteval std::uint8_t gpio_of( Signal signal )`, returning the
     table row's `gpio` and ending in `std::unreachable( )` after the loop. `consteval`, so a
     signal missing from the table is a compile error.
     The pins.h comment naming `03-pio-bus` is corrected.
   - `tests/pin_table_cases.cpp` gains one `static_assert` over a loop on the table rows
     that `gpio_of( row.signal ) == row.gpio`.
   - Check: `make test` → last line `OK`.

2. **The frame loop and the seam, host-tested.** Touches `src/hal/bus_port.h`,
   `src/hal/bus_frame.h`, `src/hal/bus_frame.cpp`, `tests/bus_frame_cases.cpp`,
   `tests/test_bus_frame.py`. None of the three `src/hal/` files includes an SDK header.
   - `bus_port.h` declares, in `namespace ps2`: `void bus_init( );`, `void att_assert( );`,
     `void att_release( );`, `[[nodiscard]] std::optional<std::uint8_t> exchange_byte(
     std::uint8_t out, bool should_wait_ack );` and `[[nodiscard]]
     std::optional<std::array<std::uint8_t, kWireBitsPerByte>> probe_wire_bits( std::uint8_t byte );`
     (implemented in step 4; the fake does not define it, and nothing in `bus_frame.cpp`
     calls it or `bus_init`).
     `exchange_byte` returns the byte shifted in, or `std::nullopt` when the byte (and its
     `ACK`, if asked for) did not complete in time.
   - `bus_frame.h` declares `[[nodiscard]] std::size_t exchange_frame( std::span<std::uint8_t>
     frame );`. It asserts `ATT`; for byte `i` calls `exchange_byte( frame[ i ], i + 1 <
     frame.size( ) )` and writes the result back into `frame[ i ]`; stops at the first
     `std::nullopt`; releases `ATT`; returns the number of bytes that completed. One exit, so
     one release.
   - `tests/bus_frame_cases.cpp` defines a fake port that records every call in order and fails
     `exchange_byte` at a scripted position. It runs frame lengths 1, 2, 5 and 9, each with no
     failure and with a failure at every position, and prints one real-run line per rule:
     - `R-SAFETY-07`: every frame's record starts with `att_assert`, ends with `att_release`,
       releases exactly once, and has no exchange after the release;
     - `R-PROTO-06`: in a frame of `n` bytes, bytes `0..n-2` ask for `ACK` and byte `n-1` does
       not; after a failed byte, no further byte is exchanged. The returned count equals the
       failure position (or `n`), and completed bytes hold the fake's responses.
   - `tests/test_bus_frame.py` carries `RULE R-SAFETY-07` and `RULE R-PROTO-06`, compiles the
     cases file with `src/hal/bus_frame.cpp`, forwards its lines, then
     runs copied-tree mutations of `src/hal/bus_frame.cpp`, each asserting its anchor matched
     and requiring the named line to turn to `FAIL`:
     - no `att_release` call → R-SAFETY-07;
     - the failure path returns before releasing → R-SAFETY-07;
     - the last byte also asks for `ACK` → R-PROTO-06;
     - no byte asks for `ACK` → R-PROTO-06;
     - the loop continues after a failed byte → R-PROTO-06.
     It prints `rejection cases: 5/5` (a case line) and fails below that.
   - Check: `python3 tests/test_bus_frame.py` → exit 0, `ok:` lines naming R-SAFETY-07 and
     R-PROTO-06, `rejection cases: 5/5`.

3. **R-SAFETY-10, and `.pio` in R-SAFETY-09.** Touches `tests/test_repo_shape.sh`.
   - `src_files( )` gains `pio` in its extension alternation.
   - `find_safety09( )`'s pattern also matches `set`, `out` or `mov` followed by whitespace and
     `pindirs`.
   - New `find_safety10( )`: `hits` over `src_files( )` restricted to `src/hal/`, with the same
     alternation as `find_safety09( )` written out in its own pattern, and exclusion pattern
     `pin\.gpio`. It reports any pin-configuring call in `src/hal/` whose line does not name
     `pin.gpio`, and any `pindirs` instruction there. Header gains `# RULE R-SAFETY-10`;
     `run_all( )` gains its `report` line.
   - Cases:
     - `find_safety09`: one reject per new alternative, each in `src/app/x.pio`
       (`set pindirs, 1`, `out pindirs, 1`, `mov pindirs, x`);
     - `find_safety10`: one reject per alternative of its pattern, each in `src/hal/x.cpp`
       with a literal GPIO (or `src/hal/x.pio` for the three `pindirs` forms);
     - `find_safety10`: accept `gpio_init( pin.gpio );` in `src/hal/x.cpp`, and accept
       `gpio_init( 2 );` in `src/app/x.cpp`;
     - a `wiring R-SAFETY-10` case.
     The accept floor rises 28 → 30 and the wiring floor 11 → 12; the reject section has no
     floor.
   - Check: `sh tests/test_repo_shape.sh` → exit 0, an `ok:` line naming R-SAFETY-10.
   - Check: `python3 tests/test_checks_are_live.py` → exit 0.

4. **The PIO port and the build.** Touches `src/hal/ps2_master.pio`, `src/hal/pio_port.cpp`,
   `CMakeLists.txt`.
   - `ps2_master.pio`, one program with its cycle budget as a comment and no `% c-sdk` block.
     One TX word per byte: bits 0–7 the byte, bit 8 "wait for `ACK`". `CLK` is an optional
     side-set, idle high. Four PIO cycles per bit — two with `CLK` low while `out pins, 1`
     changes `CMD`, two with `CLK` high, sampling `DATA` with `in pins, 1` as it rises. After
     8 bits, `out y, 1`; if set, loop on `jmp pin` (the `jmp` pin is `ACK`) until `ACK` is low;
     then `push`. It never waits for `ACK` to go high again: a byte lasts 32 µs, the pulse a
     few µs. No `pindirs` instruction.
   - `pio_port.cpp` implements `bus_port.h`:
     - `bus_init( )` claims a state machine and loads the program (the pin calls below take
       the claimed state machine), then configures pins in one `for ( const auto& pin : kMasterPins )` loop:
       - the `DriveMode::OpenDrainInputOnly` rows (`DATA`, `ACK`): `gpio_init`, `gpio_pull_up`;
       - `ATT`: `gpio_init`, driven high, then `gpio_set_dir` out — released before it can
         drive;
       - `CMD`, `CLK`: set high through the state machine, then
         `pio_sm_set_consecutive_pindirs( …, pin.gpio, 1, true )`, then `pio_gpio_init`.
       Then it configures the state machine: out base `CMD`, in base
       `DATA`, side-set base `CLK`, `jmp` pin `ACK` (all through `gpio_of`), both shifts
       right, no autopush/autopull, clock divider `clk_sys / ( 4 * kBusClockHz )`.
     - `att_assert( )`/`att_release( )`: `gpio_put` on `ATT`, low/high.
     - `exchange_byte( )` puts the word and waits for the RX word at most 8 bit periods plus
       `kAckTimeoutUs`. On timeout it disables the state machine, clears its FIFOs, restarts
       it at the program's first instruction, re-enables it, and returns `std::nullopt`.
     - Every number (cycles per bit, the slowest divider, the probe's one
       second) is a named `constexpr` (R-CLEAN-04; `make lint` reports a missed one).
     - `probe_wire_bits( )`: sets the slowest clock divider, sends `byte` with no `ACK` wait, and from the CPU samples
       the `CMD` pad on each rising edge of the `CLK` pad (`gpio_get`), in time order. It gives
       up after one second (`std::nullopt`, recovering the state machine as `exchange_byte( )`
       does), and restores the bus clock divider either way.
   - `CMakeLists.txt`: `pico_generate_pio_header( sg2hid ${CMAKE_CURRENT_LIST_DIR}/src/hal/ps2_master.pio )`
     and `hardware_pio` in `target_link_libraries`.
   - Check: `make firmware && test -f build/pico/sg2hid.uf2` → exit 0.
   - Check: `make lint` → exit 0.

5. **The loopback program.** Touches `src/app/main.cpp`.
   - Three hand-written sequences as `constexpr` arrays, in this order: `01 42 00 00 00`;
     `FF 00 A5 5A 80 01 FE 7F 3C`; `80` (one byte, whose only byte is the last and so waits
     for no `ACK`).
   - `stdio_init_all( )`, `bus_init( )`, then once a second:
     - `probe: 0x01 on the wire: b0,b1,…,b7` from `probe_wire_bits( 0x01 )`, or
       `probe: timeout`;
     - per sequence, a copy through `exchange_frame`, timed with `time_us_32( )`, then
       `loopback: seq=<i> bytes=<k>/<n> match=<yes|no> att=<high|low> us=<t>`. `match=yes` iff
       `k == n` and every byte came back equal. `att` is `gpio_get( gpio_of( Signal::Att ) )`
       after the frame.
   - Check: `make firmware` → exit 0, and `grep -cE 'gpio_(init|set_dir|pull_up)|pio_gpio_init' src/app/main.cpp` → 0.

6. **The rules.** Touches `docs/constraints.md`.
   - R-SAFETY-07 and R-PROTO-06 → `test: tests/test_bus_frame.py`; R-SAFETY-10 →
     `test: tests/test_repo_shape.sh`. Each gains a `**Scope, recorded <date>` clause saying
     what the check reads:
     - R-SAFETY-07/R-PROTO-06: the frame loop against a fake port, lengths 1, 2, 5, 9 and every
       failure position; not the PIO program, and not the timing of anything.
     - R-SAFETY-10: a line in `src/hal/` naming a configuring call or a `pindirs` instruction
       must contain `pin.gpio`; it does not prove `pin` is the loop variable over `kMasterPins`,
       and direct register writes are not seen.
   - R-SAFETY-10's "does not exist before `03-pio-bus`" sentence is corrected.
   - R-PROTO-01 → `manual:`, reason: the probe's output is read by the operator from a flashed
     Pico. Its text drops the claim that the loopback round-trip can observe bit order, and
     says why it cannot (a symmetric `CMD`→`DATA` loopback reads the same in either order).
   - R-SAFETY-09's Scope clause: `src_files( )` lists `.pio`; `set`/`out`/`mov pindirs` in a
     `.pio` outside `src/hal/` is reported; a `pindirs` inside a `;` comment is a false
     positive. R-CLEAN-03's and R-CLEAN-05's "File scope" sentences add `.pio`.
   - Check: `python3 tests/test_rule_traceability.py` → exit 0.

7. **`docs/phases/24-pio-bus/verify.md`**, for a non-specialist, with the R-PROC-02 headings
   `## What was built` and `## Check it yourself`.
   - What PIO is; what `CLK`, `CMD`, `DATA`, `ATT`, `ACK` do in one frame; LSB-first and mode 3
     in one picture.
   - The host checks: `python3 tests/test_bus_frame.py` and what its lines mean.
   - The bench, **guitar unplugged from the socket** (R-SAFETY-08): jumper socket pin 2 (CMD) to
     socket pin 1 (DATA), socket pin 6 (ATT) to socket pin 9 (ACK); flash as in phase 23; read
     the port; what each printed field should say; then unplug USB, remove the ATT–ACK jumper,
     plug back in, and read the aborted frames; then remove both jumpers.
   - Why the probe line reading `1,0,0,0,0,0,0,0` means LSB-first, and why the loopback alone
     cannot say so.
   - What this phase does not prove (last paragraph of §Goal).
   - Check: `sh tests/test_phase_docs.sh` → exit 0.

## Acceptance criteria

Host part: run from the repository root with the SDK installed, `PICO_SDK_PATH` exported and the
cask's ARM `bin` on `PATH`, in order.

```
python3 tests/test_bus_frame.py                                          # expect: exit 0; `ok:` lines naming R-SAFETY-07 and R-PROTO-06; `rejection cases: 5/5`
grep -lE '#[[:space:]]*include[[:space:]]*[<"](pico|hardware)/' src/hal/bus_port.h src/hal/bus_frame.h src/hal/bus_frame.cpp   # expect: no output
sh tests/test_repo_shape.sh                                              # expect: exit 0; an `ok:` line naming R-SAFETY-10
grep -c 'for ( const auto& pin : kMasterPins )' src/hal/pio_port.cpp     # expect: 1
grep -cE '(set|out|mov)[[:space:]]+pindirs|% c-sdk' src/hal/ps2_master.pio   # expect: 0
grep -c 'pico_generate_pio_header' CMakeLists.txt                        # expect: 1
grep -c 'planned: 24-pio-bus' docs/constraints.md                        # expect: 0
grep -cE '^- \*\*R-SAFETY-07\*\* .* — test: `tests/test_bus_frame\.py`$' docs/constraints.md   # expect: 1
grep -cE '^- \*\*R-PROTO-06\*\* .* — test: `tests/test_bus_frame\.py`$' docs/constraints.md    # expect: 1
grep -cE '^- \*\*R-SAFETY-10\*\* .* — test: `tests/test_repo_shape\.sh`$' docs/constraints.md  # expect: 1
grep -cE '^- \*\*R-PROTO-01\*\* .* — manual: ' docs/constraints.md       # expect: 1
grep -c 'is the first thing that can' docs/constraints.md                # expect: 0
make firmware && test -f build/pico/sg2hid.uf2                           # expect: exit 0
make lint                                                                # expect: exit 0
make typecheck                                                           # expect: exit 0
sh tests/test_firmware_flags.sh                                          # expect: exit 0
python3 tests/test_rule_traceability.py                                  # expect: exit 0
python3 tests/test_checks_are_live.py                                    # expect: exit 0
sh tests/test_boundaries.sh                                              # expect: exit 0
sh tests/test_phase_docs.sh                                              # expect: exit 0
mv build/pico build/pico.off; env -u PICO_SDK_PATH PATH=/opt/homebrew/opt/bash/bin:/usr/bin:/bin make test | tail -1; mv build/pico.off build/pico   # expect: OK
make test                                                                # expect: last line OK
```

Bench part: the operator runs these with the guitar unplugged, the loopback firmware flashed and
the jumpers as `verify.md` says. `/validate-phase` records the operator's output; it cannot run
them itself. The first line read from the port may be partial, hence `tail -n +2`.

```
# Jumpers CMD→DATA and ATT→ACK in place:
PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 41 "$PORT" | tail -n +2 > build/loopback.log
grep -c '^probe: 0x01 on the wire: 1,0,0,0,0,0,0,0$' build/loopback.log                      # expect: >= 5
grep '^loopback:' build/loopback.log | grep -vc 'match=yes att=high'                          # expect: 0
awk '/^loopback:/ { split($3, b, "[=/]"); split($6, u, "="); if (u[2] < 32 * b[3] || u[2] > 64 * b[3]) bad++ } END { print bad + 0 }' build/loopback.log   # expect: 0 — every frame took between 1x and 2x its 32 µs-per-byte clock time
# USB unplugged, ATT→ACK jumper removed, USB back in:
PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 41 "$PORT" | tail -n +2 > build/ackopen.log
grep '^loopback:' build/ackopen.log | grep -vcE 'bytes=0/(5|9) match=no att=high|bytes=1/1 match=yes att=high'   # expect: 0
grep -c 'bytes=1/1 match=yes att=high' build/ackopen.log                                      # expect: >= 5 — the one-byte frame, run right after two aborted frames, still completes
```

## Out of scope

- Trace output over USB CDC and the host decoder. Owner: `04-trace-mode`.
- The emulator, and any second CMake target. Owner: `05-emulator`.
- Polling: choosing a frame's length from the header, dropping the response's first byte before
  `decode`, calling `step`, and the wraparound-safe elapsed-time subtraction ADR-0011 gives
  `hal`. Owner: `06-hil-digital`.
- An `ATT`-to-first-clock or inter-byte delay, and waiting for `ACK` to go high again. Nothing
  on the loopback bench can tell whether either is needed. Owner: `06-hil-digital` against the
  emulator, confirmed by `09-guitar-observe`.
- Adding `_program_init` to R-SAFETY-09's names: no `% c-sdk` block exists, so no such helper is
  generated.
- Scanning `.c` under `src/`, and the `clang-query` upgrade of the grep checks. No row yet.
- Comments naming `03-pio-bus` outside the passages this phase rewrites (the `belay-debt:`
  headers in `tests/test_repo_shape.sh`, R-ERR-01/R-ERR-02's upgrade-path sentences).
