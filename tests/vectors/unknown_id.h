#pragma once

// A header byte that is not a declared controller id (R-PROTO-03).
//
// 0x79 is deliberately a REAL PS2 id — the DualShock 2's full 18-byte analog mode — rather
// than an invented byte like 0xAB. Refusing a plausible neighbour is the case that matters:
// an id this project does not support must be refused, not decoded on a best-effort basis
// because it happens to look well-formed. The rest of this frame IS well-formed, so nothing
// but the header can be the reason it is refused.
//
// If a later phase ever declares 0x79, this vector fails and must be repointed at another
// undeclared byte. That is the intended behaviour, not breakage.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kUnknownId[] = {
    0x79,  // header — a real controller id this project does not declare
    0x5A,  // ready byte — valid, so it cannot be the reason for the refusal
    0xFF,  // payload 0
    0xFF,  // payload 1
};

}  // namespace vectors
