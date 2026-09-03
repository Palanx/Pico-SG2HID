# Phase index — pico-sg2hid

<!-- The single source of truth for execution state. Machine-parsed (grep/awk)
     and human-read after compaction — keep the table format EXACT.

     Status vocabulary (only these six, lowercase):
       pending      indexed, not yet expanded
       expanded     spec.md written, not started
       in-progress  implementation started (also: failed validation, being fixed)
       blocked: <reason>   needs an operator decision — reason is mandatory
       superseded by <ids> this cut turned out wrong; the ids that replace it
       done         validation passed; only /validate-phase writes this

     Rules:
       - `depends` lists phase ids, comma-separated, or `-`. These are edges,
         not an ordering: rows with no path between them may run in parallel.
       - Acceptance here is the coarse, one-line form; the executable form
         lives in the phase's spec.md once expanded.
       - Rows are append-only per feature section; never renumber ids. -->

## Feature: driverless SG-to-HID firmware  (/bootstrap-project 2026-08-31)

Turns a wired PlayStation 2 Guitar Hero SG into a USB HID gamepad on macOS and Debian,
with the whammy bar as a real analog axis, using a Raspberry Pi Pico. Deliberately out
of scope: the wireless Les Paul, any other PS2 device, rumble, Windows, the Pico 2, and
any host-side software — see `docs/product/requirements.md` §Non-goals.

The ordering is a safety property, not a preference (ADR-0004, ADR-0006): pure logic
first, then one Pico in loopback, then Pico against Pico, then the real guitar observed
read-only, and only then the real guitar in full. Phases `01` and `02` have no path
between them and may run in parallel.

| id | goal | depends | acceptance (coarse) | status |
|----|------|---------|---------------------|--------|
| 00-scaffold | Repo-shape checks and the rule↔test traceability meta-test | - | `make test` runs green from a clean clone with only clang++ and python3; every check is proven live by mutations generated from its own source — no alternative of any check pattern can be deleted, and no check neutered, with the suite still green — and the fourteen rules marked `planned: 00-scaffold` move to `test:` | pending |
| 01-ps2-codec | Pure PS2 frame codec, controller-id table, negotiation state machine, button/axis map, HID report builder — all in `src/core/`, no hardware | 00-scaffold | Host tests decode every literal vector in `tests/vectors/` correctly, including a missing-`ACK` abort and an unknown controller id; R-PROTO-01..04 move to `test:` | pending |
| 02-wiring | GPIO assignment, the `src/core/pins.h` table, the wiring document, and a multimeter checklist that teaches the operator to use one | 00-scaffold | Pin-table tests enforce R-SAFETY-01..03; the operator completes the continuity and voltage checklist with the guitar unplugged and records the SG's current draw, answering the two OPEN items in requirements.md | pending |
| 03-pio-bus | PIO bus master: clock generation, LSB-first shift, `ACK` wait with timeout, `ATT` framing | 01-ps2-codec, 02-wiring | One Pico in loopback (`CMD` wired to `DATA`) round-trips known byte sequences at the target clock rate; every error and timeout path releases `ATT` (R-SAFETY-07 to `test:`) | pending |
| 04-trace-mode | USB CDC bus trace from the firmware plus the host decoder in `tools/` | 03-pio-bus | A loopback session produces a trace the decoder renders as readable frames with per-byte `ACK` timings, and the operator can read one and say what happened on the bus | pending |
| 05-emulator | Second-Pico firmware in `src/emu/` that behaves as an SG: id, report bytes, `ACK` timing, digital or analog on command, plus injectable faults | 01-ps2-codec, 02-wiring | The emulator answers a manually driven frame correctly and can be told to drop an `ACK`, answer late, or report a wrong id; R-SAFETY-06 moves to `test:` | pending |
| 06-hil-digital | Hardware-in-the-loop: the master polls the emulator in digital mode | 03-pio-bus, 05-emulator | An automated two-Pico run polls for a sustained period with zero desyncs, and each injected fault produces the specified recovery instead of a hang | pending |
| 07-analog-mode | Config-mode sequence, analog-mode verification, whammy axis, against the emulator | 06-hil-digital | The master negotiates analog mode, verifies the analog header before reading the axis, and reads the emulator's swept whammy value end to end | pending |
| 08-usb-hid | TinyUSB HID gamepad descriptor and report transport, fed by emulator data | 07-analog-mode | macOS and Debian both enumerate the device with no driver and show every fret, both strums, and a continuous whammy axis in a standard gamepad tester | pending |
| 09-guitar-observe | First contact with the real SG, read-only through trace mode; reconcile the captured bytes with the codec | 04-trace-mode, 08-usb-hid | The operator captures real traces following a written procedure; every discrepancy against the codec becomes a new literal vector and a codec fix, or a recorded finding in `docs/constraints.md` | pending |
| 10-guitar-full | Full pipeline on the real guitar: negotiation, polling, hotplug recovery, latency | 09-guitar-observe | The guitar plays through a gamepad tester on both hosts across repeated unplug/replug cycles with measured end-to-end latency inside one USB poll interval | pending |
| 11-release | Flashable UF2, build and flash instructions, host-side setup notes for both OSes | 10-guitar-full | A clean clone builds the UF2 following only the README, and a second machine flashes it and plays without further steps | pending |
