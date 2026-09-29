#pragma once

// The master Pico's pin table: every GPIO this firmware uses, declared exactly once.
//
// This table is authoritative (ADR-0006, R-SAFETY-02). `docs/wiring.md` shows the same
// assignment as a breadboard would see it and must name the same GPIO per signal; the host
// test compares the two. Only `src/hal/` configures pins, and only by iterating this table.
//
// DATA and ACK are open-drain lines the guitar drives: the Pico only ever reads them. Making
// either one `PushPull` would short two output stages together (R-SAFETY-01).
//
// Pure data (ADR-0002). A plain C array, not `std::array`, so the columns stay aligned and a
// wrong drive mode is visible at a glance.

#include <cstdint>

namespace ps2 {

enum class Signal : std::uint8_t { Data, Cmd, Att, Clk, Ack };

enum class Direction : std::uint8_t { Input, Output };

// `OpenDrainInputOnly`: open-drain on the bus, read-only on the Pico, internal pull-up on.
enum class DriveMode : std::uint8_t { PushPull, OpenDrainInputOnly };

struct PinAssignment {
    std::uint8_t gpio;
    Signal       signal;
    Direction    direction;
    DriveMode    drive;
};

// Consecutive GPIOs so 03-pio-bus can map them onto PIO pin groups.
inline constexpr PinAssignment kMasterPins[] = {
    {.gpio      = 2,
     .signal    = Signal::Data,
     .direction = Direction::Input,
     .drive     = DriveMode::OpenDrainInputOnly},
    {.gpio      = 3,
     .signal    = Signal::Cmd,
     .direction = Direction::Output,
     .drive     = DriveMode::PushPull          },
    {.gpio      = 4,
     .signal    = Signal::Att,
     .direction = Direction::Output,
     .drive     = DriveMode::PushPull          },
    {.gpio      = 5,
     .signal    = Signal::Clk,
     .direction = Direction::Output,
     .drive     = DriveMode::PushPull          },
    {.gpio      = 6,
     .signal    = Signal::Ack,
     .direction = Direction::Input,
     .drive     = DriveMode::OpenDrainInputOnly},
};

}  // namespace ps2
