# Phase 08-usb-hid — notes

## Outcome

- base: `4c9db70` (the expansion commit; this phase's work is uncommitted in the working tree)
- `src/core/hid_descriptor.h` (new): the HID item prefixes and data as named constants, and
  `kHidReportDescriptor`, the 44-byte report descriptor built from `hid_report.h`'s constants, with
  the padding `static_assert`.
- `src/core/poll.{h,cpp}`: `neutral_report( )` and `report_for( now, outcome )` — the frame's report
  while the link is `AnalogStreaming`, the neutral report (`00 00 80`) otherwise.
- `src/usb/` (new layer, namespace `usb`): `tusb_config.h` (device mode, CDC + HID),
  `usb_device.{h,cpp}` with `init( )`, `task( )`, `send_report( )` and the TinyUSB callbacks:
  device descriptor (IAD composite, VID `0x2E8A`, PID `0x000A`), configuration (CDC on interfaces 0–1,
  endpoints `0x81`/`0x02`/`0x82`; HID on interface 2, IN endpoint `0x83`, 1 ms), strings
  (`pico-sg2hid`, `SG2HID Guitar`, the board's unique id), the report descriptor, GET_REPORT (the
  stored report) and SET_REPORT (ignored).
- `CMakeLists.txt`: `sg2hid` alone links `tinyusb_device` and `pico_unique_id` and compiles
  `src/usb/usb_device.cpp`; `sg2hid_loopback` and `sg2hid_emu` are unchanged.
- `src/app/main.cpp`: `usb::init( )` before `stdio_init_all( )`; `usb::task( )` in the wait between
  polls; `Poller::report`; `poll_once( )` stores `report_for( )` and sends it; the summary line ends in
  `report=XX XX XX`.
- `tools/hil_digital.py`: `report` in `SUMMARY`; the report checks per §Goal's table; the `buttons`
  scenario after `sweep`; `--demo` (only `--emu` needed; without it `--master` is still required).
- Rules R-HID-01 and R-HID-02 under a new `### USB HID` section of `docs/constraints.md`, bound to
  `tests/test_ps2_codec.py`: two rule lines, four new `hid:` cases, two mutations (6/6 caught).
- `tests/vectors/hid_descriptor.h` and a paragraph in `tests/vectors/README.md`.
- `docs/adr/0018-usb-composite-hid.md`; `docs/phases/08-usb-hid/verify.md`; the `Makefile` header.
- Acceptance: every non-bench criterion passes (`make test` OK, `make lint`, `make typecheck`,
  `make firmware`, the greps, `--help`, `make hil` refused without ports). The carry-over grep
  (`grep -n 'spec.md\|notes.md' Makefile tests/test_ps2_codec.py tools/hil_digital.py`) has four
  hits, read by hand: `tools/hil_digital.py:11–13` (docstring, naming the 06/07/08 specs) and
  `tests/test_ps2_codec.py:129` (comment); none opens a file. The `Makefile` has no hit; its
  carry-over line stays as it was. **Owed: the three bench criteria** (Plan step 8) — they need
  the operator, the two Picos, the Mac and the Steam Deck.

## Deviations

- Plan step 3 named only `report_for( )`; `neutral_report( )` was added beside it in `poll.h`,
  because `Poller::report` (Plan step 5) and the USB layer's stored report both need the neutral
  report before the first poll, and `report_for( )` uses it. No literal restates it anywhere.
- Plan step 3's R-HID-02 mutation "drops the `now == AnalogStreaming` condition": written as
  `( true || now == LinkState::AnalogStreaming )`, because dropping the clause leaves `now` unused,
  which `-Werror` refuses before any rule line prints (the same reason as R-PROTO-10's `false &&`).
- Plan step 4 did not say how `make lint` treats TinyUSB's headers: the SDK adds
  `lib/tinyusb/src` as a plain include directory, and `.clang-tidy`'s unanchored
  `HeaderFilterRegex: '(src|tests)/.*'` matches `…/tinyusb/src/…`, so lint failed on TinyUSB's own
  magic numbers. `CMakeLists.txt` re-adds that directory to `sg2hid` as `SYSTEM`, which the compiler
  keeps over the plain one; TinyUSB is then out of lint's reach, like the rest of the SDK.
- R-CLEAN-02 (three parameters at most) and TinyUSB's mandatory `tud_hid_get_report_cb` /
  `tud_hid_set_report_cb` (five each) cannot both hold. Both callbacks carry
  `// NOLINTNEXTLINE(readability-function-size)` naming TinyUSB, and R-CLEAN-02 in
  `docs/constraints.md` gains an **Exception** sentence saying so — the first `NOLINT` in the repo.
  The rule's binding (`tests/test_style.sh`) is unchanged. **Operator: this is a rule amendment;
  reject it at validation if a different shape is wanted** (e.g. a `.c` file, which R-CLEAN-02's
  scope leaves unchecked, was rejected as evasion).
- `kHidReportDescriptor` is wrapped in `// clang-format off` / `on`, so each item stays one line
  (prefix and data together); clang-format otherwise puts every byte on its own line.
- `src/core/hid_descriptor.h`'s header comment does not name `tests/vectors/hid_descriptor.h`:
  R-PROTO-05 refuses the path `tests/vectors` anywhere under `src/`, comments included. It says
  "a hand-written vector (R-HID-01)".
- Acceptance criterion `python3 tests/test_ps2_codec.py | grep -c 'FAIL'  # expect: 0` was
  impossible as written: the passing line `ok:   rejection cases: 6/6 (each mutation flips its own
  rule to FAIL), …` contains the word. Amended in `spec.md` to `grep -c 'FAIL:'`, which only a
  failing line prints. Checked: no other statement in the spec counts `FAIL`.
- `tools/hil_digital.py`: the emulator round-trip moved out of `Harness.send( )` into a module
  function `command( emu, text )`, so `--demo` can talk to the emulator with no master port.
  `expect_late_kept` (`fault late 50`) checks `report=00 02 80`, as §Goal's table lists
  `fault late 50` in the streaming row.
- Plan step 8 asks to record "one `hil:` line from `sustained` and one from `buttons`", but the
  harness prints no summary lines. `verify.md` step 9 takes them by hand after the run with
  `cat $MASTER`: one on the setup payload (`sustained`'s state), one after
  `payload ff bf 80 80 80 80` (`buttons`' first row).
- `verify.md` step 9.1 (written this phase) said the harness left the emulator on its setup
  payload, but steps 7–8 run the demo, which ends on `payload ff ff …`; found on the bench. Amended,
  with the operator's approval: 9.1 now restores `payload 7f fe …` before copying the line, and
  says `refused` / `fault` count from power-up. Checked: no other step of `verify.md` assumes the
  emulator's state after the demo (9.2 sets its own payload, 9.3 restores the setup one).
- §Out of scope excluded editing `docs/product/requirements.md` (R1 named Debian) as an operator
  decision. After the bench the operator decided it: the Linux target is the Steam Deck, not Debian.
  On the operator's order, recorded as ADR-0019, and Debian replaced by SteamOS in
  `requirements.md` (Users, R1), `CLAUDE.md`'s summary line and `PHASES.md`'s feature summary. The
  Debt entry "Debian not run on the bench" is removed: ADR-0019 closes it. Not edited: ADR-0003 and
  ADR-0018 (immutable; their Debian mentions are context, and ADR-0019 names the debt it closes),
  `docs/phases/01-ps2-codec/verify.md` (a record of its time); this phase's `spec.md` was
  amended at validation (next entry).
- Validation round 1 (2026-10-09), spec amended, no code changed:
  - closure test: `docs/adr/0019-linux-target-steam-deck.md` was in the diff but named nowhere in the
    spec, and §Out of scope still excluded editing `docs/product/requirements.md`. The Out-of-scope
    bullet is deleted; Plan step 10 names ADR-0019 and the three Debian→SteamOS edits.
  - undecidable: §Goal's `--demo` exit conditions ("exits 0 unless the emulator refuses a line or a
    port fails") did not say whether an emulator answer timeout counts. Cut back to "It judges
    nothing."
  - undecidable: Plan step 9's "how to find each port on macOS and on the Deck" — no serial port is
    needed on the Deck. Cut to "each serial port on macOS".
  - Reconciliation checked: §Goal's operator decisions and Plan step 1 (SteamOS as the Linux host,
    consistent), §Out of scope's "A Debian install" (still true), Plan step 6's check and step 8's
    demo run (assert no exit condition), the Debian deviation above (its "spec.md not edited" clause
    was stale, corrected).
- Validation round 2 (2026-10-09), spec amended, no code changed: Plan step 3 said the R-HID-02 rule
  line aggregates "the three neutral cases"; it aggregates all four (the streaming case too), as
  R-HID-02's Scope in `docs/constraints.md` states — without it an always-neutral `report_for( )`
  passes the rule. Classified spec-side; Plan step 3 now says "the four cases above". Reconciliation
  checked: §Goal's R-HID-02 statement (the rule, not the line's cases; unchanged), Plan step 3's
  mutation and §Acceptance's `R-HID-0[12]` count (unaffected); none other names the case count.
- operator closed with open finding: §Plan CMakeLists.txt — Plan step 4 says `sg2hid` gains "the
  two `src/usb/` sources"; `CMakeLists.txt` compiles one, `src/usb/usb_device.cpp` (the other two
  `src/usb/` files are headers). Closed at the round cap, 2026-10-09.
- Operator, at validation (2026-10-09): the R-CLEAN-02 exception for TinyUSB's two five-parameter
  callbacks is confirmed as written.
- Files read that the Context pointers missed (closure-test gaps):
  `.clang-tidy` (naming for globals and enum constants, `HeaderFilterRegex`, the function-size
  thresholds); `tests/test_repo_shape.sh` (`find_proto05`); `docs/wiring-emulator.md` (the safe
  order for moving the master between hosts in `verify.md`);
  `$PICO_SDK_PATH/src/rp2_common/pico_stdio_usb/CMakeLists.txt` and
  `include/tusb_config.h` (the SDK's config is a `SYSTEM` include, so `src/usb/tusb_config.h` wins;
  its CDC buffer sizes, kept); `$PICO_SDK_PATH/src/rp2_common/pico_unique_id/include/pico/unique_id.h`
  (`pico_get_unique_board_id_string( )`); `$PICO_SDK_PATH/lib/tinyusb/src/class/hid/hid_device.c`
  (GET/SET_REPORT have no weak default, so both must be defined); `tools/trace_decode.py`'s usage.

## Debt

- No `belay-debt:` comment was added.
- A report is sent every poll whether or not it changed (ADR-0018). Ceiling: 1000 HID transfers a
  second on an idle guitar. Upgrade: send on change, if a measurement asks for it.

## For later phases

- **10-guitar-full** (latency): `tud_task( )` runs only in the wait between polls and inside
  `printf` (the SDK's stdio calls it), and the report is handed over at the end of `poll_once( )`.
  End-to-end latency is therefore one poll period plus up to one HID interval (`bInterval` 1 ms),
  before any host-side buffering — unmeasured.
- **11-release**: the master no longer carries the SDK's USB reset interface, so
  `picotool reboot -u` (06 notes, macOS accessory approval) does not reach it; flashing is
  BOOTSEL by hand. Host notes worth carrying into the README: a browser gamepad tester lists the
  device only after its first button press, and the Steam client may hold the device on SteamOS.
- **10-guitar-full**: on the Steam Deck the browser read the whammy through Linux's `joydev`
  correction — a dead zone of whammy bytes 112–142 around `80`, and a non-linear scale outside it
  (`## Bench readings`). The report itself is linear; a test on Linux that reads the axis value must
  expect the corrected numbers, or read raw `ABS_X` through evdev.
- **Bench readings:** `verify.md` §2 run through step 9.1 on 2026-10-09; the values still owed are
  marked in `## Bench readings` below.

## Bench readings

Run by the operator on 2026-10-09, relayed in session; values below are what was reported, and
anything not reported is marked owed rather than reconstructed.

Steps 3, 5, 6 and the demo runs below are copied from the operator's Terminal scrollback (16:28–16:51).

- **Step 3, ports:** `MASTER=/dev/cu.usbmodem101` (the name used before this phase was `usbmodem1101` — it changed,
  as `verify.md` warned; the first `make hil` with that name failed `FAIL: setup: [Errno 2] No
  such file or directory`), `EMU=/dev/cu.usbmodem2101`.
- **Step 5, `make hil MASTER=$MASTER EMU=$EMU`** (16:31):

  ```
  python3 tools/hil_digital.py --master "/dev/cu.usbmodem101" --emu "/dev/cu.usbmodem2101"
  ok: setup
  ok: sustained
  ok: sweep
  ok: buttons
  ok: fault ack 0
  ok: fault ack 3
  ok: fault late 200
  ok: fault id 79
  ok: fault late 50
  ok: fault decline
  hil: PASS
  ```

- **Step 6, `hidutil list --matching '{"VendorID":0x2E8A,"ProductID":0x000A,"PrimaryUsagePage":1,"PrimaryUsage":5}'`** (16:33):

  ```
  Services:
  VendorID ProductID LocationID UsagePage Usage RegistryID  Transport Class                    Product       UserClass               Built-In
  0x2e8a   0xa       0x100000   1         5     0x10007be0a USB       AppleUserHIDEventService SG2HID Guitar AppleUserHIDEventDriver 0

  Devices:
  VendorID ProductID LocationID UsagePage Usage RegistryID  Transport Class              Product       UserClass                 Built-In
  0x2e8a   0xa       0x100000   1         5     0x10007bdff USB       AppleUserHIDDevice SG2HID Guitar AppleUserUSBHostHIDDevice 0
  ```

- **Demo runs** (`python3 tools/hil_digital.py --demo --emu $EMU`, 16:34 and 16:36, steps 7 and 8):
  both printed the ten `pressed:` lines, the five `whammy:` lines (`00 40 80 C0 FF`) and `demo: done`.
- **Steps 7 and 8, browser gamepad tester, Mac and Steam Deck (SteamOS, Desktop Mode)**, as reported
  by the operator. Both hosts: Chrome on `https://hardwaretester.com/gamepad`.
  - Buttons, both hosts: ten, `B0` to `B9`. Each `pressed:` line lit exactly one, `1.00` then back to
    `0.00`, in the demo's order: green `B0`, red `B1`, yellow `B2`, blue `B3`, orange `B4`, strum up
    `B5`, strum down `B6`, start `B7`, select `B8`, tilt `B9` — the descriptor's button order.
  - Axis 0 over the whammy steps `00 40 80 C0 FF`, then the demo's closing idle payload (`80`):
    - Mac: `-1`, `-0.49804`, `0.00392`, `0.50588`, `1.0`, `0.00392`. A plain linear map,
      `2v / 255 − 1`, matches every reading.
    - Deck (a second demo run, read for this): `-1.0`, `-0.49489`, `0.0`, `0.51546`, `1.0`, `0.0`.
      Not linear, and not an error: these are exactly what the Linux joystick (`joydev`) interface's
      default correction produces for a 0..255 axis — a dead zone of ±15 around 127 (bytes 112–142
      read `0`), the rest rescaled to ±32767, then divided by 32767. Computed in session, digit for
      digit: `00 → −1`, `40 → −0.49489`, `80 → 0`, `C0 → 0.51546`, `FF → 1`. The correction formula
      is recalled from the kernel's `joydev.c`, not read from source here; the exact match of all
      five values is the evidence. It also explains the earlier report of `0` at the end of the
      Deck's first run: the same byte `80`, inside the dead zone.
  - Consequence for later: on Linux, through `joydev`, whammy bytes 112–142 read as rest. Harmless
    for a whammy that rests at `80`; worth knowing in 10-guitar-full if a game reads the axis there.
  - Step 8.3 on the Deck, `ls /dev/input/by-id/ | grep -i sg2hid` (re-run later, transcribed by hand
    from the screen by the operator):

    ```
    usb-pico-sg2hid_SG2HID_Guitar_5303284730F33A1C-if02-event-joystick
    usb-pico-sg2hid_SG2HID_Guitar_5303284730F33A1C-if02-joystick
    ```

    Operator, at validation (2026-10-09): the hand transcription stands as evidence of the expected
    entry even if it is not character-identical to the screen.
    The manufacturer and product strings, the board's unique id as serial, and `if02` — the HID
    interface number in the configuration descriptor (CDC holds 0 and 1).
- **Step 9, summary lines by hand (master back on the Mac):**
  - First line read, before restoring the setup payload:

    ```
    hil: polls=88000 refused=18191 changes=0 us=1000244 state=analog fault=ack-timeout att=high payload=FF FF whammy=80 report=00 00 80
    ```

    Not a fault: the demo (steps 7–8) ends on `payload ff ff 80 80 80 80`, all released, whose
    report is `00 00 80` (equal to the neutral report, but from a good analog frame — `state=analog`).
    `refused` and `fault` are cumulative since boot / last fault kept on recovery; the 18191 refusals
    (~18 s) match the time the master was on USB with the signal wires out while moving it back.
  - After `printf 'payload 7f fe 80 80 80 80\n' > $EMU` (the `sustained` state):

    ```
    hil: polls=235000 refused=18191 changes=1 us=1000436 state=analog fault=ack-timeout att=high payload=7F FE whammy=80 report=00 02 80
    ```

    `refused` unchanged over 147 000 further polls: no refusal once the bench was wired.
  - After `printf 'payload ff bf 80 80 80 80\n' > $EMU` (green alone, `buttons`' first row):

    ```
    hil: polls=352000 refused=18191 changes=2 us=1000388 state=analog fault=ack-timeout att=high payload=FF BF whammy=80 report=01 00 80
    ```

    Green is bit 0 of the report's first byte, as the descriptor declares; still no refusal.
  - Step 9.3 (`payload 7f fe …` restored): back to the expected state, as reported.

## Validation — 2026-10-09
- criteria: 21 passed / 0 failed (bench criteria run live: `hil: PASS` on `usbmodem101`/`usbmodem2101`, `hidutil` 2 matches)
- project gates: test pass, lint pass, typecheck pass
- gates tree: 4863cf720b677e1880dbe3c9263799ba94aa7a8b
- boundary sweep: clean
- independent review: undecidable: `--demo` exit conditions (emulator answer timeout unclassified); undecidable: "each port … on the Deck" in Plan step 9 (settled: 1 — `make hil MASTER=<master port> EMU=<emulator port> | tail -n 1` for the CDC buffer sizes in `src/usb/tusb_config.h`; the SDK's own are 64 at full speed, equal) (unstated: 4 — R-HID-02 rule line aggregates the streaming case too; `kMaxPowerMa`/`kDeviceVersion`/`kUsbVersion` values; the `kButtonCount` range `static_assert`; `--demo` ignores `--master` and prints `demo: done`)
- closure test: fail: `docs/adr/0019-linux-target-steam-deck.md` named nowhere in the spec; §Out of scope excluded the `requirements.md` edit the diff makes
- findings: 3
- finding keys: §Goal tools/hil_digital.py; §Plan docs/phases/08-usb-hid/verify.md; §Plan docs/adr/0019-linux-target-steam-deck.md
- spec size: 19803 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-09
- criteria: 21 passed / 0 failed (bench criteria run live again: `hil: PASS`, `hidutil` 2 matches)
- project gates: test pass, lint pass, typecheck pass (carried over)
- gates tree: 4863cf720b677e1880dbe3c9263799ba94aa7a8b
- boundary sweep: clean
- independent review: contradicts (spec-side): Plan step 3 "aggregating the three neutral cases" vs `rule_hid02( )` aggregating four — R-HID-02's Scope in `docs/constraints.md` names the streaming case; the spec sentence dates from `4c9db70` (settled: none) (unstated: 6 — CDC buffer sizes in `tusb_config.h`; unspecified descriptor values in `usb_device.cpp` (bMaxPower, bcdUSB, bcdDevice, langid, endpoint sizes); the `kButtonCount` range `static_assert`; `--demo` output lines, failure path and ignored `--master`; `requirements.md` wording beyond the word swap; ADR-0019's joydev note)
- closure test: pass
- findings: 1
- finding keys: §Plan tests/ps2_codec_cases.cpp
- spec size: 20040 (+237 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-09
- criteria: 21 passed / 0 failed (bench criteria run live again: `hil: PASS`, `hidutil` 2 matches)
- project gates: test pass, lint pass, typecheck pass (carried over)
- gates tree: 4863cf720b677e1880dbe3c9263799ba94aa7a8b
- boundary sweep: clean
- independent review: undecidable: Plan step 4 "`sg2hid` alone gains the two `src/usb/` sources" — `CMakeLists.txt` compiles one (`usb_device.cpp`); the spec does not name the two (settled: none) (unstated: 8 — descriptor values in `usb_device.cpp`; GET_REPORT ignores type/id; CDC buffer sizes; the `kButtonCount` range `static_assert`; `--demo` output/failure path/ignored `--master`; `--demo`'s whammy payload bytes; ADR-0019's joydev note; verify.md 8.2's move order)
- closure test: fail: the undecidable above is a missing pointer
- findings: 1
- finding keys: §Plan CMakeLists.txt
- spec size: 20037 (-3 since the previous validation)
- upstream: commands/validate-phase.md — /belay-feedback recommended (round cap reached on fresh, unrelated findings each round)
- not-ours: none
- verdict: done (operator-closed at round cap: 1 open)
