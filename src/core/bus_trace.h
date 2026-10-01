#pragma once

// The bus trace (ADR-0015): one frame as the frame loop recorded it, written as one `T1` text
// line for the operator's serial port.
//
//     T1 n=<n> k=<k> out=<b0>,…,<bn-1> in=<b0>,…,<bn-1> us=<u0>,…,<un-1>
//
// `n` is the frame length and `k` the bytes completed. `out` is every byte sent, two uppercase
// hex digits each. `in` is byte i received for i < k and `--` for the rest. `us` is the elapsed
// microseconds of every attempted byte (i < k, plus byte k when k < n) and `-` for a byte never
// sent. tools/trace_decode.py reads this format; R-PROTO-08 checks both ends against the same
// hand-written lines.
//
// Pure: no clock, no I/O, no allocation. The caller owns the buffer and prints it.

#include <cstddef>
#include <cstdint>
#include <span>

namespace ps2 {

// One byte on the wire, as src/hal/bus_frame.cpp records it. `in` and `elapsed_us` are
// written only for a byte the frame loop got to: `in` when the byte completed, `elapsed_us`
// whenever it was attempted.
struct WireByte {
    std::uint8_t  out;
    std::uint8_t  in;
    std::uint32_t elapsed_us;
};

// Writes the `T1` line for `frame`, of which the first `completed` bytes completed, into
// `line`: no newline, no terminating NUL. Returns its length. Returns 0, leaving the content
// of `line` unspecified, when `completed > frame.size()` or the line does not fit.
[[nodiscard]] std::size_t
format_trace_line( std::span<const WireByte> frame, std::size_t completed, std::span<char> line );

}  // namespace ps2
