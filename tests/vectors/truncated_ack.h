#pragma once

// A digital frame the bus cut short: the header announced 2 payload bytes and only 1
// arrived, because the controller's `ACK` never came and the master stopped shifting
// (R-PROTO-02).
//
// The one payload byte present is a legitimate value. That is the point: nothing about the
// bytes that DID arrive is wrong, so the only thing that can refuse this frame is the length
// the header announced — and the refusal must yield no frame at all, not a frame with one
// byte filled in.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kTruncatedAck[] = {
    0x41,  // header — digital; announces 2 payload bytes
    0x5A,  // ready byte
    0xEF,  // buttons 0 — arrived intact; buttons 1 never did
};

}  // namespace vectors
