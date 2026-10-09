#pragma once

// The decode of one exchanged frame, and the counters the HIL harness reads (06-hil-digital,
// 07-analog-mode). The frames the master sends are src/core/negotiation.h's. Pure: no SDK header,
// no clock, no allocation (ADR-0002). `app` exchanges the frame and reads the clock; this decides
// what the bytes mean.

#include "core/bus_trace.h"
#include "core/link.h"
#include "core/ps2_frame.h"
#include "core/ps2_protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ps2 {

// Wire bytes in a digital poll: the address byte, whose answer carries nothing, plus a digital
// frame (see ps2_protocol.h).
constexpr std::size_t kDigitalPollLen = frame_len( ControllerId::Digital ) + 1;

// Decodes the answer to a poll of which the first `completed` wire bytes completed. Wire byte 0's
// `in` is dropped, as `decode( )` expects; at most an analog frame's bytes are handed on. A frame
// cut short is refused by `decode( )` as AckTimeout, never read past `completed`.
[[nodiscard]] DecodeOutcome decode_poll( std::span<const WireByte> wire, std::size_t completed );

// Counters since boot. `refused` counts frames after which the link is Absent; `payload_changes`
// counts AnalogStreaming frames whose payload differs from the previous good analog frame's.
// Telling a desync from an expected change is the harness's job, not this one's.
struct PollTally {
    std::uint32_t                            polls           = 0;
    std::uint32_t                            refused         = 0;
    std::uint32_t                            payload_changes = 0;
    std::array<std::uint8_t, kMaxPayloadLen> last_payload    = {};
    bool                                     has_payload     = false;
};

// Counts one frame, given the link state it left (`now`) and its decode. A frame that is not a
// good analog one keeps the last good payload, so a change is always measured against the
// previous good analog frame.
void count_poll( PollTally& tally, LinkState now, const DecodeOutcome& outcome );

}  // namespace ps2
