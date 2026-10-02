// The trace formatter in src/core/bus_trace.cpp, run over hand-written frames (R-PROTO-08).
//
// NOT named test_*.cpp on purpose, for the reason tests/bus_frame_cases.cpp gives: it needs
// src/core/bus_trace.cpp linked in, and tests/test_bus_trace.py is its single entry point. This
// file prints one `T1` line per frame below and judges none of them: the driver compares them
// with the well-formed `T1` lines of tests/vectors/trace_session.txt, in order, so the expected
// text is a hand-written literal and not this file's opinion.
//
// Bytes the formatter must not read — `in` of a byte that did not complete, `elapsed_us` of a
// byte never sent — hold kJunkIn and kJunkUs, so a formatter that reads them prints them.

#include "core/bus_trace.h"

#include <array>
#include <cstdio>
#include <span>

namespace {

using ps2::WireByte;

constexpr std::uint8_t  kJunkIn = 0xEE;
constexpr std::uint32_t kJunkUs = 9999;

struct Frame {
    std::span<const WireByte> bytes;
    std::size_t               completed;
};

// One per well-formed `T1` line of tests/vectors/trace_session.txt, in the same order.
constexpr WireByte kPollComplete[] = {
    {.out = 0x01, .in = 0x01, .elapsed_us = 36},
    {.out = 0x42, .in = 0x42, .elapsed_us = 39},
    {.out = 0x00, .in = 0x00, .elapsed_us = 39},
    {.out = 0x00, .in = 0x00, .elapsed_us = 39},
    {.out = 0x00, .in = 0x00, .elapsed_us = 38},
};
constexpr WireByte kSingleComplete[] = {
    { .out = 0x80, .in = 0x80, .elapsed_us = 38 },
};
constexpr WireByte kPollAbortedAtFirst[] = {
    {.out = 0x01, .in = kJunkIn, .elapsed_us = 134    },
    {.out = 0x42, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0x00, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0x00, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0x00, .in = kJunkIn, .elapsed_us = kJunkUs},
};
constexpr WireByte kPatternAbortedMidFrame[] = {
    {.out = 0xFF, .in = 0xFF,    .elapsed_us = 41     },
    {.out = 0x00, .in = 0x00,    .elapsed_us = 52     },
    {.out = 0xA5, .in = 0xA5,    .elapsed_us = 39     },
    {.out = 0x5A, .in = 0x5A,    .elapsed_us = 40     },
    {.out = 0x80, .in = kJunkIn, .elapsed_us = 134    },
    {.out = 0x01, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0xFE, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0x7F, .in = kJunkIn, .elapsed_us = kJunkUs},
    {.out = 0x3C, .in = kJunkIn, .elapsed_us = kJunkUs},
};
constexpr WireByte kPairAbortedAtLast[] = {
    {.out = 0x01, .in = 0x01,    .elapsed_us = 39 },
    {.out = 0x42, .in = kJunkIn, .elapsed_us = 135},
};

constexpr std::array<Frame, 5> kFrames = {
    {
     { .bytes = kPollComplete, .completed = 5 },
     { .bytes = kSingleComplete, .completed = 1 },
     { .bytes = kPollAbortedAtFirst, .completed = 0 },
     { .bytes = kPatternAbortedMidFrame, .completed = 4 },
     { .bytes = kPairAbortedAtLast, .completed = 1 },
     }
};

constexpr std::size_t kLineSize  = 256;
constexpr std::size_t kShortSize = 4;  // shorter than "T1 n=" alone

}  // namespace

int main() {
    std::array<char, kLineSize> line{};
    for ( const Frame& frame : kFrames ) {
        const std::size_t length = ps2::format_trace_line( frame.bytes, frame.completed, line );
        std::printf( "%.*s\n", static_cast<int>( length ), line.data() );
    }

    std::array<char, kShortSize> short_line{};
    const bool                   is_refused =
        ps2::format_trace_line( std::span<const ps2::WireByte>{}, 0, line ) == 0 &&
        ps2::format_trace_line( kPollComplete, std::size( kPollComplete ) + 1, line ) == 0 &&
        ps2::format_trace_line( kPollComplete, std::size( kPollComplete ), short_line ) == 0;
    std::printf(
        "  %s format_trace_line returns 0 for an empty frame, completed > n and a short buffer\n",
        is_refused ? "ok:  " : "FAIL:" );
    return is_refused ? 0 : 1;
}
