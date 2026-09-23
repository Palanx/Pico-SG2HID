// Assertions over the hand-written vectors in tests/vectors/.
//
// NOT named test_*.cpp on purpose: the Makefile globs tests/test_*.cpp, builds each one and
// runs it, so that name would give this file a second entry point with no driver around it.
// tests/test_ps2_codec.py is the single entry point — it compiles this file, runs it, and
// forwards the lines below.
//
// Arrange / act / assert, in that order, visually separated, one concept per case
// (docs/constraints.md §Testing).
//
// No expected byte is written here. Every one is read out of the vector that declares it, so
// this file cannot disagree with the literal it is checking (R-PROTO-05) — and an assertion
// like "payload[ i ] equals the vector's byte at kPayloadIndex + i" pins the offset, which an
// inline `0xEF` would not.

#include "core/guitar_state.h"
#include "core/hid_report.h"
#include "core/link.h"
#include "core/ps2_frame.h"

#include "vectors/analog_idle.h"
#include "vectors/analog_whammy_full.h"
#include "vectors/config_mode.h"
#include "vectors/digital_idle.h"
#include "vectors/digital_pressed.h"
#include "vectors/digital_whammy_absent.h"
#include "vectors/not_ready.h"
#include "vectors/truncated_ack.h"
#include "vectors/truncated_not_ready.h"
#include "vectors/unknown_id.h"

#include <cstdio>
#include <iterator>
#include <span>

namespace {

constexpr std::uint8_t kZeroFill = 0x00;

// One result line in the house convention, and the verdict as a return value so the caller
// aggregates it. A case that printed its own verdict without returning it would leave the
// exit code load-bearing for nothing — the defect 00-scaffold's round 9 named.
[[nodiscard]] bool report( bool is_ok, const char* label ) {
    std::printf( "  %s %s\n", is_ok ? "ok:  " : "FAIL:", label );
    return is_ok;
}

// Does the decoded payload hold exactly the vector's payload bytes, at the right offset?
// Takes the whole vector, so the offset it checks against is the protocol's and not a number
// repeated at every call site.
[[nodiscard]] bool payload_matches( const ps2::Ps2Frame&          frame,
                                    std::span<const std::uint8_t> sent ) {
    for ( std::size_t i = 0; i < ps2::payload_len( frame.id ); ++i ) {
        if ( frame.payload[ i ] != sent[ ps2::kPayloadIndex + i ] ) {
            return false;
        }
    }
    return true;
}

// --- decode: the frames that must be accepted ------------------------------------------

[[nodiscard]] bool case_digital_idle() {
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalIdle };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = frame.has_value() && frame->id == ps2::ControllerId::Digital &&
                       payload_matches( *frame, bytes );
    return report( is_ok, "digital_idle: decodes as ControllerId::Digital, payload intact" );
}

[[nodiscard]] bool case_digital_pressed() {
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalPressed };

    const auto frame = ps2::decode( bytes );

    // The vector's two payload bytes differ, so matching them in order also pins their order.
    const bool is_ok = frame.has_value() && payload_matches( *frame, bytes );
    return report( is_ok, "digital_pressed: both payload bytes land in the order sent" );
}

[[nodiscard]] bool case_analog_idle() {
    const std::span<const std::uint8_t> bytes{ vectors::kAnalogIdle };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = frame.has_value() && frame->id == ps2::ControllerId::Analog &&
                       payload_matches( *frame, bytes );
    return report( is_ok, "analog_idle: decodes as ControllerId::Analog, payload intact" );
}

[[nodiscard]] bool case_config_mode() {
    const std::span<const std::uint8_t> bytes{ vectors::kConfigMode };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = frame.has_value() && frame->id == ps2::ControllerId::Config;
    return report( is_ok, "config_mode: a recognised id that is not a button report" );
}

// The header's low nibble is what announces the payload length. Checking that computation
// against the hand-written length of each vector is what proves the nibble arithmetic, and it
// is the one assertion here that would survive `decode` being rewritten entirely.
[[nodiscard]] bool case_announced_lengths() {
    const bool is_ok =
        ps2::frame_len( ps2::ControllerId::Digital ) == std::size( vectors::kDigitalIdle ) &&
        ps2::frame_len( ps2::ControllerId::Analog ) == std::size( vectors::kAnalogIdle ) &&
        ps2::frame_len( ps2::ControllerId::Config ) == std::size( vectors::kConfigMode );
    return report( is_ok, "every id's announced length equals its vector's written length" );
}

// A digital frame announces 2 payload bytes and the buffer holds room for 6. The four it does
// not use must be zero rather than whatever was there — which is what lets
// digital_whammy_absent reason about every byte in the buffer and not just the first two.
[[nodiscard]] bool case_digital_payload_is_zero_filled() {
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalWhammyAbsent };

    const auto frame = ps2::decode( bytes );

    bool is_ok = frame.has_value();
    for ( std::size_t i = ps2::payload_len( ps2::ControllerId::Digital );
          is_ok && i < ps2::kMaxPayloadLen;
          ++i ) {
        is_ok = frame->payload[ i ] == kZeroFill;
    }
    return report( is_ok, "digital_whammy_absent: bytes the header never announced are zero" );
}

// --- decode: the frames that must be refused -------------------------------------------

[[nodiscard]] bool case_unknown_id() {
    const std::span<const std::uint8_t> bytes{ vectors::kUnknownId };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = !frame.has_value() && frame.error() == ps2::DecodeStatus::UnknownId;
    return report( is_ok, "unknown_id: refused as UnknownId, and no frame" );
}

// The header is a declared id and the length is exactly what it announces, so neither of the
// other two refusals can fire here. That is what makes this a test of the ready-byte check and
// not of something upstream of it.
[[nodiscard]] bool case_not_ready() {
    const std::span<const std::uint8_t> bytes{ vectors::kNotReady };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = !frame.has_value() && frame.error() == ps2::DecodeStatus::NotReady;
    return report( is_ok, "not_ready: a wrong ready byte is refused, and no frame" );
}

[[nodiscard]] bool case_truncated_ack() {
    const std::span<const std::uint8_t> bytes{ vectors::kTruncatedAck };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = !frame.has_value() && frame.error() == ps2::DecodeStatus::AckTimeout;
    return report( is_ok, "truncated_ack: refused as AckTimeout, and no frame" );
}

// --- map_frame: controls out of a frame -------------------------------------------------

// Active low is the trap this case exists for: a mapper using a bare `&` would report every
// released control as pressed, and idle is the frame where that is visible.
[[nodiscard]] bool case_digital_idle_maps_to_nothing_pressed() {
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalIdle };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    bool is_ok = frame.has_value() && !state.is_strum_up && !state.is_strum_down &&
                 !state.is_start && !state.is_select && !state.is_tilt;
    for ( std::size_t i = 0; i < ps2::kFretCount; ++i ) {
        is_ok = is_ok && !state.is_fret_pressed[ i ];
    }
    return report( is_ok, "digital_idle: maps to every control released" );
}

// The vector holds exactly one fret and one strum, in different payload bytes. Asserting the
// other controls are NOT set is what makes this a mapping test rather than an "is anything
// set" test — a mapper that pressed everything would pass the positive half alone.
[[nodiscard]] bool case_digital_pressed_maps_one_fret_and_one_strum() {
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalPressed };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    const bool is_ok =
        frame.has_value() &&
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Green ) ] &&
        state.is_strum_up && !state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Red ) ] &&
        !state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Yellow ) ] &&
        !state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Blue ) ] &&
        !state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Orange ) ] &&
        !state.is_strum_down && !state.is_start && !state.is_select && !state.is_tilt;
    return report( is_ok, "digital_pressed: maps the green fret and strum up, nothing else" );
}

[[nodiscard]] bool case_analog_whammy_full() {
    const std::span<const std::uint8_t> bytes{ vectors::kAnalogWhammyFull };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    // Against the vector's own byte, so the assertion cannot disagree with the literal. The
    // vector's other three axes are centred, so reading the wrong index yields kWhammyRest and
    // fails here — that is what pins the index rather than merely "a byte changed".
    const bool is_ok =
        frame.has_value() &&
        state.whammy == vectors::kAnalogWhammyFull[ ps2::kPayloadIndex + ps2::kWhammyIndex ] &&
        state.whammy != ps2::kWhammyRest;
    return report( is_ok, "analog_whammy_full: the whammy reads full deflection" );
}

[[nodiscard]] bool case_analog_idle_whammy_is_rest() {
    const std::span<const std::uint8_t> bytes{ vectors::kAnalogIdle };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    const bool is_ok = frame.has_value() && state.whammy == ps2::kWhammyRest;
    return report( is_ok, "analog_idle: a centred whammy axis reads as rest" );
}

// A recognised id that reports no controls. It must still yield rest, not a whammy read out of
// a config-mode reply's padding.
[[nodiscard]] bool case_config_mode_whammy_is_rest() {
    const std::span<const std::uint8_t> bytes{ vectors::kConfigMode };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    const bool is_ok = frame.has_value() && state.whammy == ps2::kWhammyRest;
    return report( is_ok, "config_mode: the whammy reads rest, not a byte of the padding" );
}

// The adversarial frame, and the only thing standing behind the button half of the id gate.
// config_mode's payload is six 0x00 bytes; buttons are active low, so an ungated mapper
// reports all ten controls PRESSED here — measured, not argued (notes.md, round 2). R-PROTO-04
// stays green through that regression because it is about the whammy, so this case is it.
[[nodiscard]] bool case_config_mode_maps_to_nothing_pressed() {
    const std::span<const std::uint8_t> bytes{ vectors::kConfigMode };
    const auto                          frame = ps2::decode( bytes );

    const ps2::GuitarState state = ps2::map_frame( *frame );

    bool is_ok = frame.has_value() && !state.is_strum_up && !state.is_strum_down &&
                 !state.is_start && !state.is_select && !state.is_tilt;
    for ( std::size_t i = 0; i < ps2::kFretCount; ++i ) {
        is_ok = is_ok && !state.is_fret_pressed[ i ];
    }
    return report( is_ok, "config_mode: maps to every one of the ten controls released" );
}

// The overlap: a frame that is both cut short and carrying a wrong ready byte. R-PROTO-02
// says a cut-short frame reports the abort, so the abort wins — NotReady is reserved for a
// frame that arrived complete. Without this case the two refusals are only ever exercised
// apart, and the order they are written in is asserted by nothing.
[[nodiscard]] bool case_truncated_and_not_ready_reports_the_abort() {
    const std::span<const std::uint8_t> bytes{ vectors::kTruncatedNotReady };

    const auto frame = ps2::decode( bytes );

    const bool is_ok = !frame.has_value() && frame.error() == ps2::DecodeStatus::AckTimeout;
    return report( is_ok,
                   "truncated_not_ready: a cut-short frame reports the abort, not NotReady" );
}

// The first refusal in §Goal's order: a frame cut short before the ready slot. One byte is
// already enough to decide the header, so an undeclared one is what makes this the case where
// the abort has to outrank UnknownId. The input is the first byte of unknown_id's literal —
// what the bus delivers when it stops after the header — and the expected value is a status,
// not a byte, so nothing here is generated (R-PROTO-05).
[[nodiscard]] bool case_cut_before_the_ready_slot_reports_the_abort() {
    const std::span<const std::uint8_t> header_only =
        std::span<const std::uint8_t>{ vectors::kUnknownId }.first( ps2::kReadyIndex );

    const auto frame = ps2::decode( header_only );

    const bool is_ok = !frame.has_value() && frame.error() == ps2::DecodeStatus::AckTimeout;
    return report( is_ok, "cut before the ready slot: reports the abort, even for an unknown id" );
}

// --- the link state machine -------------------------------------------------------------
// Pure transitions over a decode outcome and an elapsed duration (ADR-0011), so these need no
// clock and no bus — which is the whole reason the signature changed.

constexpr std::uint32_t kOnePollUs = 1000;

// A fresh Link starts Absent with no fault recorded. Asserted because every case below reads
// as a transition FROM somewhere, and this is the somewhere.
[[nodiscard]] bool case_link_starts_absent() {
    const ps2::Link link{};

    const bool is_ok = link.state == ps2::LinkState::Absent &&
                       link.last_fault == ps2::FaultCause::None && link.us_in_state == 0;

    return report( is_ok, "link: a fresh Link is Absent with no fault" );
}

[[nodiscard]] bool case_link_digital_frame_streams() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> bytes{ vectors::kDigitalIdle };

    const ps2::LinkState now = ps2::step( link, ps2::decode( bytes ), kOnePollUs );

    const bool is_ok = now == ps2::LinkState::DigitalStreaming && link.state == now;
    return report( is_ok, "link: a digital frame moves the link to DigitalStreaming" );
}

[[nodiscard]] bool case_link_analog_frame_streams() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> bytes{ vectors::kAnalogIdle };

    const ps2::LinkState now = ps2::step( link, ps2::decode( bytes ), kOnePollUs );

    const bool is_ok = now == ps2::LinkState::AnalogStreaming;
    return report( is_ok, "link: an analog frame moves the link to AnalogStreaming" );
}

[[nodiscard]] bool case_link_config_frame_negotiates() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> bytes{ vectors::kConfigMode };

    const ps2::LinkState now = ps2::step( link, ps2::decode( bytes ), kOnePollUs );

    const bool is_ok = now == ps2::LinkState::Negotiating;
    return report( is_ok, "link: a config-mode frame moves the link to Negotiating" );
}

[[nodiscard]] bool case_link_unknown_id_drops() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> bytes{ vectors::kUnknownId };

    const ps2::LinkState now = ps2::step( link, ps2::decode( bytes ), kOnePollUs );

    const bool is_ok =
        now == ps2::LinkState::Absent && link.last_fault == ps2::FaultCause::UnknownId;
    return report( is_ok, "link: an unknown id drops the link and records why" );
}

// A link that was streaming and then loses the controller must go Absent, not stay where it
// was. Starting from DigitalStreaming rather than from a fresh Link is the point: a `step`
// that only ever moved forward would pass the fresh-Link case.
[[nodiscard]] bool case_link_drops_from_streaming() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> good{ vectors::kDigitalIdle };
    const std::span<const std::uint8_t> cut{ vectors::kTruncatedAck };
    const ps2::LinkState streaming = ps2::step( link, ps2::decode( good ), kOnePollUs );

    const ps2::LinkState now = ps2::step( link, ps2::decode( cut ), kOnePollUs );

    const bool is_ok = streaming == ps2::LinkState::DigitalStreaming &&
                       now == ps2::LinkState::Absent &&
                       link.last_fault == ps2::FaultCause::AckTimeout;
    return report( is_ok, "link: a cut-short frame drops a streaming link to Absent" );
}

// The timeout is the one thing elapsed_us is for, so it gets a case on both sides of the bound.
// Fed as two steps because the bound is on time spent IN Negotiating, which a single step from
// Absent cannot have accumulated.
[[nodiscard]] bool case_link_negotiation_times_out() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> config{ vectors::kConfigMode };
    const ps2::LinkState entered = ps2::step( link, ps2::decode( config ), kOnePollUs );

    const ps2::LinkState patient = ps2::step( link, ps2::decode( config ), kOnePollUs );
    const ps2::LinkState expired =
        ps2::step( link, ps2::decode( config ), ps2::kNegotiationTimeoutUs );

    const bool is_ok =
        entered == ps2::LinkState::Negotiating && patient == ps2::LinkState::Negotiating &&
        expired == ps2::LinkState::Absent && link.last_fault == ps2::FaultCause::Negotiating;
    return report( is_ok, "link: config mode past kNegotiationTimeoutUs drops the link" );
}

// The transition table is six rows and not twenty-four because the target depends on the
// outcome alone: every other link case drives one outcome from one source state, so together
// they sample the table without ever asserting the uniformity that makes it a table. This is
// the case that asserts it. Cut it, and a `step` that special-cased a single source state
// would still pass every case above.
[[nodiscard]] bool case_link_target_depends_on_the_outcome_alone() {
    const std::span<const std::uint8_t> digital{ vectors::kDigitalIdle };
    const std::span<const std::uint8_t> analog{ vectors::kAnalogIdle };
    const std::span<const std::uint8_t> config{ vectors::kConfigMode };
    const std::span<const std::uint8_t> outcomes[] = {
        digital,
        analog,
        config,
        std::span<const std::uint8_t>{ vectors::kUnknownId },
        std::span<const std::uint8_t>{ vectors::kNotReady },
        std::span<const std::uint8_t>{ vectors::kTruncatedAck },
    };
    ps2::Link  absent{};
    ps2::Link  digital_streaming{};
    ps2::Link  analog_streaming{};
    ps2::Link  negotiating{};
    // Each source state is reached by driving the link there, never by writing the member:
    // a state this machine cannot arrive at is not one it has to be uniform from.
    const bool is_set_up =
        absent.state == ps2::LinkState::Absent &&
        ps2::step( digital_streaming, ps2::decode( digital ), kOnePollUs ) ==
            ps2::LinkState::DigitalStreaming &&
        ps2::step( analog_streaming, ps2::decode( analog ), kOnePollUs ) ==
            ps2::LinkState::AnalogStreaming &&
        ps2::step( negotiating, ps2::decode( config ), kOnePollUs ) == ps2::LinkState::Negotiating;

    bool is_uniform = is_set_up;
    for ( const std::span<const std::uint8_t>& bytes : outcomes ) {
        // Copies, so each outcome is applied to the same four source states.
        ps2::Link from_absent      = absent;
        ps2::Link from_digital     = digital_streaming;
        ps2::Link from_analog      = analog_streaming;
        ps2::Link from_negotiating = negotiating;

        const ps2::LinkState target = ps2::step( from_absent, ps2::decode( bytes ), kOnePollUs );

        is_uniform = is_uniform &&
                     ps2::step( from_digital, ps2::decode( bytes ), kOnePollUs ) == target &&
                     ps2::step( from_analog, ps2::decode( bytes ), kOnePollUs ) == target &&
                     ps2::step( from_negotiating, ps2::decode( bytes ), kOnePollUs ) == target;
    }

    return report( is_uniform, "link: the target of a transition depends on the outcome alone" );
}

// The uniformity case asserts the four targets are equal and the cases above each assert one
// row from one source, which leaves the NotReady row asserted by nothing: a `step` sending
// NotReady to the same wrong state from every source passes both. Started from a streaming
// link so the drop is a real transition, and the cause is checked as well as the target.
[[nodiscard]] bool case_link_not_ready_drops_and_records_why() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> good{ vectors::kDigitalIdle };
    const std::span<const std::uint8_t> not_ready{ vectors::kNotReady };
    const ps2::LinkState streaming = ps2::step( link, ps2::decode( good ), kOnePollUs );

    const ps2::LinkState now = ps2::step( link, ps2::decode( not_ready ), kOnePollUs );

    const bool is_ok = streaming == ps2::LinkState::DigitalStreaming &&
                       now == ps2::LinkState::Absent &&
                       link.last_fault == ps2::FaultCause::NotReady;
    return report( is_ok, "link: a wrong ready byte drops the link and records NotReady" );
}

// last_fault is history: the cause of the most recent drop, kept after the link recovers,
// because that is exactly when trace mode prints it.
[[nodiscard]] bool case_link_good_frame_keeps_the_last_fault() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> cut{ vectors::kTruncatedAck };
    const std::span<const std::uint8_t> good{ vectors::kDigitalIdle };
    const ps2::LinkState                dropped = ps2::step( link, ps2::decode( cut ), kOnePollUs );

    const ps2::LinkState recovered = ps2::step( link, ps2::decode( good ), kOnePollUs );

    const bool is_ok = dropped == ps2::LinkState::Absent &&
                       recovered == ps2::LinkState::DigitalStreaming &&
                       link.last_fault == ps2::FaultCause::AckTimeout;
    return report( is_ok, "link: a good frame after a drop keeps the fault that caused it" );
}

// The bound is exclusive: exactly kNegotiationTimeoutUs in Negotiating is still negotiating,
// and one microsecond more is not. The timeout case above crosses the bound by a whole budget,
// which a `>=` passes just as well.
[[nodiscard]] bool case_link_negotiation_bound_is_exclusive() {
    constexpr std::uint32_t kOneMicrosecond = 1;

    ps2::Link                           link{};
    const std::span<const std::uint8_t> config{ vectors::kConfigMode };
    const ps2::LinkState entered = ps2::step( link, ps2::decode( config ), kOnePollUs );

    const ps2::LinkState at_bound =
        ps2::step( link, ps2::decode( config ), ps2::kNegotiationTimeoutUs );
    const ps2::LinkState past_bound = ps2::step( link, ps2::decode( config ), kOneMicrosecond );

    const bool is_ok = entered == ps2::LinkState::Negotiating &&
                       at_bound == ps2::LinkState::Negotiating &&
                       past_bound == ps2::LinkState::Absent;
    return report( is_ok, "link: exactly kNegotiationTimeoutUs in config mode is not yet past it" );
}

// The elapsed time handed to the step that ENTERS a state passed before the transition, so it
// belongs to the state being left. A long silence before the first config-mode frame must not
// count against the negotiation it precedes.
[[nodiscard]] bool case_link_time_before_entering_is_not_counted() {
    ps2::Link                           link{};
    const std::span<const std::uint8_t> config{ vectors::kConfigMode };
    const ps2::LinkState                entered =
        ps2::step( link, ps2::decode( config ), ps2::kNegotiationTimeoutUs );

    const ps2::LinkState next = ps2::step( link, ps2::decode( config ), kOnePollUs );

    const bool is_ok =
        entered == ps2::LinkState::Negotiating && next == ps2::LinkState::Negotiating;
    return report( is_ok,
                   "link: time before entering Negotiating does not count toward its timeout" );
}

// --- the HID report ---------------------------------------------------------------------

// A GuitarState with exactly one control pressed and nothing else, so a report built from it
// must have exactly one bit set. Written as a switch rather than an array of pointers so that
// adding a Button without handling it here is a -Werror -Wswitch failure, not a silent gap in
// the loop below.
[[nodiscard]] ps2::GuitarState state_with( ps2::Button button ) {
    ps2::GuitarState state{};
    state.whammy = ps2::kWhammyRest;

    switch ( button ) {
    case ps2::Button::FretGreen:
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Green ) ] = true;
        break;
    case ps2::Button::FretRed:
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Red ) ] = true;
        break;
    case ps2::Button::FretYellow:
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Yellow ) ] = true;
        break;
    case ps2::Button::FretBlue:
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Blue ) ] = true;
        break;
    case ps2::Button::FretOrange:
        state.is_fret_pressed[ static_cast<std::size_t>( ps2::Fret::Orange ) ] = true;
        break;
    case ps2::Button::StrumUp:
        state.is_strum_up = true;
        break;
    case ps2::Button::StrumDown:
        state.is_strum_down = true;
        break;
    case ps2::Button::Start:
        state.is_start = true;
        break;
    case ps2::Button::Select:
        state.is_select = true;
        break;
    case ps2::Button::Tilt:
        state.is_tilt = true;
        break;
    }
    return state;
}

// Count the set bits across the whole report's button bytes.
[[nodiscard]] std::size_t buttons_set( const ps2::HidReport& report ) {
    std::size_t count = 0;
    for ( std::size_t byte = 0; byte < ps2::kButtonBytes; ++byte ) {
        for ( std::size_t bit = 0; bit < ps2::kBitsPerByte; ++bit ) {
            if ( ( report[ byte ] & ( 1U << bit ) ) != 0 ) {
                ++count;
            }
        }
    }
    return count;
}

// The strong form, and the reason it is a loop: every button gets its own bit, in its own
// position, and no two share one. Ten hand-written cases could each pass while two buttons
// collided on the same bit; this cannot.
[[nodiscard]] bool case_report_gives_every_button_its_own_bit() {
    bool is_ok = true;

    for ( std::size_t i = 0; is_ok && i < ps2::kButtonCount; ++i ) {
        const auto button = static_cast<ps2::Button>( i );

        const ps2::HidReport report = ps2::build_report( state_with( button ) );

        const std::size_t byte = i / ps2::kBitsPerByte;
        const std::size_t bit  = i % ps2::kBitsPerByte;
        is_ok = buttons_set( report ) == 1 && ( report[ byte ] & ( 1U << bit ) ) != 0;
    }
    return report( is_ok, "hid: every button sets exactly its own bit, and no two collide" );
}

[[nodiscard]] bool case_report_idle_is_all_zero_buttons() {
    ps2::GuitarState state{};
    state.whammy = ps2::kWhammyRest;

    const ps2::HidReport built = ps2::build_report( state );

    const bool is_ok = buttons_set( built ) == 0 &&
                       built[ ps2::kWhammyOffset ] == ps2::kWhammyRest &&
                       built.size() == ps2::kReportLen;
    return report( is_ok, "hid: nothing pressed is a report with no button bits set" );
}

// End to end, bytes on the bus to bytes on the wire to the host, over one vector. The whammy
// is compared against the vector's own literal so the chain cannot disagree with it.
[[nodiscard]] bool case_report_carries_the_whammy_end_to_end() {
    const std::span<const std::uint8_t> bytes{ vectors::kAnalogWhammyFull };
    const auto                          frame = ps2::decode( bytes );

    const ps2::HidReport built = ps2::build_report( ps2::map_frame( *frame ) );

    const bool is_ok = frame.has_value() &&
                       built[ ps2::kWhammyOffset ] ==
                           vectors::kAnalogWhammyFull[ ps2::kPayloadIndex + ps2::kWhammyIndex ];
    return report( is_ok, "hid: the whammy byte reaches the report unchanged" );
}

// The report is wide enough for the buttons it declares. Derived on both sides, so this fails
// if kButtonCount grows past what kButtonBytes covers.
[[nodiscard]] bool case_report_is_wide_enough() {
    const bool is_ok = ps2::kButtonBytes * ps2::kBitsPerByte >= ps2::kButtonCount &&
                       ps2::kReportLen == ps2::kButtonBytes + 1 &&
                       ps2::kWhammyOffset < ps2::kReportLen;
    return report( is_ok, "hid: the report is wide enough for every button it declares" );
}

// --- the rule lines ---------------------------------------------------------------------
// These are what tests/test_ps2_codec.py declares in its header and what its rejection cases
// flip to FAIL. Each restates its rule over the vectors, so the line means the rule and not
// "some case above happened to pass".

[[nodiscard]] bool rule_proto03() {
    const std::span<const std::uint8_t> unknown{ vectors::kUnknownId };
    const std::span<const std::uint8_t> known{ vectors::kDigitalIdle };

    const auto refused  = ps2::decode( unknown );
    const auto accepted = ps2::decode( known );

    // Both halves: an undeclared id is refused with the right reason, AND a declared one is
    // still accepted. A decoder that refused everything would satisfy the first alone.
    const bool is_ok = !refused.has_value() && refused.error() == ps2::DecodeStatus::UnknownId &&
                       accepted.has_value();
    return report( is_ok, "R-PROTO-03 (an unknown controller id is refused, a known one is not)" );
}

[[nodiscard]] bool rule_proto04() {
    const std::span<const std::uint8_t> digital{ vectors::kDigitalWhammyAbsent };
    const std::span<const std::uint8_t> analog{ vectors::kAnalogWhammyFull };

    const auto digital_frame = ps2::decode( digital );
    const auto analog_frame  = ps2::decode( analog );

    const ps2::GuitarState from_digital = ps2::map_frame( *digital_frame );
    const ps2::GuitarState from_analog  = ps2::map_frame( *analog_frame );

    // Both halves, and the digital vector is built so that NO byte in its payload buffer
    // equals kWhammyRest: a mapper that read the whammy unconditionally cannot land on the
    // rest value by luck and pass. The analog half is what stops "always report rest" from
    // satisfying the rule.
    //
    // Config is deliberately NOT asserted here, and that was measured rather than assumed:
    // `map_frame` returns early for an id that reports no controls, so only Digital and Analog
    // ever reach the gate below and `id != Digital` is an EQUIVALENT mutant, not a violation.
    // A clause for Config could not be flipped by any single mutation, which would make it
    // green paint. `case_config_mode_whammy_is_rest` covers the composite behaviour as a case.
    const bool is_ok = digital_frame.has_value() && analog_frame.has_value() &&
                       from_digital.whammy == ps2::kWhammyRest &&
                       from_analog.whammy != ps2::kWhammyRest;
    return report( is_ok,
                   "R-PROTO-04 (the whammy is read only from an analog frame; otherwise rest)" );
}

// Sixth clause of rule_proto02, a function of its own only to keep that one under the size
// limit (R-CLEAN-02). The rule says the link transitions to Absent and does not qualify the
// source state, but every other step in the rule line starts from a fresh (Absent) link. A
// `step` that sent a cut-short frame somewhere else from ONE source state would break the rule
// and leave the line green — measured, not supposed. This function covers the three NON-Absent
// members of LinkState; the fourth, Absent, is asserted in rule_proto02 by a freshly constructed
// Link. Corrected 2026-09-22: this comment used to claim all four, and that false sentence was
// copied verbatim into R-PROTO-02's scope clause. This is not the uniformity case restated: that
// one asserts the four targets are EQUAL, this one asserts what they equal.
[[nodiscard]] bool cut_drops_the_link_from_every_source( std::span<const std::uint8_t> cut ) {
    ps2::Link                           from_digital{};
    ps2::Link                           from_analog{};
    ps2::Link                           from_negotiating{};
    const std::span<const std::uint8_t> digital_idle{ vectors::kDigitalIdle };
    const std::span<const std::uint8_t> analog_idle{ vectors::kAnalogIdle };
    const std::span<const std::uint8_t> config_frame{ vectors::kConfigMode };
    const bool                          has_reached_every_source =
        ps2::step( from_digital, ps2::decode( digital_idle ), kOnePollUs ) ==
            ps2::LinkState::DigitalStreaming &&
        ps2::step( from_analog, ps2::decode( analog_idle ), kOnePollUs ) ==
            ps2::LinkState::AnalogStreaming &&
        ps2::step( from_negotiating, ps2::decode( config_frame ), kOnePollUs ) ==
            ps2::LinkState::Negotiating;

    return has_reached_every_source &&
           ps2::step( from_digital, ps2::decode( cut ), kOnePollUs ) == ps2::LinkState::Absent &&
           ps2::step( from_analog, ps2::decode( cut ), kOnePollUs ) == ps2::LinkState::Absent &&
           ps2::step( from_negotiating, ps2::decode( cut ), kOnePollUs ) == ps2::LinkState::Absent;
}

[[nodiscard]] bool rule_proto02() {
    ps2::Link                           link{};
    ps2::Link                           overlap_link{};
    const std::span<const std::uint8_t> cut{ vectors::kTruncatedAck };
    const std::span<const std::uint8_t> cut_and_idle{ vectors::kTruncatedNotReady };
    const std::span<const std::uint8_t> whole{ vectors::kDigitalIdle };

    const auto           refused     = ps2::decode( cut );
    const ps2::LinkState now         = ps2::step( link, refused, kOnePollUs );
    const auto           overlap     = ps2::decode( cut_and_idle );
    const ps2::LinkState now_too     = ps2::step( overlap_link, overlap, kOnePollUs );
    const auto           accepted    = ps2::decode( whole );
    const auto           header_only = ps2::decode(
        std::span<const std::uint8_t>{ vectors::kUnknownId }.first( ps2::kReadyIndex ) );

    // Three claims, and the rule is all three: no frame comes out of a cut-short read, the
    // reason names the bus event, and the link transitions to Absent rather than reporting a
    // failed call. The fourth clause keeps a decoder that refused everything from passing.
    //
    // The fifth is the one this rule line was missing until 2026-09-15, and it is not an extra
    // case bolted on: R-PROTO-02 says a cut-short frame reports the abort, with no exception
    // for frames that are ALSO wrong some other way. Exercising truncation only where no other
    // refusal competes leaves the rule line green while the rule is broken by the order of two
    // `if`s — measured, and the reason this clause exists. `truncated_not_ready` is cut short
    // and carries 0xFF at the ready slot, so it is the configuration where the abort has to
    // outrank something.
    //
    // The sixth, added 2026-09-15, is cut_drops_the_link_from_every_source above.
    //
    // The seventh, added 2026-09-16: a frame cut short before the ready slot. Without it the
    // first refusal in §Goal's order was asserted by nothing — deleting that check made an
    // undeclared header that arrived alone report UnknownId instead of the abort, and every
    // line stayed green. Measured by mutating each refusal and each transition in turn, not
    // only the one already known to be covered.
    const bool is_ok = !refused.has_value() && refused.error() == ps2::DecodeStatus::AckTimeout &&
                       now == ps2::LinkState::Absent && !overlap.has_value() &&
                       overlap.error() == ps2::DecodeStatus::AckTimeout &&
                       now_too == ps2::LinkState::Absent &&
                       cut_drops_the_link_from_every_source( cut ) && !header_only.has_value() &&
                       header_only.error() == ps2::DecodeStatus::AckTimeout && accepted.has_value();
    return report( is_ok,
                   "R-PROTO-02 (a cut-short frame yields no frame and the link goes Absent)" );
}

}  // namespace

int main() {
    bool is_ok = true;

    is_ok = case_digital_idle() && is_ok;
    is_ok = case_digital_pressed() && is_ok;
    is_ok = case_analog_idle() && is_ok;
    is_ok = case_config_mode() && is_ok;
    is_ok = case_announced_lengths() && is_ok;
    is_ok = case_digital_payload_is_zero_filled() && is_ok;
    is_ok = case_unknown_id() && is_ok;
    is_ok = case_truncated_ack() && is_ok;
    is_ok = case_not_ready() && is_ok;
    is_ok = case_truncated_and_not_ready_reports_the_abort() && is_ok;
    is_ok = case_cut_before_the_ready_slot_reports_the_abort() && is_ok;

    is_ok = case_digital_idle_maps_to_nothing_pressed() && is_ok;
    is_ok = case_digital_pressed_maps_one_fret_and_one_strum() && is_ok;
    is_ok = case_analog_whammy_full() && is_ok;
    is_ok = case_analog_idle_whammy_is_rest() && is_ok;
    is_ok = case_config_mode_whammy_is_rest() && is_ok;
    is_ok = case_config_mode_maps_to_nothing_pressed() && is_ok;

    is_ok = case_link_starts_absent() && is_ok;
    is_ok = case_link_digital_frame_streams() && is_ok;
    is_ok = case_link_analog_frame_streams() && is_ok;
    is_ok = case_link_config_frame_negotiates() && is_ok;
    is_ok = case_link_unknown_id_drops() && is_ok;
    is_ok = case_link_drops_from_streaming() && is_ok;
    is_ok = case_link_negotiation_times_out() && is_ok;
    is_ok = case_link_target_depends_on_the_outcome_alone() && is_ok;
    is_ok = case_link_not_ready_drops_and_records_why() && is_ok;
    is_ok = case_link_good_frame_keeps_the_last_fault() && is_ok;
    is_ok = case_link_negotiation_bound_is_exclusive() && is_ok;
    is_ok = case_link_time_before_entering_is_not_counted() && is_ok;

    is_ok = case_report_gives_every_button_its_own_bit() && is_ok;
    is_ok = case_report_idle_is_all_zero_buttons() && is_ok;
    is_ok = case_report_carries_the_whammy_end_to_end() && is_ok;
    is_ok = case_report_is_wide_enough() && is_ok;

    is_ok = rule_proto02() && is_ok;
    is_ok = rule_proto03() && is_ok;
    is_ok = rule_proto04() && is_ok;

    return is_ok ? 0 : 1;
}
