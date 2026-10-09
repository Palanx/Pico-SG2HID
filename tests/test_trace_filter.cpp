// Host test for src/core/trace_filter.{h,cpp}: the four rules that decide which frames the
// observe firmware prints (09-guitar-observe).
//
// Named test_*.cpp on purpose: it is its own entry point, built and run by `make test`'s
// tests/test_*.cpp wildcard. The bytes are arbitrary: the filter compares them and never reads
// them as protocol, so no expected protocol byte is asserted here and none comes from
// tests/vectors/ (R-PROTO-05). Prints one `FAIL: <case>` line per failed case and exits 0 only
// when every case passed.

#include "core/trace_filter.h"

#include <array>
#include <cstdint>
#include <cstdio>

namespace {

constexpr std::size_t   kLen    = 5;
constexpr std::uint32_t kT0     = 1000;
// Just short of the gap, and exactly the gap.
constexpr std::uint32_t kBefore = kT0 + ps2::kTraceGapUs - 1;
constexpr std::uint32_t kAfter  = kT0 + ps2::kTraceGapUs;

// Arbitrary: distinct `out` and `in` bases, and a mask that changes every bit of a byte.
constexpr std::uint8_t kOutBase = 0x10;
constexpr std::uint8_t kInBase  = 0xA0;
constexpr std::uint8_t kFlip    = 0xFF;

using Wire = std::array<ps2::WireByte, kLen>;

[[nodiscard]] bool report( bool is_ok, const char* label ) {
    if ( !is_ok ) {
        std::printf( "FAIL: %s\n", label );
    }
    return is_ok;
}

[[nodiscard]] Wire base_wire() {
    Wire wire{};
    for ( std::size_t i = 0; i < kLen; ++i ) {
        wire[ i ].out = static_cast<std::uint8_t>( kOutBase + i );
        wire[ i ].in  = static_cast<std::uint8_t>( kInBase + i );
    }
    return wire;
}

[[nodiscard]] ps2::TracedFrame frame_of( const Wire& wire, std::size_t completed ) {
    return ps2::TracedFrame{
        .wire = wire, .completed = completed, .state = ps2::LinkState::DigitalStreaming };
}

// A filter that printed `base_wire( )`, fully completed, at kT0.
[[nodiscard]] ps2::TraceFilter printed_base( const Wire& wire ) {
    ps2::TraceFilter filter{};
    ps2::note_printed( filter, frame_of( wire, kLen ), kT0 );
    return filter;
}

[[nodiscard]] bool case_first_frame_is_due() {
    const Wire             wire   = base_wire();
    const ps2::TraceFilter filter = {};
    return report( ps2::is_trace_due( filter, frame_of( wire, kLen ), kT0 ),
                   "(a) the first frame is due" );
}

[[nodiscard]] bool case_identical_is_not_due() {
    const Wire             wire   = base_wire();
    const ps2::TraceFilter filter = printed_base( wire );
    const bool             is_ok  = !ps2::is_trace_due( filter, frame_of( wire, kLen ), kBefore ) &&
                                    !ps2::is_trace_due( filter, frame_of( wire, kLen ), kAfter );
    return report( is_ok, "(b) an identical frame is not due, even after the gap" );
}

[[nodiscard]] bool case_in_change_waits_for_gap() {
    const Wire             wire    = base_wire();
    const ps2::TraceFilter filter  = printed_base( wire );
    Wire                   changed = wire;
    changed[ 3 ].in ^= kFlip;
    const bool is_ok = !ps2::is_trace_due( filter, frame_of( changed, kLen ), kBefore ) &&
                       ps2::is_trace_due( filter, frame_of( changed, kLen ), kAfter );
    return report( is_ok, "(c) an in change is due only once the gap has passed" );
}

[[nodiscard]] bool case_out_change_is_due_at_once() {
    const Wire             wire    = base_wire();
    const ps2::TraceFilter filter  = printed_base( wire );
    Wire                   changed = wire;
    changed[ 1 ].out ^= kFlip;
    const std::span<const ps2::WireByte> shorter = std::span( wire ).first( kLen - 1 );
    const bool                           is_ok =
        ps2::is_trace_due( filter, frame_of( changed, kLen ), kT0 + 1 ) &&
        ps2::is_trace_due( filter,
                           ps2::TracedFrame{ .wire      = shorter,
                                             .completed = kLen - 1,
                                             .state     = ps2::LinkState::DigitalStreaming },
                           kT0 + 1 );
    return report( is_ok, "(d) an out change, or a different count, is due before the gap" );
}

[[nodiscard]] bool case_state_change_is_due_at_once() {
    const Wire             wire   = base_wire();
    const ps2::TraceFilter filter = printed_base( wire );
    ps2::TracedFrame       frame  = frame_of( wire, kLen );
    frame.state                   = ps2::LinkState::AnalogStreaming;
    return report( ps2::is_trace_due( filter, frame, kT0 + 1 ),
                   "(e) a link-state change is due before the gap" );
}

[[nodiscard]] bool case_completed_change_waits_for_gap() {
    const Wire             wire   = base_wire();
    const ps2::TraceFilter filter = printed_base( wire );
    const bool is_ok = !ps2::is_trace_due( filter, frame_of( wire, kLen - 2 ), kBefore ) &&
                       ps2::is_trace_due( filter, frame_of( wire, kLen - 2 ), kAfter );
    return report( is_ok, "(f) a completed-count change is due only once the gap has passed" );
}

}  // namespace

int main() {
    bool is_ok = case_first_frame_is_due();
    is_ok      = case_identical_is_not_due() && is_ok;
    is_ok      = case_in_change_waits_for_gap() && is_ok;
    is_ok      = case_out_change_is_due_at_once() && is_ok;
    is_ok      = case_state_change_is_due_at_once() && is_ok;
    is_ok      = case_completed_change_waits_for_gap() && is_ok;
    return is_ok ? 0 : 1;
}
