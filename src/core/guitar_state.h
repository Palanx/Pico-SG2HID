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
// Measured against the real SG, digital and analog mode alike (docs/constraints.md §Observed
// conventions, "The SG on the bus"). The SG has no tilt sensor: kMaskTilt is the documented
// position, never seen to change.

constexpr std::size_t kButtonsLowIndex  = 0;  // payload byte carrying SELECT/START/d-pad
constexpr std::size_t kButtonsHighIndex = 1;  // payload byte carrying the face and shoulder buttons

// Bits of the low buttons byte.
constexpr std::uint8_t kMaskSelect    = 0x01;  // bit 0 — SELECT
constexpr std::uint8_t kMaskStart     = 0x08;  // bit 3 — START
constexpr std::uint8_t kMaskStrumUp   = 0x10;  // bit 4 — D-pad UP
constexpr std::uint8_t kMaskStrumDown = 0x40;  // bit 6 — D-pad DOWN

// Bits of the high buttons byte.
constexpr std::uint8_t kMaskTilt       = 0x01;  // bit 0 — L2, the tilt switch (absent on this SG)
constexpr std::uint8_t kMaskFretGreen  = 0x02;  // bit 1 — R2
constexpr std::uint8_t kMaskFretYellow = 0x10;  // bit 4 — TRIANGLE
constexpr std::uint8_t kMaskFretRed    = 0x20;  // bit 5 — CIRCLE
constexpr std::uint8_t kMaskFretBlue   = 0x40;  // bit 6 — CROSS
constexpr std::uint8_t kMaskFretOrange = 0x80;  // bit 7 — SQUARE

// --- The whammy axis --------------------------------------------------------------------

// Payload index of the whammy, valid only in an analog frame. An analog payload is two
// button bytes followed by four axis bytes, and this is the last of them. Measured
// (docs/constraints.md §Observed conventions, "The SG on the bus").
constexpr std::size_t kWhammyIndex = 5;

// The SG's whammy byte: kWhammyAxisRest with the bar untouched, falling to 0x00 at its stop.
// The codec turns it around and doubles it (ADR-0022), so the reported axis runs from
// kWhammyRest at rest to 0xFE at the stop. A byte above kWhammyAxisRest reads as rest.
constexpr std::uint8_t kWhammyAxisRest  = 0x7F;
constexpr std::uint8_t kWhammyAxisScale = 2;

// The value the codec REPORTS with the bar at rest, and when there is no analog frame to read
// one from — supplied by us, never a byte reinterpreted out of a digital payload (R-PROTO-04).
constexpr std::uint8_t kWhammyRest = 0x00;

// The number of fret buttons, so a caller can size an array without counting the members.
constexpr std::size_t kFretCount = 5;

// --- The state --------------------------------------------------------------------------

// The frets in ADR-0003's order; `GuitarState::is_fret_pressed` is indexed by them.
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

    // kWhammyRest (bar untouched) to 0xFE (bar at its stop). kWhammyRest when the frame did
    // not report analog mode.
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
