# ADR-0016: The emulator drives DATA and ACK open-drain

- Status: accepted
- Date: 2026-10-02

## Context

Phase `05-emulator` adds a second Pico that plays the guitar's side of the bus. On that Pico,
`DATA` and `ACK` have to become outputs. On the master they are open-drain inputs with a
pull-up (R-SAFETY-01). In a real PS2 controller, both lines only ever pull low or let go. If the
emulator drove them push-pull, a wiring mistake would put two output stages against each other.
For example, `DATA` could land on the master's `CMD`, or a second emulator could share the bus.
R-SAFETY-10 currently forbids every `pindirs` write in a `.pio` file. That rule assumed the only
program was the master's, and that the master never makes a bus pin an output from PIO.

## Decision

The emulator drives `DATA` and `ACK` open-drain, as follows:

- `src/hal/pio_device.cpp` forces each pin's pad output low with
  `gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW )`. It does this after the pin is handed to
  PIO, because `gpio_set_function` clears the override, and before the program can set any
  direction.
- `src/hal/ps2_device.pio` then writes only `pindirs` for those pins:
  - output means "pull low";
  - input means "released", and the master's pull-up takes the line high.
- Every such `.pio` line carries the comment `; open-drain`.

R-SAFETY-10 is amended in the same change:

- It refers to "a `src/core/pins.h` table", so it covers both `kMasterPins` and
  `kEmulatorPins`.
- A `pindirs` line marked `; open-drain` is excused, but only while the outover call is
  present under `src/hal/`.

## Consequences

- **Easier.** Neither emulator pin can drive high whatever the program does, so a program bug
  cannot short `DATA` or `ACK` to a high output on the other side. The electrical behaviour
  matches the guitar this emulator stands in for.
- **Harder.** The emulator program has to invert each byte, because a 0 bit is "output" and a 1
  bit is "released".
- **Debt taken knowingly.** The R-SAFETY-10 check takes the `; open-drain` marker on trust. The
  marker does not prove the pin is open-drain. The check also confirms that the outover call
  exists somewhere, but not for each pin. R-SAFETY-10's Scope clause records both limits.
- **Rejected: push-pull emulator outputs.** This is simpler, since `out pins` needs no inversion
  and no override. But a single miswire or program bug becomes a short between two outputs,
  which is the hazard R-SAFETY-01 exists to rule out.
