# Phase 02-wiring — how to verify it on the bench

Written for someone who has never used a multimeter. It has two halves. The first needs only
the laptop. The second needs the breadboard, the PS2 socket, the Pico, the guitar and a
multimeter. Budget about 45 minutes the first time.

## What was built

Three things, and **no firmware**. Nothing in this phase ever runs on the Pico.

| File | What it is |
|---|---|
| `src/core/pins.h` | The pin table. It lists the five wires of the PS2 bus, which Pico GPIO each one uses, and whether the Pico drives it or only reads it. It is the only place in the project allowed to say this. |
| `docs/wiring.md` | The same assignment as seen from the breadboard: Pico pin, socket pin and the resistor in between. Build the circuit from this document. |
| `tests/test_pin_table.py` + `tests/pin_table_cases.cpp` | The check that fails `make test` if the table ever becomes dangerous, or if `wiring.md` and the table disagree. |

There are also new rules and a new ADR. **R-SAFETY-09** makes `make test` fail if code
outside `src/hal/` ever configures a pin (`tests/test_repo_shape.sh`). **ADR-0013** allows
exactly one early contact with the guitar in this phase: measuring its current draw with
power only.

### The five bus wires, in one paragraph each

- **CLK** (clock), **CMD** (command) and **ATT** (attention) are driven by the Pico. ATT low
  means "guitar, I'm talking to you". CLK ticks once per bit. CMD carries the Pico's bits.
- **DATA** carries the guitar's bits back. **ACK** is the guitar's "got that byte" pulse.
  The guitar drives both, and it only ever pulls them *down* to 0 V ("open-drain"). A
  resistor to 3.3 V (the "pull-up") brings them back up. If the Pico also drove one of these
  wires, two chips would fight over it, which is the short circuit R-SAFETY-01 forbids. The
  pin table marks them `Input` and `OpenDrainInputOnly`, and the test refuses anything else.

## Check it yourself

### Part A — on the laptop (2 minutes)

```sh
make test
```

**Expected:** the last line says `OK`. In the `--- tests/test_pin_table.py` block you should
see:

```
  pin: DATA GP2
  pin: CMD GP3
  pin: ATT GP4
  pin: CLK GP5
  pin: ACK GP6
  ok:   R-SAFETY-01 (DATA and ACK are inputs, never push-pull)
  ok:   R-SAFETY-02 (table): every bus signal declared exactly once
  ok:   R-SAFETY-03 (no reserved GPIO, no GPIO shared)
  ok:   rejection cases: 11/11 ...
  ok:   wiring cases: 3/3 ...
  ok:   R-SAFETY-02 (wiring doc): 5 signals match
  ok:   copied-tree rejection cases: 4/4 ...
```

**See it bite.** Open `src/core/pins.h`. In the DATA entry, change
`DriveMode::OpenDrainInputOnly` to `DriveMode::PushPull` and run
`python3 tests/test_pin_table.py`. The R-SAFETY-01 line must now say `FAIL:` and the command
must end with an error. Undo the change and run it again to see it pass. That is the rule
catching, on a laptop, the mistake that would damage hardware on the bench.

### Part B — a multimeter primer (read before touching anything)

A multimeter has one dial and usually three or four holes ("jacks") for the probes.
**The black probe always goes in `COM`.** Where the red probe goes depends on what you
are measuring:

| Mode | Dial symbol | Red probe jack | What it tells you |
|---|---|---|---|
| Resistance / continuity | `Ω`, often sharing the dial position with a speaker or diode symbol | `VΩ` (often labelled `VΩmA` or `VΩHz`) | Whether two points are connected, and through how much resistance. In continuity mode it **beeps** when they are connected (below roughly 30–50 Ω). |
| DC volts | `V` with a straight line (`V⎓`), **not** the wavy `V~` | `VΩ` | The voltage *between* the two probes. |
| DC milliamps | `mA` or `A` with a straight line | `mA` (sometimes `µAmA`), **not** the `10A` jack unless told to | The current flowing *through* the meter. |

Three things that are not obvious:

1. **Resistance and continuity are measured with the circuit unpowered.** The meter sends
   its own tiny current to measure. Anything else pushing current confuses the reading and
   can damage the meter.

2. **In current mode the meter is a piece of wire.** Measuring current means the current has
   to flow *through* the meter, so inside it is a near-zero resistance. If you put the probes
   across a voltage (for example 3V3 to GND) while in mA mode, you have connected a wire
   straight across the supply. That is a short circuit. At best it blows the meter's internal
   fuse. At worst it damages the Pico's regulator. So: **only switch to mA once the meter is
   already wired *in series*, in the gap where a wire used to be.** When you finish, move the
   red probe back to the `VΩ` jack immediately, so the next voltage measurement does not
   become a short.

3. **3V3 to GND on a Pico reads a climbing value, not a fixed one.** The Pico has capacitors
   between 3V3 and GND. A capacitor is two plates with a gap, and the meter's test current
   slowly charges it. The reading starts low (tens or hundreds of Ω) and climbs: kΩ, then
   MΩ, sometimes up to `OL` ("over limit", i.e. open). That climb is normal. **A reading that
   stays near 0 Ω and does not climb is a short.** Stop there.

"≈ 330 Ω" means anything from about 310 to 350 Ω. Resistors have a tolerance, the probes add
a little, and a meter is not perfect.

### Part C — the checklist

Build the circuit from `docs/wiring.md` first. Work in order and do not skip ahead. **If any
reading is wrong, stop.** Unplug everything, recheck the wiring against `docs/wiring.md`,
and ask before continuing.

"Pico pin N" is the physical pin number on the board edge: pin 1 is top-left with the USB
connector at the top, 1–20 run down the left side and 21–40 back up the right. "Socket
pin N" is the number printed on the PS2 breakout.

#### C1 — Unpowered: USB unplugged, guitar unplugged

Meter on resistance/continuity, red probe in `VΩ`.

| # | Probes | Expected | Your reading |
|---|---|---|---|
| 1 | Pico 4 (GP2) ↔ socket 1 (DATA) | ≈ 330 Ω | |
| 2 | Pico 5 (GP3) ↔ socket 2 (CMD) | ≈ 330 Ω | |
| 3 | Pico 6 (GP4) ↔ socket 6 (ATT) | ≈ 330 Ω | |
| 4 | Pico 7 (GP5) ↔ socket 7 (CLK) | ≈ 330 Ω | |
| 5 | Pico 9 (GP6) ↔ socket 9 (ACK) | ≈ 330 Ω | |
| 6 | Pico 36 (3V3) ↔ socket 5 | beeps (≈ 0 Ω) | |
| 7 | Pico 38 (GND) ↔ socket 4 | beeps (≈ 0 Ω) | |
| 8 | Pico 36 (3V3) ↔ Pico 38 (GND) | **not** near 0 Ω; climbs (see primer point 3) | |
| 9 | Socket 3 (7.6 V) ↔ Pico 36, then Pico 39, then Pico 40 | no beep, `OL` each time | |
| 10 | Socket 3 ↔ socket 1, 2, 6, 7, 9 (each signal) | no beep, `OL` each time | |
| 11 | Socket 5 ↔ Pico 39 (VSYS), then Pico 40 (VBUS) | no beep, `OL` each time | |
| 12 | Every pair of socket 1, 2, 6, 7, 9 (ten pairs) | none reads ≈ 0 Ω | |

Why these matter: items 1–7 prove each wire goes where `wiring.md` says. Item 8 proves the
rails are not shorted before you apply power. Items 9–10 prove the 7.6 V pin touches nothing
(R-SAFETY-04). Item 11 proves the guitar's power pin is not on a 5 V pin (R-SAFETY-05).
Item 12 proves no two signal wires touch each other.

About item 12: DATA and ACK are each joined to 3V3 by a 10 kΩ pull-up, so DATA ↔ ACK reads
about 330 + 10 000 + 10 000 + 330 ≈ 20.7 kΩ. That is expected, and it is not a bridge.
A bridge reads near 0 Ω.

#### C2 — Powered: Pico on USB, held in BOOTSEL, guitar unplugged

**BOOTSEL:** hold the white `BOOTSEL` button on the Pico, plug in the USB cable, then
release the button. The laptop shows a drive called `RPI-RP2`. In this mode the Pico runs
only its built-in bootloader: no program of ours runs, and every GPIO is an input. Do not
copy anything to that drive.

Meter on DC volts (`V⎓`), red probe in `VΩ`. Black probe on socket 4 (GND) for every row.

| # | Red probe on | Expected | Your reading |
|---|---|---|---|
| 13 | Socket 5 (3V3) | 3.2–3.4 V | |
| 14 | Socket 3 (7.6 V pin) | ≈ 0 V | |
| 15 | Socket 1 (DATA) | ≈ 3.3 V (the pull-up) | |
| 16 | Socket 9 (ACK) | ≈ 3.3 V (the pull-up) | |

Item 14 is the one that matters most. Any voltage there means something is feeding the
7.6 V pin. Unplug the USB and stop.

Unplug the USB when done.

#### C3 — Current draw: power only, guitar connected (ADR-0013)

This is the only time in this phase the guitar is connected, and only its power pins are
live. Read all the steps before starting.

1. **USB unplugged.** Remove the five 330 Ω signal resistors from the breadboard. Now no
   signal wire can reach the guitar. The 10 kΩ pull-ups stay: their far ends were on the
   Pico side of the 330 Ω resistors, so they reach nothing on the socket.
2. Remove the jumper between the **3V3 rail and socket 5**. That gap is where the meter
   goes.
3. Move the red probe to the **`mA` jack** and set the dial to **DC mA** (the 200 mA or
   400 mA range if your meter has ranges; `A` if it has no mA range). Clip or hold the
   **red probe on the 3V3 rail** and the **black probe on socket 5**. The meter is now the
   missing wire: the guitar's current has to flow through it. **Never** touch these probes
   to socket 4 or any GND point while in this mode (primer point 2).
4. Plug the guitar into the socket.
5. Hold BOOTSEL and plug in the USB, as in C2.
6. Read the meter and wait a few seconds for it to settle. Write down the value, and note
   whether it moves (for example if the guitar has LEDs that blink).
7. **Unplug the USB first**, then the guitar.
8. Move the red probe back to `VΩ` and the dial back to V or Ω. Put the 3V3 → socket 5
   jumper and the five 330 Ω resistors back.

| # | Measurement | Expected | Your reading |
|---|---|---|---|
| 17 | Guitar current on 3V3 | well under 250 mA, likely tens of mA | |
| 18 | Does it move? | steady, or small flicker | |

**Stop condition:** a reading **≥ 250 mA** means the Pico's onboard regulator is not enough
for the guitar. The datasheet asks for under 300 mA on 3V3, and the Pico itself needs some
of that. Stop and report it: that is a re-plan, not something to work around.

The meter itself drops a little voltage, so the guitar sees slightly under 3.3 V during this
measurement. That is normal. It does not tell you whether the guitar *works* on 3.3 V,
which needs bus traffic and belongs to phase `09-guitar-observe`.

#### C4 — Report back

Send the filled-in tables, plus two facts:

- **The connector:** what exactly is on the bench (for example "PS2 female socket
  breakout, all 9 pins, numbering printed on the board"), and whether the numbering
  matched item 1–7's continuity readings.
- **The current:** item 17's number in mA.

These close requirements.md's connector question and the current-draw half of its power
question. The session that receives them writes them there and into `notes.md`.

## What this phase does NOT prove

- That the guitar works on 3.3 V. That needs the guitar to answer the bus, in phase
  `09-guitar-observe`.
- That the firmware configures the pins correctly. There is no firmware yet. Phase
  `03-pio-bus` writes `src/hal/`, and R-SAFETY-10 is what will check it.
- That the function names in R-SAFETY-09's list are spelled the way the installed Pico SDK
  spells them. `03-pio-bus` checks that against the real headers.
