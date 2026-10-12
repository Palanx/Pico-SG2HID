#pragma once

// The real SG's answers to the three analog-mode negotiation frames, wire byte 0 included.
// Transcribed by hand from docs/phases/09-guitar-observe/captures/sg-analog.log lines 3, 5
// and 6 (R-PROTO-05).

#include <cstdint>

namespace vectors {

// Enter config mode, sent while the SG is still digital: it answers as a digital poll.
constexpr std::uint8_t kSgAnswerEnter[] = {
    0xFF,  // wire byte 0 — answer to the address byte (sg-analog.log:3)
    0x41,  // header — still digital (sg-analog.log:3)
    0x5A,  // ready byte (sg-analog.log:3)
    0x7F,  // buttons 0 (sg-analog.log:3)
    0xFF,  // buttons 1 (sg-analog.log:3)
};

// Select analog: config mode, six zero bytes.
constexpr std::uint8_t kSgAnswerSet[] = {
    0xFF,  // wire byte 0 (sg-analog.log:5)
    0xF3,  // header — config mode (sg-analog.log:5)
    0x5A,  // ready byte (sg-analog.log:5)
    0x00,  // payload 0 (sg-analog.log:5)
    0x00,  // payload 1 (sg-analog.log:5)
    0x00,  // payload 2 (sg-analog.log:5)
    0x00,  // payload 3 (sg-analog.log:5)
    0x00,  // payload 4 (sg-analog.log:5)
    0x00,  // payload 5 (sg-analog.log:5)
};

// Leave config mode: still config mode in this answer, six zero bytes.
constexpr std::uint8_t kSgAnswerLeave[] = {
    0xFF,  // wire byte 0 (sg-analog.log:6)
    0xF3,  // header — config mode (sg-analog.log:6)
    0x5A,  // ready byte (sg-analog.log:6)
    0x00,  // payload 0 (sg-analog.log:6)
    0x00,  // payload 1 (sg-analog.log:6)
    0x00,  // payload 2 (sg-analog.log:6)
    0x00,  // payload 3 (sg-analog.log:6)
    0x00,  // payload 4 (sg-analog.log:6)
    0x00,  // payload 5 (sg-analog.log:6)
};

}  // namespace vectors
