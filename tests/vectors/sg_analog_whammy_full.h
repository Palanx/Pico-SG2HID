#pragma once

// The real SG in analog mode with the whammy bar at its stop. Transcribed by hand from
// docs/phases/09-guitar-observe/captures/sg-analog.log line 91 (R-PROTO-05).

#include <cstdint>

namespace vectors {

// The whammy byte the SG sends with the bar at its stop (sg-analog.log:91, in byte 8).
constexpr std::uint8_t kSgWhammyFull = 0x00;

constexpr std::uint8_t kSgAnalogWhammyFull[] = {
    0x73,  // header — analog (sg-analog.log:91, in byte 1)
    0x5A,  // ready byte (sg-analog.log:91, in byte 2)
    0x7F,  // buttons 0 (sg-analog.log:91, in byte 3)
    0xFF,  // buttons 1 (sg-analog.log:91, in byte 4)
    0x7F,  // axis 0 (sg-analog.log:91, in byte 5)
    0x7F,  // axis 1 (sg-analog.log:91, in byte 6)
    0x7F,  // axis 2 (sg-analog.log:91, in byte 7)
    0x00,  // axis 3 — the whammy at its stop (sg-analog.log:91, in byte 8)
};

}  // namespace vectors
