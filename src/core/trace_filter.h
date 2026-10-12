#pragma once

// Which frames the observe firmware prints (src/app/observe.cpp). It exchanges a frame every
// millisecond and a serial line of ~170 characters per frame would drown the port, so a frame is
// printed only when it says something the last printed one did not. A frame is due when:
//
//   1. no frame has been printed yet;
//   2. its `out` bytes or their count differ from the last printed frame's;
//   3. its link state differs from the last printed frame's;
//   4. its `in` bytes or its completed count differ from the last printed frame's, and at least
//      kTraceGapUs have passed since that print.
//
// Rule 4's gap holds back analog noise; a frame that persists is still printed once the gap has
// passed. Rules 2 and 3 print every negotiation frame.
//
// Pure: the caller supplies the clock as a free-running microsecond counter, and wraparound is
// unsigned subtraction (ADR-0011).

#include "core/bus_trace.h"
#include "core/link.h"
#include "core/ps2_protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ps2 {

constexpr std::uint32_t kTraceGapUs = 10000;

// The longest frame the master sends: an analog frame plus its address byte.
constexpr std::size_t kMaxTracedLen = frame_len( ControllerId::Analog ) + 1;

// One exchanged frame, as the filter judges it.
struct TracedFrame {
    std::span<const WireByte> wire;
    std::size_t               completed;
    LinkState                 state;
};

// The last printed frame. Bytes past `len` and `in` bytes past `completed` are zero.
struct TraceFilter {
    std::array<std::uint8_t, kMaxTracedLen> out           = {};
    std::array<std::uint8_t, kMaxTracedLen> in            = {};
    std::size_t                             len           = 0;
    std::size_t                             completed     = 0;
    LinkState                               state         = LinkState::Absent;
    std::uint32_t                           printed_at_us = 0;
    bool                                    has_printed   = false;
};

// Whether `frame`, exchanged at `now_us`, is due by the rules above. A frame longer than
// kMaxTracedLen is always due: the filter cannot hold it to compare.
[[nodiscard]] bool
is_trace_due( const TraceFilter& filter, const TracedFrame& frame, std::uint32_t now_us );

// Records `frame` as printed at `now_us`.
void note_printed( TraceFilter& filter, const TracedFrame& frame, std::uint32_t now_us );

}  // namespace ps2
