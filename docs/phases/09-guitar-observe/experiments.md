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

## Open

Ordered by the agent's recommendation. Confidence is the agent's judgement, not a measurement.

| id | Hypothesis or tool | Status | What it costs | Confidence |
|---|---|---|---|---|
| X5 | **Logic analyzer at the socket** (diagnostic, not a fix): the emulator Pico flashed with an off-the-shelf logic-analyzer firmware (candidates from memory, to verify: gusmanb's LogicAnalyzer, sigrok-pico), inputs on ATT, CLK, CMD, DATA, ACK + GND. Shows whether the guitar ever pulls ACK or DATA and what reaches it. | pending: operator's call (third-party software on the bench) | flash with the guitar unplugged, wire after; no project code | — (narrows every row below) |
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
