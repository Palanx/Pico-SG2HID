#pragma once

// The overlap the other two vectors leave uncovered: a frame that is BOTH cut short AND
// carries a ready byte that is not kReadyByte.
//
// This is what an absent controller looks like after it has already answered the address
// byte: `DATA` is open-drain with a pull-up, so once the controller stops driving it the
// master shifts in 0xFF — and if the master's `ACK` wait then times out, it stops shifting
// and the frame is short as well.
//
// The asserted outcome is AckTimeout, not NotReady, and the reason is R-PROTO-02's own
// wording: a frame the bus cut short "reports the abort". The 0xFF at the ready slot is the
// idle level of an undriven line, not the controller saying it is not ready — a controller
// that died mid-frame said nothing about readiness at all. NotReady is reserved for a frame
// that arrived COMPLETE and whose ready byte is wrong, which is `not_ready.h`.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kTruncatedNotReady[] = {
    0x41,  // header — digital; announces 2 payload bytes, so 4 bytes in a whole frame
    0xFF,  // ready slot — the idle level of an undriven DATA line, not kReadyByte
    0xEF,  // buttons 0 — arrived; buttons 1 never did, so the frame is one byte short
};

}  // namespace vectors
