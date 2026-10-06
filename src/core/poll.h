#pragma once

// One digital poll as the master sends it, the decode of what came back, and the counters the
// 06-hil-digital harness reads. Pure: no SDK header, no clock, no allocation (ADR-0002). `app`
// exchanges the frame and reads the clock; this decides what the bytes mean.

#include "core/bus_trace.h"
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

// The master's side of a digital poll.
constexpr std::array<std::uint8_t, kDigitalPollLen> kDigitalPoll = {
    kFrameStart, kCmdPoll, kPadByte, kPadByte, kPadByte };

// Decodes the answer to a poll of which the first `completed` wire bytes completed. Wire byte 0's
// `in` is dropped, as `decode( )` expects; at most an analog frame's bytes are handed on. A frame
// cut short is refused by `decode( )` as AckTimeout, never read past `completed`.
[[nodiscard]] DecodeOutcome decode_poll( std::span<const WireByte> wire, std::size_t completed );

// Counters since boot. A desync is a refused poll or a payload change while the controller is
// answering normally; telling which applies is the harness's job, not this one's.
struct PollTally {
    std::uint32_t                            polls           = 0;
    std::uint32_t                            refused         = 0;
    std::uint32_t                            payload_changes = 0;
    std::array<std::uint8_t, kMaxPayloadLen> last_payload    = {};
    bool                                     has_payload     = false;
};

// Counts one poll. A refusal keeps the last good payload, so a change is always measured
// against the previous good frame.
void count_poll( PollTally& tally, const DecodeOutcome& outcome );

}  // namespace ps2
