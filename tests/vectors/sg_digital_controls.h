#pragma once

// The real SG in digital mode, one control held per frame, in ps2::Button order. Nine frames:
// the SG has no tilt sensor, so Button::Tilt has none. Transcribed by hand from
// docs/phases/09-guitar-observe/captures/sg-digital.log (R-PROTO-05). Each frame is
// [header][0x5A][buttons 0][buttons 1]; the comment names the capture line it came from.
//
// The operator called the bit-6 strum "up (toward the legs)" and the bit-4 strum "down (toward
// the face)". Here they follow the D-pad they report on: bit 4 is UP, bit 6 is DOWN.

#include <cstddef>
#include <cstdint>

namespace vectors {

constexpr std::size_t kSgControlCount = 9;
constexpr std::size_t kSgDigitalLen   = 4;

constexpr std::uint8_t kSgDigitalControls[ kSgControlCount ][ kSgDigitalLen ] = {
    {0x41, 0x5A, 0x7F, 0xFD}, // green       — sg-digital.log:4,  buttons 1 bit 1 clear
    {0x41, 0x5A, 0x7F, 0xDF}, // red         — sg-digital.log:6,  buttons 1 bit 5 clear
    {0x41, 0x5A, 0x7F, 0xEF}, // yellow      — sg-digital.log:8,  buttons 1 bit 4 clear
    {0x41, 0x5A, 0x7F, 0xBF}, // blue        — sg-digital.log:10, buttons 1 bit 6 clear
    {0x41, 0x5A, 0x7F, 0x7F}, // orange      — sg-digital.log:12, buttons 1 bit 7 clear
    {0x41, 0x5A, 0x6F, 0xFF}, // strum up    — sg-digital.log:16, buttons 0 bit 4 clear
    {0x41, 0x5A, 0x3F, 0xFF}, // strum down  — sg-digital.log:14, buttons 0 bit 6 clear
    {0x41, 0x5A, 0x77, 0xFF}, // start       — sg-digital.log:18, buttons 0 bit 3 clear
    {0x41, 0x5A, 0x7E, 0xFF}, // select      — sg-digital.log:20, buttons 0 bit 0 clear
};

}  // namespace vectors
