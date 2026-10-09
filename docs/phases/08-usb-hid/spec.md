# Phase 08-usb-hid — the master is a driverless USB HID gamepad fed by the emulator

<!-- Written by /expand-phase, immediately before implementation, never earlier
     (P4). Amendable during implementation ONLY together with a Deviations
     entry in notes.md.
     Closure test (P5): a session — or a person — reading CLAUDE.md + this
     directory + the files pointed to below must be able to complete the
     phase. If either would need anything else, add the pointer or re-cut the
     phase. -->

## Goal

The master firmware (`build/pico/sg2hid.uf2`) enumerates as one composite USB device with two
interfaces: the CDC serial port it already has, unchanged in behaviour, and a HID gamepad. Each
poll, the master computes one HID report from the frame it just exchanged and hands it to the
HID interface. Fed by the emulator, macOS and Linux show the five frets, both strums, start,
select, tilt and a continuous whammy axis in a gamepad tester with no driver installed.

Operator decisions, 2026-10-09:

- CDC + HID composite: the serial port keeps `make hil`, `link:` and `T1` trace working;
- while the link is not `AnalogStreaming`, the host gets the **neutral report**: no button set,
  whammy `kWhammyRest`;
- the whammy is the Generic Desktop X usage;
- the harness checks the report bytes through the summary line; the gamepad-tester pass is
  manual, on macOS and on Linux. The Linux host is the operator's Steam Deck (SteamOS), not
  Debian: the same kernel path (`usbhid`, `hid-generic`), a different distribution.

**USB identity** (ADR-0003, unchanged): VID `0x2E8A`, PID `0x000A`; manufacturer string
`pico-sg2hid`; product string `SG2HID Guitar`; serial number the board's unique id string. The
device descriptor declares the IAD composite class (`TUSB_CLASS_MISC`, `MISC_SUBCLASS_COMMON`,
`MISC_PROTOCOL_IAD`). One configuration, bus-powered, two interfaces: CDC (interfaces 0 and 1,
notification endpoint `0x81`, data endpoints `0x02` and `0x82`), then HID (interface 2, one
interrupt IN endpoint `0x83`, `bInterval` 1 ms, no OUT endpoint, no boot protocol).

**The HID report descriptor** lives in `src/core/hid_descriptor.h` as `kHidReportDescriptor`,
a `constexpr std::array<std::uint8_t, N>` built from `src/core/hid_report.h`'s constants. Its
items, in order:

| bytes | item | derived from |
|---|---|---|
| `05 01` `09 05` `A1 01` | Usage Page Generic Desktop, Usage Game Pad, Collection Application | — |
| `05 09` `19 01` `29 0A` | Usage Page Button, Usage Minimum 1, Usage Maximum 10 | `kButtonCount` |
| `15 00` `25 01` `75 01` `95 0A` `81 02` | Logical 0..1, Report Size 1, Report Count 10, Input Data,Var,Abs | `kButtonCount` |
| `75 01` `95 06` `81 03` | Report Size 1, Report Count 6, Input Const,Var,Abs (padding) | `kButtonBytes * kBitsPerByte - kButtonCount` |
| `05 01` `09 30` | Usage Page Generic Desktop, Usage X | — |
| `15 00` `26 FF 00` `75 08` `95 01` `81 02` | Logical 0..255, Report Size 8, Report Count 1, Input Data,Var,Abs | — |
| `C0` | End Collection | — |

44 bytes. No report id. A `static_assert` refuses a padding count of 0 (a button count that is a
multiple of 8 needs the padding items removed, not a Report Count 0). The input report is
`kReportLen` (3) bytes: `build_report( )`'s output, unchanged.

**The report for a frame** is `report_for( now, outcome )` in `src/core/poll.h`:
`build_report( map_frame( *outcome ) )` when `now` is `AnalogStreaming` and the outcome has a
value, else the neutral report.

**The master firmware:**

- initialises TinyUSB before stdio, and runs the TinyUSB device task while it waits for the next
  poll period;
- after each poll, stores `report_for( now, outcome )` and sends it when the HID interface is
  ready (a report is dropped, never queued, when it is not);
- answers a host GET_REPORT with the last stored report;
- prints the summary line with one more trailing field, `report=<XX XX XX>`: the last stored
  report, `kReportLen` uppercase hex bytes separated by single spaces.

**The harness** `tools/hil_digital.py` keeps its name, rules, windows and scenarios. Its
`SUMMARY` regex gains `report`, and:

| check | report it requires |
|---|---|
| `expect_streaming` (`sustained`, the recovery check that ends each `fault …` scenario, `fault late 50`) | `00 02 80` (setup payload `7F FE`: tilt pressed) |
| `sweep`, after each value `<V>` | `00 02 <V>` |
| `expect_dropped` (`fault ack 0`, `fault ack 3`, `fault late 200`, `fault id 79`) | `00 00 80` |
| `expect_declined` (`fault decline`) | `00 00 80` |

A new scenario, `buttons`, runs after `sweep`. For each row below, in order, it sends
`payload <b0> <b1> 80 80 80 80` and, over a 1-summary window, requires `state=analog`, the
report shown, and Δrefused = 0. It ends by sending the setup payload line again. The table is
hand-written in the harness, never computed from `core`:

| control | `b0 b1` | report |
|---|---|---|
| green | `ff bf` | `01 00 80` |
| red | `ff df` | `02 00 80` |
| yellow | `ff ef` | `04 00 80` |
| blue | `ff 7f` | `08 00 80` |
| orange | `ff fb` | `10 00 80` |
| strum up | `ef ff` | `20 00 80` |
| strum down | `bf ff` | `40 00 80` |
| start | `f7 ff` | `80 00 80` |
| select | `fe ff` | `00 01 80` |
| tilt | `ff fe` | `00 02 80` |

A new mode, `--demo`, needs only `--emu`. It sends `fault none`, `mode digital` and
`payload ff ff 80 80 80 80`, then presses each control of the `buttons` table for `DEMO_HOLD_S`
(2 s), printing `pressed: <control>` first, then sends the whammy values `00`, `40`, `80`,
`C0`, `FF` for `DEMO_HOLD_S` each, printing `whammy: <V>`, and ends by sending
`payload ff ff 80 80 80 80` and `fault none`. It judges nothing and exits 0 unless the emulator
refuses a line or a port fails. Without `--demo`, `--master` is still required.

**Rules** added to `docs/constraints.md` under a new `### USB HID` section, both bound
`test: tests/test_ps2_codec.py`:

- **R-HID-01** — the HID report descriptor equals the hand-written vector
  `tests/vectors/hid_descriptor.h`: ten buttons from bit 0, padding to the byte, then the whammy
  as Generic Desktop X, 0..255, at byte `kWhammyOffset`.
- **R-HID-02** — while the link is not `AnalogStreaming`, the report is the neutral report.

## Context pointers

- `CLAUDE.md` — layering (`usb` → `core` only; `app` → `core`, `hal`, `usb`), hardware safety,
  conventions.
- `docs/constraints.md`:
  - §Layering, and §Invariants R-ARCH-01..03, R-ERR-02, R-ERR-04, R-CLEAN-04, R-STYLE-02,
    R-PROTO-04, R-PROTO-05, R-SAFETY-08, R-SAFETY-09, R-PROC-01, R-PROC-02;
  - §Error handling: the firmware never stops; every error path returns to "absent, retry".
- `docs/adr/0003-generic-hid-gamepad-own-identity.md` — the control set and the USB identity.
- `docs/adr/0017-analog-negotiation.md` — the newest ADR, whose shape ADR-0018 copies.
- `src/core/hid_report.h`, `src/core/hid_report.cpp` — `Button`, `kButtonCount`,
  `kButtonBytes`, `kBitsPerByte`, `kWhammyOffset`, `kReportLen`, `HidReport`, `build_report( )`.
- `src/core/guitar_state.h` — `GuitarState`, `map_frame( )`, `kWhammyRest`, and the payload
  masks the `buttons` table was computed from (active low).
- `src/core/poll.h`, `src/core/poll.cpp` — where `report_for( )` goes; `DecodeOutcome` comes
  from `src/core/ps2_frame.h`, `LinkState` from `src/core/link.h`.
- `src/app/main.cpp` — the poller: `Poller`, `poll_once( )`, `print_summary( )`, `main( )`.
- `CMakeLists.txt` — `add_master_firmware( )`, which builds both `sg2hid` and `sg2hid_loopback`;
  only `sg2hid` gains USB HID.
- `$PICO_SDK_PATH/src/rp2_common/pico_stdio_usb/include/pico/stdio_usb.h` and
  `stdio_usb.c` — linking `tinyusb_device` sets `LIB_TINYUSB_DEVICE`, which turns off the SDK's
  own descriptors, its `tusb_init( )` call (it then asserts `tud_inited( )`) and its background
  task; stdio keeps using CDC interface 0.
- `$PICO_SDK_PATH/lib/tinyusb/examples/device/hid_composite/src/` (`usb_descriptors.c`,
  `tusb_config.h`) and `$PICO_SDK_PATH/lib/tinyusb/examples/device/cdc_msc/src/usb_descriptors.c`
  — the descriptor callbacks, `TUD_CDC_DESCRIPTOR`, `TUD_HID_DESCRIPTOR`, and the config macros.
  TinyUSB is 0.18.0 in the pinned SDK.
- `$PICO_SDK_PATH/lib/tinyusb/src/class/hid/hid_device.h` — `tud_hid_ready( )`,
  `tud_hid_report( )`, and the three callbacks the application must define.
- `tests/ps2_codec_cases.cpp`, `tests/test_ps2_codec.py` — where the R-HID cases, rule lines
  (`report( is_ok, "R-… (…)" )`), `RULE` markers and `MUTATIONS` go; the existing `hid:` cases.
- `tests/vectors/README.md`, `tests/vectors/analog_whammy_full.h`, `tests/vectors/unknown_id.h`
  — the vector convention, and the frames the R-HID-02 cases decode.
- `tests/test_style.sh` — lints `src/usb/` with the firmware's compile database
  (`build/pico/compile_commands.json`).
- `tools/hil_digital.py` — the harness.
- `docs/phases/07-analog-mode/spec.md` §Goal — the summary line's current fields and the
  scenario table this phase extends.
- `docs/phases/07-analog-mode/notes.md` §For later phases and §Bench readings — the
  `Poller` hook for the report, and the bench ports and procedure.
- `docs/phases/07-analog-mode/verify.md` — the flashing and port-finding procedure this phase's
  `verify.md` reuses, and its shape (two R-PROC-02 headings).
- `docs/phases/06-hil-digital/notes.md` §For later phases — the macOS accessory approval.
- `Makefile` — the header comment for `make firmware` and `make hil`.
- `.claude/workflow/carry-over-exempt` — lists `Makefile`, `tests/test_ps2_codec.py` and
  `tools/hil_digital.py`, which this phase changes; each line is re-checked when its file
  changes.

## Plan

1. **ADR.** Write `docs/adr/0018-usb-composite-hid.md`. It records: one composite CDC + HID
   device under ADR-0003's identity; the report descriptor in `core`, derived from
   `hid_report.h`, so a host test reads it; the neutral report while the link is not
   `AnalogStreaming`; the whammy as Generic Desktop X; a report sent each poll when the
   interface is ready and dropped otherwise; and the Linux bench host being SteamOS, not Debian.

   Check: `test -f docs/adr/0018-usb-composite-hid.md` → exit 0.

2. **Descriptor and R-HID-01.**
   - New `src/core/hid_descriptor.h`: the HID item prefixes and usages as named constants, and
     `kHidReportDescriptor` as in §Goal's table, with the padding `static_assert`.
   - New `tests/vectors/hid_descriptor.h`: the same 44 bytes as a literal, one comment per item,
     written by hand from §Goal's table. `tests/vectors/README.md` gains a paragraph: these are
     USB bytes the device sends the host, written from the HID 1.11 item encoding.
   - `tests/ps2_codec_cases.cpp`: one case comparing the two byte for byte, and the rule line
     `R-HID-01 (…)`.
   - `tests/test_ps2_codec.py`: a `RULE R-HID-01` docstring marker, and one mutation in
     `src/core/hid_descriptor.h` that changes the X usage to `0x31` (Y).
   - `docs/constraints.md`: the `### USB HID` section and R-HID-01.

   Check: `make typecheck` → exit 0; `python3 tests/test_ps2_codec.py | grep -c 'ok: .*R-HID-01'`
   → 1; `python3 tests/test_rule_traceability.py` → exit 0.

3. **`report_for( )` and R-HID-02.**
   - `src/core/poll.{h,cpp}`: `[[nodiscard]] HidReport report_for( LinkState now,
     const DecodeOutcome& outcome )` as in §Goal.
   - `tests/ps2_codec_cases.cpp` cases:
     - `kAnalogWhammyFull` with `now = AnalogStreaming` → `build_report( map_frame( … ) )`;
     - the same frame with `now` `Negotiating`, then `Absent` → the neutral report;
     - a refused outcome (`kUnknownId`) with `now = Absent` → the neutral report;
     - the rule line `R-HID-02 (…)`, aggregating the three neutral cases.
   - `tests/test_ps2_codec.py`: a `RULE R-HID-02` marker, and one mutation that drops the
     `now == AnalogStreaming` condition from `report_for( )`.
   - `docs/constraints.md`: R-HID-02.

   Check: `make test` → `OK`; `python3 tests/test_ps2_codec.py | grep -c 'ok: .*R-HID-02'` → 1.

4. **`usb` layer.** New files under `src/usb/`, namespace `usb`:
   - `tusb_config.h`: device mode on root port 0, `CFG_TUD_CDC 1`, `CFG_TUD_HID 1`,
     `CFG_TUD_MSC 0`, `CFG_TUD_MIDI 0`, `CFG_TUD_VENDOR 0`, HID endpoint buffer at least
     `kReportLen`;
   - `usb_device.h`: `void init( )` (calls `tusb_init( )`), `void task( )` (calls
     `tud_task( )`), `void send_report( const ps2::HidReport& report )` (stores it; sends it
     with `tud_hid_report( 0, … )` only when `tud_hid_ready( )`);
   - `usb_device.cpp`: those three, plus the TinyUSB callbacks with C linkage — device,
     configuration, string and HID report descriptor (`kHidReportDescriptor`), GET_REPORT
     (copies the stored report), SET_REPORT (ignored) — all per §Goal's identity.

   `CMakeLists.txt`: `sg2hid` alone gains the two `src/usb/` sources (with the same
   `-Wall;-Wextra;-Werror` source property), `src/usb` as an include directory, and
   `tinyusb_device` plus `pico_unique_id`. `sg2hid_loopback` and `sg2hid_emu` are unchanged.

   Check: `make firmware` → exit 0; `make lint` → exit 0;
   `grep -rlE '#include "(hal|app)/' src/usb | wc -l` → 0.

5. **Master firmware.** `src/app/main.cpp`:
   - `main( )` calls `usb::init( )` before `stdio_init_all( )`, and `usb::task( )` inside the
     wait for the next poll period;
   - `Poller` gains `ps2::HidReport report`, initialised to the neutral report;
   - `poll_once( )` stores `report_for( now, outcome )` and calls `usb::send_report( )`;
   - `print_summary( )` appends `report=`;
   - the header comment says the master is also the HID gamepad.

   `Makefile`'s header comment for `make firmware` and `make hil` names this phase.

   Check: `make firmware` → exit 0; `make lint` → exit 0;
   `grep -c 'report=' src/app/main.cpp` → ≥ 1.

6. **Harness.** `tools/hil_digital.py`: the `report` field, the report checks, the `buttons`
   scenario and `--demo`, as in §Goal; the docstring points at this spec.

   Check: `python3 tools/hil_digital.py --help` → exit 0;
   `python3 tools/hil_digital.py --emu /dev/null 2>&1; test $? -ne 0` → exit 0 (no `--master`,
   no `--demo` → refused).

7. **Carry-over list.** Re-read the `.claude/workflow/carry-over-exempt` lines for `Makefile`,
   `tests/test_ps2_codec.py` and `tools/hil_digital.py`. Each must still name a phase path only
   in a comment or docstring and open none; otherwise delete the line.

   Check: `grep -n 'spec.md\|notes.md' Makefile tests/test_ps2_codec.py tools/hil_digital.py` →
   every hit is a comment or docstring (read by hand, recorded in `notes.md`).

8. **Bench, two Picos, no guitar (R-SAFETY-08).** Flash `sg2hid.uf2` to the master and
   `sg2hid_emu.uf2` to the emulator. The master's serial port name may change now that it is a
   composite device: list `/dev/cu.usbmodem*` again.
   - `make hil MASTER=… EMU=…` → `hil: PASS`.
   - macOS: `hidutil list --matching '{"VendorID":0x2E8A,"ProductID":0x000A,"PrimaryUsagePage":1,"PrimaryUsage":5}'`
     lists a line containing `SG2HID Guitar`.
   - macOS, then the Steam Deck (Desktop Mode, master plugged into the Deck): open a browser
     gamepad tester, run `python3 tools/hil_digital.py --demo --emu …`, and confirm each
     `pressed:` control lights its own button and each `whammy:` value moves axis 0. On the
     Deck, `ls /dev/input/by-id/ | grep -i sg2hid` shows an `-event-joystick` entry.

   Record in `notes.md` §Bench readings: one `hil:` line from `sustained` and one from
   `buttons`; the `hidutil` line; the `/dev/input/by-id` line; for each host, the tester's
   button index per control and the axis readings at `00`, `80`, `FF`.

9. **`docs/phases/08-usb-hid/verify.md`**, for a non-specialist, with the two R-PROC-02
   headings. It explains what a HID report descriptor is and why no driver is needed; what a
   composite device is and why the serial port stays; what the neutral report is for. It gives
   §Plan step 8's procedure step by step, including how to find each port on macOS and on the
   Deck, Desktop Mode, the macOS accessory approval, that a browser gamepad tester shows a
   device only after its first button press, and that the Steam client may hold the device
   (quit Steam if the tester shows nothing).

   Check: `grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' docs/phases/08-usb-hid/verify.md`
   → 2.

## Acceptance criteria

Run in order. The last three need the bench.

```
make test 2>&1 | tail -n 1                                       # expect: OK
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
make firmware && test -f build/pico/sg2hid.uf2 && test -f build/pico/sg2hid_emu.uf2 && test -f build/pico/sg2hid_loopback.uf2   # expect: exit 0
python3 tests/test_ps2_codec.py | grep -c 'ok: .*R-HID-0[12]'    # expect: 2
python3 tests/test_ps2_codec.py | grep -c 'FAIL'                 # expect: 0
grep -c '^- \*\*R-HID-0[12]\*\*' docs/constraints.md             # expect: 2
python3 tests/test_rule_traceability.py                          # expect: exit 0
test -f docs/adr/0018-usb-composite-hid.md                       # expect: exit 0
test -f tests/vectors/hid_descriptor.h                           # expect: exit 0
grep -rl 'tests/vectors' src | wc -l                             # expect: 0
grep -c 'tinyusb_device' CMakeLists.txt                          # expect: >= 1
grep -rlE '#include ["<](tusb|class/|device/)' src/core | wc -l   # expect: 0
grep -rlE '#include "(hal|app)/' src/usb | wc -l                 # expect: 0
grep -c 'report=' src/app/main.cpp tools/hil_digital.py | grep -c ':0$'   # expect: 0
git diff --quiet main -- src/hal src/emu src/app/loopback.cpp    # expect: exit 0
python3 tools/hil_digital.py --help >/dev/null                   # expect: exit 0
make hil >/dev/null 2>&1; test $? -ne 0                          # expect: exit 0 (no ports → refused)
grep -n 'spec.md\|notes.md' Makefile tests/test_ps2_codec.py tools/hil_digital.py   # expect: every hit a comment or docstring naming a path; none opens one (read by hand)
grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' docs/phases/08-usb-hid/verify.md   # expect: 2
make hil MASTER=<master port> EMU=<emulator port> | tail -n 1     # expect: hil: PASS (bench; exit 0)
hidutil list --matching '{"VendorID":0x2E8A,"ProductID":0x000A,"PrimaryUsagePage":1,"PrimaryUsage":5}' | grep -c 'SG2HID Guitar'   # expect: >= 1 (bench, macOS, master plugged in)
grep -c 'event-joystick' docs/phases/08-usb-hid/notes.md         # expect: >= 1 (the Steam Deck reading, recorded with the two testers' readings in §Bench readings)
```

## Out of scope

- Latency measurement and the end-to-end "inside one USB poll interval" claim —
  `10-guitar-full`.
- The real guitar, and whether the whammy sits on payload index 5 — `09-guitar-observe`.
- Rescaling or calibrating the whammy (rest value, dead zone) — not wanted; the byte passes
  straight through (`hid_report.cpp`).
- A Debian install, Windows, and host-side mapping software — ADR-0003, ADR-0010,
  requirements §Non-goals.
- Changing the emulator, `src/hal/`, or `sg2hid_loopback`.
- Sending reports only on change, report ids, a second HID interface, a boot protocol, an OUT
  endpoint, or remote wakeup.
- Renaming `tools/hil_digital.py` (R-PROTO-06 and R-PROTO-07 cite the name).
- The USB reset interface the SDK's default descriptors carry (`picotool reboot` over USB);
  flashing stays BOOTSEL by hand.
- A per-suite key for the mutant cache, and the three open tech-debt entries in
  `.claude/rules/tech-debt.md`.
- Editing `docs/product/requirements.md`, whose R1 says Debian — an operator decision, raised
  at expansion.
