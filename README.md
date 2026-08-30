# pico-sg2hid

RP2040 firmware that turns a Raspberry Pi Pico into a driverless USB HID gamepad
for PlayStation 2 guitar controllers (RedOctane SG / Les Paul), with proper
analog-mode negotiation for the whammy bar. Linux and macOS.

## Status

Early. Nothing is implemented yet — this repo currently holds only the license
and this file. Scope and phases are still being defined.

## Why this exists

PS2 guitar controllers speak the standard PS2 controller protocol: a
bidirectional SPI-like bus, LSB-first, mode 3, clocked at roughly 250 kHz, with
a separate open-drain `ACK` line the controller pulls low after each byte.

Off-the-shelf PS2-to-USB adapters usually fail with these guitars for two
reasons:

- **Timing.** They poll on their own schedule and ignore or under-wait the `ACK`
  line, so the controller drops out of sync or is read as disconnected.
- **Analog mode.** The whammy bar is an analog axis. It only appears once the
  controller is put into analog mode (`0x79` response), which requires the
  config-mode command sequence. Adapters that only read the digital
  (`0x41`) report lose the whammy entirely.

Driving the bus directly from the Pico's PIO makes both of these controllable.

## Hardware

- Raspberry Pi Pico (RP2040 — the original board, not the Pico 2).
- PS2 female controller port.
- Pull-up resistors on `DATA` and `ACK`; both are open-drain on the controller
  side.

Signals used: `DATA`, `CMD`, `ATT` (chip select), `CLK`, `ACK`, `3V3`, `GND`.
The controller's logic is 3.3 V, so no level shifting is needed for the Pico.
The 7.6 V rail on the PS2 connector is only required by rumble motors and is
unused here.

GPIO assignment: to be defined.

## Host support

The device enumerates as a generic USB HID gamepad via TinyUSB, so no driver or
host-side software is needed on Linux or macOS.

## Building

Not yet — there is no source to build.

## License

MIT. See [LICENSE](LICENSE).
