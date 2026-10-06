# Phase 06-hil-digital — notes

## Outcome

- base: c3f7c66
- `src/core/poll.{h,cpp}` (new, pure): `kDigitalPollLen`, `kDigitalPoll` (`01 42 00 00 00`), `decode_poll( wire, completed )`, `PollTally` and `count_poll( )`. `decode_poll( )` drops wire byte 0's answer and hands `decode( )` at most an analog frame, never reading past `completed`.
- `tests/test_poll.cpp` (new): the spec's step-2 cases, built from `tests/vectors/` only. `make test` runs it through its `tests/test_*.cpp` wildcard. The step-2 mutation was run: with `decode_poll( )` reading wire byte 0 as well, 5 of the 9 checks fail and the test exits 1. The file was then restored byte for byte, and the test is green again.
- `src/app/loopback.cpp` (new): today's loopback program. Only its first comment line changed, to name `sg2hid_loopback.uf2`.
- `src/app/main.cpp` (rewritten): the link poller.
  - It polls every 1000 µs, start to start: `decode_poll( )` → `count_poll( )` → `step( )`. `elapsed_us` is the unsigned difference of two `time_us_32( )` readings.
  - Every 1000 polls it prints a `hil:` summary. On a link change it prints a `link:` line, then that poll's `T1` line.
  - It configures no pin.
- `CMakeLists.txt`: the `src/app/*.cpp` glob is gone. `add_master_firmware( target main_source )` builds `sg2hid` from `main.cpp` and `sg2hid_loopback` from `loopback.cpp`. Both get the same settings: per-source `-Wall;-Wextra;-Werror`, `ps2_master.pio`, `pico_stdlib hardware_pio`, USB stdio on, UART stdio off, and extra outputs.
- `Makefile`: the header names the three UF2s and `make hil`. New `hil` target, also added to `.PHONY`.
- `tools/hil_digital.py` (new, executable, standard library only): the seven scenarios and their recovery checks, the `master silent` timeout, the `att=high` and `us` rules, and `fault none` on exit.
- `docs/constraints.md`: R-PROTO-01's `manual:` reason names `build/pico/sg2hid_loopback.uf2`. R-PROTO-06 and R-PROTO-07 each gain a dated `**Observed 2026-10-06 (`06-hil-digital`):**` sentence; R-PROTO-07's "(inferred, not yet observed)" is removed. No binding changed.
- The flash lines in `docs/phases/24-pio-bus/verify.md:128`, `docs/phases/04-trace-mode/verify.md:155` and `docs/phases/05-emulator/verify.md:94` now copy `sg2hid_loopback.uf2`.
- `docs/phases/06-hil-digital/verify.md` (new).
- Acceptance: all 18 criteria pass on 2026-10-06. `make test` → `OK`, and the bench → `hil: PASS` (see `## Bench readings`). The firmware was rebuilt after the bench run with no source change.

## Deviations

- Spec step 3 lists the settings the two master targets share. They are written once, in a CMake `function( add_master_firmware … )` called twice, not as two copied blocks, so the targets cannot drift apart.
- Spec step 5 Check says `make hil` with no variables exits 1. The recipe does exit 1 after printing the usage line, but `make` reports any failed recipe as exit 2, and the recipe cannot change that. The acceptance criterion (`test $? -ne 0`) holds.
- Spec step 6 has the operator flash both Picos with the signal wires out. On the operator's instruction, the agent flashed them instead: the master through `picotool load -f --ser` and a copy to `RPI-RP2`, the emulator by a copy to `RPI-RP2`. The emulator was in BOOTSEL and unwired when it was flashed. No guitar was on the bench at any point (R-SAFETY-08).

## Debt

- None.

## For later phases

- **07-analog-mode**:
  - The poller's loop (`poll_once( )` in `src/app/main.cpp`) is where the config-mode sequence goes.
  - `decode_poll( )` already accepts up to a 9-byte analog poll. Only `kDigitalPoll` and its length are digital-specific.
  - `PollTally::last_payload` compares all six payload bytes, so a whammy sweep counts as payload changes. Any check meant to detect desyncs in analog mode must allow for that.
- **09-guitar-observe**: the master already prints a `T1` line for every poll that changes the link. Against the guitar, `python3 tools/trace_decode.py <master port>` shows every dropout with its per-byte ACK timing, and the firmware needs no change for that.
- **Bench hazard (any phase that rewires the two-Pico bench)**: after a replug, a wiring mistake looks exactly like a firmware fault. Every poll aborts at byte 0 with `ack-timeout`, while the emulator still answers commands. On 2026-10-06 the first `make hil` failed this way (`FAIL: sustained: state=absent payload=--`), and rewiring fixed it. Isolate it by flashing `sg2hid_loopback.uf2` and checking that seq 0 reads `5/5`. `verify.md` §If something looks wrong gives this procedure.
- **macOS accessory approval**: on this Mac, a Pico rebooted into BOOTSEL by `picotool reboot -u -f` enumerated as `RP2 Boot`, but was not reachable (no `RPI-RP2` volume, picotool saw nothing) until it was physically replugged after the "Allow accessory" approval. Once approved, `picotool load -x -f --ser <serial>` worked without a button press. In BOOTSEL the board reports a different serial (`E0C9125B0D9B`) from the one in application mode (`5303284730F33A1C`).

## Bench readings

2026-10-06. Master on `/dev/cu.usbmodem101` (`sg2hid.uf2`, USB serial `5303284730F33A1C`). Emulator on `/dev/cu.usbmodem2101` (`sg2hid_emu.uf2`). Wired per `docs/wiring-emulator.md`, no guitar.

```
$ make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101
ok: setup
ok: sustained
ok: fault ack 0
ok: fault ack 3
ok: fault late 200
ok: fault id 79
ok: fault late 50
hil: PASS
```

After the run, a capture through `tools/trace_decode.py` of `fault ack 3`, then `fault none`:

```
hil: polls=105000 refused=12001 changes=0 us=999997 state=digital fault=unknown-id att=high payload=7F FE
link: digital -> absent fault=ack-timeout
frame: 3/5 bytes, aborted at byte 3: no ACK after 133 us
  byte 0: out 01 in FF 48 us ack 11 us
  byte 1: out 42 in 41 47 us ack 10 us
  byte 2: out 00 in 5A 47 us ack 10 us
  byte 3: out 00 in -- 133 us
  byte 4: out 00 in -- not sent
hil: polls=106000 refused=13000 changes=0 us=1000049 state=absent fault=ack-timeout att=high payload=7F FE
hil: polls=107000 refused=14000 changes=0 us=999998 state=absent fault=ack-timeout att=high payload=7F FE
link: absent -> digital fault=ack-timeout
frame: 5/5 bytes, complete
  byte 0: out 01 in FF 47 us ack 10 us
  byte 1: out 42 in 41 46 us ack 9 us
  byte 2: out 00 in 5A 46 us ack 9 us
  byte 3: out 00 in 7F 46 us ack 9 us
  byte 4: out 00 in FE 36 us
hil: polls=108000 refused=14212 changes=0 us=999953 state=digital fault=ack-timeout att=high payload=7F FE
```

- `us` per 1000 polls: 999 953 to 1 000 049, well inside the 1 100 000 limit.
- The run's total `refused` was 12 001. Each fault window ran for whole seconds, so those refusals come in thousands; the odd 1 fell outside every judged window. It may be the first poll after boot or a refusal inside a discarded summary. It was not traced, and no judged window saw it.
