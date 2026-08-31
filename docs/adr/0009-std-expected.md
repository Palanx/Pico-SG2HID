# ADR-0009: Decoding returns `std::expected`, not a hand-rolled result struct

- Status: accepted
- Date: 2026-08-31
- Supersedes: the pure-decoding mechanism of ADR-0007. The other half of ADR-0007 — the
  link lifecycle as a state machine, where a missing `ACK` is a transition to `Absent` and
  not a failed call — is unchanged and remains the decision of record.

## Context

ADR-0007 chose a hand-rolled `DecodeResult { status; frame; }` and explicitly rejected a
generic `Result<T, E>`, on the grounds that a template written and tested by us to save
repeating a two-field struct three times is code we own for nothing. That reasoning was
sound *given C++17*, which has no such type in the standard library.

ADR-0008 moved the project to C++23 on measured evidence. `std::expected` is now
available, and the rejection argument inverts: the standard library provides the type, so
writing our own is the thing that costs something. The two decisions were taken
independently and turned out to be coupled — the error model was chosen before the
language standard it depends on.

The open question was never availability. It was whether `std::expected` survives the
constraints this firmware actually runs under: `-fno-exceptions -fno-rtti` (R-ERR-03), no
heap (R-ARCH-03), and a code-size budget on an M0+.

## Decision

Pure decoding returns `std::expected<Ps2Frame, DecodeStatus>`. The hand-rolled
`DecodeResult` is not written.

Measured on `arm-none-eabi-g++ 15.3.1`, cortex-m0plus, `-Os -ffunction-sections
-fno-exceptions -fno-rtti`, equivalent `decode` + caller in both versions:

```
C++17  hand-rolled DecodeResult    .text = 88 bytes
C++23  std::expected               .text = 86 bytes
```

It compiles under `-fno-exceptions -fno-rtti`, pulls in no unwinder, and costs nothing.
The size difference is one function's worth of measurement, not a law — the point is the
absence of a penalty, not the two bytes.

**`.value()` is forbidden, and this is the whole hazard.** Under `-fno-exceptions`,
`std::expected::value()` on an error does not throw; disassembly shows it calls `abort`.
An `abort()` is precisely the outcome `docs/constraints.md` §Error handling rules out —
firmware that stops leaves the host holding a HID device that has silently gone quiet, and
that is a worse failure than a wrong report. Access goes through `has_value()` and
`operator*`, never `.value()`.

That hazard is greppable, so it becomes a rule with a test rather than a warning in a
comment: **R-ERR-04**.

## Consequences

Easier: one less type to write, test and document; a vocabulary type a future reader
already knows; `constexpr`-usable in C++23, so host tests compare whole expected values
against literals.

Harder: `std::expected` carries `.value()`, an API that is correct everywhere else in C++
and fatal here. The type invites a call that R-ERR-04 forbids — the hand-rolled struct
had no such member and therefore no such trap. The rule and its test are the price of
using the standard type, and they are cheaper than owning the type.

Rejected:
- **Keeping the hand-rolled struct anyway, to avoid the `.value()` trap.** Trades a
  greppable rule for a type we write, test, and maintain forever. The rule is smaller.
- **Wrapping `std::expected` in our own type that hides `.value()`.** Reintroduces the
  owned code that ADR-0007 rejected, plus a layer, to solve what one grep solves.
- **Using `std::optional`.** Loses the reason for the failure, which trace mode (R5) needs
  in order to be the instrument the operator judges the firmware with.
