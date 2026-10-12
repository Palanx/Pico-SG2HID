# ADR-0021: The master polls the guitar every 8 ms

- Status: accepted
- Date: 2026-10-12

## Context

With the socket rewired (`09-guitar-observe`, X9) the real SG answers every frame, but at the
1 ms poll period both master firmwares used, it never refreshes its buttons: green, start and
strum held for a second each left every frame at `FF,41,5A,7F,FF`. The observe firmware was
rebuilt at other periods and the operator pressed green, start and strum on each
(`docs/phases/09-guitar-observe/captures/x10-poll-*.log`, experiment X10):

| Period | Buttons |
|---|---|
| 1 ms | never change |
| 2 ms | never change |
| 4 ms | green `FD`, start `77`, strum `3F` |
| 8 ms | the same |
| 16 ms (a PS2's ~60 Hz) | the same |

The guitar's own microcontroller presumably scans its buttons between polls and is starved below
some period between 2 and 4 ms. That is inferred, not measured; 3 ms was not tried.

## Decision

`ps2::kPollPeriodUs = 8000` in `src/core/ps2_protocol.h`, used by both `src/app/main.cpp` and
`src/app/observe.cpp` in place of their own 1000 µs constants. `main.cpp` prints its `hil:`
summary every 125 frames, so a summary still arrives about once a second and
`tools/hil_digital.py`'s `MAX_SUMMARY_US` holds. The operator chose 8 ms over 4 and 16
(2026-10-12).

## Consequences

- **Easier.** One constant for both firmwares; a different guitar or a re-measurement changes
  one line.
- **Harder.** The firmware adds up to 8 ms of latency, 4 ms on average. A game's lag
  calibration absorbs the average; about ±4 ms of jitter remains, small next to a note's hit
  window. `docs/product/requirements.md` §Constraints is reworded: the old "well inside one 1 ms
  USB interval" no longer holds. The HID report now changes at most every 8 ms; the USB
  interface still answers the host every 1 ms (ADR-0018 unchanged).
- **Harder.** `kNegotiationTimeoutUs` (100 ms) was two orders of magnitude above a negotiation at
  1 ms; at 8 ms a handful of frames is ~40 ms, so the margin is ~2x. G4/G5 time the real
  sequence.
- **Rejected: 4 ms.** Half the latency, but one untested step (3 ms) from the failing 2 ms; the
  limit is the guitar chip's and may move with supply or temperature.
- **Rejected: 16 ms.** Proven and console-like, but twice the latency of 8 ms for margin nothing
  measured asks for.
- **No new rule.** The value is a calibration knob, like `kAttSetupUs`; a test pinning 8000 would
  only restate the constant.
