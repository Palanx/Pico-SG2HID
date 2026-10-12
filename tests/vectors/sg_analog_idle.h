#pragma once

// The real SG in analog mode, nothing touched. Transcribed by hand from
// docs/phases/09-guitar-observe/captures/sg-analog.log line 8 (R-PROTO-05).

#include <cstdint>

namespace vectors {

// The whammy byte the SG sends with the bar untouched (sg-analog.log:8, in byte 8).
constexpr std::uint8_t kSgWhammyAtRest = 0x7F;

constexpr std::uint8_t kSgAnalogIdle[] = {
    0x73,  // header — analog (sg-analog.log:8, in byte 1)
    0x5A,  // ready byte (sg-analog.log:8, in byte 2)
    0x7F,  // buttons 0 — bit 7 always clear on the SG (sg-analog.log:8, in byte 3)
    0xFF,  // buttons 1 — every fret released (sg-analog.log:8, in byte 4)
    0x7F,  // axis 0 (sg-analog.log:8, in byte 5)
    0x7F,  // axis 1 (sg-analog.log:8, in byte 6)
    0x7F,  // axis 2 (sg-analog.log:8, in byte 7)
    0x7F,  // axis 3 — the whammy, untouched (sg-analog.log:8, in byte 8)
};

}  // namespace vectors
