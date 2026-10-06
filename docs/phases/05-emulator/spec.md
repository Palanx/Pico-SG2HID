# Phase 05-emulator — second-Pico SG emulator

<!-- Written by /expand-phase, immediately before implementation, never earlier (P4).
     Closure test (P5): a session — or a person — reading CLAUDE.md + this directory + the files
     pointed to below must be able to complete the phase. If either would need anything else,
     add the pointer or re-cut the phase.
     Operator decisions: DATA and ACK are driven open-drain (ADR-0016); R-SAFETY-06 is
     strengthened to "no signal is an output in both tables"; control is USB CDC line commands,
     and the config-mode bus sequence stays with 07-analog-mode. -->

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
that byte on and `0xFF` on every later wire byte, so `DATA` stays released; the master aborts
there. A master byte past the frame's end is answered with
`0xFF` and no ACK. Each ACK is a low pulse on `ACK` that starts at least `kAckDelayUs` after the
byte's last rising `CLK` edge and lasts `kAckPulseUs`. Both are named in `src/emu/sg_model.h`
(10 µs and 2 µs) and carry a `belay-debt:` marker naming `09-guitar-observe`, because they are
budgets, not measurements.

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
| `fault late <us>` | every ACK waits `us` µs instead of `kAckDelayUs`, decimal `1`–`10000` | — |
| `fault id <hh>` | wire byte 1 is `hh` instead of the id; the frame length still follows the mode | — |

One fault is active at a time: a `fault` line replaces the previous one. Refused lines include,
and are not limited to: an unknown first word, a `payload` with other than six bytes, a byte that
is not two hex digits, `fault ack 8`, `fault late 0`, `fault late 10001`, and a line longer than
`kMaxLineLen` (64) characters.

**Rules.** R-SAFETY-06 moves from `planned: 05-emulator` to `test: tests/test_pin_table.py`
(step 3). R-SAFETY-10 covers either pin table and excuses open-drain `pindirs` lines, and
R-PROTO-07's scan narrows to `src/hal/ps2_master.pio` (step 4). Two new rules, R-EMU-01 (the
faithful answer) and R-EMU-02 (faults and commands), are bound to `tests/test_emulator.py`
(step 5).

**Unchanged.** The master firmware: `src/app/main.cpp`, `src/hal/ps2_master.pio`,
`src/hal/pio_port.cpp`, `src/hal/bus_frame.cpp`, `src/hal/bus_frame.h`, and `docs/wiring.md`. On
the bench, the 24-pio-bus loopback firmware drives the frames the operator reads.

## Context pointers

Files this phase writes:

- `docs/adr/0016-emulator-open-drain-outputs.md` — the open-drain decision (step 1).
- `src/core/pins.h` — `kEmulatorPins`, two `DriveMode` members, a table parameter for `gpio_of`
  (step 2).
- `tests/pin_table_cases.cpp`, `tests/test_pin_table.py` — R-SAFETY-06's check, cases and
  copied-tree mutation (step 3).
- `docs/constraints.md` — §Invariants: R-SAFETY-06, R-SAFETY-10, R-PROTO-07, and a new
  `### Emulator` section with R-EMU-01 and R-EMU-02 (steps 3–5). §Observed conventions: one
  finding (step 6).
- `tests/test_repo_shape.sh` — `find_safety10( )`, `find_safety10od( )`, `find_proto07( )`,
  `hits( )`, `reject( )`, `accept( )`, `accept_pair( )`, `wiring( )`, `run_all`, the case floors
  (step 4).
- `src/emu/sg_model.h`, `src/emu/sg_model.cpp`, `tests/emulator_cases.cpp`,
  `tests/test_emulator.py`, `tests/vectors/poll_exchange.h`, `tests/vectors/README.md` — the
  model and R-EMU-01/02 (step 5).
- `src/hal/device_port.h`, `src/hal/pio_device.cpp`, `src/hal/ps2_device.pio` — the device port
  and PIO program (step 6).
- `src/emu/main.cpp`, `CMakeLists.txt`, `Makefile` — the emulator firmware and its build (step 7).
- `docs/wiring-emulator.md` — two-Pico wiring (step 8).
- `docs/phases/05-emulator/verify.md` — the operator's checks and bench table (step 9).

Files this phase reads only:

- `CLAUDE.md` — hardware safety rules, layering, the rule↔test binding, style.
- `docs/adr/0004-second-pico-as-guitar-emulator.md` — why the emulator exists; its tests compare
  against vectors, never against `core`.
- `docs/adr/0006-fail-safe-hardware-policy.md` — one pin table per role, configured only by
  `src/hal/` iterating it. `docs/adr/0002-hardware-free-core.md` — `hal` owns the PIO.
- `docs/templates/adr.md` — the template for ADR-0016.
- `src/core/ps2_protocol.h` — `kFrameStart`, `kCmdPoll`, `kReadyByte`, `kIdDigital`, `kIdAnalog`,
  `ControllerId`, `frame_len`, `kWireBitsPerByte`.
- `src/hal/bus_port.h`, `src/hal/pio_port.cpp`, `src/hal/ps2_master.pio`, `src/hal/bus_frame.h`,
  `src/app/main.cpp` — the master: the pattern the device port copies, and the three loopback
  sequences (`kSeqPoll`, `kSeqPattern`, `kSeqSingle`) the emulator answers on the bench.
- `tests/test_bus_frame.py`, `tests/bus_frame_cases.cpp` — the pattern `tests/test_emulator.py`
  and `tests/emulator_cases.cpp` copy.
- `tests/test_checks_are_live.py` — discovers finders and `LIVE` labels on its own; not edited.
- `tests/test_rule_traceability.py` — the grammar a rule line follows.
- `tests/vectors/digital_idle.h`, `tests/vectors/digital_pressed.h`,
  `tests/vectors/analog_idle.h` — R-EMU-01's expected bytes.
- `docs/wiring.md` — the master's breadboard the emulator attaches to.
- `docs/phases/24-pio-bus/verify.md`, `docs/phases/04-trace-mode/verify.md` — the shape of a
  `verify.md` and decoding `T1` lines with `tools/trace_decode.py`.

## Plan

1. **ADR-0016, "The emulator drives DATA and ACK open-drain"** — touches
   `docs/adr/0016-emulator-open-drain-outputs.md`. The pad output is forced low by
   `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`, the PIO program writes only `pindirs` for
   those pins, and every such `.pio` line carries `; open-drain`. It names the R-SAFETY-10 change
   and records the rejected alternative, push-pull emulator outputs. Status: accepted.

   Check: `grep -q 'Status: accepted' docs/adr/0016-emulator-open-drain-outputs.md` → exit 0.

2. **Emulator pin table** — touches `src/core/pins.h`.
   - `DriveMode::Input` (read-only, no pull: the other Pico drives it push-pull) and
     `DriveMode::OpenDrainOutput` (pulls low or releases, output forced low).
   - `kEmulatorPins`, on the same GPIOs as `kMasterPins` (2–6). `DATA` and `ACK` are
     `Output`/`OpenDrainOutput`; `CMD`, `ATT` and `CLK` are `Input`/`Input`.
   - `gpio_of( Signal signal, std::span<const PinAssignment> table = kMasterPins )`; no existing
     call site changes.

   Check: `make typecheck` → exit 0.

3. **R-SAFETY-06** — touches `tests/pin_table_cases.cpp`, `tests/test_pin_table.py` and
   `docs/constraints.md`.
   - `check_two_pico( master, emulator )` FAILs its line when the emulator table does not declare
     each of the five signals exactly once, uses a reserved or shared GPIO, declares `DATA` or
     `ACK` `PushPull`, or when any signal is `Direction::Output` in both tables.
   - Five rejection cases through the aggregate: emulator `CMD` a push-pull output, `CLK` an
     open-drain output, `ATT` dropped, `DATA` `PushPull`, `CLK` on GPIO 25. One wiring case.
   - One copied-tree mutation in `test_pin_table.py`: the real `kEmulatorPins` `CMD` row set to
     `Direction::Output` turns the R-SAFETY-06 line to FAIL. A `RULE R-SAFETY-06` marker.
   - The rule line: "`kEmulatorPins` declares each of the five bus signals exactly once on a GPIO
     the board does not reserve, never declares `DATA` or `ACK` push-pull, and no signal is an
     output in both `kMasterPins` and `kEmulatorPins`", then a `**Strengthened 2026-10-02` clause
     naming the old push-pull-only wording, then a `**Scope, recorded 2026-10-02` clause (table
     data only: not what `src/hal/` does with the table, not the physical wiring), then
     `— test: \`tests/test_pin_table.py\``.

   Check: `python3 tests/test_pin_table.py` → exit 0 with an `ok:` line naming R-SAFETY-06.

4. **Rule checks with a second `.pio` under `src/hal/`** — touches `tests/test_repo_shape.sh`
   and `docs/constraints.md`.
   - `find_proto07( )` reads only `src/hal/ps2_master.pio`. Its existing cases move to that path,
     and one accept case (`src/hal/ps2_device.pio`, `pull block`) shows the device program is not
     read. R-PROTO-07's Scope clause says it was narrowed on 2026-10-02 and why.
   - R-SAFETY-10 is checked by two finders, both reported on the single `R-SAFETY-10` line of
     `run_all`:
     - `find_safety10( )` excuses a line that starts with a PIO `set` or `out` to `pindirs` and
       carries `; open-drain`. No other line is excused by the marker.
     - `find_safety10od( )` reports a `.pio` line under `src/hal/` marked `; open-drain` while no
       `.cpp` under `src/hal/` calls `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`.
   - Reject cases: `out pindirs, 1 ; drives DATA` (a comment that is not the marker); a C call
     `pio_sm_set_pindirs_with_mask( pio0, 0, 0, 0 /* ; open-drain */ );` in a `.cpp`; for
     `find_safety10od( )`, a marked `.pio` line with no outover.
   - Accept cases: `out pindirs, 1 ; open-drain`; an indented `set pindirs, 1 ; open-drain`; and,
     through a two-file `accept_pair( )`, a marked `.pio` line with the outover call in a `.cpp`.
     False-positive floor 38. One wiring case for the outover half; wiring count 14.
   - R-SAFETY-10's text says "a `src/core/pins.h` table (`kMasterPins` or `kEmulatorPins`)" with
     an `**Amended 2026-10-02` clause, and its Scope clause names the excuse, `find_safety10od( )`,
     and what they give up: the marker is not proof that the pin is open-drain, and the outover is
     checked for presence, not for each pin.

   Check: `sh tests/test_repo_shape.sh` → exit 0; `python3 tests/test_checks_are_live.py` →
   exit 0.

5. **Emulator model, SDK-free** — touches `src/emu/sg_model.h`, `src/emu/sg_model.cpp`,
   `tests/emulator_cases.cpp`, `tests/test_emulator.py`, `tests/vectors/poll_exchange.h`,
   `tests/vectors/README.md` and `docs/constraints.md`.
   - `SgModel::reset( )` starts a frame: wire byte 0's answer is `0xFF`.
   - `step( received )` returns `ByteAnswer{ next, should_ack, ack_delay_us }`. Once byte 0 or
     byte 1 breaks the frame, `next` is `0xFF` for the rest of the frame (landed 2026-10-05).
   - `apply( std::string_view line )` returns `std::expected<void, std::string_view>` per the
     Goal's grammar.
   - It includes no SDK header, and only `ps2_protocol.h` from `core`.
   - `poll_exchange.h` holds the master's poll bytes `01 42 00 00 00 00 00 00 00` and the
     address reply `0xFF`, hand-written (R-PROTO-05). The README gains a paragraph naming it.
   - **R-EMU-01** cases: the bytes after the leading `0xFF` equal the vector for default digital
     (`kDigitalIdle`), `mode analog` (`kAnalogIdle`), and `payload` set to `kDigitalPressed`'s
     payload plus four `80` (`kDigitalPressed`); every byte but the last is ACKed with
     `kAckDelayUs`. A frame starting `0x42` gets no ACK at byte 0, and `01 43` none at byte 1; in
     both, every byte sent after the breaking byte is `0xFF`.
   - **R-EMU-02** cases: `fault ack 2` withholds only byte 2's ACK; `fault late 50` changes only
     the delay; `fault id 79` changes only byte 1; `fault none` restores the R-EMU-01 answer; a
     trailing `\r` is accepted; nine refused lines each return an error and leave the next frame
     identical.
   - Copied-tree mutations in `test_emulator.py`, each asserting its anchor matched: ACK on the
     last byte turns R-EMU-01 to FAIL; the id fault ignored turns R-EMU-02 to FAIL.
   - `RULE R-EMU-01` and `RULE R-EMU-02` markers. A new `### Emulator` section in
     `docs/constraints.md` §Invariants holds both rules, each with a Scope clause and
     `— test: \`tests/test_emulator.py\``. R-EMU-01's text says a broken frame gets `0xFF` on
     every later wire byte, and its Scope clause says the two broken-frame cases check the bytes
     sent as well as the ACK.

   Check: `python3 tests/test_emulator.py` → exit 0 with `ok:` lines for R-EMU-01 and R-EMU-02.

6. **Device port and PIO program** — touches `src/hal/device_port.h` (declarations, no SDK
   header), `src/hal/pio_device.cpp`, `src/hal/ps2_device.pio` and `docs/constraints.md`
   §Observed conventions.
   - `device_init( )` configures pins in one loop over `kEmulatorPins`, every configuring call on
     a line naming `pin.gpio`. An `OpenDrainOutput` pin's PIO direction is set to input, then
     `pio_gpio_init( )`, then the outover LOW, so its direction is never output before the
     outover. An `Input` pin gets `gpio_init( )` and `gpio_disable_pulls( )`.
   - `device_restart( )` halts the state machine, clears its FIFOs, restarts it, releases `DATA`
     and `ACK` in a second loop over `kEmulatorPins`, and loads the word for `0xFF`.
   - The program pulls a word (bit 0: ACK flag; bits 1–8: the next byte, inverted). When the flag
     is set it pulses `ACK` with `set pindirs`. It then shifts eight bits: on each falling `CLK`
     edge `out pindirs` sets `DATA`'s direction (output for a 0), and on each rising edge it
     samples `CMD`. It releases `DATA` with `mov osr, null` and `out pindirs`, and pushes the
     byte. Every `pindirs` line carries `; open-drain`.
   - `kDeviceAckPulseUs` in `src/hal/device_port.h` mirrors `kAckPulseUs`, because `hal` cannot
     include `emu`. `pio_device.cpp` `static_assert`s `CLK` = `CMD` + 2 in `kEmulatorPins` and
     `ps2_device_ACK_PULSE_CYCLES == kDeviceAckPulseUs * 8`.
   - The §Observed conventions finding: `pio_gpio_init( )` clears a pin's output override, with
     `src/hal/pio_device.cpp` as its reference file.

   Check: `sh tests/test_repo_shape.sh` → exit 0; it compiles in step 7.

7. **Emulator firmware and build** — touches `src/emu/main.cpp`, `CMakeLists.txt` and the
   `Makefile` header comment.
   - `CMakeLists.txt` builds two targets from explicit lists: `sg2hid` = `src/core/*.cpp` +
     `src/hal/bus_frame.cpp` + `src/hal/pio_port.cpp` + `src/app/*.cpp` with `ps2_master.pio`;
     `sg2hid_emu` = `src/core/*.cpp` + `src/hal/pio_device.cpp` + `src/emu/*.cpp` with
     `ps2_device.pio`. Each target sets the same `-Wall;-Wextra;-Werror` source property on every
     source in its list, USB stdio on, UART stdio off, and extra outputs.
   - `main.cpp`, while `ATT` is high, reads one command character per pass and drops any byte
     completed then, re-arming the state machine. It keeps the first `kMaxLineLen` + 2 characters
     of a line and drops the rest, so a longer line still reaches `apply( )` too long and is
     refused. While `ATT` is low it takes each byte, asks the
     model, busy-waits the ACK delay (abandoning the frame if `ATT` rises) and hands the port the
     answer. It configures no pin itself, and `static_assert`s
     `kDeviceAckPulseUs == kAckPulseUs`.

   Check: `make firmware` → `build/pico/sg2hid.uf2` and `build/pico/sg2hid_emu.uf2` both exist;
   `make lint` → exit 0.

8. **Two-Pico wiring** — touches `docs/wiring-emulator.md`.
   - One table row per signal, written `| SIGNAL | … |`, joining the emulator's GPIO to the
     socket-side row of that signal's 330 Ω resistor on the master breadboard; plus GND to GND.
   - The emulator's 3V3 and VBUS/VSYS are never connected (R-SAFETY-04).
   - Both Picos are on USB before the signal wires are connected; the doc explains back-feeding
     through the pins and the 330 Ω limit.
   - It is a separate file from `docs/wiring.md`, whose `| SIGNAL | GP<n> |` rows R-SAFETY-02
     reads.

   Check: `grep -cE '^\| (DATA|CMD|ATT|CLK|ACK) \|' docs/wiring-emulator.md` → 5.

9. **Bench and `verify.md`** — touches `docs/phases/05-emulator/verify.md` (headings
   `## What was built` and `## Check it yourself`, written for a non-specialist).
   - No guitar is on the bench (R-SAFETY-08). The master runs `sg2hid.uf2`, the emulator
     `sg2hid_emu.uf2`, wired per step 8, both serial ports open.
   - Expected readings, from the master's `loopback:` lines and their `T1` lines decoded with
     `tools/trace_decode.py`:

     | emulator state | seq 0 (`01 42 00 00 00`) | seq 1 (starts `FF`) | seq 2 (`80`) |
     |---|---|---|---|
     | defaults | all 5 bytes complete; `in` = `FF 41 5A FF FF` | `bytes=0/9`: the master's 9-byte pattern stops at byte 0, `att=high` | 1/1, `in` = `FF` |
     | `mode analog` | 5/5, `in` = `FF 73 5A FF FF` | as above | as above |
     | `fault ack 2` | stops at byte 2, `att=high` | as above | as above |
     | `fault late 200` | stops at byte 0, `att=high` | as above | as above |
     | `fault late 50` | 5/5; decoded `ack` delays near 50 µs | as above | as above |
     | `fault id 79` | 5/5; second `in` byte `79` | as above | as above |
     | `fault none`, then `mode digital` | the defaults row again | as above | as above |

   - Each row's actual reading, taken with `sg2hid_emu.uf2` built after step 5's broken-frame
     change, is recorded in `notes.md`.

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
python3 tests/test_checks_are_live.py                        # expect: exit 0 (every finder, find_safety10od( ) included, is mutation-live)
```

Plus, manual (R-PROC-02): the step-9 bench table, read on two Picos and recorded row by row in
`notes.md`.

## Out of scope

- Answering the config-mode sequence (`0x43`, `0x44`) and sweeping the whammy —
  `07-analog-mode`.
- A master that polls the emulator, an automated two-Pico run, and fault recovery —
  `06-hil-digital`.
- Any change to the master firmware, `ps2_master.pio` or `tools/trace_decode.py` — no phase.
- A check of `docs/wiring-emulator.md` against `kEmulatorPins` — no phase; the GPIOs mirror
  the master's.
- Widening the scope clauses of R-SAFETY-01, R-SAFETY-02 and R-SAFETY-03 to `kEmulatorPins` —
  R-SAFETY-06 covers that table.
- Measuring the real ACK delay and pulse width — `09-guitar-observe`.
- Per-frame logging on the emulator's serial port — not wanted; the master's `T1` trace shows
  the bus.
