#pragma once

// The HID gamepad report: the exact bytes phase 08-usb-hid hands to TinyUSB.
//
// This file is the single place the layout is decided, and it is decided here rather than in
// the descriptor because two phases have to agree on it: 08-usb-hid writes the HID report
// descriptor FROM these constants. A descriptor and a builder that disagree produce a device
// that enumerates cleanly and reports the wrong control, which is the worst kind of bug to
// find from the host side.
//
// The control set is ADR-0003's, exactly: five frets, strum up, strum down, start, select,
// tilt, and the whammy as one analog axis.
//
// HID buttons are ACTIVE HIGH — a set bit means pressed. PS2 buttons are active low. The
// inversion happens in guitar_state.cpp, so by the time a GuitarState reaches this file the
// booleans already mean what they say.

#include "core/guitar_state.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ps2 {

// Buttons in report order. The enumerators are their HID button indices: button N lives in bit
// N % 8 of byte N / 8, so the descriptor needs to declare a count and never a table. Left
// unnumbered for the same reason as Fret — the language fixes them at 0..9 and a written
// number is one more thing that can drift.
enum class Button : std::uint8_t {
    FretGreen,
    FretRed,
    FretYellow,
    FretBlue,
    FretOrange,
    StrumUp,
    StrumDown,
    Start,
    Select,
    Tilt,
};

constexpr std::size_t kButtonCount = 10;
constexpr std::size_t kBitsPerByte = 8;

// Derived, not written down: adding an eleventh button widens the report instead of silently
// writing past the end of it.
constexpr std::size_t kButtonBytes = ( kButtonCount + kBitsPerByte - 1 ) / kBitsPerByte;

// The whammy axis sits immediately after the button bytes.
constexpr std::size_t kWhammyOffset = kButtonBytes;
constexpr std::size_t kReportLen    = kButtonBytes + 1;

// Just the bytes. A struct wrapping this array would buy a name and nothing else; the layout
// constants above are what carry the meaning.
using HidReport = std::array<std::uint8_t, kReportLen>;

// Pack one guitar state into the report bytes. Every bit not named by a Button is zero, so the
// padding bits the descriptor declares are never stale.
[[nodiscard]] HidReport build_report( const GuitarState& state );

}  // namespace ps2
