#pragma once

// The seam between the frame loop and the hardware: what src/hal/bus_frame.cpp calls to move
// one byte and to frame it with ATT.
//
// Declarations only, and no SDK header, so the frame loop compiles on the laptop. The firmware
// links src/hal/pio_port.cpp, which drives the PIO state machine; tests/bus_frame_cases.cpp
// links a fake that records every call.

#include "core/ps2_protocol.h"

#include <array>
#include <cstdint>
#include <optional>

namespace ps2 {

// Configures the bus pins from kMasterPins and starts the PIO master. Once, at boot.
void bus_init();

// ATT low: the controller is selected and listens from the next clock edge.
void att_assert();

// ATT high: the controller is deselected and the bus is idle.
void att_release();

// One byte's outcome: the byte shifted in, or std::nullopt when the byte, or its ACK when one
// was asked for, did not complete within kAckTimeoutUs; and on both paths the microseconds from
// handing the byte over until it completed or was given up on (ADR-0015).
struct ByteExchange {
    std::optional<std::uint8_t> in;
    std::uint32_t               elapsed_us;
};

// Shifts `out` onto CMD and one byte in from DATA, LSB first. With `should_wait_ack`, then
// waits for the controller to pull ACK low.
[[nodiscard]] ByteExchange exchange_byte( std::uint8_t out, bool should_wait_ack );

// The bit-order probe (R-PROTO-01): shifts `byte` out at a slow clock and reads the CMD pad at
// each rising CLK edge, in time order. std::nullopt when eight edges did not come within a
// second. Not part of any frame; the fake port does not define it.
[[nodiscard]] std::optional<std::array<std::uint8_t, kWireBitsPerByte>>
probe_wire_bits( std::uint8_t byte );

}  // namespace ps2
