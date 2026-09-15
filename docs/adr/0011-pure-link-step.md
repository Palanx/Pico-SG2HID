# ADR-0011: `step` is a pure transition over a decode outcome, not a call that performs I/O

- Status: accepted
- Date: 2026-09-11
- Supersedes: **only** the `step( Link&, BusIo&, uint32_t now_us )` signature sketch in
  ADR-0007. Everything else ADR-0007 decided about the link is unchanged and remains the
  decision of record: the four states, and above all that a missing `ACK` is a transition to
  `Absent` rather than a failed call.

## Context

ADR-0007 sketched the link lifecycle as:

```cpp
LinkState step( Link& link, BusIo& io, uint32_t now_us );
```

That signature cannot be written in `src/core/`, and the reason is two rules pulling in
opposite directions rather than a matter of taste.

An injected `BusIo` is one of two things. If it is an interface with a virtual `poll`, it is
virtual dispatch inside `core`, which **R-CLEAN-09** forbids outright — `core` is plain data
plus free functions, and that is not a style preference but what keeps it testable without a
harness. If it is a concrete type, then `core` reaches the bus, which **ADR-0002** and
**R-ARCH-01** forbid: `core` performs no I/O, includes no SDK header, and compiles unmodified
against the host compiler, which is the entire mechanism by which `make test` runs it on a
laptop with no hardware attached.

The `uint32_t now_us` parameter has the same problem one step further in. A function taking
"what time is it now" is a function that has to be told the time by something holding a
clock, and `core` reads no clock.

This was found while implementing phase `01-ps2-codec`, whose Plan step 6 is the first code
to need the signature. It is not a defect in ADR-0007: that ADR was written before ADR-0008
moved the project to C++23 and before any `core` file existed, and its subject was the error
*model*, which survives intact.

## Decision

`step` is a pure function of three things the caller already holds — the current link, the
outcome of decoding the frame that just arrived, and how many microseconds have elapsed since
the previous step:

```cpp
[[nodiscard]] LinkState step( Link& link, const DecodeOutcome& outcome, std::uint32_t elapsed_us );
```

No `BusIo`. No clock. The caller — `app`, which may depend on `hal` — does the poll, hands
the bytes to `decode`, and hands the result here. `core` decides *what the link now is*; it
never finds out for itself.

**Elapsed, not `now_us`.** This is the load-bearing half of the change and the reason the
phase spec calls it "given elapsed microseconds, decide", which is what `CLAUDE.md`
§Architecture already says timing-dependent behaviour looks like in this repo. An absolute
timestamp makes the function's behaviour depend on when it is called; a duration makes it
depend only on its arguments. The practical consequence is that the 32-bit microsecond
counter's wraparound — every ~71.6 minutes — is `hal`'s problem to subtract correctly, once,
rather than a latent bug reachable from every state in `core`.

`Link::last_fault` carries why the link last dropped, as ADR-0007 decided, because trace mode
(requirement R5) has to print it.

## Consequences

Easier: the state machine is a transition table over values, so the host tests enumerate it
exhaustively with no fake bus, no mock and no clock to stub — which is the cheapest possible
thing to test and was ADR-0007's own stated reason for wanting a state machine. `core` keeps
its one property that everything else rests on.

Harder: the caller now carries the sequence — poll, decode, step — instead of calling one
function that does all three. That is three lines in `app` rather than one, and it is where
those three lines belong: `app` is the only layer allowed to know about both the bus and the
codec. A caller that forgets to call `step` on a failed poll would leave the link stale;
`[[nodiscard]]` on the return makes ignoring the result a compile error under `-Werror`, which
is the closest a signature can come to preventing it.

Also harder, and worth naming: nothing in `core` can now enforce that `elapsed_us` is
truthful. A caller passing a constant would freeze every timeout. That is the price of the
clock living outside, and the check for it is `03-pio-bus`'s loopback test, where a real clock
exists to be wrong.

Rejected:
- **Keep ADR-0007's signature and template `step` on the bus type.** Removes the virtual
  dispatch but not the dependency: `core` would still name a bus concept and the host test
  would still need something to stand in for one. It trades a forbidden edge for a forbidden
  edge plus a template.
- **Pass a function pointer instead of a `BusIo&`.** Same objection with less type safety, and
  it makes `core` the thing that decides *when* a poll happens, which is scheduling — `app`'s
  job.
- **Take `now_us` and store the previous timestamp in `Link`.** Works, and moves the
  wraparound bug inside `core` where no test with a real clock can reach it. The subtraction
  has to happen somewhere; it belongs where the counter is.
- **Fold `step` into `decode`.** Collapses the two mechanisms ADR-0007 separated on purpose:
  one input and one output is a result value, anything that remembers what happened last time
  is the state machine. A `decode` that remembered would be both.
