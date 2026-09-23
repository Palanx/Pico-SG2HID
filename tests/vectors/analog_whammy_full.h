#pragma once

// Analog-mode poll response with the whammy bar at full deflection and every other axis
// centred (R-PROTO-04).
//
// The other three axes stay centred on purpose: a decoder reading the whammy from the wrong
// axis index gets the rest value and fails this vector, so the vector pins the index and not
// merely "some byte changed".
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kAnalogWhammyFull[] = {
    0x73,  // header — analog controller; low nibble 3 announces 3*2 = 6 payload bytes
    0x5A,  // ready byte
    0xFF,  // buttons 0 — every button released
    0xFF,  // buttons 1 — likewise
    0x80,  // axis 0 — centred
    0x80,  // axis 1 — centred
    0x80,  // axis 2 — centred
    0xFF,  // axis 3 — whammy pushed all the way
};

}  // namespace vectors
