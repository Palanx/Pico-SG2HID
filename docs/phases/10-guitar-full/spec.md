# Phase 10-guitar-full — the full pipeline on the real guitar

## Goal

`build/pico/sg2hid.uf2`, with the real SG on the bus, negotiates analog mode, streams it, and
recovers from the guitar being unplugged and plugged back in, while the USB device (HID gamepad
plus serial port) never re-enumerates. Three changes to the firmware and one to the harness make
that observable:

1. **Negotiation bound.** `kNegotiationTimeoutUs` (`src/core/link.h`) becomes
   `6 * kPollPeriodUs` (48 000 µs), twice the SG's measured negotiation (G10: two `Negotiating`
   frames, ~24 ms). Operator's decision, 2026-10-13.
2. **Handoff measurement.** Each `hil:` summary gains two fields after `report=`:
   `hand=<µs>`, the longest time over that summary's frames from the frame's start
   (`time_us_32( )` before `exchange_frame( )`) to the return of `usb::send_report( )`; and
   `busy=<n>`, the number of that summary's frames whose report was not queued because the HID
   endpoint was not ready (`tud_hid_ready( )` false). Both reset after each summary.
3. **Latency bound** (operator's decision, 2026-10-13, replacing "inside one USB poll interval",
   which ADR-0021 made unreachable). Worst-case end-to-end latency, from the guitar's bytes changing
   to the host being able to read the report, is `kPollPeriodUs` + `hand` + one `bInterval`
   (1 ms). The firmware owns `hand`; the phase requires `hand` ≤ 1000 µs in every summary and
   `busy` = 0 while a host is attached, so the bound is ≤ 10 ms. The guitar's internal button
   scan and the host's own buffering are outside the measurement and stated as such in
   `verify.md`.
4. **Guitar harness.** `tools/hil_digital.py --guitar --master <port>` needs no emulator. It runs
   three scenarios, printing `ok: <scenario>` or `FAIL: <scenario>: <reason>`, stopping at the
   first FAIL, and ending with `guitar: PASS` (exit 0) or `guitar: FAIL` (exit 1):
   - `streaming` — after `DISCARDED_SUMMARIES`, `--seconds` summaries (default 60), each
     `state=analog` and `busy=0`, with Δ`refused` = 0 over the window.
   - `controls` — for each of the ten controls in `BUTTONS` (green, red, yellow, blue, orange,
     strum up, strum down, start, select, tilt) it prints `hold <control>` and waits up to
     `CONTROL_WAIT_S` for a summary whose report has that control's bits set (the report's third
     column in `BUTTONS`, ANDed); then it prints `push the whammy to the stop and hold` and waits
     for a summary whose `whammy` is ≥ `WHAMMY_PUSHED_MIN` (`C0`, hand-written).
   - `replug` — `--cycles` times (default 5): prints `unplug the guitar`, waits up to
     `HUMAN_WAIT_S` for a `link: analog -> absent` line, then requires the next summary to show
     `state=absent` and `report=` `NEUTRAL_REPORT`; prints `plug the guitar back in`, waits up to
     `HUMAN_WAIT_S` for a `link: <any> -> analog` line, then requires the next summary to show
     `state=analog`.
   The master's port stays open from start to end; any port error is a FAIL, because it means
   the device re-enumerated (R4). `checked_summary( )` rejects a summary whose `hand` exceeds
   `MAX_HANDOFF_US` (1000) in both modes; `busy` is judged in `--guitar` mode only.

The game-facing check is the operator's: a browser gamepad tester on the Mac and on the Steam
Deck shows every control and the whammy, before and after replug cycles (`verify.md`).

## Context pointers

- `CLAUDE.md` — R-SAFETY-08's order of first contact: host tests, then `make hil` on the
  emulator, then the guitar.
- `docs/phases/09-guitar-observe/notes.md` — the G table (G1–G11), the 8 ms poll period, the
  breakout mirroring, and G10's re-owning of the negotiation bound to this phase.
- `docs/phases/08-usb-hid/notes.md` §For later phases — where `tud_task( )` runs, the latency
  decomposition, and the Steam Deck `joydev` correction.
- `docs/adr/0021-poll-period-8ms.md` — why the poll period is 8 ms and the latency it adds.
- `docs/adr/0018-usb-composite-hid.md` — the composite device and the 1 ms `bInterval`.
- `docs/constraints.md` §Observed conventions "The SG on the bus" and "This bench's PS2 breakout"
  — what the real guitar does; wiring by PS2 number.
- `docs/wiring.md` — the bench wiring the guitar meets.
- `src/core/link.h`, `src/core/link.cpp` — `kNegotiationTimeoutUs` and `step( )`.
- `src/core/ps2_protocol.h` — `kPollPeriodUs`.
- `tests/ps2_codec_cases.cpp` — the existing `kNegotiationTimeoutUs` boundary cases (symbolic,
  around line 400 and 495); the new case lands beside them.
- `src/app/main.cpp` — `Poller`, `poll_once( )`, `print_summary( )`: where `hand` and `busy` are
  measured and printed.
- `src/usb/usb_device.h`, `src/usb/usb_device.cpp` — `send_report( )`; gains the readiness query.
- `src/app/console.cpp` — the `link: <from> -> <to> fault=<fault>` line format the harness reads.
- `tools/hil_digital.py` — `SUMMARY`, `checked_summary( )`, `Harness`, `BUTTONS`,
  `NEUTRAL_REPORT`, `DISCARDED_SUMMARIES`, `scenarios( )`, `main( )`.
- `Makefile` — the `hil` target; `guitar` lands beside it.
- `docs/phases/08-usb-hid/verify.md` — the gamepad-tester procedure on both hosts this phase's
  `verify.md` reuses.
- `docs/phases/09-guitar-observe/verify.md` Part 4 — the guitar-connection procedure (power-off
  order, C1/C2 re-check) this phase's `verify.md` reuses.

## Plan

1. **Negotiation bound.** `src/core/link.h`: `kNegotiationTimeoutUs = 6 * kPollPeriodUs`,
   including `core/ps2_protocol.h`; its comment states the 2x margin over G10 and drops the
   "~40 ms" estimate and "Tightening the bound is 10-guitar-full's". `tests/ps2_codec_cases.cpp`:
   one new case — a link entering `Negotiating`, then stepped six more times with a config-mode
   decode and `kPollPeriodUs` each, is still `Negotiating` after the sixth and `Absent` with
   `FaultCause::Negotiating` after the seventh — registered with the other link cases. Check:
   `make test` → OK; by hand, `kNegotiationTimeoutUs` set back to `100000` makes the new case
   print a `FAIL:` line (restore it after).
2. **Readiness query.** `src/usb/usb_device.{h,cpp}`: `[[nodiscard]] bool is_report_ready( )`,
   returning `tud_hid_ready( )`. `send_report( )` is unchanged. Check: `make firmware` → exit 0;
   `make lint` → exit 0.
3. **Measure and print.** `src/app/main.cpp`: `Poller` gains `hand_max_us` and `busy`;
   `poll_once( )` takes the frame's start time, reads `usb::is_report_ready( )` before
   `send_report( )` (false → `busy` + 1), and after `send_report( )` returns updates
   `hand_max_us`; `print_summary( )` appends ` hand=%lu busy=%lu` after `report=` and resets
   both. The header comment names the two fields. Check: `make firmware` → exit 0; `make lint` →
   exit 0.
4. **Harness, both modes.** `tools/hil_digital.py`: `SUMMARY` reads `hand` and `busy`
   (both added to `COUNTERS`); `MAX_HANDOFF_US = 1000`; `checked_summary( )` fails a summary with
   `hand` above it. The module docstring names the new fields and `--guitar`. Check:
   the `python3 - <<'PY'` criterion in §Acceptance → exit 0 (`hand=999` parses, `hand=1001`
   raises `Failure`).
5. **Harness, guitar mode.** `tools/hil_digital.py`: `--guitar`, `--cycles` (default 5,
   `at_least_one`), the three scenarios of §Goal item 4, `CONTROL_WAIT_S` and `HUMAN_WAIT_S`
   (30 s each), `WHAMMY_PUSHED_MIN`. `--guitar` refuses `--emu` and `--demo`; `--master` is
   required with it. `Harness` works with no emulator port. `Makefile`: `make guitar MASTER=<port>`
   runs `python3 tools/hil_digital.py --guitar --master "$(MASTER)"`, with the same usage message
   shape as `hil`, and the header comment names it. Check: `python3 tools/hil_digital.py --guitar
   --emu x --master y` → exits 2 with an argparse error; `make guitar` with no `MASTER` → prints
   the usage line and exits non-zero.
6. **Emulator regression (R-SAFETY-08).** Flash the new `sg2hid.uf2` and `sg2hid_emu.uf2` on the
   two-Pico bench (`docs/phases/08-usb-hid/verify.md` §2) and run `make hil`. Check: `hil: PASS`,
   every summary carrying `hand=` ≤ 1000.
7. **Write `docs/phases/10-guitar-full/verify.md`** for a non-specialist, with `## What was built`
   and `## Check it yourself`: what the negotiation bound, `hand` and `busy` mean; the latency
   bound and what it does not measure (the guitar's button scan, host buffering); the emulator
   regression (step 6); connecting the guitar (09 `verify.md` Part 4's order, wiring by PS2
   number); `make guitar` on the Mac; the same script on the Steam Deck (copy
   `tools/hil_digital.py` over, `python3 hil_digital.py --guitar --master /dev/ttyACM0`, port
   found with `ls /dev/ttyACM*`); the gamepad tester on each host before and after three replug
   cycles; how to unplug and replug safely (at the guitar's plug, Pico powered, never touching the
   breadboard); and what a failure of each scenario points to. Check: `grep -cE '^#+ What was
   built|^#+ .*[Cc]heck it' docs/phases/10-guitar-full/verify.md` → 2.
8. **The guitar, on the bench.** Operator runs `verify.md`: `make guitar` on the Mac, the harness
   on the Deck, the gamepad tester on both. Results, including the largest `hand` seen and any
   non-zero `busy`, go to `notes.md`. Check: `guitar: PASS` on each host, reported by the
   operator.

## Acceptance criteria

```
make test                                                        # expect: exit 0, OK
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
make firmware                                                    # expect: exit 0; build/pico/sg2hid.uf2 exists
grep -n 'kNegotiationTimeoutUs = 6 \* kPollPeriodUs' src/core/link.h   # expect: one line
grep -rn '10-guitar-full' src/                                   # expect: no output, exit 1
grep -c 'hand=%lu busy=%lu' src/app/main.cpp                     # expect: 1
python3 - <<'PY'                                                 # expect: exit 0
import sys; sys.path.insert(0, "tools"); import hil_digital as h
line = ("hil: polls=125 refused=0 changes=0 us=1000000 state=analog fault=none att=high"
        " payload=7F FE whammy=00 report=00 02 00 hand={} busy=0")
h.checked_summary(line.format(999))
try:
    h.checked_summary(line.format(1001))
    sys.exit("hand=1001 accepted")
except h.Failure:
    pass
PY
python3 tools/hil_digital.py --guitar --emu x --master y         # expect: exit 2, argparse error
make guitar                                                      # expect: usage line, non-zero exit
grep -cE '^#+ What was built|^#+ .*[Cc]heck it' docs/phases/10-guitar-full/verify.md   # expect: 2
make hil MASTER=<master port> EMU=<emulator port>                # bench, emulator: expect hil: PASS
make guitar MASTER=<master port>                                 # bench, real SG, Mac: expect guitar: PASS
python3 hil_digital.py --guitar --master /dev/ttyACM0            # bench, real SG, Steam Deck: expect guitar: PASS
```

The `python3 - <<'PY'` criterion is one shell command: the lines up to `PY` are its input.

The last three are bench criteria: run by the operator, reported in `notes.md`. The gamepad
tester readings on both hosts are operator-verified per `verify.md` and carry no command
(docs/constraints.md §Testing).

## Out of scope

- Changing `kPollPeriodUs`, the bus timing (ADR-0020) or the whammy mapping (ADR-0022) — settled
  by `09-guitar-observe`; changing them needs a superseding ADR.
- Deferring a not-ready report to `usb::task( )` — only if `busy` is non-zero on the bench;
  that finding goes to `notes.md` and to the operator.
- Measuring host-side delivery (evdev timestamps, USB capture) — declined by the operator,
  2026-10-13.
- The Steam Deck `joydev` dead zone (bytes 112–142), which now sits mid-travel of the whammy —
  observed and recorded in `verify.md`, not corrected; no phase.
- The lost first `T1` line after a physical USB plug (09 `experiments.md` S1) — `observe` only;
  no phase.
- Rewording `docs/product/requirements.md`'s latency clause — operator's call after step 8.
- Stale comments the 09 reviewer noted in `src/core/trace_filter.h` and 09's `verify.md` — no
  phase.
- A UF2 release, README build or flash instructions — `11-release`.
