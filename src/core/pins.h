#pragma once

// The pin tables: every GPIO each firmware uses, declared exactly once per role. `kMasterPins` is
// the master Pico's; `kEmulatorPins` is the second Pico that plays the guitar (05-emulator).
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
#include <span>
#include <utility>

namespace ps2 {

enum class Signal : std::uint8_t { Data, Cmd, Att, Clk, Ack };

enum class Direction : std::uint8_t { Input, Output };

// `OpenDrainInputOnly`: open-drain on the bus, read-only on the Pico, internal pull-up on.
// `Input`: read-only, no pull; the other Pico drives the line push-pull.
// `OpenDrainOutput`: pulls the line low or releases it; the pad output is forced low, so it can
// never drive high (ADR-0016).
enum class DriveMode : std::uint8_t { PushPull, OpenDrainInputOnly, Input, OpenDrainOutput };

struct PinAssignment {
    std::uint8_t gpio;
    Signal       signal;
    Direction    direction;
    DriveMode    drive;
};

// Consecutive GPIOs, so the PIO program in src/hal/ps2_master.pio can take CMD, DATA and CLK as
// pin-group bases.
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

// The emulator Pico (ADR-0004), on the same GPIOs as the master so the two breadboards mirror
// each other. It drives DATA and ACK open-drain (ADR-0016) and only reads CMD, ATT and CLK,
// which the master drives push-pull: no signal is an output on both Picos (R-SAFETY-06). CMD
// and CLK stay two GPIOs apart, which src/hal/ps2_device.pio relies on.
inline constexpr PinAssignment kEmulatorPins[] = {
    {.gpio      = 2,
     .signal    = Signal::Data,
     .direction = Direction::Output,
     .drive     = DriveMode::OpenDrainOutput                                                        },
    {.gpio = 3,      .signal = Signal::Cmd, .direction = Direction::Input, .drive = DriveMode::Input},
    {.gpio = 4,      .signal = Signal::Att, .direction = Direction::Input, .drive = DriveMode::Input},
    {.gpio = 5,      .signal = Signal::Clk, .direction = Direction::Input, .drive = DriveMode::Input},
    {.gpio      = 6,
     .signal    = Signal::Ack,
     .direction = Direction::Output,
     .drive     = DriveMode::OpenDrainOutput                                                        },
};

// The GPIO a signal is wired to in `table`, the master's unless named. `consteval`: a signal
// missing from the table reaches `std::unreachable`, which is not a constant expression, so the
// build fails instead of the firmware configuring a wrong pin.
consteval std::uint8_t gpio_of( Signal                         signal,
                                std::span<const PinAssignment> table = kMasterPins ) {
    for ( const PinAssignment& pin : table ) {
        if ( pin.signal == signal ) {
            return pin.gpio;
        }
    }
    std::unreachable();
}

}  // namespace ps2
