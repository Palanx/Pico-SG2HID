# Requirements — pico-sg2hid

<!-- Written once by /bootstrap-project, amended only by operator decision.
     This is WHAT and WHY. HOW lives in constraints.md and the ADRs. -->

## Purpose

Firmware for a Raspberry Pi Pico (RP2040) that reads a wired PlayStation 2 Guitar
Hero SG controller over the PS2 controller bus and presents it to the host as a
driverless USB HID gamepad, with the whammy bar exposed as a real analog axis.
Off-the-shelf PS2-to-USB adapters fail at this for two reasons: they poll on their
own schedule instead of waiting for the controller's `ACK` handshake, and they
never put the controller into analog mode, so the whammy does not exist. Driving
the bus from the RP2040's PIO makes both controllable.

The operator is not an electronics or driver specialist. A second goal, equal in
weight to the firmware itself, is that every phase leaves behind an explanation and
a physical check the operator can perform to judge whether the work is correct.

## Users

- **The operator** (project owner): plugs the Pico into a Mac or a Debian machine
  and plays. Also builds, flashes, and physically verifies each phase.
- **Any host OS**, macOS or Debian, acting as a plain USB HID consumer with no
  driver, kernel module, daemon, or configuration software installed.

## Capabilities

- **R1 — Driverless HID gamepad.** The device enumerates on macOS and Debian as a
  standard USB HID gamepad. No driver, no host-side software, no configuration.
- **R2 — Correct PS2 bus mastering.** The firmware clocks the bus LSB-first, SPI
  mode 3, at the documented rate, and waits for the controller's open-drain `ACK`
  after every byte but the last. A missing `ACK` aborts the frame and marks the
  controller absent rather than corrupting state.
- **R3 — Analog mode with a working whammy.** The firmware runs the config-mode
  sequence, verifies the controller answers with the analog header, and maps the
  whammy bar to an analog HID axis. It never reads an analog byte from a frame that
  did not report analog mode.
- **R4 — Hotplug survival.** Unplugging and replugging the guitar, or a transient
  bus fault, is detected and recovered from by re-running negotiation. The host
  never sees the HID device disappear.
- **R5 — Bus trace mode.** The firmware can dump every byte it sends and receives,
  plus `ACK` timing, over USB CDC in a format a host script decodes into readable
  frames. This is the project's instrument: it replaces the logic analyzer the
  operator does not have, and it is how the operator checks the firmware's claims
  against reality.
- **R6 — Guitar emulator on a second Pico.** A separate binary makes a second Pico
  behave as an SG on the bus, so the whole pipeline can be tested automatically,
  end to end, without the real guitar being connected at all.
- **R7 — Operator verification path.** Every phase ships a `verify.md` written for a
  non-specialist: what was built, what it should do, and the exact physical steps
  and expected readings that confirm it.

## Non-goals

Deliberately excluded. Each is a decision, not an oversight.

- **The wireless Les Paul and its dongle.** The dongle speaks the same protocol but
  adds link latency and spontaneous disconnects; supporting it means a different
  reconnection model. Wired SG only.
- **Any other PS2 device** — World Tour, Rock Band, DualShock, DualShock 2, guitar
  controllers from other games. The firmware targets one controller id and one
  button map. Broad device support is a different project.
- **Rumble.** Rumble motors need the 7.6 V rail on the PS2 connector. That rail
  stays unconnected, which is also a safety invariant (see Hard constraints).
- **Windows.** It may well work, since the device is generic HID. It will not be
  tested, and a Windows-only bug is not a defect here.
- **Pico 2 / RP2350.** Original RP2040 Pico only. The PIO differences are real and
  supporting both doubles the hardware matrix with one board to test on.
- **Host-side software of any kind** — no driver, no daemon, no mapping GUI, no
  configuration file. If the device needs host software to be usable, the design is
  wrong. (The trace decoder in `tools/` is a development instrument, not part of the
  product; the device works with it absent.)
- **Impersonating the PS3 guitar's USB identity.** Cloning another product's
  VID/PID would make some games auto-detect it. It is another vendor's identifier
  and it is not being used. See ADR-0003.
- **More than one guitar per Pico.** One bus, one controller.
- **Custom PCB, enclosure, or a soldered product.** Breadboard and a PS2 female
  socket. The deliverable is firmware plus a wiring document.
- **Preserving the guitar's original PS2 function.** The guitar is expected to keep
  working on a real PS2 afterwards, but nothing is done to guarantee it and it is
  not tested.

## Hard constraints

Externally imposed. Violating one of these damages hardware or breaks the product
promise; they are not negotiable by a later ADR without new hardware.

- **3.3 V only.** The PS2 controller's logic is 3.3 V and the RP2040's GPIOs are not
  5 V tolerant. No signal above 3.3 V may reach any Pico GPIO, ever.
- **`DATA` and `ACK` are controller-driven, open-drain.** The Pico must never drive
  them as outputs. Driving `DATA` while the controller drives it is a direct short
  between two output stages.
- **The 7.6 V rail on the PS2 connector is never connected to anything.** It exists
  for rumble motors, which are a non-goal.
- **The guitar is powered from the Pico's 3V3 pin**, not VBUS and not VSYS (both
  5 V). The onboard regulator's budget is the ceiling.
- **USB full-speed HID, 1 ms polling.** The host contract is a standard HID gamepad
  report; latency added by the firmware must stay well inside one poll interval.
- **No kernel driver on either host OS**, and no code signing, entitlement, or
  privileged install step on macOS.
- **Every rule stated as a project rule must either be bound to a deterministic
  automated test, or explicitly marked as manual with the reason it cannot be
  machine-checked.** An unbound rule is tracked technical debt with an owning phase,
  not an accepted state. See ADR-0005 and `docs/constraints.md` §Invariants.

## Open questions

- OPEN: Does the SG operate correctly from the Pico's 3V3 rail alone, and what is
  its actual current draw? Answered by the operator with a multimeter in phase
  `02-wiring`. If the draw exceeds the onboard regulator's headroom, phases `03`
  onward need an external 3.3 V supply and the wiring document changes shape.
- OPEN: Is the physical connector on hand a PS2 female socket with all pins broken
  out, or a cut controller extension cable? This changes phase `02-wiring` from
  "identify pins by datasheet" to "identify pins by continuity test", which is a
  different set of instructions for the operator. Answered by the operator.

<!-- Byte-level unknowns (exact config-mode sequence, whammy byte position, rest and
     full-deflection values, the controller id this specific SG reports) are NOT
     listed here. They do not block design: the codec is written against the
     documented protocol with literal test vectors, and phase 09-guitar-observe
     exists specifically to confront it with the real device and correct it. -->
