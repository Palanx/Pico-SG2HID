#pragma once

// The master's side of a poll, and the controller's answer to its first byte. R-PROTO-05:
// hand-written literal. Read by tests/emulator_cases.cpp, which drives the emulator model with
// these bytes and compares what comes back with digital_idle.h, digital_pressed.h and
// analog_idle.h.

#include <cstdint>

namespace vectors {

// Long enough for an analog frame; a digital one uses the first five.
constexpr std::uint8_t kPollCommand[] = {
    0x01,  // address the controller
    0x42,  // poll: read buttons and axes
    0x00,  // pad — the controller sends its ready byte here
    0x00,  // pad — payload byte 0
    0x00,  // pad — payload byte 1
    0x00,  // pad — payload byte 2 (analog only)
    0x00,  // pad — payload byte 3 (analog only)
    0x00,  // pad — payload byte 4 (analog only)
    0x00,  // pad — payload byte 5 (analog only)
};

// What the controller sends while the master sends 0x01: nothing, released DATA.
constexpr std::uint8_t kAddressReply = 0xFF;

}  // namespace vectors
