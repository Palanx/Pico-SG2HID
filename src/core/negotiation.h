#pragma once

// The analog-mode sequence, master side (ADR-0017): which frame to send next, and what the
// answer to it means. The whammy exists only in an analog frame (R-PROTO-04), so the master asks
// for analog mode before it polls and never streams digital.
//
// Pure, like `step` (ADR-0011): `app` exchanges the bytes and reads the clock; this decides.

#include "core/bus_trace.h"
#include "core/link.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ps2 {

// Where the master is in the sequence, in the order it walks it. Every frame that leaves the
// link Absent sends it back to EnterConfig.
enum class NegotiationStage : std::uint8_t {
    EnterConfig,  // ask for config mode; any mode's answer will do
    SetAnalog,    // select analog mode and lock it
    LeaveConfig,  // leave config mode, keeping the mode just set
    Polling,      // read buttons and axes from an analog frame
};

struct Master {
    Link             link;
    NegotiationStage stage = NegotiationStage::EnterConfig;
};

// The bytes the master sends at `stage`: 5 for EnterConfig, 9 for every other stage.
[[nodiscard]] std::span<const std::uint8_t> command_for( NegotiationStage stage );

// One exchanged frame: every wire byte, of which the first `completed` completed.
struct Exchanged {
    std::span<const WireByte> wire;
    std::size_t               completed;
};

// Judges one exchanged frame at the master's current stage, steps the link and the stage, and
// returns the link's state.
//
// - EnterConfig: accepted when every byte completed and the prefix holds (a declared id, then
//   kReadyByte); the stage moves on and the link is not stepped. Otherwise the link is stepped.
// - Any later stage: a valid prefix with an id the stage does not expect is declined: the link
//   drops to Absent with FaultCause::Declined. Otherwise the link is stepped, and the stage moves
//   on unless the link is now Absent.
[[nodiscard]] LinkState
advance( Master& master, const Exchanged& exchanged, std::uint32_t elapsed_us );

}  // namespace ps2
