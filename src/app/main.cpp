// Master firmware, build/pico/sg2hid.uf2 (phase 06-hil-digital): a link poller. Every
// kPollPeriodUs, start to start, it exchanges one digital poll, decodes the answer, advances the
// link and counts the poll. It never stops: a refused poll drops the link to Absent and the next
// one goes out on schedule anyway (docs/constraints.md §Error handling). Over USB serial it prints
// a `hil:` summary every kPollsPerSummary polls, and a `link:` line plus that poll's `T1` trace
// line (ADR-0015) whenever the link state changes; tools/hil_digital.py reads both. The loopback
// program this file used to be is src/app/loopback.cpp. Configures no pin itself: bus_init( ) in
// src/hal/ does, from src/core/pins.h (R-SAFETY-09, R-SAFETY-10).
#include "core/bus_trace.h"
#include "core/link.h"
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
// A five-byte `T1` line is about 100 characters; the rest is headroom.
constexpr std::size_t   kTraceLineSize   = 256;
// "XX XX" and its NUL.
constexpr std::size_t   kPayloadTextSize = 6;

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
    }
    return "?";
}

struct Poller {
    ps2::Link      link{};
    ps2::PollTally tally{};
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
    std::array<ps2::WireByte, ps2::kDigitalPollLen> wire{};
    for ( std::size_t i = 0; i < wire.size(); ++i ) {
        wire[ i ].out = ps2::kDigitalPoll[ i ];
    }
    const std::size_t completed = ps2::exchange_frame( wire );
    const auto        outcome   = ps2::decode_poll( wire, completed );
    ps2::count_poll( poller.tally, outcome );

    const ps2::LinkState was = poller.link.state;
    const ps2::LinkState now = ps2::step( poller.link, outcome, elapsed_us );
    if ( now == was ) {
        return;
    }
    const std::string_view from  = state_name( was );
    const std::string_view to    = state_name( now );
    const std::string_view fault = fault_name( poller.link.last_fault );
    std::printf( "link: %.*s -> %.*s fault=%.*s\n",
                 static_cast<int>( from.size() ),
                 from.data(),
                 static_cast<int>( to.size() ),
                 to.data(),
                 static_cast<int>( fault.size() ),
                 fault.data() );
    print_trace( wire, completed );
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
    const std::string_view state = state_name( poller.link.state );
    const std::string_view fault = fault_name( poller.link.last_fault );
    std::printf( "hil: polls=%lu refused=%lu changes=%lu us=%lu state=%.*s fault=%.*s att=%s "
                 "payload=%s\n",
                 static_cast<unsigned long>( tally.polls ),
                 static_cast<unsigned long>( tally.refused ),
                 static_cast<unsigned long>( tally.payload_changes ),
                 static_cast<unsigned long>( took ),
                 static_cast<int>( state.size() ),
                 state.data(),
                 static_cast<int>( fault.size() ),
                 fault.data(),
                 is_att_high ? "high" : "low",
                 payload.data() );
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
