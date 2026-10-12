#pragma once

// Digital-mode poll response with one fret and one strum direction held.
//
// The two payload bytes carry DIFFERENT values on purpose: with the same value in both, a
// decoder that swapped the two byte positions would pass this vector.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kDigitalPressed[] = {
    0x41,  // header — digital
    0x5A,  // ready byte
    0xEF,  // buttons 0 — bit 4 (UP) clear: strum up held. 0xFF & ~0x10
    0xFD,  // buttons 1 — bit 1 (R2) clear: green fret held, as the SG reports it. 0xFF & ~0x02
};

}  // namespace vectors
