# ADR-0003: Enumerate as a generic HID gamepad under our own USB identity

- Status: accepted
- Date: 2026-08-31

## Context

The device has to work on macOS and Debian with no driver. Two shapes achieve that: a
generic HID gamepad with a descriptor we author, or a clone of the official PlayStation
3 Guitar Hero controller's USB identity, which rhythm games recognise without the user
mapping anything. The second is more convenient and uses another vendor's VID/PID and
descriptor as our own.

## Decision

The device presents a HID gamepad descriptor authored in this repo: five fret buttons,
strum up and strum down, start and select, the tilt/effects switch if present, and the
whammy bar as a single analog axis. It enumerates under a USB identity that is ours —
during development, the Raspberry Pi vendor id with a Pico SDK test product id, which
is what the SDK ships and what is correct for a non-distributed device.

No VID/PID, product string, or descriptor is copied from another vendor's product.
Games that need a specific device get mapped by hand, once, by the user.

## Consequences

Easier: no legal or identity ambiguity; the descriptor says exactly what the hardware
is, so `evtest` on Debian and Hammerspoon or a browser gamepad tester on macOS show
sensible names; a bug in the descriptor is our bug and readable in our source.

Harder: software that auto-detects controllers by VID/PID — Clone Hero among them —
will not recognise it as a guitar without a one-time manual mapping. That mapping is a
documented step in the release phase, not a defect.

Rejected:
- **Cloning the PS3 guitar's identity** — better out-of-box behaviour in one
  application, obtained by claiming to be someone else's product.
- **HID keyboard output** — universally compatible and throws away the whammy axis
  entirely, which is the single feature this project exists to recover.
