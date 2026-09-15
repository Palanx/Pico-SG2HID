#pragma once

// The link lifecycle: what the controller currently is to us, and why it last stopped being
// it.
//
// ADR-0007 decided this shape and one thing about it that is easy to undo by accident: a
// missing `ACK` is not a failed call, it is a transition to `Absent`. ADR-0011 superseded only
// the signature — `step` is a pure function of the link, the outcome of one decode, and how
// much time elapsed, because a `BusIo&` parameter would be either virtual dispatch in `core`
// (R-CLEAN-09) or I/O reached from `core` (ADR-0002, R-ARCH-01).

#include "core/ps2_frame.h"

#include <cstdint>

namespace ps2 {

enum class LinkState : std::uint8_t {
    // Nothing usable on the bus. Every error path arrives here, and the firmware retries
    // negotiation from here rather than stopping (docs/constraints.md §Error handling).
    Absent,

    // The controller answered, in config mode: the analog-mode sequence is in progress.
    Negotiating,

    // Streaming digital frames. Usable, but the whammy does not exist in this mode.
    DigitalStreaming,

    // Streaming analog frames. The whammy axis is real here and only here (R-PROTO-04).
    AnalogStreaming,
};

// Why the link last dropped to Absent. A superset of DecodeStatus on purpose: 07-analog-mode
// adds causes that are not decode failures, such as a controller that answered the config
// sequence and then declined analog mode.
//
// There is deliberately no function anywhere that returns a FaultCause. R-ERR-01 forbids it:
// a status enum lives in std::expected's error slot or as a member of Link, never as a return
// type of its own. The mapping from DecodeStatus happens inside `step`, whose return is a
// LinkState.
enum class FaultCause : std::uint8_t {
    None,         // no fault recorded yet — the value a freshly constructed Link carries
    AckTimeout,   // the frame was cut short (R-PROTO-02)
    UnknownId,    // the header byte was not a declared controller id (R-PROTO-03)
    NotReady,     // a known id, but the byte after it was not kReadyByte
    Negotiating,  // config mode outlasted kNegotiationTimeoutUs without producing a report
};

// How long the controller may stay in config mode before the link gives up and starts over.
// The sequence is a handful of frames at a ~1 ms poll interval, so this is generous by two
// orders of magnitude: it is a "something is wrong" bound, not a schedule.
//
// belay-debt: the value is a budget, not a measurement — nothing has yet timed a real
// negotiation. 07-analog-mode drives the sequence for the first time and owns tightening it.
constexpr std::uint32_t kNegotiationTimeoutUs = 100000;

// Plain data. `us_in_state` is what makes the timeout above decidable without `core` ever
// reading a clock: the caller reports how much time passed, this accumulates it, and it resets
// on every state change.
struct Link {
    LinkState     state       = LinkState::Absent;
    FaultCause    last_fault  = FaultCause::None;
    std::uint32_t us_in_state = 0;
};

// Advance the link by one poll and return the state it is now in.
//
// `outcome` is what `decode` said about the bytes that just arrived. `elapsed_us` is the time
// since the previous call — elapsed, never an absolute timestamp, so the result depends only on
// the arguments and the 32-bit counter's ~71.6-minute wraparound stays `hal`'s subtraction to
// get right (ADR-0011).
[[nodiscard]] LinkState step( Link& link, const DecodeOutcome& outcome, std::uint32_t elapsed_us );

}  // namespace ps2
