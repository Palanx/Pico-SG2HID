# Wiring — master Pico to the PS2 socket

What connects to what on the bench. `src/core/pins.h` is the authority for the GPIO numbers;
this table must name the same GPIO for every signal, and `make test` fails if it does not
(R-SAFETY-02).

## Pin table

| Signal | Pico GPIO | Pico physical pin | PS2 socket pin | Direction | Drive mode | On the breadboard |
|---|---|---|---|---|---|---|
| DATA | GP2 | 4 | 1 | Input | OpenDrainInputOnly | 330 Ω in series; 10 kΩ pull-up to 3V3 on the Pico side |
| CMD | GP3 | 5 | 2 | Output | PushPull | 330 Ω in series |
| ATT | GP4 | 6 | 6 | Output | PushPull | 330 Ω in series |
| CLK | GP5 | 7 | 7 | Output | PushPull | 330 Ω in series |
| ACK | GP6 | 9 | 9 | Input | OpenDrainInputOnly | 330 Ω in series; 10 kΩ pull-up to 3V3 on the Pico side |
| VCC 3.3 V | 3V3(OUT) | 36 | 5 | — | — | direct |
| GND | GND | 38 | 4 | — | — | direct |
| 7.6 V | — | — | 3 | — | — | **not connected** (R-SAFETY-04) |
| unused | — | — | 8 | — | — | not connected |

"Pico physical pin" counts the board's 40 edge pins: pin 1 is top-left with the USB
connector at the top, pins 1–20 run down the left edge and 21–40 run back up the right edge.
Pin 39 is VSYS and pin 40 is VBUS — **neither is ever used here** (R-SAFETY-05); both carry
up to 5 V and the RP2040 is not 5 V tolerant.

## Parts

- 1 × PS2 female socket with all nine pins broken out.
- 5 × 330 Ω resistors (one per signal line).
- 2 × 10 kΩ resistors (pull-ups on DATA and ACK).
- 1 × Raspberry Pi Pico on a breadboard.
- Jumper wires, and a small piece of electrical tape or heat-shrink for socket pin 3.

## Breadboard layout, in words

1. Seat the Pico across the breadboard's centre channel, USB end at the top.
2. Run the Pico's 3V3(OUT) (pin 36) to one breadboard power rail and GND (pin 38) to the
   other. Call them the 3V3 rail and the GND rail. Nothing else feeds these rails.
3. For each of the five signals, put one 330 Ω resistor in its own breadboard row: one leg
   in a row wired to the Pico pin, the other leg in a row wired to the socket pin. The
   resistor bridges the gap; nothing else connects the Pico pin to the socket pin.
4. For DATA and ACK only, add a 10 kΩ resistor from the **Pico side** of that line's 330 Ω
   resistor to the 3V3 rail.
5. Wire socket pin 5 to the 3V3 rail and socket pin 4 to the GND rail.
6. Leave socket pins 3 and 8 unconnected. Insulate pin 3 (below).

## Why each resistor is there

**The five 330 Ω series resistors.** They are insurance against a mistake, not part of
normal operation. If a wire is swapped or a pin is configured wrong, two outputs can end up
driving one wire in opposite directions. Without a resistor that is a short limited only by
the chips themselves. With 330 Ω in the way, the worst case is 3.3 V / 330 Ω = 10 mA,
inside what an RP2040 pin can survive. At the bus's clock rate they cost nothing
measurable.

**The 10 kΩ pull-up on DATA.** DATA is open-drain: the guitar can pull it to 0 V but never
drives it to 3.3 V. Something has to bring it back up when the guitar lets go, or the line
floats and reads random values. The Pico's internal pull-up is enabled too, but it is weak
(roughly 50 kΩ) and gives slow edges; the external 10 kΩ sets the real edge rate. It sits on
the Pico side so the Pico always sees a defined level even with the guitar unplugged.

**The 10 kΩ pull-up on ACK.** Same reasoning as DATA: ACK is the guitar's open-drain
"I got that byte" pulse, and the pull-up restores it to 3.3 V between pulses.

## Socket pin 3 — 7.6 V, never connected

On a console, pin 3 carries 7.6 V for the vibration motors. The SG has no motors to feed,
and 7.6 V on any Pico pin destroys it (R-SAFETY-04). Leave the breakout's pin 3 wire or
terminal empty, and cover it: a wrap of electrical tape over the bare end, or a short piece
of heat-shrink, so it cannot touch a breadboard row by accident. The multimeter checklist in
`docs/phases/02-wiring/verify.md` confirms it is isolated before anything is powered.

## Sources

- Raspberry Pi Pico datasheet — pinout figure and §"Powering Pico" (3V3(OUT) load should be
  kept under 300 mA): <https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf>
- PS2 controller pinout (pin 1 DATA, 2 CMD, 3 motor power, 4 GND, 5 power, 6 ATT, 7 CLK,
  8 unused, 9 ACK): Curious Inventor, "Interfacing a PS2 (PlayStation 2) Controller",
  <https://store.curiousinventor.com/guides/PS2>

**Viewing side.** The cited source numbers the pins but does not say which face of the
connector its drawing is seen from, and pinout drawings online disagree on it. So this
document does not rely on a drawing: pins are identified from the numbering printed on the
breakout socket itself, then confirmed by continuity with the multimeter
(`docs/phases/02-wiring/verify.md`). If the breakout carries no numbering, stop and ask
before wiring.
