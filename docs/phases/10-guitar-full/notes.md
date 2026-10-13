# Phase 10-guitar-full — notes

## Outcome

- base: `99ba0a6` (the expansion commit; the phase's work is the diff after it)
- **Where implementation stopped (2026-10-13):** Plan steps 1–5 and 7 done by the agent. Owed by
  the operator: step 6 (`make hil` on the new UF2s) and step 8 (the guitar: `make guitar` on the
  Mac, the harness on the Deck, the gamepad tester on both), in `verify.md` order.
- Step 1: `src/core/link.h` — `kNegotiationTimeoutUs = 6 * kPollPeriodUs` (48 000 µs), includes
  `core/ps2_protocol.h`, comment states the 2x margin over G10. `tests/ps2_codec_cases.cpp` — new
  `case_link_negotiation_lasts_six_polls`, registered after the exclusive-bound case. Hand
  mutation (2026-10-13): with `100000`, the codec cases print exactly one `FAIL:`, that case; with
  the real value, `ok:`. The existing timeout cases are symbolic and pass unchanged.
- Step 2: `src/usb/usb_device.{h,cpp}` — `[[nodiscard]] bool usb::is_report_ready( )`, returning
  `tud_hid_ready( )`.
- Step 3: `src/app/main.cpp` — `Poller::hand_max_us` and `Poller::busy`; `poll_once( )` reads
  `time_us_32( )` before building the frame, counts `busy` when `is_report_ready( )` is false just
  before `send_report( )`, and takes the handoff after `send_report( )` returns; `print_summary( )`
  appends ` hand=%lu busy=%lu` and resets both. Header comment names them and the latency bound.
- Steps 4–5: `tools/hil_digital.py` — `SUMMARY` reads `hand` and `busy` (in `COUNTERS`), `LINK`
  parses `link:` lines, `MAX_HANDOFF_US = 1000` checked in `checked_summary( )`; `--guitar`,
  `--cycles`, `CONTROL_WAIT_S`, `HUMAN_WAIT_S`, `WHAMMY_PUSHED_MIN`; `Harness.summary_until( )`,
  `Harness.link_until( )`, `has_bits( )`, `ask( )`, `guitar_streaming( )`, `guitar_controls( )`,
  `guitar_replug( )`, `run_guitar( )`. `Makefile` — `make guitar MASTER=<port>`, header entry,
  `.PHONY`.
- Steps 4–5 checked without hardware (2026-10-13): the spec's `python3 - <<'PY'` criterion exits 0;
  `--guitar --emu x --master y` exits 2; `make guitar` prints its usage line and fails; plain
  `--master y` (no `--emu`, no `--guitar`) exits 2. The `--guitar` flow was also driven end to end
  through a pty by a scratch simulator (not committed) that answered each prompt as a master
  would: `guitar: PASS` on good lines; `FAIL: streaming: hand=1200 over 1000` with `hand=1200`;
  `FAIL: streaming: state=analog busy=1` with `busy=1`.
- Step 7: `docs/phases/10-guitar-full/verify.md`.
- Host acceptance criteria run 2026-10-13, after steps 1–5 and 7: `make test` 0, `make lint` 0,
  `make typecheck` 0, `make firmware` 0 with `build/pico/sg2hid.uf2`; the `kNegotiationTimeoutUs`
  grep one line; `grep -rn '10-guitar-full' src/` no output, exit 1; `hand=%lu busy=%lu` count 1;
  the `python3 - <<'PY'` criterion 0; `--guitar --emu x` exit 2; `make guitar` usage, exit 2; the
  `verify.md` heading count 2. The three bench criteria are not run (operator, steps 6 and 8).

## Deviations

- Goal item 4 lists the `controls` prompts; the harness also prints `let go` after the whammy is
  seen, so the person releases the bar before `replug` starts. Prompt only, judged by nothing.
- Plan step 5 says `--guitar` refuses `--emu` and `--demo`; `--emu` stopped being an argparse
  `required=True` argument to allow that, and is now required by a `parser.error( )` "unless
  --guitar". Same exit code (2) as before for a missing `--emu`.
- operator amended criterion: `python3 hil_digital.py --guitar --master /dev/ttyACM0` →
  `python3 hil_digital.py --guitar --master /dev/ttyACM<n>`, `<n>` from `ls /dev/ttyACM*` —
  on the bench the Deck gave the master `/dev/ttyACM1` because another device held `ttyACM0`;
  the run is the same. Reconciled in the same edit: spec Plan step 7's `verify.md` item and
  `verify.md` §6 step 3 (checked: PHASES.md names no port).

## Debt

- `tools/hil_digital.py`'s guitar mode has no automated test: `tools/` has none (the HIL harness
  never had one), and the pty simulation that exercised it stayed in scratch. Ceiling: a
  regression in `--guitar` is found at the bench. Upgrade: commit the simulator as a
  `tests/test_hil_guitar.py` if the harness changes again.

## For later phases

- **11-release**: the guitar run needs only `tools/hil_digital.py` on the host (stdlib Python);
  on the Deck it was copied over by hand. Whether `/dev/ttyACM0` opens without `sudo` for the
  `deck` user is unverified until step 8 runs. **Answered at the bench (2026-10-13):** no `sudo`
  needed; but the port was `/dev/ttyACM1`, not `ttyACM0` (another device held `ttyACM0`), so
  release instructions must say to find it with `ls /dev/ttyACM*`, not hard-code it.
- **If `busy` is non-zero at the bench**: the fix is re-sending the stored report from
  `usb::task( )` when the HID interface becomes ready (spec §Out of scope); operator's call.

## Bench readings

Run jointly by the agent (commands it can run from the Mac) and the operator (everything
physical), in `verify.md` order. Tree: `99ba0a6` + the uncommitted phase diff.

- **verify §1 (agent, 2026-10-13 16:47):** `make firmware` exit 0, nothing rebuilt (UF2s current).
  `sg2hid.uf2` sha256 `fafe1f1fd668293cb496965090a5a679cc8a8eb49b3337c2023cf8237e2d5f8c`,
  `sg2hid_emu.uf2` sha256 `7717503a10b27e10cb5a0d40da0f3f8171b6c6c4f7ac8e806713e45209eb9929`.
  `make test`, `make lint`, `make typecheck`: exit 0 (see Outcome, same tree). Python 3.14.6.
  No Pico on USB at that time.
- **verify §2 flash (agent, 2026-10-13):** master put in BOOTSEL by the operator; agent copied
  `sg2hid.uf2` to `RPI-RP2`; it rebooted as `/dev/cu.usbmodem101`. Alone, no bus wired, it prints
  `hil: polls=750 refused=750 changes=0 us=1000017 state=absent fault=ack-timeout att=high
  payload=-- whammy=-- report=00 00 00 hand=198 busy=0` — the new `hand=`/`busy=` fields, so the
  new firmware is on it. Handoff with every poll timing out: 182–203 µs.
- **verify §2 flash, emulator (agent, 2026-10-13):** first attempt booted normally (BOOTSEL not
  held; it answered `ok: fault none` on its old firmware); second attempt mounted `RPI-RP2`,
  agent copied `sg2hid_emu.uf2`, it rebooted as `/dev/cu.usbmodem2101`.
- **verify §2 ports (agent, 2026-10-13):** one board at a time: `MASTER=/dev/cu.usbmodem101`,
  `EMU=/dev/cu.usbmodem2101`. Emulator answered `ok: fault none` before wiring.
- **verify §2 `make hil` (agent ran it, operator wired, 2026-10-13):** `make hil
  MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101` →
  `ok: setup`, `ok: sustained`, `ok: sweep`, `ok: buttons`, `ok: fault ack 0`, `ok: fault ack 3`,
  `ok: fault late 200`, `ok: fault id 79`, `ok: fault late 50`, `ok: fault decline`, `hil: PASS`.
  Master lines right after: `hil: polls=30750 refused=10697 changes=16 us=1000017 state=analog
  fault=declined att=high payload=7F FE whammy=00 report=00 02 00 hand=671 busy=0`; `hand=` 655–674
  over three lines (analog, emulator) — under the 1000 bound with ~330 µs to spare.
- **verify §3 multimeter C1 (operator, 2026-10-13), socket by PS2 number via the mirrored
  breakout:** 1 DATA 334 Ω, 2 CMD 334 Ω, 3 ATT 332 Ω, 4 CLK 329 Ω, 5 ACK 335 Ω; 6 and 7 beep;
  8 3V3↔GND starts 80 kΩ and climbs; 9 and 10 `OL`, no beep; 11 socket 5↔VSYS starts 70 kΩ and
  climbs, ↔VBUS starts 700 kΩ and falls, no beep either. 12a: CMD, ATT, CLK ↔ pin 38 `OL`; ACK ↔
  pin 38 starts 220 kΩ and climbs past 500 kΩ (the first "400" reading was against pin 39, VSYS,
  re-taken); DATA ↔ pin 38 starts 1.5 MΩ and climbs. All pass (far above 330 Ω).
  12 (socket pairs by PS2 number): 1↔2 4.2 MΩ, 1↔6 `OL`, 1↔7 `OL`, 1↔9 22 kΩ (the two pull-ups,
  expected ≈ 20.7 kΩ), 2↔6 `OL`, 2↔7 `OL`, 2↔9 2.7 MΩ, 6↔7 `OL`, 6↔9 2.7 MΩ, 7↔9 3 MΩ. No bridge.
  C1 passes.
- **verify §3 multimeter C2 (operator, master in BOOTSEL confirmed by the agent: `RPI-RP2`
  mounted, no serial port; guitar unplugged; black on socket 4):** 13 socket 5 3.3 V; 14 socket 3
  0 V; 15 socket 1 DATA 2.7 V; 16 socket 9 ACK 2.7 V. C2 passes.
- **verify §3 guitar plugged in (operator: master unplugged, guitar in socket, then master USB;
  agent read the port, 2026-10-13):** `MASTER=/dev/cu.usbmodem101`. First lines:
  `hil: polls=1250 refused=32 changes=0 us=1000367 state=analog fault=unknown-id att=high
  payload=7F FE whammy=00 report=00 02 00 hand=641 busy=0`, then `refused` stays at 32 and
  `payload` flips to `7F FF` / `report=00 00 00` (tilt bit, the guitar being handled). The 32
  refusals with `fault=unknown-id` happened in the first second after power-up, before the
  link reached analog; not judged by anything in this phase. `hand=` 641–669 on the real SG.
- **verify §4 `make guitar` on the Mac (operator ran it, 2026-10-13):** `make guitar
  MASTER=/dev/cu.usbmodem101` → `ok: streaming`; prompts `hold green` … `hold tilt`, `push the
  whammy to the stop and hold`, `let go`, `ok: controls`; five `unplug the guitar (n/5)` /
  `plug the guitar back in (n/5)` cycles, `ok: replug`; `guitar: PASS`. The serial port stayed
  open through all five replugs (any USB drop would have failed the run with `[Errno …]`).
- **verify §5 gamepad tester on the Mac (operator pressed, agent read, 2026-10-13):** Chrome on
  hardwaretester.com/gamepad, with an agent-injected script logging `navigator.getGamepads( )`
  transitions every 10 ms. Device `SG2HID Guitar (Vendor: 2e8a Product: 000a)`, 10 buttons,
  1 axis. Buttons, each pressed alone and each lighting exactly one button: green b0, red b1,
  yellow b2, blue b3, orange b4, strum (bar physically up) b5, strum (physically down) b6,
  start b7, select b8, tilt b9. Whammy, pushed over ~1.9 s: axis 0 climbs monotonically in
  steps of ≤ 0.10 from rest to 0.96 at the stop, holds, and returns monotonically to -1 on
  release. Before the first whammy movement the tester showed axis 0 = 0, not -1, then jumped
  to -0.92 on the first movement; after release it read -1. Inference, not checked: the browser
  reports 0 for an axis until the host delivers its first change (rest is report byte 00, so it
  never changes while untouched). Three replugs, green pressed after each: b0 lit each time
  (2–8 s after the previous one), no `gamepaddisconnected` event in the whole session, and no
  button or axis transition logged while the guitar was out.
- **verify §6 Steam Deck harness (operator, 2026-10-13):** `hil_digital.py` fetched on the Deck
  with `curl` from a temporary agent-run `python3 -m http.server` on the Mac's LAN address
  (scratch copy of the file only; stopped right after). Master moved to the Deck with the guitar
  left in its socket. The master was `/dev/ttyACM1` (something else already held `ttyACM0`);
  no `sudo` needed, the `deck` user opened it directly.
  `python3 hil_digital.py --guitar --master /dev/ttyACM1`: operator reports
  "the same results as on the Mac", i.e. `ok: streaming`, `ok: controls`, `ok: replug`,
  `guitar: PASS`.
- **verify §6 gamepad tester on the Deck (operator, 2026-10-13):** buttons identical to the Mac
  (green 0 … tilt 9). Whammy: the tester shows axis 0 = 0 when it first detects the gamepad, and
  -1 at rest once the whammy has been moved — the same as on the Mac.
- **Whammy shows mid-travel (0) in a browser until first moved, on both macOS and SteamOS**
  (bench, 2026-10-13). The device's report carries whammy byte `00` (rest) from the start; the
  browser reports axis 0 = 0 until the first change arrives. Inference, not checked: browsers
  start an axis at 0 and only update it on a value change. Whether a game (SDL, Clone Hero) reads
  the initial value correctly is unverified → **11-release** should check it in the game before
  calling the whammy done; a firmware nudge (one report with a non-rest whammy at enumeration)
  is the fallback, operator's call.
- **Whammy at the stop (operator, both hosts, 2026-10-13):** about 0.96 held at the stop (as the
  Mac log shows); pressing harder past the stop reaches 1. The last few percent is how hard the
  bar is pushed, not the firmware.
- **verify §6 Deck tester, continued (operator, 2026-10-13):** on the first push after the
  gamepad is detected the tester stayed at 0 through the rest-to-midpoint half and only moved
  past the midpoint — the numbers start only after the joydev flat spot is crossed (on the Mac
  the first small movement already shows, -0.92 in the log); once the whammy had been moved, every slow push runs -1 → 0 → 1 in full.
  Joydev flat spot (step 6.5): seen — a short stretch at 0 around mid-travel. Three replugs, green
  after each: the gamepad never left the tester, as on the Mac.
- **All three bench criteria pass (2026-10-13), on the tree whose UF2 hashes are recorded
  above:** `make hil` → `hil: PASS`; `make guitar` on the Mac → `guitar: PASS`; the harness on
  the Deck → `guitar: PASS` (port `/dev/ttyACM1`, see For later phases).
