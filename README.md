# pico-sg2hid

RP2040 firmware that turns a Raspberry Pi Pico into a driverless USB HID gamepad
for PlayStation 2 guitar controllers (RedOctane SG / Les Paul), with proper
analog-mode negotiation for the whammy bar. Linux and macOS.

## Status

Scope, architecture and the phase plan are defined. **No firmware is implemented
yet** — the repository currently holds the project documentation, the build entry
point, and the style gate.

- Phase index: [`docs/phases/PHASES.md`](docs/phases/PHASES.md) — 12 phases, all
  `pending`.
- `make test` and `make lint` run today; there is nothing to build for the Pico
  yet.

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
- A **second** Pico, which runs a guitar emulator so the firmware can be tested
  end to end without the real controller attached. See
  [ADR-0004](docs/adr/0004-second-pico-as-guitar-emulator.md).
- PS2 female controller port.
- Pull-up resistors on `DATA` and `ACK`; both are open-drain on the controller
  side.

Signals used: `DATA`, `CMD`, `ATT` (chip select), `CLK`, `ACK`, `3V3`, `GND`.
The controller's logic is 3.3 V, so no level shifting is needed for the Pico.
The 7.6 V rail on the PS2 connector is only required by rumble motors and is
unused here.

GPIO assignment: defined in phase `02-wiring`, in `src/core/pins.h`.

## Safety

The RP2040 is **not 5 V tolerant**, and the guitar is not replaceable. Four rules
hold at all times; the full set, with their enforcement, is in
[`docs/constraints.md`](docs/constraints.md) §Invariants and
[ADR-0006](docs/adr/0006-fail-safe-hardware-policy.md):

- `DATA` and `ACK` are driven by the controller. The Pico never configures them
  as outputs — doing so shorts two output stages together.
- The 7.6 V rail is never connected to anything.
- The guitar is powered from the Pico's `3V3` pin, never VBUS or VSYS.
- Order of first contact is fixed: host tests → loopback on one Pico → Pico
  against the emulator → the real guitar observed read-only → the real guitar in
  full. Untested bus code never meets the guitar.

## Host support

The device enumerates as a generic USB HID gamepad via TinyUSB, so no driver or
host-side software is needed on Linux or macOS. It uses its own USB identity and
does not impersonate any other product; software that auto-detects controllers by
USB id needs a one-time manual mapping. See
[ADR-0003](docs/adr/0003-generic-hid-gamepad-own-identity.md).

## Building

`make` is the only entry point.

```sh
make test      # host tests: logic only, no hardware, no network
make lint      # style gate: clang-format layout + clang-tidy naming
make firmware  # .uf2 images (not yet — no sources)
make clean
```

`make test` needs only a C++17 compiler and `python3`. `make lint` additionally
needs `clang-format` and `clang-tidy` (`brew install clang-format llvm`);
`make test` skips the style check when they are absent, `make lint` fails.

`make firmware` needs `cmake`, `arm-none-eabi-gcc` and `PICO_SDK_PATH`. None of
that is required until phase `03-pio-bus`.

## Layout

```
src/core/    pure logic and data: PS2 codec, state machine, HID mapping, pin table.
             No I/O, no SDK headers — this is what the host tests compile and run.
src/hal/     PIO programs and GPIO, for both the master and the emulator roles.
src/usb/     TinyUSB HID descriptor and report transport.
src/app/     main() and wiring.
src/emu/     second-Pico firmware: behaves as an SG on the bus.
tests/       host tests and repo-shape checks. `make test` runs everything here.
tools/       host-side development instruments (bus trace decoder).
```

Dependencies point inward toward `core`; the allowed edges are enforced on every
edit from `.claude/workflow/boundaries.rules`.

## Documentation

| What | Where |
|---|---|
| What is being built, and what is explicitly out of scope | [`docs/product/requirements.md`](docs/product/requirements.md) |
| Standing rules, the rule catalog, invariants | [`docs/constraints.md`](docs/constraints.md) |
| Decisions and their rationale | [`docs/adr/`](docs/adr/) |
| Phase index and status | [`docs/phases/PHASES.md`](docs/phases/PHASES.md) |

Every rule in the catalog carries an id and exactly one binding: a deterministic
test, an explicit `manual:` with the reason it cannot be machine-checked, or
`planned:` naming the phase that owes the test. Drift between a rule and its test
fails the build. See [ADR-0005](docs/adr/0005-rule-test-traceability.md).

## License

MIT. See [LICENSE](LICENSE).
