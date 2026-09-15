#include "core/link.h"

#include <limits>

namespace ps2 {

namespace {

// Saturating, because the alternative is a wraparound that silently resets the negotiation
// timeout and turns a stuck controller into one that looks fine every 71.6 minutes.
[[nodiscard]] std::uint32_t add_saturating( std::uint32_t base, std::uint32_t addend ) {
    constexpr std::uint32_t kMax = std::numeric_limits<std::uint32_t>::max();
    if ( base > kMax - addend ) {
        return kMax;
    }
    return base + addend;
}

// The state one good frame implies, ignoring history. History is `step`'s business.
[[nodiscard]] LinkState state_for( ControllerId id ) {
    switch ( id ) {
    case ControllerId::Analog:
        return LinkState::AnalogStreaming;
    case ControllerId::Digital:
        return LinkState::DigitalStreaming;
    case ControllerId::Config:
        return LinkState::Negotiating;
    }
    // Unreachable for a Ps2Frame: `decode` refuses every header that is not one of the three
    // above, so a frame in hand cannot carry a fourth id. Returning Absent rather than
    // asserting keeps the "firmware never stops" rule true even if a later phase adds an id
    // here and forgets this switch — and -Werror's -Wswitch is what will catch that first.
    return LinkState::Absent;
}

}  // namespace

LinkState step( Link& link, const DecodeOutcome& outcome, std::uint32_t elapsed_us ) {
    const LinkState was   = link.state;
    LinkState       next  = LinkState::Absent;
    // Seeded with the existing cause, not with None: last_fault means "why the link last
    // dropped", so a good frame must not erase the reason the previous one failed.
    FaultCause      fault = link.last_fault;

    if ( !outcome.has_value() ) {
        // A missing ACK is a transition, not a failed call (ADR-0007). No frame reached us, so
        // no field of one is readable — which is the other half of R-PROTO-02.
        next = LinkState::Absent;
        switch ( outcome.error() ) {
        case DecodeStatus::AckTimeout:
            fault = FaultCause::AckTimeout;
            break;
        case DecodeStatus::UnknownId:
            fault = FaultCause::UnknownId;
            break;
        case DecodeStatus::NotReady:
            fault = FaultCause::NotReady;
            break;
        }
    } else {
        next = state_for( outcome->id );

        // Config mode is a step on the way to streaming, not a place to live. Checked against
        // the time already spent in Negotiating, so a controller that keeps answering in config
        // mode drops the link instead of holding it there forever.
        const bool is_stuck =
            was == LinkState::Negotiating && next == LinkState::Negotiating &&
            add_saturating( link.us_in_state, elapsed_us ) > kNegotiationTimeoutUs;
        if ( is_stuck ) {
            next  = LinkState::Absent;
            fault = FaultCause::Negotiating;
        }
    }

    link.state       = next;
    link.last_fault  = fault;
    // Time in state, so entering a state restarts its clock and staying in one accumulates.
    link.us_in_state = ( next == was ) ? add_saturating( link.us_in_state, elapsed_us ) : 0;

    return link.state;
}

}  // namespace ps2
