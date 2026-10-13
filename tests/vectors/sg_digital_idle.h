#pragma once

// The real SG in digital mode, nothing touched. Transcribed by hand from
// docs/phases/09-guitar-observe/captures/sg-digital.log line 3 (R-PROTO-05).

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kSgDigitalIdle[] = {
    0x41,  // header — digital (sg-digital.log:3, in byte 1)
    0x5A,  // ready byte (sg-digital.log:3, in byte 2)
    0x7F,  // buttons 0 — bit 7 always clear on the SG, nothing else (sg-digital.log:3, in byte 3)
    0xFF,  // buttons 1 — every fret released (sg-digital.log:3, in byte 4)
};

}  // namespace vectors
