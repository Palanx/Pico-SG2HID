// Assertions over the frame loop in src/hal/bus_frame.cpp (R-SAFETY-07, R-PROTO-06).
//
// NOT named test_*.cpp on purpose: the Makefile builds and runs every tests/test_*.cpp, and this
// file needs src/hal/bus_frame.cpp linked in. tests/test_bus_frame.py is the single entry
// point — it compiles this file with the frame loop, runs it, forwards the lines below, and
// mutates the frame loop to prove each line bites.
//
// The fake port below stands in for src/hal/pio_port.cpp. It records every call in order and
// fails one scripted exchange, so every abort position of every frame length is exercised.

#include "hal/bus_frame.h"
#include "hal/bus_port.h"

#include <cstdio>
#include <vector>

namespace {

enum class CallKind : std::uint8_t { Assert, Release, Exchange };

struct Call {
    CallKind     kind;
    std::uint8_t out;
    bool         should_wait_ack;
};

// What the fake shifts in for a byte: distinct from the byte shifted out, so a frame that was
// never written back is visible.
constexpr std::uint8_t kResponseMask = 0xA5;

// Frame lengths under test: one byte (the last is the first), two, a digital poll, an analog
// poll.
constexpr std::size_t kFrameLengths[] = { 1, 2, 5, 9 };

constexpr std::size_t kMaxFrame = 9;

std::vector<Call> calls;
std::size_t       fail_at;  // the exchange that returns nullopt; the frame length for none
std::size_t       exchanges;

[[nodiscard]] std::uint8_t response_to( std::uint8_t out ) {
    return static_cast<std::uint8_t>( out ^ kResponseMask );
}

}  // namespace

namespace ps2 {

void att_assert() {
    calls.push_back( { .kind = CallKind::Assert, .out = 0, .should_wait_ack = false } );
}

void att_release() {
    calls.push_back( { .kind = CallKind::Release, .out = 0, .should_wait_ack = false } );
}

std::optional<std::uint8_t> exchange_byte( std::uint8_t out, bool should_wait_ack ) {
    calls.push_back(
        { .kind = CallKind::Exchange, .out = out, .should_wait_ack = should_wait_ack } );
    if ( exchanges++ == fail_at ) {
        return std::nullopt;
    }
    return response_to( out );
}

}  // namespace ps2

namespace {

struct Run {
    std::size_t  length;
    std::size_t  fail_at;
    std::size_t  count;
    std::uint8_t sent[ kMaxFrame ];
    std::uint8_t frame[ kMaxFrame ];
};

[[nodiscard]] Run run_frame( std::size_t length, std::size_t failure ) {
    Run run{ .length = length, .fail_at = failure, .count = 0, .sent = {}, .frame = {} };
    for ( std::size_t i = 0; i < length; ++i ) {
        run.sent[ i ]  = static_cast<std::uint8_t>( i + 1 );
        run.frame[ i ] = run.sent[ i ];
    }
    calls.clear();
    fail_at   = failure;
    exchanges = 0;
    run.count = ps2::exchange_frame( std::span<std::uint8_t>( run.frame, length ) );
    return run;
}

// R-SAFETY-07: the record opens with one assert and closes with one release, and nothing is
// exchanged after it.
[[nodiscard]] bool att_framed( const std::vector<Call>& record ) {
    std::size_t asserts  = 0;
    std::size_t releases = 0;
    for ( const Call& call : record ) {
        asserts += call.kind == CallKind::Assert ? 1 : 0;
        releases += call.kind == CallKind::Release ? 1 : 0;
    }
    return !record.empty() && record.front().kind == CallKind::Assert &&
           record.back().kind == CallKind::Release && asserts == 1 && releases == 1;
}

// R-PROTO-06: bytes 0..n-2 ask for ACK and byte n-1 does not; nothing is exchanged after a
// failed byte; the count is where the frame stopped; completed bytes hold what came back.
[[nodiscard]] bool ack_waits_right( const Run& run, const std::vector<Call>& record ) {
    const bool        fails    = run.fail_at < run.length;
    const std::size_t expected = fails ? run.fail_at : run.length;
    std::size_t       index    = 0;
    for ( const Call& call : record ) {
        if ( call.kind != CallKind::Exchange ) {
            continue;
        }
        if ( call.out != run.sent[ index ] || call.should_wait_ack != ( index + 1 < run.length ) ) {
            return false;
        }
        ++index;
    }
    if ( index != ( fails ? run.fail_at + 1 : run.length ) || run.count != expected ) {
        return false;
    }
    for ( std::size_t i = 0; i < expected; ++i ) {
        if ( run.frame[ i ] != response_to( run.sent[ i ] ) ) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool report( bool is_ok, const char* label ) {
    std::printf( "  %s %s\n", is_ok ? "ok:  " : "FAIL:", label );
    return is_ok;
}

}  // namespace

int main() {
    bool is_framed = true;
    bool is_acked  = true;
    for ( const std::size_t length : kFrameLengths ) {
        // failure == length is the run where nothing fails.
        for ( std::size_t failure = 0; failure <= length; ++failure ) {
            const Run run = run_frame( length, failure );
            if ( !att_framed( calls ) ) {
                std::printf(
                    "    frame of %zu failing at %zu: ATT not framed once\n", length, failure );
                is_framed = false;
            }
            if ( !ack_waits_right( run, calls ) ) {
                std::printf( "    frame of %zu failing at %zu: ACK waits or count wrong\n",
                             length,
                             failure );
                is_acked = false;
            }
        }
    }
    bool is_ok = report( is_framed,
                         "R-SAFETY-07 (ATT asserted once and released once, last, on every path)" );
    is_ok =
        report(
            is_acked,
            "R-PROTO-06 (ACK waited after every byte but the last; stop at the first failure)" ) &&
        is_ok;
    return is_ok ? 0 : 1;
}
