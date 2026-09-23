# ADR-0007: Errors are result structs when decoding, and state transitions when linking

- Status: accepted — three parts are superseded, each by one later ADR and each only in
  part: the pure-decoding mechanism by ADR-0009 (`std::expected`), the `step` signature
  sketch by ADR-0011 (a pure transition over a decode outcome and elapsed microseconds),
  and the `DecodeStatus` membership sketched below by ADR-0012. The link-lifecycle state
  machine itself is unchanged and remains the decision of record — above all that a missing
  `ACK` is a transition to `Absent` and not a failed call.
- Date: 2026-08-31

## Context

`core` has to report that a bus read did not produce a usable frame: the `ACK` never
came, the header byte was not a controller id we know, the controller refused analog
mode, or there is nothing on the bus at all. The operator's engineering guidelines
require the error-handling approach to be chosen explicitly with alternatives on the
table, not adopted by default — and the choice constrains every signature in `core`, so
it is expensive to reverse once phases `01` and `03` are written against it.

Two properties narrow the field before any option is considered. `core` is
allocation-free, clock-free plain data (ADR-0002), so the mechanism cannot own a heap
buffer or a global. And the polling loop lives inside a 1 ms USB interval, so no error
path may take an unbounded amount of time.

C++ exceptions are therefore out. The Pico SDK ships them off; turning them on pulls in
the unwinder — flash cost, unmeasured here — and gives error propagation a duration
nobody can bound. Firmware that stops is a worse outcome than firmware that reports a
wrong byte, because the host is left holding a HID device that has silently gone quiet.
That inverts the "fail fast, fail loud" guideline on purpose, and the inversion is
recorded in `docs/constraints.md` §Engineering guidelines rather than left implicit.

## Decision

Two mechanisms, at two different levels, because the two questions are different ones.

**Pure decoding returns a result struct.** A function that takes bytes and produces a
frame reports both through one object, so the value cannot be reached without the status
that qualifies it:

```cpp
struct DecodeResult {
    DecodeStatus status;   // Ok | AckTimeout | UnknownId | NotAnalog | Absent
    Ps2Frame     frame;    // meaningful only when status == Ok
    [[nodiscard]] bool is_ok( ) const;
};
[[nodiscard]] DecodeResult decode( const uint8_t* bytes, size_t n );
```

**The link lifecycle is a state machine, and a fault is a transition, not an error.** A
missing `ACK` does not mean "this function failed"; it means the controller is now
absent. The reason for the transition is carried alongside, because the trace mode (R5)
is the instrument that has to show it:

```cpp
enum class LinkState { Absent, Negotiating, DigitalStreaming, AnalogStreaming };
struct Link {
    LinkState  state;
    FaultCause last_fault;
};
LinkState step( Link& link, BusIo& io, uint32_t now_us );
```

Every function that can fail is `[[nodiscard]]`. Status is never returned alongside a
separate out-parameter holding the value, because that shape leaves the value readable
when the status says it is meaningless.

## Consequences

Easier: both shapes are plain data, so a host test asserts against a whole literal
result rather than reconstructing one field at a time; the link becomes a transition
table, which is the cheapest possible thing to test exhaustively; and the reason for
every disconnection is already in hand when trace mode needs to print it.

Harder: two mechanisms is one more than one, and a contributor has to know which level
they are writing at. The line is the presence of a lifecycle — one input and one output
is a `DecodeResult`, anything that remembers what happened last time is `Link`. And
`DecodeResult::frame` is still physically readable when `status != Ok`; the type makes
that harder to do by accident, not impossible.

Rejected:
- **One result struct everywhere, including the link.** Uniform to read, and it forces
  the invention of `StepStatus` values like `Negotiating` that are not the outcome of
  decoding anything — a lifecycle wearing a result type's clothes.
- **Enum return plus an out-parameter (the Pico SDK's own shape).** Zero cost and zero
  code of ours, and it leaves the out-parameter readable regardless of the status, which
  is the failure this decision is buying protection from. The seam with `hal` where the
  SDK does use this shape is one layer wide and translated there.
- **A generic `Result<T, E>` template.** Three uses at most. A template written and
  tested by us to save repeating a two-field struct three times is code we own for
  nothing; `std::expected` in C++23 would be the reason to revisit.
- **Exceptions.** See Context.
