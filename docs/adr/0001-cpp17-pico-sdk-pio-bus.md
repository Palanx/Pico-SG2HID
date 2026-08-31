# ADR-0001: Write the firmware in C++17 on the Pico SDK, and drive the PS2 bus from PIO

- Status: accepted
- Date: 2026-08-31

## Context

The PS2 controller bus is a bidirectional, SPI-like, LSB-first, mode-3 bus at roughly
250 kHz, with an out-of-band open-drain `ACK` line that the controller pulls low for a
few microseconds after each byte. The whole reason existing adapters fail is timing:
they under-wait or ignore `ACK`. So the bus driver needs deterministic, microsecond-
scale control of clock edges and of the gap between bytes — the exact thing an
interpreted runtime cannot promise and a general-purpose SPI peripheral cannot express,
because the RP2040's SPI block has no notion of waiting on an unrelated GPIO between
bytes. The operator asked for C++ if practical.

## Decision

Firmware is C++17 built with the Raspberry Pi Pico SDK, targeting the RP2040 Pico
only. The PS2 bus is driven by a PIO state machine: PIO generates `CLK`, shifts `CMD`
out and `DATA` in LSB-first, and the `ACK` wait is a PIO `wait` on the `ACK` pin with a
CPU-side timeout, so no byte gap depends on interrupt latency or scheduler jitter.

`make` is the single entry point for the operator. `make test` compiles and runs the
host tests with the system C++ compiler alone — no CMake, no ARM toolchain, no SDK —
so logic work is never blocked on a toolchain install. `make firmware` shells out to
CMake and the Pico SDK, which is the only build path the SDK supports.

## Consequences

Easier: exact bus timing, expressed once in a PIO program that is short enough to read
in full; the same C++ source compiles for the host, which is what makes the pure logic
testable without hardware (ADR-0002).

Harder: PIO is an unusual assembly language with 32 instructions of program space per
state machine, and a bug in it is invisible without an instrument — which is why the
trace mode (R5) and the emulator (ADR-0004) are phases, not extras. Two build systems
coexist; the Makefile is the seam and CMake is never invoked directly.

Rejected:
- **MicroPython / CircuitPython** — no deterministic byte-gap timing; the `ACK` wait
  is precisely what would be lost, and that is the bug being fixed.
- **Hardware SPI peripheral** — cannot wait on `ACK` between bytes, and the PS2 bus's
  chip-select and inter-byte gap semantics do not match the block's framing.
- **Bit-banged GPIO from the CPU** — workable, but every clock edge becomes hostage to
  USB interrupt latency, which is the failure mode being designed against.
- **Plain C** — C++17 buys `constexpr` tables, `enum class`, and `std::array` bounds
  in the pure logic, at no runtime cost with exceptions and RTTI off.
