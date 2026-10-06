# Phase 05-emulator — notes

## Outcome

- base: 2d3a529 (working tree; nothing committed yet)
- not-ours: CLAUDE.md — operator-ordered belay-bypass line, 2026-10-05

A second firmware, `build/pico/sg2hid_emu.uf2`, plays a wired SG on the bus. It was built and host-tested, and the two-Pico bench passed on 2026-10-05 (see `## Bench readings`).

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
| Manual bench table | all 7 rows match, 2026-10-05 |

**Re-expansion implementation round, 2026-10-05.** The spec now requires a broken frame to get `0xFF` on every later wire byte.
- `SgModel::step( )` in `src/emu/sg_model.cpp` answers `kIdleByte` once `m_is_broken` is set. The `src/emu/sg_model.h` header comment says the same.
- `tests/emulator_cases.cpp` gains `released_after( )` and two R-EMU-01 cases: `42 42 …` sends `0xFF` after byte 0, and `01 43 …` sends `0xFF` after byte 1. With `step( )` reverted, both FAIL and R-EMU-01 turns to FAIL; the source was restored afterwards.
- R-EMU-01's text and Scope clause in `docs/constraints.md` say the same.
- `verify.md` line 31 is reworded (see Deviations).
- All 14 acceptance criteria pass. `sg2hid_emu.uf2` was rebuilt 23:00, the operator re-flashed it, and the bench was re-read (see `## Bench readings`).

## Acceptance: make test

2026-10-02, full tree: last line `OK`, exit 0. `tests/test_checks_are_live.py` passed with the new finder, cases and LIVE accounting. `tests/test_emulator.py` passed: R-EMU-01 ok, R-EMU-02 ok, rejection cases 2/2.

## Bench readings

2026-10-05. Two Picos, no guitar. Master on `/dev/cu.usbmodem101` (`sg2hid.uf2`), emulator on `/dev/cu.usbmodem2101` (`sg2hid_emu.uf2`). Wired per `docs/wiring-emulator.md`, signal wires inserted after both boards were running. Loopback jumpers removed. The operator ran `verify.md` end to end by hand. The table below is the recorded capture of part 2 step 6, read through `tools/trace_decode.py`. Every row matches `verify.md`.

| emulator state | seq 0 (`01 42 00 00 00`) | seq 1 | seq 2 |
|---|---|---|---|
| defaults (`fault none`, `mode digital`) | `5/5`, in `FF 41 5A FF FF`, ack 9/9/8/8 µs | `0/9`, aborted at byte 0, `att=high` | `1/1`, in `FF` |
| `mode analog` | `5/5`, in `FF 73 5A FF FF`, ack 10/9/9/9 µs | same | same |
| `fault ack 2` | `2/5`, in `FF 41`, aborted at byte 2, `att=high` | same | same |
| `fault late 200` | `0/5`, aborted at byte 0, `att=high` | same | same |
| `fault late 50` | `5/5`, in `FF 41 5A FF FF`, ack 50/49/48/48 µs | same | same |
| `fault id 79` | `5/5`, in `FF 79 5A FF FF`, ack 9/8/9/8 µs | same | same |
| `fault none`, then `mode digital` | `5/5`, in `FF 41 5A FF FF`, ack 9/8/8/8 µs | same | same |

Every command was answered `ok: <line>` on the emulator port.

Reading notes:
- `match=no` on every `loopback:` line is expected. The master still runs the phase-04 loopback comparison, and the emulator's answers differ from what was sent.
- The decoder prints `no ACK after 133 us`. That is the byte time (~36 µs) plus the master's ~100 µs ACK wait, so it agrees with the 100 µs budget in `verify.md`.

Bench incident, no damage observed: on the first attempt the second Pico was still running `sg2hid.uf2`, the master firmware. Both ports printed `loopback:` lines. CMD, ATT and CLK were then push-pull on both boards, with one 330 Ω in each path, which limits contention to ≈ 10 mA. The signal wires were pulled, the board was reflashed with `sg2hid_emu.uf2`, and the run above followed. Identify the emulator by its silence plus an `ok:` reply to `fault none` before inserting the signal wires.

**Re-read 2026-10-05, after the broken-frame change.** Same ports and wiring. The operator re-flashed the emulator and confirmed `ok: fault none` before reconnecting the signal wires. The agent then drove both ports, sending each row's commands to the emulator and decoding about 2 s of the master with `tools/trace_decode.py`. Every command was answered `ok: <line>`. All 7 rows match `verify.md`:

| emulator state | seq 0 | seq 1 | seq 2 |
|---|---|---|---|
| `mode digital` | `5/5`, in `FF 41 5A FF FF`, ack 10/10/9/9 µs | `0/9`, aborted at byte 0, `att=high` | `1/1`, in `FF` |
| `mode analog` | `5/5`, in `FF 73 5A FF FF`, ack 10/10/9/10 µs | same | same |
| `fault ack 2` | `2/5`, in `FF 41`, aborted at byte 2 | same | same |
| `fault late 200` | `0/5`, aborted at byte 0 | same | same |
| `fault late 50` | `5/5`, in `FF 41 5A FF FF`, ack 50/49/49/49 µs | same | same |
| `fault id 79` | `5/5`, in `FF 79 5A FF FF`, ack 10/10/9/9 µs | same | same |
| `fault none`, then `mode digital` | `5/5`, in `FF 41 5A FF FF`, ack 10/9/9/9 µs | same | same |

The broken-frame change is not visible on this bench. The master aborts at the first missing ACK, so it never clocks the bytes after a break. Only the host cases show it.

## Deviations

- **ACK pulse width is declared twice, tied by asserts.**
  - Spec: `kAckPulseUs` is named in `src/emu/sg_model.h`.
  - Done: it is still there. But `src/hal/pio_device.cpp` cannot include `emu/sg_model.h`, because `deny hal -> emu` in `.claude/workflow/boundaries.rules` (the boundary hook rejected it). So `src/hal/device_port.h` declares `kDeviceAckPulseUs = 2`. `pio_device.cpp` asserts the PIO cycle count against it, and `src/emu/main.cpp` (where `emu -> hal` is allowed) asserts it equals `kAckPulseUs`. Changing one without the other is a build failure.
  - Also: `kIdleByte` from the model was replaced in the port by a local `kReleasedByte = 0xFF`.
  - Checked: no other statement in the spec names where the pulse constant lives.
- **The outover-presence check is a separate finder, `find_safety10od( )`.**
  - Spec step 4: `find_safety10( )` reports a marked line with no outover.
  - Done: `find_safety10( )` itself gains only one exclusion, `^[0-9]+:[[:space:]]*(set|out)[[:space:]]+pindirs[^;]*;[[:space:]]*open-drain` (narrowed in round 2, see below). The presence check is `find_safety10od( )`, and `run_all` reports both under the single `R-SAFETY-10` line.
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
- **Round 2 (validation 2026-10-05): the `; open-drain` excuse covers PIO instructions only.**
  - Finding: spec step 4 limits the excuse to `.pio` lines. The round-1 exclusion `pindirs[^;]*;[[:space:]]*open-drain` excused any `src/hal/` line with the marker after `pindirs`. The reviewer's example, `pio_sm_set_pindirs_with_mask( … ); /* ; open-drain */`, was in fact still reported, because `[^;]*` stops at the call's own `;`. But `pio_sm_set_pindirs_with_mask( pio0, 0, 0, 0 /* ; open-drain */ );` was excused. I proved this by running the new reject case against the old pattern: `FAIL: … did not fire`.
  - Done: the exclusion is anchored to a line that starts with a PIO `set` or `out` to `pindirs`. `mov` is left out because RP2040 has no `mov` to `pindirs`. New cases: one reject (the call above, in `src/hal/x.cpp`) and one accept (an indented `set pindirs, 1 ; open-drain`). The false-positive floor goes 37 → 38.
  - Why by syntax and not by file extension: `hits( )` applies the exclusion before it prefixes the file name, and `tests/test_checks_are_live.py` mutates only one-line finders with at most two patterns. A C/C++ line that starts with `set pindirs`/`out pindirs` is not code. So a configuring call that carries the marker cannot be excused, whatever file it is in.
  - Spec amended: in step 4, "In a `.pio` under `src/hal/`, a `pindirs` line …" becomes "A line that starts with a PIO `set` or `out` to `pindirs` …". R-SAFETY-10's Scope clause in `docs/constraints.md` was changed in the same edit; the rule's test is `tests/test_repo_shape.sh`, which changed with it.
  - Property enumerated: "the open-drain marker never excuses a C call". Checked every finder that reads `; open-drain`. `find_safety10( )` was fixed. `find_safety10od( )` was already limited to `.pio` (`grep -E "\.pio$"`), and there it is a trigger, not an excuse. No other finder (`find_safety09( )`, R-SAFETY-01/06 in `tests/test_pin_table.py`) has the marker.
- **Round 2: spec step 6, outover order.** The spec said "gets the outover LOW before its direction is ever set". `device_init( )` sets the PIO direction to *input* first, then `pio_gpio_init( )`, then the outover. The literal order is impossible: `pio_gpio_init( )` clears OUTOVER (see the `§Observed conventions` finding). Amended to "… before its direction is ever set to output", which is what the code does. Reconciled against the spec Goal §Electrically, spec step 1, ADR-0016 ("before the program can set any direction") and the constraints finding. All of them already agree, so none changed.
- **Round 2: `verify.md` "Eight" → "Nine" refused lines**, to match `kRefused` and R-EMU-02's Scope.
- **Validation round 2 (2026-10-05), spec amended for two unsettled `undecidable` findings:**
  - ACK start point. The Goal said the ACK "starts `kAckDelayUs` after the byte's last rising `CLK` edge". The code starts counting only after `serve_frame` sees the RX push, and the PIO `pull`/`out`/`jmp` adds a few cycles. Amended to "starts at least `kAckDelayUs` after". The `fault late` row ("every ACK starts `us` µs after its byte") becomes "every ACK waits `us` µs instead of `kAckDelayUs`". Both rest on `src/emu/main.cpp` `serve_frame` and `src/hal/ps2_device.pio`, and the Debt entry "ACK delay is CPU-timed" already records the jitter. Reconciled: step 5 ("with `kAckDelayUs`", the model's `ack_delay_us`, still exact) and the step-9 rows `fault late 200`/`fault late 50` are unchanged and still agree.
  - seq 1 frame length. `verify.md` writes `bytes=0/9`, and the spec said only "stops at byte 0". The step-9 defaults row now reads "`bytes=0/9`: the master's 9-byte pattern stops at byte 0, `att=high`". This rests on `kSeqPattern` in `src/app/main.cpp` (9 bytes) and the bench output. The other rows say "as above", so they need no change.
- **Re-expansion 2026-10-05 (after the validation round-3 escape).** `spec.md` was rewritten from scratch by `/expand-phase`, on the operator's order, with no change to `src/`, `tests/`, the build, `verify.md`, ADR-0016 or `docs/wiring-emulator.md` (all final as of 108b48f). Every amendment above (round 1, round 2, validation rounds 1–3) is folded into base text. The header no longer makes amendments conditional on a Deviations entry (package defect, reported via `/belay-feedback` 2026-10-05). Context pointers now name every file in `git diff --name-only 2d3a529` (exempt: `spec.md`, `notes.md`, `PHASES.md`, `docs/index/`; `CLAUDE.md` is not-ours) and `docs/constraints.md` §Observed conventions. Step 4 names `find_safety10od( )`; a 14th criterion, `python3 tests/test_checks_are_live.py` → exit 0 (measured 2026-10-05, ~6 min), settles the finder question mechanically.
- **Validation round 4 fix (2026-10-05): misplaced §Layering pointer.** The `docs/constraints.md` pointer under "Files this phase writes" ended with "§Layering: `emu` → `core`, `hal`.", but no hunk touches §Layering. Deleted that sentence; nothing added. Reconciled: the only other statement of the layering in the spec is the `CLAUDE.md` read pointer ("layering"), which stays and is correct; step 6's "`hal` cannot include `emu`" agrees with `CLAUDE.md` §Architecture. No other sentence relied on it.
- **Validation round 5 fix (2026-10-05): two step-7 sentences, spec only, no code change.**
  - Compile flags: the `-Wall;-Wextra;-Werror` / stdio / extra-outputs list was attached to `sg2hid_emu` alone, so the reviewer could not tell whether the master's flags on the shared `src/core/*.cpp` changed. Reworded to "Each target sets the same `-Wall;-Wextra;-Werror` source property on every source in its list, USB stdio on, UART stdio off, and extra outputs." Founded on `CMakeLists.txt` lines 42 and 47–49 (master, line 42 identical at 2d3a529) and 56 and 62–64 (emulator).
  - Line overflow: added "It keeps the first `kMaxLineLen` + 2 characters of a line and drops the rest, so a longer line still reaches `apply( )` too long and is refused." Founded on `kLineBufferSize = ps2::kMaxLineLen + 2` and the `reader.len < kLineBufferSize` guard in `src/emu/main.cpp`, and on `SgModel::apply( )` stripping a trailing `\r` before refusing any line over `kMaxLineLen`.
  - Reconciled: the Goal's "a line longer than `kMaxLineLen` (64) characters" refused line and "It answers every line with one line" agree and stay; step 5's "nine refused lines" is unaffected; step 7's Check is unchanged. Nothing became redundant. Closure re-checked: every file in the phase's file set is still named in the spec.
- **Re-expansion round: `verify.md` line 31 reworded.**
  - The spec's step 5 names the code change, but not this `verify.md` sentence.
  - The old text, "It also stays silent if byte 1 is not `42`", was false for byte 1 itself: its answer, the id, is decided at byte 0 and goes out while the master's byte 1 is still arriving.
  - Now: "it has already sent its id on that byte … and it is silent from byte 2 on". This is in scope: spec step 9 touches `verify.md`, and the Goal says "`0xFF` on every **later** wire byte".
  - Property enumerated, "`DATA` is released whenever this controller is not being answered", at every place it must hold:
    - `ATT` high: `device_restart( )` loads the word for `0xFF`. Holds.
    - Past the frame's end: `byte_at( )` returns `kIdleByte`. Holds.
    - After a break at byte 0 or 1: fixed this round.
    - The broken byte 1 itself: cannot hold, because the answer is already on the wire. Now stated in `verify.md`.
  - The other "silent" claims in `verify.md` (line 86, "stayed silent for a frame that starts wrong"; line 138, seq 1 starts `FF`) are true as written and were left unchanged.
- **Validation round 7 fixes (2026-10-06).**
  - **Code (tests only).** `tests/vectors/poll_exchange.h` gains `kReleasedByte = 0xFF`, hand-written. `tests/emulator_cases.cpp` uses it where it used the model's `ps2::kIdleByte`: in `faithful( )` for bytes past a frame's end, and in `released_after( )`.
    - Proof: with `kIdleByte` temporarily set to `0xFE` in `src/emu/sg_model.h`, R-EMU-01 and R-EMU-02 both FAIL, every case included. Before this change the past-end and broken-frame `0xFF` checks would have followed the model. The header was then restored.
    - Property enumerated, "no expected value in `tests/emulator_cases.cpp` comes from `src/`". Every `ps2::` use in the file was checked:
      - `kIdleByte` at line 41 is `drive( )`'s stand-in for the PIO port's byte 0, an input to the harness, not an expected value. Left as is.
      - `kAckDelayUs` is a project budget, not a protocol byte, so R-PROTO-05 does not cover it. Left as is.
      - `kMaxLineLen` sizes a buffer. Left as is.
  - **Spec, step 5.**
    - "`SgModel::reset( )` starts a frame: wire byte 0's answer is `0xFF`" is cut to "starts a frame". Nothing in the model produces byte 0: `drive( )` sets it, and the firmware loads it from `pio_device.cpp`.
    - The `poll_exchange.h` bullet now names `kReleasedByte` and says every expected `0xFF` after byte 0 comes from these literals. That rests on `faithful( )` (lines 62, 64) and `released_after( )` (line 87).
  - **Spec, step 8.** Added: "The emulator adds no resistors: it relies on the master breadboard's 330 Ω series resistors and its 10 kΩ pull-ups on `DATA` and `ACK` (`docs/wiring.md`)." This rests on `docs/wiring.md` lines 11 and 15 and the "Why no new resistors" section of `docs/wiring-emulator.md`.
  - **Reconciled.**
    - The Goal's "a master byte past the frame's end is answered with `0xFF`" agrees.
    - The step-5 R-EMU-01 sentence "every byte sent after the breaking byte is `0xFF`" agrees.
    - R-EMU-01's Scope ("driven with `tests/vectors/poll_exchange.h`") and the `### Emulator` preamble ("expected answers are the hand-written vectors") in `docs/constraints.md` are now literally true, so neither changed.
    - The `docs/wiring.md` Context pointer stays as is.
    - Nothing became redundant.
- **Validation round 8 fix (2026-10-06): the spelling `find_safety10od( )` matches.**
  - Step 4's `find_safety10od( )` bullet gains "in the one spelling `clang-format` produces (R-STYLE-01)". The spec only, nothing else.
  - What it rests on: R-STYLE-01 checks every `.cpp` against `.clang-format` (`sources( )` in `tests/test_style.sh`, run by `make test`), and `.clang-format` sets `SpacesInParens`. So `gpio_set_outover( pin.gpio, … )` is the only form that passes the build, and the finder's pattern matches that form.
  - Reconciled: the Goal (line 36) and step 1 (ADR-0016) state the same call as behaviour, not as what the check matches, so they stay as they are. The R-SAFETY-10 Scope clause in `docs/constraints.md` names the call and was left unchanged. Nothing became redundant.
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
- **06-hil-digital** — a broken frame now releases `DATA` for the rest of the frame, but no bench has seen it: the master always aborts at the first missing ACK. A master that keeps clocking after a missing ACK, if 06 builds one for recovery tests, is the first thing that can observe it.
- **07-analog-mode** — `SgModel::step( )` answers only `01 42 …`. A `0x43`/`0x44` frame gets an ACK at byte 0 and nothing after, so the config-mode sequence has to be added to `step( )` there, together with R-EMU-01's cases. `mode analog` already gives the 9-byte `0x73` frame.
- **09-guitar-observe** — replace `kAckDelayUs`/`kAckPulseUs` with measured values. Keep `kDeviceAckPulseUs` and `ACK_PULSE_CYCLES` in `ps2_device.pio` in step; the static asserts enforce it.
- **Validation 2026-10-05, reviewer taste (not blocking):**
  - `verify.md` part 1 says "Eight bad command lines"; `kRefused` in `tests/emulator_cases.cpp` and R-EMU-02's Scope both say nine. Fixed in round 2.
  - `tests/pin_table_cases.cpp` reflows the 11 existing `Rejection` initialisers (diff noise only).
  - `next_word( )` accepts repeated and leading spaces; `main.cpp` drops a byte completed while `ATT` is high. Neither is in the spec's grammar, neither is forbidden.
- **Validation round 4 (after re-expansion), reviewer taste (not blocking):** the un-rewrapped R-SAFETY-10 comment line in `tests/test_repo_shape.sh` (already noted in round 3); `refused_lines( )` in `tests/emulator_cases.cpp` applies each refused line to a fresh default model, so it shows "leaves the default frame", not "changes nothing" from a non-default state; one `belay-debt:` comment covers both `kAckDelayUs` and `kAckPulseUs` in `src/emu/sg_model.h`; `tests/pin_table_cases.cpp` reflow churn (already noted).
- **Validation round 5, reviewer taste (not blocking):** `src/emu/main.cpp` uses `std::size_t` without `<cstddef>` and strips `\r` itself as well as in `SgModel::apply( )`; `Fixture::pins` in `tests/pin_table_cases.cpp` is sized from `kMasterPins` while it also holds emulator tables (correct only because both have 5 rows); `device_restart( )`'s `pio_sm_exec( … pio_encode_jmp( device_offset ) )` is not listed in step 6; the over-long R-SAFETY-10 comment line (again).
- **Validation round 6, reviewer taste (not blocking):** the over-long R-SAFETY-10 comment line and the `tests/pin_table_cases.cpp` reflow (again); R-EMU-02 in `docs/constraints.md` takes its grammar from `docs/phases/05-emulator/spec.md` §Goal, so a standing rule rests on a phase document (`src/emu/sg_model.h` would outlive it); `gpio_set_outover` and the other pad-override setters are not in R-SAFETY-09's call-name list, so one outside `src/hal/` would not be reported.
- **Validation round 7, reviewer taste (not blocking):** R-EMU-02 takes its grammar from a phase document (again); two over-long comment lines in `tests/test_repo_shape.sh` (`find_safety10`, `find_proto07` headers); the `ps2_device.pio` header states a 250 ns DATA settle time that nothing measured; `poll_command( )` strips `\r` a second time after `apply( )` already did.
- **Validation round 8, reviewer taste (not blocking):** the un-reflowed `find_safety10( )` comment line and the `tests/pin_table_cases.cpp` reflow (again); two names for released DATA, `kReleasedByte` in `src/hal/pio_device.cpp` and `kIdleByte` in `src/emu/sg_model.h`, and the model never produces byte 0 itself; R-EMU-02 points at a phase document for its grammar (again); `next_word( )` splits on spaces only, so a tab is refused and `fault ack 02` is accepted, and the grammar says nothing about either.
- **Validation round 9, reviewer taste (not blocking):** `drive( )` writes `sent[ 0 ]` from `ps2::kIdleByte`, so byte 0 is never exercised from the model; the un-reflowed `find_safety10( )` comment line (again); `src/emu/main.cpp` uses `std::size_t` without `<cstddef>`; `verify.md` step 6 calls `head -n 15` "about two seconds" when it is about 2.5 s.

## Validation — 2026-10-05
- criteria: 13 passed / 0 failed. Manual bench re-run by the agent on the operator's still-wired two Picos (master `/dev/cu.usbmodem101`, emulator `/dev/cu.usbmodem2101`): all 7 `verify.md` rows reproduce the `## Bench readings` table (ack 8–11 µs default, 48–50 µs under `fault late 50`). Extra: `payload 7f fe 80 80 80 80` arrives as `in … 7F FE` (asymmetric bytes, so bit order holds); `fault id zz`, `fault late 10001` and `fault ack 8` answer `error: usage: …` and the next frame is unchanged.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review: contradicts (code-side): spec Plan step 4 "In a `.pio` under `src/hal/`, a `pindirs` line that carries `; open-drain` is excused" — `find_safety10( )` in `tests/test_repo_shape.sh` applies the exclusion `pindirs[^;]*;[[:space:]]*open-drain` to every `src/hal/` file, so a `.cpp` line such as `pio_sm_set_pindirs_with_mask( … ); /* ; open-drain */` is excused too. Evidence for code-side: the sentence is from the expansion (2d3a529), no Deviations entry records widening it, and the excuse is a weakening of a safety check. | contradicts (spec-side): spec Plan step 6 "An `OpenDrainOutput` pin gets the outover LOW before its direction is ever set" — `device_init( )` sets the PIO direction to input, then `pio_gpio_init( )`, then the outover. Evidence for spec-side: the `§Observed conventions` finding in `docs/constraints.md` (`pio_gpio_init( )` clears OUTOVER) shows the literal order loses the override; the code's order never lets the direction be output before the outover is set. Fix: "before its direction is ever set" → "before its direction is ever set to output"; checked the spec's Goal §Electrically and ADR-0016's wording, neither states the order. (settled: 1 — `make test` for "`tests/test_checks_are_live.py` has no change": `PY_TESTS := $(wildcard tests/test_*.py)` runs it, and the criterion passed)
- closure test: pass (every changed file is named in the spec; four notes sections present)
- findings: 2
- spec size: 20349 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation — `/implement-phase 05-emulator`: restrict R-SAFETY-10's `; open-drain` excuse to `.pio` files (with a reject case: a `.cpp` `pindirs` call carrying `; open-drain` in a block comment), fix "Eight" → "nine" in `verify.md`; and amend spec step 6 as above with its Deviations entry.
- **Validation round 2, reviewer taste (not blocking):**
  - `find_safety10od( )` triggers on `pindirs[^;]*;[[:space:]]*open-drain`, which is looser than `find_safety10( )`'s anchored excuse. It can only over-trigger: the outover then has to be present. It does not excuse anything.
  - `kDeviceAckPulseUs` has no `belay-debt:` marker of its own. It is tied to `kAckPulseUs` by a `static_assert` (see Deviations).
  - `device_restart( )` has a second loop over `kEmulatorPins`, which releases DATA and ACK. Step 6's "one loop" refers to `device_init( )`.
  - The R-SAFETY-06 line puts a `Strengthened` clause before the Scope clause.

## Validation — 2026-10-05 (round 2)
- criteria: 13 passed / 0 failed. Firmware unchanged since the round-1 bench re-run, so the manual bench reading stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review: undecidable: ACK start point (counted from the byte's last rising edge vs. from when the CPU sees the push) — no criterion | undecidable: seq 1's frame length `0/9` in `verify.md`, not stated in the spec — no criterion (settled: 1 — `make test` for the separate `find_safety10od( )` finder / `test_checks_are_live.py` not edited)
- closure test: fail: the two unsettled `undecidable` findings are missing pointers. The spec was amended for both in this round (see Deviations).
- findings: 2
- spec size: 20410 (+61 since the previous validation; measured after this round's amendment)
- upstream: none
- not-ours: none
- verdict: returned to spec. The spec is already amended; re-run `/validate-phase 05-emulator`.
- **Validation round 3, reviewer taste (not blocking):** an un-rewrapped comment line in `tests/test_repo_shape.sh` ("…still reported. The same alternation as find_safety09( ), written"); `verify.md`'s last row reads "`fault none`, then `mode digital`" where the spec says "`fault none`" (consistent: `fault none` does not reset the mode); `tests/pin_table_cases.cpp` reflow churn; `find_safety10od( )`'s unanchored trigger (already noted in round 2).

## Validation — 2026-10-05 (round 3)
- criteria: 13 passed / 0 failed (firmware unchanged; round-1 bench reading stands)
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review:
  - undecidable: no proof that the round-2 spec amendments are authorized. The spec's header requires a Deviations entry in `notes.md`, which the reviewer is withheld. The entries do exist (round-1 and round-2 Deviations), but no criterion settles this.
  - undecidable: the new `§Observed conventions` entry in `docs/constraints.md`. The spec names `docs/constraints.md` only for `§Invariants`/`§Layering`.
  - undecidable: the separate `find_safety10od( )` finder. Step 4 puts the outover report inside `find_safety10( )`; recorded in round-1 Deviations, but the spec never names the finder.
  - settled: 1. `make test` for the untouched `test_checks_are_live.py` (`PY_TESTS := $(wildcard tests/test_*.py)`).
- closure test: fail: the three unsettled `undecidable` findings are missing pointers.
- findings: 3
- spec size: 20410 (+0 since the previous validation)
- upstream: `docs/templates/spec.md`. Its header makes a spec amendment depend on a `notes.md` Deviations entry, but `/validate-phase` step 5 withholds `notes.md` from the reviewer. So every amended spec yields an undecidable finding that no criterion can settle. /belay-feedback recommended.
- not-ours: none
- verdict: escaped to /expand-phase: spec re-expanded. Round 3, findings 2 → 2 → 3, not strictly falling. Status set to `pending`.

## Validation — 2026-10-05 (round 4, first after re-expansion)
- precondition: status was `expanded`, not `in-progress`; the code was final at 108b48f and the operator ordered this run, so the status was set to `in-progress` at its start.
- criteria: 14 passed / 0 failed. `src/`, `tests/`, `CMakeLists.txt` and `Makefile` are unchanged since 108b48f, so the round-1 manual bench reading stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review: undecidable: spec Context pointers list `docs/constraints.md` "§Layering: `emu` → `core`, `hal`" under "Files this phase writes", but no hunk touches §Layering — no criterion (settled: 1 — `python3 tests/test_checks_are_live.py` for whether `find_safety10od( )` is mutation-live)
- closure test: fail: the unsettled `undecidable` is a misplaced pointer — §Layering is read-only context, written into the "writes" list at re-expansion.
- findings: 1
- spec size: 20477 (+67 since the previous validation)
- upstream: none
- not-ours: CLAUDE.md subtracted
- verdict: returned to spec. Fix: delete "§Layering: `emu` → `core`, `hal`." from the `docs/constraints.md` pointer under "Files this phase writes" (CLAUDE.md §Architecture already states the layering), add a Deviations entry, re-run `/validate-phase 05-emulator`.

## Validation — 2026-10-05 (round 5)
- criteria: 14 passed / 0 failed. Code unchanged since 108b48f; the round-1 bench reading stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review: undecidable: `CMakeLists.txt` sets `-Wall;-Wextra;-Werror` as a source property on `${SG2HID_EMU_ALL_SOURCES}`, which includes the `src/core/*.cpp` the master also compiles; the spec does not say whether the master's flags stay unchanged — no criterion (fact, checked in the tree: the master sets the identical property on `${SG2HID_SOURCES}`, line 42, as at 2d3a529) | undecidable: `poll_command( )` in `src/emu/main.cpp` drops characters past `kLineBufferSize` and hands the truncated line to `apply( )`; the Goal does not say how an overflowing line is handled — no criterion (settled: 1 — `python3 tests/test_checks_are_live.py` for `find_safety10od( )` liveness)
- closure test: fail: the two unsettled `undecidable` findings are missing pointers.
- findings: 2
- spec size: 20440 (-37 since the previous validation)
- upstream: none
- not-ours: CLAUDE.md subtracted
- verdict: returned to spec. Second round against the re-expanded spec (1 → 2); the iteration-3 escape is not yet in reach. Proposed fixes: step 7 states that both targets set `-Wall;-Wextra;-Werror` on every source they compile (reword of the existing `sg2hid_emu` sentence); step 7 states that `main.cpp` keeps at most `kMaxLineLen` + 2 characters of a line, so a longer line still reaches `apply( )` too long and is refused.

## Validation — 2026-10-05 (round 6)
- criteria: 14 passed / 0 failed. Code unchanged since 108b48f; the round-1 bench reading stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review: undecidable: what the emulator puts on `DATA` after a broken frame — `SgModel::step( )` sets `m_is_broken`, which clears only `should_ack`; `next` is still `byte_at( index + 1 )` (id, `0x5A`, payload), so if the master kept clocking past a wrong byte 0 the emulator would drive `DATA`. `verify.md` lines 31, 86 and 138 tell the operator it "stays silent". The spec says only "gets no ACK from that byte on" — no criterion (R-EMU-01's `0x42…` and `01 43` cases check the ACK, not the bytes sent). Confirmed by reading `src/emu/sg_model.cpp`. Harmless on the step-9 bench: the master aborts at the first missing ACK.
- closure test: fail: the unsettled `undecidable` is a missing statement about `DATA` after a broken frame.
- findings: 1
- spec size: 20674 (+234 since the previous validation)
- upstream: none
- not-ours: CLAUDE.md subtracted
- verdict: escaped to /expand-phase: spec re-expanded. Third round against the re-expanded spec, findings 1 → 2 → 1, not strictly falling. Status set to `pending`. For the re-expansion: the spec must say what `DATA` carries after a broken byte, and that decides which side changes — a spec statement that the emulator keeps shifting the frame's bytes with ACK withheld (then `verify.md`'s "stays silent" is wrong and needs rewording), or a code change that answers `0xFF` once broken (then a code-side fix plus an R-EMU-01 case on the bytes sent). `verify.md` and `src/` were outside the operator's re-expansion order, so this is the operator's call.

## Validation — 2026-10-05 (round 7, first after the second re-expansion)
- criteria: 14 passed / 0 failed, on the committed tree (9ed2fcd). The bench was re-read after the broken-frame change (see `## Bench readings`).
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- index: stale after the commit; rebuilt.
- independent review:
  - undecidable: `tests/emulator_cases.cpp` takes the expected `0xFF` for bytes past the frame's end (`faithful( )`, line 64) and after a break (`released_after( )`, line 87) from the model's own `ps2::kIdleByte`, not from a hand-written vector. The `### Emulator` preamble says the expected answers are the vectors under `tests/vectors/`. Also, "`SgModel::reset( )` … wire byte 0's answer is `0xFF`" cannot be observed: `drive( )` sets `sent[ 0 ]` itself, and the firmware loads byte 0 from `pio_device.cpp`'s `kReleasedByte`. No criterion. Confirmed by reading the file. A test that compares the model with its own constant would pass if `kIdleByte` changed, which is the shared-constant hazard of ADR-0004.
  - undecidable: `docs/wiring-emulator.md` relies on "the master's 10 kΩ pull-ups", which the spec never states. They exist (`docs/wiring.md` lines 11, 15, 42), but the reviewer had no way to know. No criterion.
  - settled: 1 — `python3 tests/test_checks_are_live.py` (`find_safety10od( )` liveness).
- closure test: fail: the two unsettled `undecidable` findings are missing pointers.
- findings: 2
- spec size: 21165 (+491 since the previous validation)
- upstream: none
- not-ours: CLAUDE.md subtracted
- verdict: returned to implementation. Finding 1 needs a test change: add a hand-written `0xFF` literal for released `DATA` to `tests/vectors/poll_exchange.h`, use it at `tests/emulator_cases.cpp` lines 64 and 87, and say so in spec step 5; cut "wire byte 0's answer is `0xFF`" from the step-5 `reset( )` bullet. Finding 2 is spec only: step 8 states that the wiring relies on the master breadboard's 330 Ω series resistors and 10 kΩ pull-ups on DATA and ACK (`docs/wiring.md`). Operator decision pending.

## Validation — 2026-10-06 (round 8)
- criteria: 14 passed / 0 failed. Only tests and docs changed since 9ed2fcd, so the 2026-10-05 bench re-read stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- index: stale (round-7 test edits); rebuilt.
- independent review: undecidable: `find_safety10od( )`'s outover pattern `gpio_set_outover\([[:space:]]*pin\.gpio,…` allows no space between the name and `(`, and the spec does not say whether other spellings must be tolerated — no criterion named. In fact R-STYLE-01 (`clang-format` over every `.cpp`, in `make test`) admits only `gpio_set_outover( pin.gpio, … )`, but the spec does not say so. (settled: 2 — `python3 tests/test_checks_are_live.py` for `find_safety10od( )` liveness; `make test` for the `# RULE R-EMU-0n` marker form in `tests/test_emulator.py`)
- closure test: fail: the unsettled `undecidable` is a missing pointer.
- findings: 1
- spec size: 21529 (+364 since the previous validation)
- upstream: none
- not-ours: CLAUDE.md subtracted
- verdict: returned to spec. Second round against this spec; findings are falling (2 → 1). Proposed fix: in step 4's `find_safety10od( )` bullet, after the call text, add "in the one spelling `clang-format` produces (R-STYLE-01)". Operator decision pending.

## Validation — 2026-10-06 (round 9)
- criteria: 14 passed / 0 failed. Only tests and docs changed since 9ed2fcd, so the 2026-10-05 bench re-read stands.
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/core/`, `src/hal/`, `src/emu/`)
- independent review:
  - undecidable: `verify.md` "How to read the rows" tells the operator the master waits at most 100 µs for an ACK. The spec never states the master's ACK timeout. No criterion. True in the tree: `kAckTimeoutUs = 100` in `src/core/ps2_protocol.h:49`, used by `src/hal/pio_port.cpp:32`.
  - undecidable: whether `make test` runs `tests/test_emulator.py`. The spec does not say that `make test` finds `tests/test_*.py` on its own. The reviewer found no deciding criterion (`make test` passes either way). True in the tree: `PY_TESTS := $(wildcard tests/test_*.py)` in `Makefile:29`, run at line 37.
  - settled: 0.
- closure test: fail: the two unsettled `undecidable` findings are missing pointers.
- findings: 2
- spec size: 21594 (+65 since the previous validation)
- upstream: `.claude/commands/validate-phase.md`. The iteration-3 escape compares raw finding counts. Over rounds 7–9 the counts were 2 → 1 → 2, and no finding recurred: each round's findings were closed and fresh reviewers surfaced different, true-in-tree gaps. The command's own `belay-debt:` names the upgrade path: compare recurring findings, not counts. /belay-feedback recommended.
- not-ours: CLAUDE.md subtracted
- verdict: escaped to /expand-phase: spec re-expanded. Third round against the second re-expanded spec, findings 2 → 1 → 2, not strictly falling. Status set to `pending`. Both findings are one clause each: the step-9 `fault late` rows name `kAckTimeoutUs` (100 µs, `src/core/ps2_protocol.h`) as the master's ACK wait; and the step-5 Check (or Acceptance criteria) names `make test`'s `tests/test_*.py` wildcard, which runs `tests/test_emulator.py`.

## Operator decision — 2026-10-06
- decision: phase closed `done` by the operator, overriding the round-9 escape. No re-expansion. This is the operator's call, recorded here as such; `/validate-phase` did not pass this phase.
- why: the escape fired twice on raw finding counts (1 → 2 → 1, then 2 → 1 → 2). No finding ever recurred, and each fresh reviewer surfaced a new, true-in-tree gap. Another re-expansion would not have changed that. The package defect is reported via /belay-feedback (`commands/validate-phase.md`, 2026-10-06); the operator fixes the package and reinstalls it separately.
- evidence at closure (round 9, tree at 2321271 plus this record):
  - 14/14 acceptance criteria pass.
  - test, lint and typecheck pass.
  - Boundary sweep clean.
  - No `contradicts` in any round since the first re-expansion.
  - Two-Pico bench re-read 2026-10-05 after the last firmware change: 7/7 rows match.
- open findings accepted, both true in the tree but not stated in the spec:
  - The master's ACK timeout that `verify.md` quotes (100 µs) is `kAckTimeoutUs` in `src/core/ps2_protocol.h:49`.
  - `make test` runs `tests/test_emulator.py` through `PY_TESTS := $(wildcard tests/test_*.py)` (`Makefile:29`).
- not done: the spec does not carry those two clauses. The non-blocking reviewer notes collected under `## For later phases` stand as recorded.

