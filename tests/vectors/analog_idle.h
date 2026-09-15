#pragma once

// Analog-mode poll response, nothing pressed and every stick centred. R-PROTO-05.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kAnalogIdle[] = {
    0x73,  // header — analog controller; low nibble 3 announces 3*2 = 6 payload bytes
    0x5A,  // ready byte
    0xFF,  // buttons 0 — every button released (active low)
    0xFF,  // buttons 1 — likewise
    0x80,  // axis 0 — centred
    0x80,  // axis 1 — centred
    0x80,  // axis 2 — centred
    0x80,  // axis 3 — centred, and this is the one the whammy is read from
};

}  // namespace vectors
