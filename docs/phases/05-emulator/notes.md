# Phase 05-emulator — notes

## Outcome

- base: 2d3a529 (working tree; nothing committed yet)

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
- **Validation 2026-10-05, reviewer taste (not blocking):**
  - `verify.md` part 1 says "Eight bad command lines"; `kRefused` in `tests/emulator_cases.cpp` and R-EMU-02's Scope both say nine. Fixed in round 2.
  - `tests/pin_table_cases.cpp` reflows the 11 existing `Rejection` initialisers (diff noise only).
  - `next_word( )` accepts repeated and leading spaces; `main.cpp` drops a byte completed while `ATT` is high. Neither is in the spec's grammar, neither is forbidden.

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
