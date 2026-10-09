#include "app/console.h"

#include <array>
#include <cstdio>

namespace console {

namespace {

// A nine-byte `T1` line is about 170 characters; the rest is headroom.
constexpr std::size_t kTraceLineSize = 256;

}  // namespace

std::string_view state_name( ps2::LinkState state ) {
    switch ( state ) {
    case ps2::LinkState::Absent:
        return "absent";
    case ps2::LinkState::Negotiating:
        return "negotiating";
    case ps2::LinkState::DigitalStreaming:
        return "digital";
    case ps2::LinkState::AnalogStreaming:
        return "analog";
    }
    return "?";
}

std::string_view fault_name( ps2::FaultCause fault ) {
    switch ( fault ) {
    case ps2::FaultCause::None:
        return "none";
    case ps2::FaultCause::AckTimeout:
        return "ack-timeout";
    case ps2::FaultCause::UnknownId:
        return "unknown-id";
    case ps2::FaultCause::NotReady:
        return "not-ready";
    case ps2::FaultCause::Negotiating:
        return "negotiating";
    case ps2::FaultCause::Declined:
        return "declined";
    }
    return "?";
}

void print_trace( std::span<const ps2::WireByte> frame, std::size_t completed ) {
    std::array<char, kTraceLineSize> line{};
    const std::size_t                length = ps2::format_trace_line( frame, completed, line );
    if ( length == 0 ) {
        std::printf( "trace: line too long\n" );
        return;
    }
    std::printf( "%.*s\n", static_cast<int>( length ), line.data() );
}

void print_link( ps2::LinkState from, ps2::LinkState to, ps2::FaultCause fault ) {
    const std::string_view from_name  = state_name( from );
    const std::string_view to_name    = state_name( to );
    const std::string_view fault_text = fault_name( fault );
    std::printf( "link: %.*s -> %.*s fault=%.*s\n",
                 static_cast<int>( from_name.size() ),
                 from_name.data(),
                 static_cast<int>( to_name.size() ),
                 to_name.data(),
                 static_cast<int>( fault_text.size() ),
                 fault_text.data() );
}

}  // namespace console
