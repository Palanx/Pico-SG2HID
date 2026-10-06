#pragma once

// The emulator's side of the bus: what src/emu/main.cpp calls to answer the master one byte at a
// time. Declarations only, and no SDK header, as in src/hal/bus_port.h. The firmware links
// src/hal/pio_device.cpp, which drives the PIO program in src/hal/ps2_device.pio.

#include <cstdint>
#include <optional>

namespace ps2 {

// How long the ACK pulse lasts: the PIO program's pulse, static_asserted against its cycle count
// in pio_device.cpp, and against the emulator model's kAckPulseUs in src/emu/main.cpp.
constexpr std::uint32_t kDeviceAckPulseUs = 2;

// Configures the bus pins from kEmulatorPins and starts the PIO device with DATA and ACK
// released. Once, at boot.
void device_init();

// The start of a frame's wait: the state machine is halted, emptied, has DATA and ACK
// released, and is put back with 0xFF loaded for wire byte 0. Called while ATT is high.
void device_restart();

// The byte the master just shifted in, or std::nullopt when none has completed yet.
[[nodiscard]] std::optional<std::uint8_t> device_take_byte();

// Hands the state machine its next word: pulse ACK now for the byte just taken when
// `should_ack`, then shift `next` out on the master's next eight clocks.
void device_answer( std::uint8_t next, bool should_ack );

// ATT's level: high while the master has the controller deselected.
[[nodiscard]] bool device_att_is_high();

}  // namespace ps2
