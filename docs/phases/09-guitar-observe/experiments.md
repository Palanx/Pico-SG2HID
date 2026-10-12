# Phase 09-guitar-observe — first-contact experiments

The real SG never ACKs byte 0 of any frame (stop condition G1, `notes.md` §Outcome). This is
the list of everything that could explain it and what was done about each.

**How to run one.** Branch from `feat/09-guitar-observe` as
`feat/09-guitar-observe-prototype-<name>`, do the experiment there, commit its outcome there.
Back on `feat/09-guitar-observe`, update its row here: `done: fixed`, `done: no effect` or
`done: ruled out`, plus the branch and the commit. A branch that fixed it is the one whose work
continues; the others stay as a record and never merge.

**Base.** `feat/09-guitar-observe` at `e327fdc`: no spec or ADR change since first contact.

## Settled

| id | Hypothesis | Status | Evidence | Branch |
|---|---|---|---|---|
| X1 | Signal wiring or socket contacts | done: ruled out | Part 2 passed with the emulator's wires pushed into the socket's own contacts (2026-10-10) | — |
| X2 | No supply at socket pin 5 / loose 3V3 wire | done: ruled out | 3.3 V at Pico pin 36, the 3V3 rail and socket pin 5 unloaded; 3.25 V at socket pin 5 with the guitar plugged in (2026-10-10) | — |
| X3 | The guitar is broken | done: ruled out | operator played it on a PS2 (2026-10-10) | — |
| X4 | Master too fast around ATT and between bytes | done: no effect | ADR-0020 + R-PROTO-11: 20 µs after ATT falls and before every later byte. Part 2 passed; the guitar stayed silent through 20 s of green presses (2026-10-10) | `feat/09-guitar-observe-prototype-timing` @ `36813ad` |
| X5 | Logic analyzer at the socket (diagnostic) | done: found the cause | uLA 0.0.5 (dotcypress/ula, generic UF2) on the emulator Pico, inputs only (checked in its source), read by a stdlib SUMP client. At the socket the master's frame was textbook (ATT low, `01` LSB-first at 250 kHz, CLK idle high) and DATA/ACK had 0 edges in 20 frames, so the fault was between the socket and the guitar (2026-10-12) | `feat/09-guitar-observe-prototype-logic-analyzer` |
| X9 | Breakout prints its pin numbers mirrored (terminal N = PS2 pin 10 − N) | done: fixed | diode mode, guitar plugged, rails disconnected: printed 6 → 5 reads 0.59 V (GND → VCC), printed 3 and 8 read 1.57 V like inputs instead of `OL`. Rewired by PS2 number (`docs/wiring.md`); after fixing two loose contacts the rewire left (ACK wire, ATT resistor) the guitar answered: `link: absent -> digital`, `in=FF,41,5A,FF,FF`, `k=5` (`captures/x5-first-answer.log`, 2026-10-12). Answers G1 (yes, on 3.3 V) and G2 (`41`) | `feat/09-guitar-observe-prototype-logic-analyzer` |

The master that got the answer was running X4's firmware, not this branch's: the X5 capture shows
CLK starting 26.8 µs after ATT falls, X4's 20 µs wait plus setup, where this branch clocks 1–2 µs
after it. Whether the guitar also answers without X4's waits is untested, and decides whether X4
merges: flash this branch's `sg2hid_observe.uf2` and read one frame.

X1's evidence did not cover this: the emulator's wires went into the breakout by its printed
numbers too, so a mirrored breakout passed. X6–X8 below are moot unless a later G check fails.

## Open

Ordered by the agent's recommendation. Confidence is the agent's judgement, not a measurement.

| id | Hypothesis or tool | Status | What it costs | Confidence |
|---|---|---|---|---|
| X6 | Supply too low: 3.25 V against a PS2's 3.4 V | pending | a supply above 3.3 V, an ADR, and a review of R-SAFETY-04/05 | low: Curious Inventor saw every controller work at 3.3 V; Santroller powers PS2 guitars from a Pico's 3V3 OUT |
| X7 | Pull-ups: 10 kΩ + 330 Ω series against Santroller's 1 kΩ and no series resistor | pending | hardware change; with 1 kΩ, the 330 Ω series puts a low at ~0.8 V, the Pico's VIL limit, so the series resistors would change too (`docs/wiring.md`, an ADR) | low: pull-up strength shapes rising edges, and a missing ACK is a falling edge |
| X8 | Clock: 250 kHz | pending | `kBusClockHz`; moves the trace decoder's shift and G11's calibration | low: Curious Inventor reports the Guitar Hero controller at 250 kHz |

## Side issue

| id | Issue | Status |
|---|---|---|
| S1 | The first `T1` line is lost on the first port open after a physical USB plug, never after a `picotool reboot -f`; nothing held the port when checked. It would cost G1's evidence line in the 4.5 capture. | open |

## Sources

- Curious Inventor, "Interfacing a PS2 (PlayStation 2) Controller":
  https://store.curiousinventor.com/guides/PS2
- Santroller, "Building an Adaptor for a PS2 Controller":
  https://santroller.github.io/guitar-configurator/guides/ps2.html
- Arduino PS2X, `PS2X_lib.{h,cpp}`: https://github.com/madsci1016/Arduino-PS2X
