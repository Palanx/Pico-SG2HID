// Observe firmware, build/pico/sg2hid_observe.uf2 (09-guitar-observe): the instrument the real SG
// first meets the bus through. No HID interface; USB stdio is its only USB function. Every
// kPollPeriodUs, start to start, it exchanges one frame in one of two modes:
//
//   digital  at power-up. Sends only the read poll `01 42 00 00 00`, so the guitar stays in its
//            power-up mode: that is what "read-only" means here. The link is judged by step( ).
//   analog   after the line `analog` on the serial port, in either mode. Runs the analog-mode
//            sequence `sg2hid` runs (ADR-0017) from a fresh Master; the link is judged by
//            advance( ).
//
// It exchanges no frame until the host opens its serial port. It prints a `link:` line when the
// link state changes, and a frame's `T1` line (ADR-0015) when the trace filter
// (src/core/trace_filter.h) says the frame is due. It answers `analog` with `ok: analog` and every
// other non-empty line with `error: unknown command`. Configures no pin itself: bus_init( ) in
// src/hal/ does, from src/core/pins.h (R-SAFETY-09, R-SAFETY-10).
#include "app/console.h"
#include "core/bus_trace.h"
#include "core/link.h"
#include "core/negotiation.h"
#include "core/poll.h"
#include "core/ps2_protocol.h"
#include "core/trace_filter.h"
#include "hal/bus_frame.h"
#include "hal/bus_port.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

namespace {

constexpr std::uint32_t kPollPeriodUs = 1000;
constexpr std::uint32_t kPortWaitMs   = 10;

// The read poll: the only frame digital mode sends.
constexpr std::array<std::uint8_t, ps2::kDigitalPollLen> kReadPoll = {
    ps2::kFrameStart, ps2::kCmdPoll, ps2::kPadByte, ps2::kPadByte, ps2::kPadByte };

constexpr std::string_view kAnalogCommand  = "analog";
// The longest accepted line, its '\r', and one more, so a longer line is still refused whole.
constexpr std::size_t      kLineBufferSize = kAnalogCommand.size() + 2;

enum class Mode : std::uint8_t {
    Digital,
    Analog,
};

struct LineReader {
    std::array<char, kLineBufferSize> text{};
    std::size_t                       len         = 0;
    bool                              is_overlong = false;
};

struct Observer {
    Mode             mode = Mode::Digital;
    ps2::Link        link{};                          // digital mode's link
    ps2::Master      master{};                        // analog mode's sequence and link
    ps2::LinkState   shown = ps2::LinkState::Absent;  // the state the last `link:` line ended in
    ps2::TraceFilter filter{};
    LineReader       reader{};
};

// Reads at most one waiting character; on '\n', answers the line and, for `analog`, switches.
void poll_command( Observer& observer ) {
    const int c = getchar_timeout_us( 0 );
    if ( c == PICO_ERROR_TIMEOUT ) {
        return;
    }
    LineReader& reader = observer.reader;
    if ( c != '\n' ) {
        if ( reader.len < reader.text.size() ) {
            reader.text[ reader.len++ ] = static_cast<char>( c );
        } else {
            reader.is_overlong = true;
        }
        return;
    }
    std::string_view line( reader.text.data(), reader.len );
    const bool       is_overlong = reader.is_overlong;
    reader                       = LineReader{};
    if ( !line.empty() && line.back() == '\r' ) {
        line.remove_suffix( 1 );
    }
    if ( line.empty() && !is_overlong ) {
        return;
    }
    if ( line != kAnalogCommand || is_overlong ) {
        std::printf( "error: unknown command\n" );
        return;
    }
    observer.mode   = Mode::Analog;
    observer.master = ps2::Master{};
    std::printf( "ok: analog\n" );
}

// Exchanges one frame in the current mode and prints what is due.
void observe_once( Observer& observer, std::uint32_t start, std::uint32_t elapsed_us ) {
    const std::span<const std::uint8_t> command = observer.mode == Mode::Digital
                                                      ? std::span<const std::uint8_t>( kReadPoll )
                                                      : ps2::command_for( observer.master.stage );
    std::array<ps2::WireByte, ps2::kMaxTracedLen> wire{};
    for ( std::size_t i = 0; i < command.size(); ++i ) {
        wire[ i ].out = command[ i ];
    }
    const std::span<ps2::WireByte> frame     = std::span( wire ).first( command.size() );
    const std::size_t              completed = ps2::exchange_frame( frame );

    ps2::LinkState  now   = ps2::LinkState::Absent;
    ps2::FaultCause fault = ps2::FaultCause::None;
    if ( observer.mode == Mode::Digital ) {
        now   = ps2::step( observer.link, ps2::decode_poll( frame, completed ), elapsed_us );
        fault = observer.link.last_fault;
    } else {
        now = ps2::advance(
            observer.master, ps2::Exchanged{ .wire = frame, .completed = completed }, elapsed_us );
        fault = observer.master.link.last_fault;
    }
    if ( now != observer.shown ) {
        console::print_link( observer.shown, now, fault );
        observer.shown = now;
    }
    const ps2::TracedFrame traced{ .wire = frame, .completed = completed, .state = now };
    if ( ps2::is_trace_due( observer.filter, traced, start ) ) {
        console::print_trace( frame, completed );
        ps2::note_printed( observer.filter, traced, start );
    }
}

}  // namespace

int main() {
    stdio_init_all();
    ps2::bus_init();
    // No frame until the host opens the serial port: USB stdio drops output until then, and the
    // first frame's `link: absent -> digital` line is the capture's evidence that the guitar
    // answered (G1). The bus idles meanwhile, ATT high.
    while ( !stdio_usb_connected() ) {
        sleep_ms( kPortWaitMs );
    }

    Observer      observer{};
    std::uint32_t poll_start = time_us_32();
    for ( ;; ) {
        while ( time_us_32() - poll_start < kPollPeriodUs ) {
            poll_command( observer );
        }
        // Unsigned, so the counter's ~71.6-minute wraparound subtracts correctly (ADR-0011).
        const std::uint32_t start      = time_us_32();
        const std::uint32_t elapsed_us = start - poll_start;
        poll_start                     = start;
        observe_once( observer, start, elapsed_us );
    }
}
