#pragma once

// The real SG in digital mode, one control held per frame, in ps2::Button order. Transcribed by
// hand from docs/phases/09-guitar-observe/captures/sg-digital.log, and tilt from
// sg-tilt-digital.log (R-PROTO-05). Each frame is [header][0x5A][buttons 0][buttons 1]; each
// byte's comment names the capture line it came from.
//
// The operator called the bit-6 strum "up (toward the legs)" and the bit-4 strum "down (toward
// the face)". Here they follow the D-pad they report on: bit 4 is UP, bit 6 is DOWN (operator's
// decision, 2026-10-12).

#include <cstddef>
#include <cstdint>

namespace vectors {

constexpr std::size_t kSgControlCount = 10;
constexpr std::size_t kSgDigitalLen   = 4;

// clang-format off
constexpr std::uint8_t kSgDigitalControls[ kSgControlCount ][ kSgDigitalLen ] = {
    {
        0x41,  // green: header — digital (sg-digital.log:4, in byte 1)
        0x5A,  // green: ready byte (sg-digital.log:4, in byte 2)
        0x7F,  // green: buttons 0 — nothing held (sg-digital.log:4, in byte 3)
        0xFD,  // green: buttons 1 — bit 1 clear (sg-digital.log:4, in byte 4)
    },
    {
        0x41,  // red: header — digital (sg-digital.log:6, in byte 1)
        0x5A,  // red: ready byte (sg-digital.log:6, in byte 2)
        0x7F,  // red: buttons 0 — nothing held (sg-digital.log:6, in byte 3)
        0xDF,  // red: buttons 1 — bit 5 clear (sg-digital.log:6, in byte 4)
    },
    {
        0x41,  // yellow: header — digital (sg-digital.log:8, in byte 1)
        0x5A,  // yellow: ready byte (sg-digital.log:8, in byte 2)
        0x7F,  // yellow: buttons 0 — nothing held (sg-digital.log:8, in byte 3)
        0xEF,  // yellow: buttons 1 — bit 4 clear (sg-digital.log:8, in byte 4)
    },
    {
        0x41,  // blue: header — digital (sg-digital.log:10, in byte 1)
        0x5A,  // blue: ready byte (sg-digital.log:10, in byte 2)
        0x7F,  // blue: buttons 0 — nothing held (sg-digital.log:10, in byte 3)
        0xBF,  // blue: buttons 1 — bit 6 clear (sg-digital.log:10, in byte 4)
    },
    {
        0x41,  // orange: header — digital (sg-digital.log:12, in byte 1)
        0x5A,  // orange: ready byte (sg-digital.log:12, in byte 2)
        0x7F,  // orange: buttons 0 — nothing held (sg-digital.log:12, in byte 3)
        0x7F,  // orange: buttons 1 — bit 7 clear (sg-digital.log:12, in byte 4)
    },
    {
        0x41,  // strum up: header — digital (sg-digital.log:16, in byte 1)
        0x5A,  // strum up: ready byte (sg-digital.log:16, in byte 2)
        0x6F,  // strum up: buttons 0 — bit 4 clear (sg-digital.log:16, in byte 3)
        0xFF,  // strum up: buttons 1 — nothing held (sg-digital.log:16, in byte 4)
    },
    {
        0x41,  // strum down: header — digital (sg-digital.log:14, in byte 1)
        0x5A,  // strum down: ready byte (sg-digital.log:14, in byte 2)
        0x3F,  // strum down: buttons 0 — bit 6 clear (sg-digital.log:14, in byte 3)
        0xFF,  // strum down: buttons 1 — nothing held (sg-digital.log:14, in byte 4)
    },
    {
        0x41,  // start: header — digital (sg-digital.log:18, in byte 1)
        0x5A,  // start: ready byte (sg-digital.log:18, in byte 2)
        0x77,  // start: buttons 0 — bit 3 clear (sg-digital.log:18, in byte 3)
        0xFF,  // start: buttons 1 — nothing held (sg-digital.log:18, in byte 4)
    },
    {
        0x41,  // select: header — digital (sg-digital.log:20, in byte 1)
        0x5A,  // select: ready byte (sg-digital.log:20, in byte 2)
        0x7E,  // select: buttons 0 — bit 0 clear (sg-digital.log:20, in byte 3)
        0xFF,  // select: buttons 1 — nothing held (sg-digital.log:20, in byte 4)
    },
    {
        0x41,  // tilt: header — digital (sg-tilt-digital.log:4, in byte 1)
        0x5A,  // tilt: ready byte (sg-tilt-digital.log:4, in byte 2)
        0x7F,  // tilt: buttons 0 — nothing held (sg-tilt-digital.log:4, in byte 3)
        0xFE,  // tilt: buttons 1 — bit 0 clear (sg-tilt-digital.log:4, in byte 4)
    },
};
// clang-format on

}  // namespace vectors
