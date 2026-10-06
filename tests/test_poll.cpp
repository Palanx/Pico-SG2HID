// Host test for src/core/poll.{h,cpp}: decode_poll( ) and count_poll( ).
//
// Named test_*.cpp on purpose: it is its own entry point, built and run by `make test`'s
// tests/test_*.cpp wildcard. Every wire byte is built from tests/vectors/ (R-PROTO-05); wire
// byte 0's answer is always kAddressReply. Prints one `FAIL: <case>` line per failed case and
// exits 0 only when every case passed.

#include "core/poll.h"

#include "vectors/digital_idle.h"
#include "vectors/digital_pressed.h"
#include "vectors/poll_exchange.h"
#include "vectors/unknown_id.h"

#include <array>
#include <cstdio>
#include <span>

namespace {

// Longer than any poll, so `completed` past the frame's end can be asked for.
constexpr std::size_t kWireLen = 10;

using Wire = std::array<ps2::WireByte, kWireLen>;

[[nodiscard]] bool report( bool is_ok, const char* label ) {
    if ( !is_ok ) {
        std::printf( "FAIL: %s\n", label );
    }
    return is_ok;
}

// kAddressReply at wire byte 0, then `answer`'s bytes; zeros past them.
[[nodiscard]] Wire wire_of( std::span<const std::uint8_t> answer ) {
    Wire wire{};
    wire[ 0 ].in = vectors::kAddressReply;
    for ( std::size_t i = 0; i < answer.size() && i + 1 < kWireLen; ++i ) {
        wire[ i + 1 ].in = answer[ i ];
    }
    return wire;
}

[[nodiscard]] bool is_idle_frame( const ps2::DecodeOutcome& outcome ) {
    return outcome.has_value() && outcome->id == ps2::ControllerId::Digital &&
           outcome->payload[ 0 ] == vectors::kDigitalIdle[ 2 ] &&
           outcome->payload[ 1 ] == vectors::kDigitalIdle[ 3 ];
}

[[nodiscard]] bool is_refused_as( const ps2::DecodeOutcome& outcome, ps2::DecodeStatus status ) {
    return !outcome.has_value() && outcome.error() == status;
}

[[nodiscard]] bool case_idle_decodes() {
    const Wire wire = wire_of( vectors::kDigitalIdle );

    const auto outcome = ps2::decode_poll( wire, ps2::kDigitalPollLen );

    return report( is_idle_frame( outcome ), "idle, completed 5: Digital, payload intact" );
}

[[nodiscard]] bool case_cut_is_ack_timeout() {
    const Wire                           wire  = wire_of( vectors::kDigitalIdle );
    constexpr std::array<std::size_t, 4> kCuts = { 0, 1, 3, 4 };

    bool is_ok = true;
    for ( const std::size_t completed : kCuts ) {
        const auto outcome = ps2::decode_poll( wire, completed );
        is_ok              = is_ok && is_refused_as( outcome, ps2::DecodeStatus::AckTimeout );
    }

    return report( is_ok, "idle, completed 0/1/3/4: AckTimeout" );
}

[[nodiscard]] bool case_overlong_count_is_clamped() {
    const Wire            wire  = wire_of( vectors::kDigitalIdle );
    constexpr std::size_t kOver = 9;

    const auto whole = ps2::decode_poll( wire, ps2::kDigitalPollLen );
    const auto over  = ps2::decode_poll( wire, kOver );

    const bool is_ok = over.has_value() && whole.has_value() && over->id == whole->id &&
                       over->payload == whole->payload;
    return report( is_ok, "idle, completed 9: equals completed 5" );
}

[[nodiscard]] bool case_unknown_id() {
    constexpr std::size_t kHead = 4;
    const Wire            wire  = wire_of( std::span( vectors::kUnknownId ).first( kHead ) );

    const auto outcome = ps2::decode_poll( wire, ps2::kDigitalPollLen );

    return report( is_refused_as( outcome, ps2::DecodeStatus::UnknownId ),
                   "unknown id, completed 5: UnknownId" );
}

[[nodiscard]] bool case_tally() {
    const Wire     idle_wire    = wire_of( vectors::kDigitalIdle );
    const Wire     pressed_wire = wire_of( vectors::kDigitalPressed );
    const auto     idle         = ps2::decode_poll( idle_wire, ps2::kDigitalPollLen );
    const auto     pressed      = ps2::decode_poll( pressed_wire, ps2::kDigitalPollLen );
    const auto     refused      = ps2::decode_poll( idle_wire, 0 );
    ps2::PollTally tally{};

    ps2::count_poll( tally, idle );
    ps2::count_poll( tally, idle );
    const bool is_same_kept = tally.payload_changes == 0;
    ps2::count_poll( tally, refused );
    const bool is_refusal_counted = tally.refused == 1;
    ps2::count_poll( tally, idle );
    const bool is_kept_across_refusal = tally.payload_changes == 0;
    ps2::count_poll( tally, pressed );

    bool is_ok = report( is_same_kept, "tally: idle, idle -> no change" );
    is_ok      = report( is_refusal_counted, "tally: a refusal -> refused 1" ) && is_ok;
    is_ok = report( is_kept_across_refusal, "tally: idle after a refusal -> no change" ) && is_ok;
    is_ok = report( tally.payload_changes == 1, "tally: pressed -> one change" ) && is_ok;
    constexpr std::uint32_t kPolls = 5;
    return report( tally.polls == kPolls, "tally: polls 5" ) && is_ok;
}

}  // namespace

int main() {
    bool is_ok = case_idle_decodes();
    is_ok      = case_cut_is_ack_timeout() && is_ok;
    is_ok      = case_overlong_count_is_clamped() && is_ok;
    is_ok      = case_unknown_id() && is_ok;
    is_ok      = case_tally() && is_ok;
    std::printf( "test_poll: %s\n", is_ok ? "ok" : "FAIL" );
    return is_ok ? 0 : 1;
}
