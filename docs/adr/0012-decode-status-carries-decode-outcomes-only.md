# ADR-0012: `DecodeStatus` carries decode outcomes only

- Status: accepted
- Date: 2026-09-11
- Supersedes: **only** the `DecodeStatus` membership sketched in ADR-0007's
  `Ok | AckTimeout | UnknownId | NotAnalog | Absent`. Everything else ADR-0007 decided is
  unchanged and remains the decision of record: two mechanisms at two levels, and a missing
  `ACK` as a transition rather than a failed call. ADR-0011 superseded that ADR's `step`
  signature and said nothing about this list; this is the other half.

## Context

ADR-0007 listed five statuses before any `core` file existed and before ADR-0009 replaced its
result struct with `std::expected`. Writing the decoder in phase `01-ps2-codec` produced three
members that could not be written down, and the reason is different for each one — which is
what makes this a decision rather than a slip to be patched in a header comment.

`Ok` has no place to live. ADR-0009 made success `has_value()` on the `std::expected`, so an
"ok status" would be a value that can be stored, compared and passed around while asserting
nothing — exactly the shape ADR-0009 removed. Keeping it would reintroduce
`if ( result.status == Ok )` beside `if ( result )`, two spellings of one question.

`Absent` is not a property of any byte sequence. It is where the *link* goes, and it is
already a `LinkState`. A decoder that could return `Absent` would be answering a question
about the lifecycle from a function that sees one frame and remembers nothing — the exact
confusion ADR-0007 separated its two mechanisms to prevent, arriving back through the enum.

`NotAnalog` is decidable, but not here and not yet. "The controller declined analog mode" is a
judgement about a *negotiation reply* — which frame was expected at which point in the config
sequence — and phase `01-ps2-codec` §Out of scope gives running that sequence to
`07-analog-mode`. A `decode` over one frame's bytes has no way to know a reply is late or
wrong, because it does not know what was asked.

Left over is a fourth member ADR-0007 did not sketch: `NotReady`, for a declared id followed
by a byte that is not `kReadyByte`. It is decidable from the bytes of one frame, so by the
rule below it belongs.

## Decision

`DecodeStatus` is `AckTimeout | UnknownId | NotReady`, and the membership rule is the part
that constrains the future:

**A status belongs in `DecodeStatus` if and only if it is decidable from the bytes of a single
frame, with no history and no knowledge of what was asked.** Anything needing to remember a
previous frame is a `FaultCause` on the link. Anything describing what the controller *is* to
us is a `LinkState`.

`07-analog-mode` is the first phase this will bind: its "answered the config sequence, then
declined analog mode" is a `FaultCause`, not a `DecodeStatus`, because deciding it requires
knowing which frame was expected.

## Consequences

Easier: `decode` stays a pure function of one buffer, which is what lets every vector in
`tests/vectors/` be asserted without constructing any history. The rule gives the next phase a
test it can apply on its own instead of a precedent it has to infer from three absences.

Harder: a contributor who reads ADR-0007 alone will look for `Ok` and `Absent` and not find
them, which is what this record exists to answer. And the boundary has one genuinely arguable
case: `NotReady` could be read as "something else is talking on the bus", which sounds like a
statement about the link. It is not — the ready byte is at a fixed offset in the frame in
hand, so a single buffer decides it.

Rejected:
- **Keep ADR-0007's five members and leave the three unused.** An enum with members no code
  can produce is a list of things a reader will assume are reachable, and `-Wswitch` stops
  helping the moment a `switch` has to handle cases that cannot occur.
- **Fold `NotReady` into `UnknownId`.** Both are "the bytes are not a frame we know", and
  merging them costs the trace mode (R5) the distinction between *no controller we support*
  and *a controller we support that stopped answering* — which are different things to do next
  and different things for the operator to read off a trace.
- **Fold `NotReady` into `AckTimeout`.** Worse: it names a bus event that did not happen. The
  bytes arrived.
- **Record the three absences as a finding in §Observed conventions.** A finding is a verified
  fact about how the code or hardware *is*, and stops being true when the code changes. This
  is a rule about where a future status goes, which is the opposite: it is meant to bind
  `07-analog-mode` precisely when someone is about to change the code.
