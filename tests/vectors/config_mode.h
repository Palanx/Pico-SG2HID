#pragma once

// The controller's response while it is in config mode: a recognised id that is NOT a
// button report. Decoding must succeed and report ControllerId::Config; reading buttons or
// a whammy out of it is what must not happen.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kConfigMode[] = {
    0xF3,  // header — config mode; low nibble 3 announces 3*2 = 6 payload bytes
    0x5A,  // ready byte
    0x00,  // payload 0 — config-mode replies pad with zero; no button meaning
    0x00,  // payload 1
    0x00,  // payload 2
    0x00,  // payload 3
    0x00,  // payload 4
    0x00,  // payload 5
};

}  // namespace vectors
