# ADR-0006: Make hardware damage structurally hard — one pin table, verified before power

- Status: accepted — amended by ADR-0013: the "nothing is powered before it is measured"
  ordering sentence is superseded for the power pins only, so the guitar's current draw
  can be measured in phase `02-wiring` with 3V3 and GND alone. Everything else stands.
- Date: 2026-08-31

## Context

The operator's explicit requirement is that it must not be possible to destroy the Pico
or the guitar. The realistic ways to do that here are few and well understood:

1. Driving `DATA` or `ACK` as a push-pull output while the controller drives them —
   two output stages fighting, current limited only by the silicon.
2. Putting more than 3.3 V on a GPIO. The RP2040 is not 5 V tolerant; the 7.6 V rail
   on the PS2 connector is right there on the same socket.
3. Powering the guitar from VBUS or VSYS (5 V) instead of the 3V3 pin.
4. Miswiring — a swapped pair, a short between 3V3 and GND — discovered by applying
   power.
5. Configuring a pin the Pico board reserves for its own use (GPIO 23, 24, 25, 29).

None of these are subtle, and all of them happen when pin configuration is scattered
across files and when power is applied before anything has been measured.

## Decision

**One pin table.** Every GPIO this project uses is declared exactly once, in
`src/core/pins.h`, as `constexpr` data: number, signal name, direction, and drive mode
(`input_pullup`, `push_pull`, `open_drain_input_only`). No file outside `src/hal/` may
call `gpio_init`, `gpio_set_dir` or `gpio_pull_up`, and `src/hal/` configures pins only
by iterating that table. Because the table is `constexpr` data in `core`, the host tests
can assert its contents directly — that `DATA` and `ACK` are never `push_pull`, that no
entry lands on a reserved GPIO, that no two signals share a pin, and that the emulator's
table does not put a push-pull output opposite the master's.

**Nothing is powered before it is measured.** Phase `02-wiring` produces a wiring
document and a multimeter checklist the operator runs *with the guitar unplugged and
the Pico unpowered*: continuity on every intended connection, absence of continuity
between 3V3 and GND, correct pin identification on the socket. The guitar is connected
only after the checklist passes and after the firmware is already running and verified
against the emulator. That checklist includes an explanation of how to use a multimeter,
because the operator has asked for one.

**The order of first contact is fixed**, and is the ordering in ADR-0004: loopback on
one Pico, then Pico against Pico, then the real guitar observed read-only through trace
mode, then the real guitar in full. Untested bus code never meets the guitar.

Rules 1, 3 and 5 above become invariants with tests; rules 2 and 4 are physical and
become `manual:` invariants owned by the phase-`02` checklist. Both kinds are in
`docs/constraints.md` §Invariants under ADR-0005's grammar.

## Consequences

Easier: the dangerous configurations are unreachable from code without editing one
`constexpr` table, and editing it wrongly fails `make test` before anything is flashed.
The operator gets a procedure instead of a warning.

Harder: a genuine need for a pin configured outside the table — a debug LED, a mode
jumper — has to go through the table too, which is mild friction on purpose. The
checklist is manual and can be skipped by a person in a hurry; nothing in software
prevents that, and pretending otherwise would be worse than saying it here.

Rejected:
- **Series resistors on every line as the primary defence** — worth adding as belt and
  braces on `DATA` and `ACK`, and it is not a substitute: it limits fault current, it
  does not prevent the fault, and it does nothing about 7.6 V.
- **Comments warning "do not drive DATA"** — a comment is not an enforcement mechanism,
  which is the entire premise of ADR-0005.
