#pragma once

// What the guitar is doing, as controls rather than as bytes: five frets, both strum
// directions, start, select, tilt, and the whammy as one analog axis. This is the shape
// ADR-0003 promised the HID descriptor would carry, one layer before it becomes HID bytes.
//
// Pure data and a pure function (ADR-0002). Nothing here reads a clock or a pin.

#include "core/ps2_frame.h"

#include <cstddef>
#include <cstdint>

namespace ps2 {

// --- Where each control lives in a digital payload --------------------------------------
//
// Digital buttons are ACTIVE LOW: the bit is 1 when the button is released. So a control is
// pressed when its bit is CLEAR, which is the opposite of the obvious reading and is why
// `is_pressed` exists instead of a bare `&`.
//
// TODO(09-guitar-observe): every position and mask in this section is read from the PS2
// controller protocol documentation and from how Guitar Hero controllers are commonly
// reported to map onto it. NONE of it has been measured against this guitar. Phase
// 09-guitar-observe confronts all of it with the real device; until then a vector in this
// repo asserts that the codec does what the codec says, not that the SG agrees.

constexpr std::size_t kButtonsLowIndex  = 0;  // payload byte carrying SELECT/START/d-pad
constexpr std::size_t kButtonsHighIndex = 1;  // payload byte carrying the face and shoulder buttons

// Bits of the low buttons byte.
constexpr std::uint8_t kMaskSelect    = 0x01;  // bit 0 — SELECT
constexpr std::uint8_t kMaskStart     = 0x08;  // bit 3 — START
constexpr std::uint8_t kMaskStrumUp   = 0x10;  // bit 4 — D-pad UP
constexpr std::uint8_t kMaskStrumDown = 0x40;  // bit 6 — D-pad DOWN

// Bits of the high buttons byte.
constexpr std::uint8_t kMaskTilt       = 0x01;  // bit 0 — L2, the tilt/effects switch
constexpr std::uint8_t kMaskFretOrange = 0x04;  // bit 2 — L1
constexpr std::uint8_t kMaskFretYellow = 0x10;  // bit 4 — TRIANGLE
constexpr std::uint8_t kMaskFretRed    = 0x20;  // bit 5 — CIRCLE
constexpr std::uint8_t kMaskFretGreen  = 0x40;  // bit 6 — CROSS
constexpr std::uint8_t kMaskFretBlue   = 0x80;  // bit 7 — SQUARE

// --- The whammy axis --------------------------------------------------------------------

// Payload index of the whammy, valid only in an analog frame. An analog payload is two
// button bytes followed by four axis bytes, and this is the last of them.
//
// TODO(09-guitar-observe): which of the four axes the SG reports the whammy on is a guess,
// and so are the two values below. Confronting them with the real device is exactly what
// 09-guitar-observe is for.
constexpr std::size_t kWhammyIndex = 5;

// The value the codec REPORTS when there is no analog frame to read one from — supplied by
// us, never a byte reinterpreted out of a digital payload (R-PROTO-04).
constexpr std::uint8_t kWhammyRest = 0x80;

// The number of fret buttons, so a caller can size an array without counting the members.
constexpr std::size_t kFretCount = 5;

// --- The state --------------------------------------------------------------------------

// Plain data, comparable as a whole, in ADR-0003's order. `frets` is indexed by
// ControllerFret so the HID descriptor and this array cannot disagree about which bit is
// green.
// The enumerators are left unnumbered on purpose: C++ fixes them at 0..4 in declaration
// order, which is exactly the contract `is_fret_pressed` needs, and writing the numbers out
// would add five literals that could drift from it.
enum class Fret : std::uint8_t {
    Green,
    Red,
    Yellow,
    Blue,
    Orange,
};

struct GuitarState {
    // Indexed by Fret. Named as an assertion because it holds assertions (R-CLEAN-03): the
    // check's grep does not see an array declaration, so this one is on the writer.
    bool is_fret_pressed[ kFretCount ];
    bool is_strum_up;
    bool is_strum_down;
    bool is_start;
    bool is_select;
    bool is_tilt;

    // 0x00..0xFF, one axis. kWhammyRest when the frame did not report analog mode.
    std::uint8_t whammy;
};

// Turn one decoded frame into controls.
//
// Everything is gated on the frame's id, in both directions. The whammy is read ONLY from an
// analog frame; every other id yields kWhammyRest (R-PROTO-04). The buttons are read ONLY
// from an id that reports controls — Digital and Analog carry them in the same two payload
// bytes; Config is a valid frame that reports no controls at all, so it maps to every
// control released rather than to a reading of its padding.
[[nodiscard]] GuitarState map_frame( const Ps2Frame& frame );

}  // namespace ps2
