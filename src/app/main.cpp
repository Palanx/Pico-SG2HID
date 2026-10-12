// Master firmware, build/pico/sg2hid.uf2 (phases 06-hil-digital, 07-analog-mode, 08-usb-hid): an
// analog-mode poller and a USB HID gamepad. Every ps2::kPollPeriodUs, start to start, it exchanges
// one frame of the analog-mode sequence (src/core/negotiation.h, ADR-0017): enter config mode,
// select analog, leave config mode, then analog polls for as long as the controller answers them.
// It never streams digital, and it never stops: a frame that leaves the link Absent restarts the
// sequence on the next frame (docs/constraints.md §Error handling). After each frame it hands the
// HID interface that frame's report, or the neutral report while the link is not AnalogStreaming
// (src/usb/, ADR-0018). Over the USB serial port, the composite device's other interface, it
// prints a `hil:` summary every kPollsPerSummary frames, ending in the whammy of the last good
// analog frame and the last report, and a `link:` line plus that frame's `T1` trace line
// (ADR-0015) whenever the link state changes; tools/hil_digital.py reads both. Configures no pin
// itself: bus_init( ) in src/hal/ does, from src/core/pins.h (R-SAFETY-09, R-SAFETY-10).
#include "app/console.h"
#include "core/bus_trace.h"
#include "core/guitar_state.h"
#include "core/hid_report.h"
#include "core/link.h"
#include "core/negotiation.h"
#include "core/pins.h"
#include "core/poll.h"
#include "core/ps2_protocol.h"
#include "hal/bus_frame.h"
#include "hal/bus_port.h"
#include "pico/stdlib.h"
#include "usb/usb_device.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

namespace {

// One summary a second at ps2::kPollPeriodUs; tools/hil_digital.py's MAX_SUMMARY_US assumes it.
constexpr std::uint32_t kPollsPerSummary = 125;
// "XX XX" and its NUL.
constexpr std::size_t   kPayloadTextSize = 6;
// "XX" and its NUL.
constexpr std::size_t   kWhammyTextSize  = 3;
// "XX " per report byte; the last space's slot holds the NUL.
constexpr std::size_t   kReportTextSize  = 3 * ps2::kReportLen;
// The longest frame the master sends: an analog frame plus its address byte.
constexpr std::size_t   kWireLen         = ps2::frame_len( ps2::ControllerId::Analog ) + 1;

struct Poller {
    ps2::Master    master{};
    ps2::PollTally tally{};
    std::uint8_t   whammy      = 0;  // of the last good analog frame; meaningful once has_whammy
    bool           has_whammy  = false;
    ps2::HidReport report      = ps2::neutral_report();  // the last one handed to the HID interface
    std::uint32_t  poll_start  = 0;                      // time_us_32( ) at the latest poll's start
    std::uint32_t  batch_start = 0;  // time_us_32( ) when the current summary's polls began
};

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
    poller.report = ps2::report_for( now, outcome );
    usb::send_report( poller.report );
    if ( now == ps2::LinkState::AnalogStreaming && outcome.has_value() ) {
        poller.whammy     = ps2::map_frame( *outcome ).whammy;
        poller.has_whammy = true;
    }
    if ( now == was ) {
        return;
    }
    console::print_link( was, now, poller.master.link.last_fault );
    console::print_trace( frame, completed );
}

// The report as `kReportLen` uppercase hex bytes separated by single spaces.
[[nodiscard]] std::array<char, kReportTextSize> report_text( const ps2::HidReport& report ) {
    std::array<char, kReportTextSize> text{};
    std::size_t                       used = 0;
    for ( const std::uint8_t byte : report ) {
        used += static_cast<std::size_t>( std::snprintf(
            text.data() + used, text.size() - used, used == 0 ? "%02X" : " %02X", byte ) );
    }
    return text;
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
    const std::array<char, kReportTextSize> report = report_text( poller.report );
    const std::string_view                  state = console::state_name( poller.master.link.state );
    const std::string_view fault = console::fault_name( poller.master.link.last_fault );
    std::printf( "hil: polls=%lu refused=%lu changes=%lu us=%lu state=%.*s fault=%.*s att=%s "
                 "payload=%s whammy=%s report=%s\n",
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
                 whammy.data(),
                 report.data() );
}

}  // namespace

int main() {
    // TinyUSB first: this firmware owns it, and stdio_init_all( ) asserts it is running.
    usb::init();
    stdio_init_all();
    ps2::bus_init();

    Poller poller{};
    poller.poll_start  = time_us_32();
    poller.batch_start = poller.poll_start;
    for ( ;; ) {
        while ( time_us_32() - poller.poll_start < ps2::kPollPeriodUs ) {
            usb::task();
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
