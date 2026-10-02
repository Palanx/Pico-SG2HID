# Phase 05-emulator — second-Pico SG emulator

<!-- Written by /expand-phase 2026-10-02, immediately before implementation (P4).
     Amendable during implementation ONLY together with a Deviations entry in notes.md.
     Operator decisions taken at expansion (2026-10-02): DATA and ACK are driven open-drain
     (ADR-0016); R-SAFETY-06 is strengthened to "no signal is an output in both tables";
     control is USB CDC line commands, and the config-mode bus sequence stays with
     07-analog-mode. -->

## Goal

A second Pico flashed with `build/pico/sg2hid_emu.uf2` behaves as a wired PS2 SG on the bus.

**On the bus.** While `ATT` is high it holds `DATA` and `ACK` released. Once selected, it shifts
out the controller's side of a poll, one wire byte per master byte, LSB first, putting each bit
on `DATA` while `CLK` is low and sampling `CMD` as `CLK` rises:

| wire byte | emulator sends | it ACKs this byte when |
|---|---|---|
| 0 | `0xFF` | the master sent `0x01` (`kFrameStart`) |
| 1 | the id: `0x41` digital, `0x73` analog | the master sent `0x42` (`kCmdPoll`) |
| 2 | `0x5A` (`kReadyByte`) | always, unless it is the frame's last byte |
| 3 … | payload bytes 0, 1, … (2 digital, 6 analog) | it is not the frame's last byte |

A frame's last byte is wire byte `frame_len( id )` (5 bytes on the wire digital, 9 analog), and it
gets no ACK. A frame whose byte 0 is not `0x01`, or whose byte 1 is not `0x42`, gets no ACK from
that byte on, so the master aborts there. A master byte past the frame's end is answered with
`0xFF` and no ACK. Each ACK is a low pulse on `ACK` that starts `kAckDelayUs` after the byte's
last rising `CLK` edge and lasts `kAckPulseUs`. Both are named in `src/emu/sg_model.h` and carry a
`belay-debt:` marker naming `09-guitar-observe`, because they are budgets, not measurements.

**Electrically.** `DATA` and `ACK` are open-drain (ADR-0016). Their pad output is forced low with
`gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`, and the PIO program changes only their
direction, so neither pin can drive high. `CMD`, `ATT` and `CLK` are inputs with no pull.

**Control.** The emulator's own USB serial port takes one command per line, terminated by `\n`
(a trailing `\r` is ignored). It answers every line with one line: `ok: <the line>` or
`error: <reason>`. A refused line changes nothing. A new line is read only while `ATT` is high,
so a command takes effect from the next frame.

| line | effect | default |
|---|---|---|
| `mode digital` / `mode analog` | the id and frame length of every later frame | digital |
| `payload h0 h1 h2 h3 h4 h5` | exactly six two-digit hex bytes (either case); digital sends the first two | `FF FF 80 80 80 80` |
| `fault none` | no fault | active |
| `fault ack <n>` | no ACK after wire byte `n`, decimal `0`–`7`; every other byte as without the fault | — |
| `fault late <us>` | every ACK starts `us` µs after its byte instead of `kAckDelayUs`, decimal `1`–`10000` | — |
| `fault id <hh>` | wire byte 1 is `hh` instead of the id; the frame length still follows the mode | — |

One fault is active at a time: a `fault` line replaces the previous one. Refused lines include,
and are not limited to: an unknown first word, a `payload` with other than six bytes, a byte that
is not two hex digits, `fault ack 8`, `fault late 0`, `fault late 10001`, and a line longer than
`kMaxLineLen` (64) characters.

**Rules.** R-SAFETY-06 moves from `planned: 05-emulator` to `test: tests/test_pin_table.py` with
the strengthened text in Plan step 3. R-SAFETY-10 is amended for open-drain `pindirs`, and
R-PROTO-07's scan narrows to `src/hal/ps2_master.pio` (step 4). Two new rules, R-EMU-01 (the
faithful answer) and R-EMU-02 (faults and commands), are bound to `tests/test_emulator.py`
(step 5).

**Unchanged.** The master firmware: `src/app/main.cpp`, `src/hal/ps2_master.pio`,
`src/hal/pio_port.cpp`, `src/hal/bus_frame.cpp`, `src/hal/bus_frame.h`, and `docs/wiring.md`. On
the bench, the 24-pio-bus loopback firmware is what drives the frames the operator reads.

## Context pointers

- `CLAUDE.md` — hardware safety rules, layering, the rule↔test binding, style.
- `docs/constraints.md` §Invariants — R-SAFETY-06, R-SAFETY-10 and R-PROTO-07 change here, and
  a new `### Emulator` section takes R-EMU-01 and R-EMU-02. §Layering: `emu` → `core`, `hal`.
- `docs/adr/0004-second-pico-as-guitar-emulator.md` — why the emulator exists, and the
  shared-`core` hazard that makes its tests compare against vectors and never against `core`.
- `docs/adr/0006-fail-safe-hardware-policy.md` — one pin table per role in `src/core/pins.h`,
  configured only by `src/hal/` iterating it.
- `docs/adr/0002-hardware-free-core.md` — `hal` owns the PIO of both bus roles.
- `docs/templates/adr.md` — the template for ADR-0016.
- `src/core/pins.h` — gains `kEmulatorPins`, two `DriveMode` members, and a table parameter for
  `gpio_of`.
- `src/core/ps2_protocol.h` — read-only: `kFrameStart`, `kCmdPoll`, `kReadyByte`, `kIdDigital`,
  `kIdAnalog`, `ControllerId`, `frame_len`, `kWireBitsPerByte`, `kBusClockHz`.
- `src/hal/bus_port.h`, `src/hal/pio_port.cpp`, `src/hal/ps2_master.pio` — read-only: the
  pattern the device port copies (SDK-free declarations; one configuring loop over the table;
  word layout documented in the `.pio` header; `recover( )`).
- `src/hal/bus_frame.h` — read-only: `exchange_frame` returns how many bytes completed, which is
  how the bench output reads.
- `src/app/main.cpp` — read-only: the master's loopback firmware and its three sequences
  (`kSeqPoll`, `kSeqPattern`, `kSeqSingle`), the frames the emulator answers on the bench.
- `tests/pin_table_cases.cpp`, `tests/test_pin_table.py` — where R-SAFETY-06's check, cases and
  copied-tree mutation land.
- `tests/test_bus_frame.py`, `tests/bus_frame_cases.cpp` — the pattern `tests/test_emulator.py`
  and `tests/emulator_cases.cpp` copy: a cases file not named `test_*.cpp`, compiled with one
  `src/` file and the `make test` flags, its lines forwarded, and copied-tree mutations that assert
  their anchor matched.
- `tests/test_repo_shape.sh` — `find_safety10( )`, `find_proto07( )`, `hits( )`, `reject( )`,
  `accept( )`, and the case floors.
- `tests/test_checks_are_live.py` — every new finder alternative and LIVE label must be
  accounted for.
- `tests/test_rule_traceability.py` — the grammar a new rule line must follow.
- `tests/vectors/README.md`, `tests/vectors/digital_idle.h`, `tests/vectors/digital_pressed.h`,
  `tests/vectors/analog_idle.h` — the expected bytes for R-EMU-01 (response with the first byte
  dropped).
- `CMakeLists.txt`, `Makefile` — the firmware build and its header comment.
- `docs/wiring.md` — the master's breadboard, which the emulator attaches to. Unchanged.
- `docs/phases/02-wiring/notes.md` §For later phases — the check functions take any
  `std::span<const PinAssignment>`.
- `docs/phases/24-pio-bus/notes.md` §For later phases — the master leaves `CMD` at its last bit
  between frames; ACK slack is about 95 µs.
- `docs/phases/24-pio-bus/verify.md`, `docs/phases/04-trace-mode/verify.md` — the shape of a
  `verify.md`, opening the serial port, and decoding `T1` lines with `tools/trace_decode.py`.

## Plan

1. **ADR-0016, "The emulator drives DATA and ACK open-drain"** — touches
   `docs/adr/0016-emulator-open-drain-outputs.md`. It decides the following: the pad output is
   forced low by `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`, the PIO program writes only
   `pindirs` for those pins, and every such `.pio` line carries `; open-drain`. It names the
   R-SAFETY-10 amendment that goes with it, and it records the rejected alternative, push-pull
   emulator outputs. Status: accepted. Check:
   `grep -q 'Status: accepted' docs/adr/0016-emulator-open-drain-outputs.md` → exit 0.

2. **Emulator pin table** — touches `src/core/pins.h`.
   - Add `DriveMode::Input` (read-only, no pull: the other Pico drives it push-pull) and
     `DriveMode::OpenDrainOutput` (pulls low or releases, output forced low).
   - Add `kEmulatorPins`, on the same GPIOs as `kMasterPins`. `DATA` and `ACK` are
     `Output`/`OpenDrainOutput`, and `CMD`, `ATT` and `CLK` are `Input`/`Input`.
   - Make `gpio_of` take `( Signal signal, std::span<const PinAssignment> table = kMasterPins )`,
     so that no existing call site changes.

   Check: `make typecheck` → exit 0; `make test` → `OK`.

3. **R-SAFETY-06** — touches `tests/pin_table_cases.cpp`, `tests/test_pin_table.py` and
   `docs/constraints.md`.
   - A check over `( kMasterPins, kEmulatorPins )` whose line FAILs when any of these holds:
     - the emulator table does not declare each of the five signals exactly once (reuse
       `check_each_signal_once`);
     - it uses a reserved or shared GPIO (reuse `check_gpios_free_and_distinct`);
     - it declares `DATA` or `ACK` `PushPull`;
     - any signal is `Direction::Output` in both tables.
   - Rejection cases through the aggregate, one per failure form: emulator `CMD` as a push-pull
     output; emulator `CLK` as an open-drain output; emulator `ATT` dropped; emulator `DATA`
     `PushPull`; emulator `CLK` on GPIO 25. Add one wiring case.
   - In `test_pin_table.py`, one copied-tree mutation: the real `kEmulatorPins` `CMD` row set to
     `Direction::Output` must turn the R-SAFETY-06 line to FAIL. Add the `RULE R-SAFETY-06` marker.
   - Rewrite the rule line as: "**R-SAFETY-06** — In the two-Pico setup, `kEmulatorPins`
     declares each of the five bus signals exactly once on a GPIO the board does not reserve,
     never declares `DATA` or `ACK` push-pull, and no signal is an output in both `kMasterPins`
     and `kEmulatorPins`." Follow it with a `**Scope, recorded 2026-10-02` clause (table data
     only: not what `src/hal/` does with the table, and not the physical wiring) and
     `— test: \`tests/test_pin_table.py\``.

   Check: `python3 tests/test_pin_table.py` → exit 0 with an `ok:` line naming R-SAFETY-06;
   `python3 tests/test_rule_traceability.py` → exit 0.

4. **Rule checks with a second `.pio` under `src/hal/`** — touches `tests/test_repo_shape.sh`
   and `docs/constraints.md`.
   - `find_proto07( )` reads only `src/hal/ps2_master.pio`. The device program has `pull` and
     `in pins` lines without the master's side-set, and those must not fail the master's rule.
     Update R-PROTO-07's Scope sentence ("`.pio` files under `src/hal/` (`src/hal/ps2_master.pio`
     today)") in the same edit.
   - `find_safety10( )`:
     - In a `.pio` under `src/hal/`, a `pindirs` line that carries `; open-drain` is excused.
     - If any such line exists while no `.cpp` under `src/hal/` contains
       `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`, it reports that.
     - Reject cases: a marked line with no outover anywhere; an unmarked `out pindirs`.
     - Accept case: a marked line with the outover present.
     - Raise the floors.
   - In the same edit, R-SAFETY-10's text changes in two ways. It says "a `src/core/pins.h` table"
     (`kMasterPins` or `kEmulatorPins`). Its Scope clause gains the excuse and what it gives up:
     the marker is not proof that the pin is open-drain, and the outover is checked for presence,
     not for each pin.

   Check: `sh tests/test_repo_shape.sh` → exit 0; `python3 tests/test_checks_are_live.py` →
   exit 0.

5. **Emulator model, SDK-free** — touches `src/emu/sg_model.h`, `src/emu/sg_model.cpp`,
   `tests/emulator_cases.cpp`, `tests/test_emulator.py`, `tests/vectors/poll_exchange.h`,
   `tests/vectors/README.md` and `docs/constraints.md`.
   - The model has `reset( )` for the start of a frame, which makes wire byte 0's answer `0xFF`.
   - Its per-byte step takes the byte the master sent and returns three things: the byte to send
     next, whether to ACK the byte just received, and the ACK delay in µs.
   - Its `apply( std::string_view line )` returns `std::expected<…>` per the Goal's grammar.
   - It includes no SDK header, and only `ps2_protocol.h` from `core`.
   - `poll_exchange.h` holds the master's poll bytes `01 42 00 00 00 00 00 00 00` and the address
     reply `0xFF`, hand-written (R-PROTO-05). The README gains one line naming it.
   - **R-EMU-01** cases. For each of the following, the bytes after the leading `0xFF` equal the
     vector: default digital = `kDigitalIdle`; `mode analog` = `kAnalogIdle`; `payload` set to
     `kDigitalPressed`'s payload bytes plus four `80` = `kDigitalPressed`. In each, every byte but
     the last is ACKed, with `kAckDelayUs`. Also: a frame starting `0x42` gets no ACK at byte 0;
     `01 43` gets no ACK at byte 1.
   - **R-EMU-02** cases:
     - `fault ack 2` withholds only byte 2's ACK.
     - `fault late 50` changes only the delay.
     - `fault id 79` changes only byte 1.
     - `fault none` restores the R-EMU-01 answer.
     - Each refused line named in §Goal returns an error and leaves the next frame identical.
   - Copied-tree mutations in `test_emulator.py`, each asserting its anchor matched: ACK on the
     last byte must turn R-EMU-01 to FAIL; the id fault ignored must turn R-EMU-02 to FAIL.
   - Add the `RULE R-EMU-01` and `RULE R-EMU-02` markers. A new `### Emulator` section in
     `docs/constraints.md` §Invariants holds both rules, worded as the Goal states them, each with
     a measured Scope clause and `— test: \`tests/test_emulator.py\``.

   Check: `python3 tests/test_emulator.py` → exit 0 with `ok:` lines for R-EMU-01 and R-EMU-02;
   `make test` → `OK`.

6. **Device port and PIO program** — touches `src/hal/device_port.h` (declarations, no SDK
   header), `src/hal/pio_device.cpp` and `src/hal/ps2_device.pio`.
   - `device_init( )` configures pins in one loop over `kEmulatorPins`, with every configuring
     call on a line naming `pin.gpio`. An `OpenDrainOutput` pin gets the outover LOW before its
     direction is ever set.
   - The program releases `DATA` before the first edge. On each falling `CLK` edge it sets
     `DATA`'s direction from the next bit (output for a 0) with `out pindirs` marked
     `; open-drain`, and it samples `CMD` on each rising edge. After eight bits it pushes the
     byte. It then pulls the next word, which holds the next byte and an ACK flag, and when the
     flag is set it pulses `ACK` with `set pindirs` marked `; open-drain`.
   - The CPU waits the ACK delay before pushing the word.
   - When `ATT` rises, the port restarts the state machine with `DATA` and `ACK` released and
     `0xFF` loaded for byte 0.
   - Every pin offset the program assumes is `static_assert`ed against `kEmulatorPins` in
     `pio_device.cpp`. The TX and RX word layouts are documented in the `.pio` header comment.

   Check: `sh tests/test_repo_shape.sh` → exit 0 (R-SAFETY-09, R-SAFETY-10, R-PROTO-07 ok); it
   compiles in step 7.

7. **Emulator firmware and build** — touches `src/emu/main.cpp`, `CMakeLists.txt` and the
   `Makefile` header comment.
   - `CMakeLists.txt` replaces the source glob with two explicit lists:
     - `sg2hid` = `src/core/*.cpp` + `src/hal/bus_frame.cpp` + `src/hal/pio_port.cpp` +
       `src/app/*.cpp`, with the `ps2_master.pio` header;
     - `sg2hid_emu` = `src/core/*.cpp` + `src/hal/pio_device.cpp` + `src/emu/*.cpp`, with the
       `ps2_device.pio` header.
   - `sg2hid_emu` gets the same `-Wall;-Wextra;-Werror` source property, USB stdio on, UART stdio
     off, and extra outputs.
   - `main.cpp` serves the bus while `ATT` is low and reads command characters only while `ATT`
     is high. It configures no pin itself (R-SAFETY-09).

   Check: `make firmware` → `build/pico/sg2hid.uf2` and `build/pico/sg2hid_emu.uf2` both exist;
   `make lint` → exit 0; `sh tests/test_firmware_flags.sh` → an `ok:` R-ERR-05 line.

8. **Two-Pico wiring** — touches `docs/wiring-emulator.md` (new).
   - A table of one row per signal, written `| SIGNAL | … |`. Each row joins the emulator's GPIO
     for that signal to the socket-side row of that signal's 330 Ω resistor on the master
     breadboard, plus GND to GND.
   - The emulator's 3V3 and VBUS/VSYS are never connected, socket pin 5 is not wired to the
     emulator, and pin 3 stays insulated (R-SAFETY-04).
   - Both Picos are powered from USB and are plugged in before the signal wires are connected.
     The doc explains why: an unpowered RP2040 is back-fed through its pins, and the 330 Ω limits
     that to under 10 mA.
   - It uses a different file from `docs/wiring.md`, because R-SAFETY-02's wiring check reads
     every `| SIGNAL | GP<n> |` row there.

   Check: `grep -cE '^\| (DATA|CMD|ATT|CLK|ACK) \|' docs/wiring-emulator.md` → 5;
   `python3 tests/test_pin_table.py` → exit 0.

9. **Bench and `verify.md`** — touches `docs/phases/05-emulator/verify.md` (headings
   `## What was built` and `## Check it yourself`, written for a non-specialist).
   - No guitar is anywhere on the bench (R-SAFETY-08). The master gets `sg2hid.uf2` and the
     emulator `sg2hid_emu.uf2`, wired per step 8, and both serial ports are open.
   - Expected readings, from the master's `loopback:` lines and their `T1` lines decoded with
     `tools/trace_decode.py`:

     | emulator state | seq 0 (`01 42 00 00 00`) | seq 1 (starts `FF`) | seq 2 (`80`) |
     |---|---|---|---|
     | defaults | all 5 bytes complete; `in` = `FF 41 5A FF FF` | stops at byte 0, `att=high` | 1/1, `in` = `FF` |
     | `mode analog` | 5/5, `in` = `FF 73 5A FF FF` | as above | as above |
     | `fault ack 2` | stops at byte 2, `att=high` | as above | as above |
     | `fault late 200` | stops at byte 0, `att=high` | as above | as above |
     | `fault late 50` | 5/5; decoded `ack` delays near 50 µs | as above | as above |
     | `fault id 79` | 5/5; second `in` byte `79` | as above | as above |
     | `fault none` | the defaults row again | as above | as above |

   - Each row's actual reading is recorded in `notes.md`.

   Check: `sh tests/test_phase_docs.sh` → exit 0, and `notes.md` holds one recorded reading per
   row.

## Acceptance criteria

```
make test                                                    # expect: last line OK, exit 0
make lint                                                    # expect: exit 0
make typecheck                                               # expect: exit 0 (also builds the firmware when PICO_SDK_PATH is set)
make firmware && test -f build/pico/sg2hid.uf2 && test -f build/pico/sg2hid_emu.uf2   # expect: exit 0
python3 tests/test_pin_table.py | grep -E '^ +ok: +R-SAFETY-06'                       # expect: one line, exit 0
python3 tests/test_emulator.py | grep -cE '^ +ok: +R-EMU-0[12]'                        # expect: 2
grep -cE '^- \*\*R-SAFETY-06\*\* .* — test: `tests/test_pin_table.py`$' docs/constraints.md   # expect: 1
grep -cE '^- \*\*R-EMU-0[12]\*\* .* — test: `tests/test_emulator.py`$' docs/constraints.md    # expect: 2
grep -c 'planned: 05-emulator' docs/constraints.md           # expect: 0
grep -q 'Status: accepted' docs/adr/0016-emulator-open-drain-outputs.md                # expect: exit 0
grep -cE '^\| (DATA|CMD|ATT|CLK|ACK) \|' docs/wiring-emulator.md                        # expect: 5
git diff --quiet main -- src/app/main.cpp src/hal/ps2_master.pio src/hal/pio_port.cpp src/hal/bus_frame.cpp src/hal/bus_frame.h docs/wiring.md   # expect: exit 0
sh tests/test_phase_docs.sh                                  # expect: exit 0
```

Plus, manual (R-PROC-02): the step-9 bench table, read on two Picos and recorded row by row in
`notes.md`.

## Out of scope

- Answering the config-mode sequence (`0x43`, `0x44`) and sweeping the whammy —
  `07-analog-mode`.
- A master that polls the emulator, an automated two-Pico run, and fault recovery —
  `06-hil-digital`.
- Any change to the master firmware, `ps2_master.pio` or `tools/trace_decode.py`, including the
  `SHIFT_US` debt in `.claude/rules/tech-debt.md` — no phase; logged debt.
- A check of `docs/wiring-emulator.md` against `kEmulatorPins` — no phase; the GPIOs mirror
  the master's.
- Widening the scope clauses of R-SAFETY-01, R-SAFETY-02 and R-SAFETY-03 to `kEmulatorPins` —
  R-SAFETY-06 covers that table.
- Measuring the real ACK delay and pulse width — `09-guitar-observe`.
- Per-frame logging on the emulator's serial port — not wanted; the master's `T1` trace shows
  the bus.
