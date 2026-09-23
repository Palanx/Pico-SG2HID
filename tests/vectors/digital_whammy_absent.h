#pragma once

// Digital-mode poll response whose payload is chosen so that a decoder reading a whammy
// byte out of it produces a NON-rest axis, whichever byte it reads (R-PROTO-04).
//
// That is the whole design of this vector. A digital frame carries 2 payload bytes; the
// whammy lives at an analog-only index, so `decode` leaves the rest of the fixed payload
// buffer zero. Neither of the two real bytes below, nor the zero filling after them, equals
// the rest value — so a mutation that reads the whammy unconditionally cannot accidentally
// land on "at rest" and pass.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kDigitalWhammyAbsent[] = {
    0x41,  // header — digital
    0x5A,  // ready byte
    0x7F,  // buttons 0 — bit 7 (LEFT) clear. Not the rest value, and not zero.
    0xBF,  // buttons 1 — bit 6 (CROSS) clear: green fret held. Also not the rest value.
};

}  // namespace vectors
