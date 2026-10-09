#pragma once

// The master's side of the analog-mode sequence (07-analog-mode, ADR-0017), in the order it is
// sent: enter config mode, select analog, leave config mode. Polling then uses kPollCommand from
// poll_exchange.h. R-PROTO-05: hand-written literal. Read by tests/ps2_codec_cases.cpp, which
// compares the master's commands with these bytes, and by tests/emulator_cases.cpp, which drives
// the emulator model with them.

#include <cstdint>

namespace vectors {

// Short on purpose: the answer is judged on its prefix only, whatever mode the controller is in.
constexpr std::uint8_t kEnterConfig[] = {
    0x01,  // address the controller
    0x43,  // config command
    0x00,  // pad — the controller sends its ready byte here
    0x01,  // 0x01 = enter config mode
    0x00,  // pad
};

constexpr std::uint8_t kSetAnalog[] = {
    0x01,  // address the controller
    0x44,  // set mode — accepted only in config mode
    0x00,  // pad — the controller sends its ready byte here
    0x01,  // 0x01 = analog
    0x03,  // 0x03 = lock the mode, so the ANALOG button cannot switch it back
    0x00,  // pad
    0x00,  // pad
    0x00,  // pad
    0x00,  // pad
};

constexpr std::uint8_t kLeaveConfig[] = {
    0x01,  // address the controller
    0x43,  // config command
    0x00,  // pad — the controller sends its ready byte here
    0x00,  // 0x00 = leave config mode
    0x00,  // pad
    0x00,  // pad
    0x00,  // pad
    0x00,  // pad
    0x00,  // pad
};

}  // namespace vectors
