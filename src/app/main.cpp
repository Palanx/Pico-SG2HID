// Master firmware, build/pico/sg2hid.uf2 (phases 06-hil-digital, 07-analog-mode): an analog-mode
// poller. Every kPollPeriodUs, start to start, it exchanges one frame of the analog-mode sequence
// (src/core/negotiation.h, ADR-0017): enter config mode, select analog, leave config mode, then
// analog polls for as long as the controller answers them. It never streams digital, and it
// never stops: a frame that leaves the link Absent restarts the sequence on the next frame
// (docs/constraints.md §Error handling). Over USB serial it prints a `hil:` summary every
// kPollsPerSummary frames, ending in the whammy of the last good analog frame, and a `link:` line
// plus that frame's `T1` trace line (ADR-0015) whenever the link state changes;
// tools/hil_digital.py reads both. Configures no pin itself: bus_init( ) in src/hal/ does, from
// src/core/pins.h (R-SAFETY-09, R-SAFETY-10).
#include "core/bus_trace.h"
#include "core/guitar_state.h"
#include "core/link.h"
#include "core/negotiation.h"
#include "core/pins.h"
#include "core/poll.h"
#include "hal/bus_frame.h"
#include "hal/bus_port.h"
#include "pico/stdlib.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

namespace {

constexpr std::uint32_t kPollPeriodUs    = 1000;
constexpr std::uint32_t kPollsPerSummary = 1000;
// A nine-byte `T1` line is about 170 characters; the rest is headroom.
constexpr std::size_t   kTraceLineSize   = 256;
// "XX XX" and its NUL.
constexpr std::size_t   kPayloadTextSize = 6;
// "XX" and its NUL.
constexpr std::size_t   kWhammyTextSize  = 3;
// The longest frame the master sends: an analog frame plus its address byte.
constexpr std::size_t   kWireLen         = ps2::frame_len( ps2::ControllerId::Analog ) + 1;

[[nodiscard]] std::string_view state_name( ps2::LinkState state ) {
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

[[nodiscard]] std::string_view fault_name( ps2::FaultCause fault ) {
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

struct Poller {
    ps2::Master    master{};
    ps2::PollTally tally{};
    std::uint8_t   whammy      = 0;  // of the last good analog frame; meaningful once has_whammy
    bool           has_whammy  = false;
    std::uint32_t  poll_start  = 0;  // time_us_32( ) at the latest poll's start
    std::uint32_t  batch_start = 0;  // time_us_32( ) when the current summary's polls began
};

void print_trace( std::span<const ps2::WireByte> wire, std::size_t completed ) {
    std::array<char, kTraceLineSize> line{};
    const std::size_t                length = ps2::format_trace_line( wire, completed, line );
    if ( length == 0 ) {
        std::printf( "trace: line too long\n" );
        return;
    }
    std::printf( "%.*s\n", static_cast<int>( length ), line.data() );
}

void poll_once( Poller& poller, std::uint32_t elapsed_us ) {
    const std::span<const std::uint8_t> command = ps2::command_for( poller.master.stage );
    std::array<ps2::WireByte, kWireLen> wire{};
    for ( std::size_t i = 0; i < command.size(); ++i ) {
        wire[ i ].out = command[ i ];
    }
    const std::span<ps2::WireByte> frame     = std::span( wire ).first( command.size() );
    const std::size_t              completed = ps2::exchange_frame( frame );
    const auto                     outcome   = ps2::decode_poll( frame, completed );

    const ps2::LinkState was = poller.master.link.state;
    const ps2::LinkState now = ps2::advance(
        poller.master, ps2::Exchanged{ .wire = frame, .completed = completed }, elapsed_us );
    ps2::count_poll( poller.tally, now, outcome );
    if ( now == ps2::LinkState::AnalogStreaming && outcome.has_value() ) {
        poller.whammy     = ps2::map_frame( *outcome ).whammy;
        poller.has_whammy = true;
    }
    if ( now == was ) {
        return;
    }
    const std::string_view from  = state_name( was );
    const std::string_view to    = state_name( now );
    const std::string_view fault = fault_name( poller.master.link.last_fault );
    std::printf( "link: %.*s -> %.*s fault=%.*s\n",
                 static_cast<int>( from.size() ),
                 from.data(),
                 static_cast<int>( to.size() ),
                 to.data(),
                 static_cast<int>( fault.size() ),
                 fault.data() );
    print_trace( frame, completed );
}

void print_summary( Poller& poller ) {
    const bool          is_att_high = gpio_get( ps2::gpio_of( ps2::Signal::Att ) );
    const std::uint32_t now         = time_us_32();
    const std::uint32_t took        = now - poller.batch_start;
    poller.batch_start              = now;

    const ps2::PollTally&              tally = poller.tally;
    std::array<char, kPayloadTextSize> payload{ '-', '-' };
    if ( tally.has_payload ) {
        std::snprintf( payload.data(),
                       payload.size(),
                       "%02X %02X",
                       tally.last_payload[ 0 ],
                       tally.last_payload[ 1 ] );
    }
    std::array<char, kWhammyTextSize> whammy{ '-', '-' };
    if ( poller.has_whammy ) {
        std::snprintf( whammy.data(), whammy.size(), "%02X", poller.whammy );
    }
    const std::string_view state = state_name( poller.master.link.state );
    const std::string_view fault = fault_name( poller.master.link.last_fault );
    std::printf( "hil: polls=%lu refused=%lu changes=%lu us=%lu state=%.*s fault=%.*s att=%s "
                 "payload=%s whammy=%s\n",
                 static_cast<unsigned long>( tally.polls ),
                 static_cast<unsigned long>( tally.refused ),
                 static_cast<unsigned long>( tally.payload_changes ),
                 static_cast<unsigned long>( took ),
                 static_cast<int>( state.size() ),
                 state.data(),
                 static_cast<int>( fault.size() ),
                 fault.data(),
                 is_att_high ? "high" : "low",
                 payload.data(),
                 whammy.data() );
}

}  // namespace

int main() {
    stdio_init_all();
    ps2::bus_init();

    Poller poller{};
    poller.poll_start  = time_us_32();
    poller.batch_start = poller.poll_start;
    for ( ;; ) {
        while ( time_us_32() - poller.poll_start < kPollPeriodUs ) {
            tight_loop_contents();
        }
        // Unsigned, so the counter's ~71.6-minute wraparound subtracts correctly (ADR-0011).
        const std::uint32_t start      = time_us_32();
        const std::uint32_t elapsed_us = start - poller.poll_start;
        poller.poll_start              = start;
        poll_once( poller, elapsed_us );
        if ( poller.tally.polls % kPollsPerSummary == 0 ) {
            print_summary( poller );
        }
    }
}
