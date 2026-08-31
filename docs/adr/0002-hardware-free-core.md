# ADR-0002: Keep all decision logic in a hardware-free core compiled by the host tests

- Status: accepted
- Date: 2026-08-31

## Context

Every bug class this project cares about — a mis-decoded frame, a wrong button map, a
whammy axis read from a digital frame, a negotiation state machine that gets stuck —
is pure logic. If that logic lives inside code that only compiles for ARM and only
runs with a guitar attached, then every test costs a flash cycle and a physical setup,
and the operator, who is not an embedded specialist, has no way to inspect it. The
project also has a hard requirement that rules be bound to deterministic tests; a test
that needs hardware is not deterministic.

## Decision

Layers, as directory prefixes, with dependencies pointing inward toward `core`:

| Layer | Directory | May depend on |
|---|---|---|
| `core` | `src/core/` | nothing outside itself and the freestanding C++ standard library |
| `hal` | `src/hal/` | `core` |
| `usb` | `src/usb/` | `core` |
| `app` | `src/app/` | `core`, `hal`, `usb` |
| `emu` | `src/emu/` | `core`, `hal` |

`core` holds the PS2 frame codec, the controller-id table, the negotiation state
machine, the button/axis mapping and the HID report builder, plus the pin declaration
table. It contains no `#include` of a Pico SDK, TinyUSB, or CMSIS header, performs no
I/O, and compiles unmodified with the host compiler — which is how `tests/` reaches it.

`hal` owns the PIO programs and GPIO for both bus roles (master, and the emulator's
device role). `usb` owns the TinyUSB descriptor and report transport. `app` is `main()`
and wiring. `emu` is a separate binary for the second Pico.

The prose above is enforced mechanically by `.claude/workflow/boundaries.rules` and the
edit hook. The one edge that file cannot express — "`core` must not reach the SDK",
since the SDK is not a repo directory — is enforced twice over: the host test build
gives `core` no SDK include paths, so a violation fails to compile, and a repo-wide
grep test names the rule explicitly.

## Consequences

Easier: the interesting logic is testable in milliseconds on the laptop with literal
byte vectors; the operator can read a test and see the protocol stated as data. A bug
found on the real guitar (phase `09`) becomes a new vector in `core`'s tests, not a
debugging session with hardware attached.

Harder: `core` cannot log, allocate, or touch a timer, so timing-dependent behaviour has
to be expressed as "given elapsed microseconds, decide" with the caller supplying the
clock. That inversion is deliberate and is also what makes timeouts testable.

Rejected:
- **One flat firmware tree** — fastest to start, and it makes the hard requirement
  (deterministic tests per rule) unsatisfiable, so it was never really available.
- **Compiling the SDK for the host with stubs** — drags the whole SDK into the test
  build to avoid writing an interface; the seam is cheaper than the shim.
