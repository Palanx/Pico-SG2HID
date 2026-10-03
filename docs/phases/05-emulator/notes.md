# Phase 05-emulator — notes

## Outcome

- base: 2d3a529 (working tree; nothing committed yet)

A second firmware, `build/pico/sg2hid_emu.uf2`, plays a wired SG on the bus. It was built and host-tested. **It has not yet been run on two Picos**: the step-9 bench is owed (see `## Bench readings`).

- **`src/core/pins.h`**
  - Adds `DriveMode::Input`, `DriveMode::OpenDrainOutput`, and `kEmulatorPins` on GPIO 2–6. DATA and ACK are `Output`/`OpenDrainOutput`; CMD, ATT and CLK are `Input`/`Input`.
  - `gpio_of( Signal, std::span<const PinAssignment> table = kMasterPins )`. No existing call site changed.
- **`src/emu/sg_model.{h,cpp}`** — `SgModel`, which includes no SDK header:
  - `reset( )`.
  - `step( received )` → `ByteAnswer{ next, should_ack, ack_delay_us }`.
  - `apply( line )` → `std::expected<void, std::string_view>`. It is built from three parse functions (`parse_mode`, `parse_payload`, `parse_fault`), each returning the new value or a reason. State changes only through `.transform( )` on success, so "a refused line changes nothing" holds by construction.
  - `kAckDelayUs = 10`, `kAckPulseUs = 2`, `kMaxLineLen = 64`, with a `belay-debt:` marker naming 09-guitar-observe.
- **`src/hal/ps2_device.pio`** — the device program at 8 PIO cycles/µs. Each word is pulled, carrying an ACK flag (bit 0) and the next byte inverted (bits 1–8).
  - When the flag is set, it pulses ACK with `set pindirs`.
  - It shifts 8 bits: `wait 0 pin 2` (CLK falls), `out pindirs, 1`, `wait 1 pin 2`, `in pins, 1`.
  - It releases DATA with `mov osr, null` + `out pindirs, 1`, then pushes.
  - Every `pindirs` line carries `; open-drain`.
- **`src/hal/device_port.h`, `src/hal/pio_device.cpp`**
  - `device_init`: one loop over `kEmulatorPins`. Open-drain pins go PIO direction input → `pio_gpio_init` → `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`. Input pins go `gpio_init` → `gpio_disable_pulls`.
  - `device_restart`: halt, clear the FIFOs, restart, release DATA and ACK by a second table loop, jump to the start, load the word for `0xFF`, enable.
  - Also `device_take_byte`, `device_answer` and `device_att_is_high`.
  - Static asserts tie CLK = CMD + 2 and `ps2_device_ACK_PULSE_CYCLES == kDeviceAckPulseUs * 8`.
- **`src/emu/main.cpp`**
  - While ATT is high, it reads one command character per pass and answers `ok: <line>` / `error: <reason>` on `\n`. It drops any byte clocked in while ATT is high (the master's boot probe makes some) and re-arms the state machine.
  - While ATT is low, it runs `serve_frame`: take a byte → `step` → busy-wait the delay (abandoned if ATT rises) → `device_answer`.
  - A `static_assert` ties `kDeviceAckPulseUs == kAckPulseUs`.
- **`CMakeLists.txt`** — two explicit targets: `sg2hid` (core + `bus_frame.cpp` + `pio_port.cpp` + app, `ps2_master.pio`) and `sg2hid_emu` (core + `pio_device.cpp` + emu, `ps2_device.pio`). Each has `-Wall;-Wextra;-Werror` on its own sources, USB stdio on, UART off, and extra outputs. The `Makefile` header names both UF2s.
- **Rules**
  - R-SAFETY-06 is `test: tests/test_pin_table.py`, with the strengthened text. It has `check_two_pico( )`, 5 new rejection cases, 1 new wiring case, and 1 copied-tree mutation (emulator CMD made an output).
  - R-SAFETY-10 is amended: it covers any pin table, and excuses `; open-drain` `pindirs` lines while the outover call is present.
  - R-PROTO-07 reads `src/hal/ps2_master.pio` only.
  - The new R-EMU-01 and R-EMU-02 are bound to `tests/test_emulator.py`, with 2 copied-tree mutations.
- **Docs**
  - ADR-0016 (accepted).
  - `docs/wiring-emulator.md`.
  - `docs/phases/05-emulator/verify.md`.
  - `tests/vectors/poll_exchange.h` plus one README paragraph.
  - One finding in `docs/constraints.md` §Observed conventions: `pio_gpio_init( )` clears the output override.

Acceptance criteria run 2026-10-02:

| Criterion | Result |
|---|---|
| `make lint`, `make typecheck`, `make firmware` (both UF2s exist) | exit 0 |
| `R-SAFETY-06` ok line | 1 |
| `R-EMU-0[12]` ok lines | 2 |
| constraints greps | 1 / 2 / 0 |
| ADR status | accepted |
| wiring-emulator rows | 5 |
| `git diff --quiet main -- <master files>` | exit 0 |
| `test_phase_docs.sh` | exit 0 |
| `make test` | `OK`, exit 0 |
| Manual bench table | **owed** |

## Acceptance: make test

2026-10-02, full tree: last line `OK`, exit 0. `tests/test_checks_are_live.py` passed with the new finder, cases and LIVE accounting. `tests/test_emulator.py` passed: R-EMU-01 ok, R-EMU-02 ok, rejection cases 2/2.

## Bench readings

Owed by the operator. Run `verify.md` part 2 and record one reading per row, as seen on seq 0, seq 1 and seq 2:

| emulator state | seq 0 | seq 1 | seq 2 |
|---|---|---|---|
| defaults | — | — | — |
| `mode analog` | — | — | — |
| `fault ack 2` | — | — | — |
| `fault late 200` | — | — | — |
| `fault late 50` | — | — | — |
| `fault id 79` | — | — | — |
| `fault none` | — | — | — |

## Deviations

- **ACK pulse width is declared twice, tied by asserts.**
  - Spec: `kAckPulseUs` is named in `src/emu/sg_model.h`.
  - Done: it is still there. But `src/hal/pio_device.cpp` cannot include `emu/sg_model.h`, because `deny hal -> emu` in `.claude/workflow/boundaries.rules` (the boundary hook rejected it). So `src/hal/device_port.h` declares `kDeviceAckPulseUs = 2`. `pio_device.cpp` asserts the PIO cycle count against it, and `src/emu/main.cpp` (where `emu -> hal` is allowed) asserts it equals `kAckPulseUs`. Changing one without the other is a build failure.
  - Also: `kIdleByte` from the model was replaced in the port by a local `kReleasedByte = 0xFF`.
  - Checked: no other statement in the spec names where the pulse constant lives.
- **The outover-presence check is a separate finder, `find_safety10od( )`.**
  - Spec step 4: `find_safety10( )` reports a marked line with no outover.
  - Done: `find_safety10( )` itself gains only the exclusion `pindirs[^;]*;[[:space:]]*open-drain`. The presence check is `find_safety10od( )`, and `run_all` reports both under the single `R-SAFETY-10` line.
  - Why: `tests/test_checks_are_live.py` mutates only one-line finders with at most two pattern strings. A multi-line `find_safety10( )` would have silently dropped its whole alternation from mutation coverage.
  - Added for the new finder: an `accept_pair( )` helper, because the accept case needs two files; one reject case; one accept case; and one wiring case for the outover half, so the wiring count goes 13 → 14. The false-positive floor goes 34 → 37.
- **Unmarked `out pindirs` reject case.** Spec: add one. The unmarked `out pindirs, 1` case already existed from 24-pio-bus. The new case is `out pindirs, 1 ; drives DATA`: a comment that is not the marker excuses nothing.
- **R-PROTO-07 case paths.** The existing reject, accept and wiring cases moved from `src/hal/x.pio` to `src/hal/ps2_master.pio`, because the finder now reads only that file. A new accept case (`src/hal/ps2_device.pio`, `pull block`) proves the device file is not read.
- **R-SAFETY-10 false positives from `;` comments.** `hits( )` strips only `//`. The first draft of `ps2_device.pio` mentioned `out pindirs` / `set pindirs` / `mov pindirs` inside `;` comments, and those were reported, which is the false positive the R-SAFETY-09 Scope already records. The comments were reworded; no check changed.
- **DATA release after each byte.** Spec step 6 names `out pindirs` and `set pindirs` only. RP2040 PIO has no `mov` to `pindirs` (that destination arrived with the RP2350), so DATA is released after the eighth bit with `mov osr, null` + `out pindirs, 1 ; open-drain`. This is within scope: the spec says the program releases DATA.
- **pins.h layout.** `clang-format` folded the three short emulator rows onto one line each. That works against the header's "columns stay aligned" intent, but it is what `make lint` requires. The R-SAFETY-06 copied-tree anchor matches the one-line form.
- **`tests/pin_table_cases.cpp` shape.**
  - `report_rules( )` now takes `( master, emulator )`.
  - `Rejection` gained an `emulator` fixture that defaults to the real `kEmulatorPins`, so the existing master-table cases break only the master.
  - `good_fixture( )` takes its source table.
- **Spec sufficiency.** None missing. Every file read was in the Context pointers or the Plan. The exceptions are the pico-sdk sources (`hardware_gpio/gpio.c`, `hardware_pio/pio.c`), read to confirm the outover ordering, and `docs/wiring.md`'s resistor values, which was a pointer.

## Debt

- `src/emu/sg_model.h` `belay-debt:` — `kAckDelayUs = 10` and `kAckPulseUs = 2` are budgets, not measurements. 09-guitar-observe measures the SG and replaces them. A pulse longer than 4 µs needs more than one `[delay]` at 8 cycles/µs, because one instruction's delay is at most 31 cycles, so it would need a loop in the program.
- **ACK delay is CPU-timed with interrupts on.** It counts from when `serve_frame` sees the RX word, not from the rising edge itself, and a USB interrupt can add a few µs.
  - Ceiling: `fault late <us>` is accurate to a few µs. The default 10 µs plus jitter stays far inside the master's 100 µs budget.
  - Upgrade, if 06-hil-digital needs exact delays: disable interrupts while ATT is low, or time the delay in PIO.
- **First frame after a mid-clock boot.** If the emulator boots while the master is part-way through clocking a byte with ATT high, part of a byte sits in the ISR, and the first frame is misaligned. Every ATT rise re-arms the state machine, so it heals on the next frame. Upgrade path: none needed unless 06-hil-digital counts that first frame as a desync.
- **R-SAFETY-10's open-drain excuse is trusted.** The marker is not proof, and the outover is checked for presence, not per pin. This is recorded in the rule's Scope clause and in ADR-0016.
- **`docs/wiring-emulator.md` is checked by nothing.** This is out of scope per the spec; the GPIOs mirror the master's.

## For later phases

- **06-hil-digital**
  - The emulator reads commands only while ATT is high, and one character per pass of its idle loop. A test harness should send a line between frames and wait for its `ok:` before relying on it.
  - Commands are line-based on the emulator's own CDC port: `mode`, `payload`, `fault none|ack <n>|late <us>|id <hh>`.
  - The emulator ignores clocks while ATT is high: any byte completed then is dropped and the state machine re-armed.
  - The ACK pulse is 2 µs, and the master never waits for ACK to rise. If the master's turnaround between bytes ever drops under about 2 µs, the device would still be pulsing when CLK falls. It recovers within the same half-bit, but this has not been measured.
- **07-analog-mode** — `SgModel::step( )` answers only `01 42 …`. A `0x43`/`0x44` frame gets an ACK at byte 0 and nothing after, so the config-mode sequence has to be added to `step( )` there, together with R-EMU-01's cases. `mode analog` already gives the 9-byte `0x73` frame.
- **09-guitar-observe** — replace `kAckDelayUs`/`kAckPulseUs` with measured values. Keep `kDeviceAckPulseUs` and `ACK_PULSE_CYCLES` in `ps2_device.pio` in step; the static asserts enforce it.
