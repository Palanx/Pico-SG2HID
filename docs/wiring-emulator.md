# Wiring: emulator Pico to the master's breadboard

This page covers the two-Pico bench from phase `05-emulator`. The master Pico stays wired exactly as `docs/wiring.md` describes. A second Pico, flashed with `build/pico/sg2hid_emu.uf2`, takes the guitar's place: it connects where the PS2 socket would. **No guitar is anywhere on this bench** (R-SAFETY-08).

The emulator's GPIOs come from `kEmulatorPins` in `src/core/pins.h`, and they are the same numbers as the master's. This is a separate file from `docs/wiring.md` because R-SAFETY-02's check reads every `| SIGNAL | GP<n> |` row in that file as the master's table. Nothing checks this table against `kEmulatorPins`. It mirrors the master's GPIOs, so keep it that way.

## Pin table

| Signal | Emulator GPIO | Emulator physical pin | Joins, on the master breadboard | Emulator drives it? |
|---|---|---|---|---|
| DATA | GP2 | 4 | the socket-side row of DATA's 330 Ω resistor | yes, open-drain: it pulls low or lets go |
| CMD | GP3 | 5 | the socket-side row of CMD's 330 Ω resistor | no, it only reads |
| ATT | GP4 | 6 | the socket-side row of ATT's 330 Ω resistor | no, it only reads |
| CLK | GP5 | 7 | the socket-side row of CLK's 330 Ω resistor | no, it only reads |
| ACK | GP6 | 9 | the socket-side row of ACK's 330 Ω resistor | yes, open-drain: it pulls low or lets go |

Also connect **GND**: emulator physical pin 38 to the master breadboard's GND rail. Without a shared ground, the two Picos do not agree on what 0 V is, and no signal means anything.

"Socket-side row" means the breadboard row where the 330 Ω resistor's far leg sits, away from the master Pico. If the PS2 socket breakout is still on the breadboard, that is the row wired to the socket pin. Put the emulator's wire in that same row.

## What is never connected

- **The emulator's 3V3 (pin 36), VSYS (pin 39) and VBUS (pin 40).** Each Pico powers itself from its own USB cable. Joining two Picos' power pins puts two regulators against each other.
- **Socket pin 5 (the guitar's 3.3 V supply).** That supply is for a guitar, and there is no guitar here. Do not run a wire from it to the emulator.
- **Socket pin 3 (7.6 V).** It stays insulated, exactly as `docs/wiring.md` says (R-SAFETY-04). Nothing on this bench changes that.

## Power order: both Picos on USB before the signal wires

1. Plug **both** Picos into USB, master first or emulator first, it does not matter. Wait until both are running: each one's serial port appears on the Mac.
2. Only then push the five signal wires and the GND wire into place.
3. To take the bench apart, pull the signal wires first, then unplug USB.

Why: an RP2040 with no power still has protection diodes from every pin to its own 3.3 V supply. If the master is powered and the emulator is not, the master's 3.3 V signals flow through those diodes into the unpowered chip. This is called back-feeding, and it half-powers the unpowered chip in an undefined way. The 330 Ω resistors on the master breadboard limit that current to under 3.3 V / 330 Ω = 10 mA per line, which a pin survives. Powering both first means it never happens at all.

## Why no new resistors

Each signal already passes through one 330 Ω resistor on the master breadboard, so a mistake still meets the same 10 mA limit as in `docs/wiring.md`. Examples of mistakes: a wire in the wrong row, or a firmware bug. The emulator also adds no pull-ups. It lets DATA and ACK go, and the master's 10 kΩ pull-ups bring them back to 3.3 V, just as they would for the guitar.

When the emulator pulls one of those lines low, the master's pin sees about 3.3 V × 330 Ω / (10 kΩ + 330 Ω) ≈ 0.1 V. That reads as a clean 0.

Each emulator pin is forced so it can only pull low, never drive 3.3 V (ADR-0016). So even if DATA landed on the master's CMD by mistake, two outputs could never push against each other.
