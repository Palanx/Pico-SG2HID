#pragma once

// The real SG in analog mode, one control held per frame, in ps2::Button order. Transcribed by
// hand from docs/phases/09-guitar-observe/captures/sg-analog.log, and tilt from
// sg-tilt-analog.log (R-PROTO-05). Each frame is [header][0x5A][buttons 0][buttons 1][axis 0..3];
// each byte's comment names the capture line it came from. Strum is named by D-pad bit, as in
// sg_digital_controls.h.

#include <cstddef>
#include <cstdint>

namespace vectors {

constexpr std::size_t kSgAnalogCount = 10;
constexpr std::size_t kSgAnalogLen   = 8;

// clang-format off
constexpr std::uint8_t kSgAnalogControls[ kSgAnalogCount ][ kSgAnalogLen ] = {
    {
        0x73,  // green: header — analog (sg-analog.log:9, in byte 1)
        0x5A,  // green: ready byte (sg-analog.log:9, in byte 2)
        0x7F,  // green: buttons 0 — nothing held (sg-analog.log:9, in byte 3)
        0xFD,  // green: buttons 1 — bit 1 clear (sg-analog.log:9, in byte 4)
        0x7F,  // green: axis 0 (sg-analog.log:9, in byte 5)
        0x7F,  // green: axis 1 (sg-analog.log:9, in byte 6)
        0x7F,  // green: axis 2 (sg-analog.log:9, in byte 7)
        0x7F,  // green: axis 3, whammy at rest (sg-analog.log:9, in byte 8)
    },
    {
        0x73,  // red: header — analog (sg-analog.log:11, in byte 1)
        0x5A,  // red: ready byte (sg-analog.log:11, in byte 2)
        0x7F,  // red: buttons 0 — nothing held (sg-analog.log:11, in byte 3)
        0xDF,  // red: buttons 1 — bit 5 clear (sg-analog.log:11, in byte 4)
        0x7F,  // red: axis 0 (sg-analog.log:11, in byte 5)
        0x7F,  // red: axis 1 (sg-analog.log:11, in byte 6)
        0x7F,  // red: axis 2 (sg-analog.log:11, in byte 7)
        0x7F,  // red: axis 3, whammy at rest (sg-analog.log:11, in byte 8)
    },
    {
        0x73,  // yellow: header — analog (sg-analog.log:13, in byte 1)
        0x5A,  // yellow: ready byte (sg-analog.log:13, in byte 2)
        0x7F,  // yellow: buttons 0 — nothing held (sg-analog.log:13, in byte 3)
        0xEF,  // yellow: buttons 1 — bit 4 clear (sg-analog.log:13, in byte 4)
        0x7F,  // yellow: axis 0 (sg-analog.log:13, in byte 5)
        0x7F,  // yellow: axis 1 (sg-analog.log:13, in byte 6)
        0x7F,  // yellow: axis 2 (sg-analog.log:13, in byte 7)
        0x7F,  // yellow: axis 3, whammy at rest (sg-analog.log:13, in byte 8)
    },
    {
        0x73,  // blue: header — analog (sg-analog.log:15, in byte 1)
        0x5A,  // blue: ready byte (sg-analog.log:15, in byte 2)
        0x7F,  // blue: buttons 0 — nothing held (sg-analog.log:15, in byte 3)
        0xBF,  // blue: buttons 1 — bit 6 clear (sg-analog.log:15, in byte 4)
        0x7F,  // blue: axis 0 (sg-analog.log:15, in byte 5)
        0x7F,  // blue: axis 1 (sg-analog.log:15, in byte 6)
        0x7F,  // blue: axis 2 (sg-analog.log:15, in byte 7)
        0x7F,  // blue: axis 3, whammy at rest (sg-analog.log:15, in byte 8)
    },
    {
        0x73,  // orange: header — analog (sg-analog.log:17, in byte 1)
        0x5A,  // orange: ready byte (sg-analog.log:17, in byte 2)
        0x7F,  // orange: buttons 0 — nothing held (sg-analog.log:17, in byte 3)
        0x7F,  // orange: buttons 1 — bit 7 clear (sg-analog.log:17, in byte 4)
        0x7F,  // orange: axis 0 (sg-analog.log:17, in byte 5)
        0x7F,  // orange: axis 1 (sg-analog.log:17, in byte 6)
        0x7F,  // orange: axis 2 (sg-analog.log:17, in byte 7)
        0x7F,  // orange: axis 3, whammy at rest (sg-analog.log:17, in byte 8)
    },
    {
        0x73,  // strum up: header — analog (sg-analog.log:25, in byte 1)
        0x5A,  // strum up: ready byte (sg-analog.log:25, in byte 2)
        0x6F,  // strum up: buttons 0 — bit 4 clear (sg-analog.log:25, in byte 3)
        0xFF,  // strum up: buttons 1 — nothing held (sg-analog.log:25, in byte 4)
        0x7F,  // strum up: axis 0 (sg-analog.log:25, in byte 5)
        0x7F,  // strum up: axis 1 (sg-analog.log:25, in byte 6)
        0x7F,  // strum up: axis 2 (sg-analog.log:25, in byte 7)
        0x7F,  // strum up: axis 3, whammy at rest (sg-analog.log:25, in byte 8)
    },
    {
        0x73,  // strum down: header — analog (sg-analog.log:23, in byte 1)
        0x5A,  // strum down: ready byte (sg-analog.log:23, in byte 2)
        0x3F,  // strum down: buttons 0 — bit 6 clear (sg-analog.log:23, in byte 3)
        0xFF,  // strum down: buttons 1 — nothing held (sg-analog.log:23, in byte 4)
        0x7F,  // strum down: axis 0 (sg-analog.log:23, in byte 5)
        0x7F,  // strum down: axis 1 (sg-analog.log:23, in byte 6)
        0x7F,  // strum down: axis 2 (sg-analog.log:23, in byte 7)
        0x7F,  // strum down: axis 3, whammy at rest (sg-analog.log:23, in byte 8)
    },
    {
        0x73,  // start: header — analog (sg-analog.log:19, in byte 1)
        0x5A,  // start: ready byte (sg-analog.log:19, in byte 2)
        0x77,  // start: buttons 0 — bit 3 clear (sg-analog.log:19, in byte 3)
        0xFF,  // start: buttons 1 — nothing held (sg-analog.log:19, in byte 4)
        0x7F,  // start: axis 0 (sg-analog.log:19, in byte 5)
        0x7F,  // start: axis 1 (sg-analog.log:19, in byte 6)
        0x7F,  // start: axis 2 (sg-analog.log:19, in byte 7)
        0x7F,  // start: axis 3, whammy at rest (sg-analog.log:19, in byte 8)
    },
    {
        0x73,  // select: header — analog (sg-analog.log:21, in byte 1)
        0x5A,  // select: ready byte (sg-analog.log:21, in byte 2)
        0x7E,  // select: buttons 0 — bit 0 clear (sg-analog.log:21, in byte 3)
        0xFF,  // select: buttons 1 — nothing held (sg-analog.log:21, in byte 4)
        0x7F,  // select: axis 0 (sg-analog.log:21, in byte 5)
        0x7F,  // select: axis 1 (sg-analog.log:21, in byte 6)
        0x7F,  // select: axis 2 (sg-analog.log:21, in byte 7)
        0x7F,  // select: axis 3, whammy at rest (sg-analog.log:21, in byte 8)
    },
    {
        0x73,  // tilt: header — analog (sg-tilt-analog.log:2, in byte 1)
        0x5A,  // tilt: ready byte (sg-tilt-analog.log:2, in byte 2)
        0x7F,  // tilt: buttons 0 — nothing held (sg-tilt-analog.log:2, in byte 3)
        0xFE,  // tilt: buttons 1 — bit 0 clear (sg-tilt-analog.log:2, in byte 4)
        0x7F,  // tilt: axis 0 (sg-tilt-analog.log:2, in byte 5)
        0x7F,  // tilt: axis 1 (sg-tilt-analog.log:2, in byte 6)
        0x7F,  // tilt: axis 2 (sg-tilt-analog.log:2, in byte 7)
        0x7F,  // tilt: axis 3, whammy at rest (sg-tilt-analog.log:2, in byte 8)
    },
};
// clang-format on

}  // namespace vectors
