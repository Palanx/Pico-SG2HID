#pragma once

// Digital-mode poll response, nothing pressed. R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kDigitalIdle[] = {
    0x41,  // header — digital controller; low nibble 1 announces 1*2 = 2 payload bytes
    0x5A,  // ready byte
    0xFF,  // buttons 0 — active low, so every bit set means every button released
    0xFF,  // buttons 1 — likewise
};

}  // namespace vectors
