#pragma once

// A header byte that is not a declared controller id (R-PROTO-03).
//
// 0x79 is deliberately a REAL PS2 id — the DualShock 2's full 18-byte analog mode — rather
// than an invented byte like 0xAB. Refusing a plausible neighbour is the case that matters:
// an id this project does not support must be refused, not decoded on a best-effort basis
// because it happens to look well-formed. The rest of this frame IS well-formed, so nothing
// but the header can be the reason it is refused.
//
// That last sentence is why this vector is 20 bytes and not 4. 0x79's low nibble announces
// 9 * 2 = 18 payload bytes, so a whole frame is 2 + 18. A shorter vector would be cut short
// AS WELL AS undeclared, and would then be refused for either of two reasons — it would stop
// isolating the one thing it exists to measure. The outcome would not change, because §Goal's
// precedence takes the id before the length, but an outcome that merely agrees with the
// precedence does not test the precedence. `truncated_not_ready.h` is the vector for the
// overlap; this one measures a single fault. Lengthened 2026-09-15 for that reason.
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
    0xFF,  // buttons 0 — active low, nothing pressed
    0xFF,  // buttons 1 — likewise
    0x80,  // right stick X — centred
    0x80,  // right stick Y — centred
    0x80,  // left stick X — centred
    0x80,  // left stick Y — centred
    0x00,  // pressure: right      — the twelve button-pressure bytes a full analog
    0x00,  // pressure: left         frame carries. 0x00 is "not pressed", which agrees
    0x00,  // pressure: up           with the two button bytes above.
    0x00,  // pressure: down
    0x00,  // pressure: triangle
    0x00,  // pressure: circle
    0x00,  // pressure: cross
    0x00,  // pressure: square
    0x00,  // pressure: L1
    0x00,  // pressure: R1
    0x00,  // pressure: L2
    0x00,  // pressure: R2
};

}  // namespace vectors
