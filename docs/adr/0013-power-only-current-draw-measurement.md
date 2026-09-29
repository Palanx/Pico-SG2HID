# ADR-0013: Measure the guitar's current draw in phase 02, with power-only wiring

- Status: accepted
- Date: 2026-09-24

## Context

ADR-0006 says the guitar is connected "only after the checklist passes and after the
firmware is already running and verified against the emulator". Read literally, that
forbids any contact with the guitar until phase `09-guitar-observe`. But the power budget
is an open question now: the Pico's onboard regulator recommends under 300 mA on 3V3, and
if the SG draws more than that the power design changes, which is cheaper to learn before
the bus, the emulator and the HID layers are built on top of it. The risk ADR-0006 guards
against is untested bus code meeting the guitar; a meter in the 3V3 line exercises no bus
code at all.

## Decision

In phase `02-wiring`, and only there, measure the SG's current draw with **power-only
wiring**: only 3V3 (Pico physical pin 36 to socket pin 5) and GND (Pico pin 38 to socket
pin 4) reach the guitar, through the meter in series on its mA range; the five signal
resistors are removed from the breadboard so no signal line is connected; the 7.6 V pin 3
stays unconnected; and the Pico is held in BOOTSEL, so no firmware of this project runs.
This supersedes only ADR-0006's ordering sentence, and only for the power pins: R-SAFETY-08's
ordering keeps governing every signal line, and nothing is flashed while the guitar is
connected.

## Consequences

Easier: the power question is answered with a number before any bus code exists, and a
draw ≥ 250 mA stops the project at a re-plan instead of after three phases of work.

Harder: the guitar meets the bench earlier, so the procedure in
`docs/phases/02-wiring/verify.md` has to be followed exactly — resistors out, BOOTSEL,
meter in series and never across the rail. Nothing in software enforces that; it is a
manual procedure like the rest of R-SAFETY-08.

Rejected:
- **Wait until `09-guitar-observe`** — learns the power budget after everything that
  depends on it has been built.
- **Measure with the signal lines connected** — would put a Pico pin on a live guitar bus
  before any bus code has been tested, which is exactly what R-SAFETY-08 forbids.
- **Measure from a bench supply instead of the Pico** — the operator has none, and the
  Pico's own 3V3 rail is the one the design has to live with.
