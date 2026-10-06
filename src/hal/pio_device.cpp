// The firmware's side of src/hal/device_port.h: the emulator's half of the PS2 bus on a PIO state
// machine.
//
// The only file in the emulator firmware that configures a pin, in one loop over kEmulatorPins
// (R-SAFETY-10), with every configuring call on a line naming `pin.gpio`. DATA and ACK are
// open-drain (ADR-0016): their pad output is forced low, and the program changes only their
// direction.
//
// Order matters, and it is the reason for the order below: gpio_set_function( ), which
// pio_gpio_init( ) calls, clears every field of the pin's control register but the function,
// the output override included (pico-sdk 2.3.1, hardware_gpio/gpio.c). So the override is set
// after the pin is handed to PIO, while its direction is still input, and before the program
// can make it an output.

#include "hal/device_port.h"

#include "core/pins.h"
#include "core/ps2_protocol.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ps2_device.pio.h"

namespace ps2 {

namespace {

constexpr std::uint32_t kDeviceCyclesPerUs = 8;
constexpr std::uint32_t kUsPerSecond       = 1000000;
constexpr std::uint32_t kShiftBits         = 32;  // a whole FIFO word; no autopush or autopull
constexpr std::uint32_t kRxByteShift       = kShiftBits - kWireBitsPerByte;
constexpr std::uint32_t kAckFlag           = 1U;
constexpr std::uint32_t kByteShift         = 1;     // the next byte sits above the ACK flag
constexpr std::uint8_t  kReleasedByte      = 0xFF;  // wire byte 0: DATA released throughout

// The pin offsets the program assumes (see ps2_device.pio's header).
constexpr std::uint32_t kClkFromInBase = 2;  // `wait … pin 2`
static_assert( gpio_of( Signal::Clk, kEmulatorPins ) ==
               gpio_of( Signal::Cmd, kEmulatorPins ) + kClkFromInBase );
static_assert( ps2_device_ACK_PULSE_CYCLES == kDeviceAckPulseUs * kDeviceCyclesPerUs );

PIO  device_pio    = pio0;
uint device_sm     = 0;
uint device_offset = 0;

// The word for the next byte. `next` is inverted: a 1 in pindirs pulls DATA low, a 0 on the wire.
[[nodiscard]] std::uint32_t word_for( std::uint8_t next, bool should_ack ) {
    const auto inverted = static_cast<std::uint8_t>( ~next );
    return ( static_cast<std::uint32_t>( inverted ) << kByteShift ) |
           ( should_ack ? kAckFlag : 0U );
}

}  // namespace

void device_init() {
    device_sm     = static_cast<uint>( pio_claim_unused_sm( device_pio, true ) );
    device_offset = static_cast<uint>( pio_add_program( device_pio, &ps2_device_program ) );

    for ( const auto& pin : kEmulatorPins ) {
        if ( pin.drive == DriveMode::OpenDrainOutput ) {
            pio_sm_set_consecutive_pindirs( device_pio, device_sm, pin.gpio, 1, false );
            pio_gpio_init( device_pio, pin.gpio );
            gpio_set_outover( pin.gpio, GPIO_OVERRIDE_LOW );  // never drives high
        } else {
            gpio_init( pin.gpio );
            gpio_disable_pulls( pin.gpio );  // the master drives it push-pull
        }
    }

    pio_sm_config config = ps2_device_program_get_default_config( device_offset );
    sm_config_set_out_pins( &config, gpio_of( Signal::Data, kEmulatorPins ), 1 );
    sm_config_set_set_pins( &config, gpio_of( Signal::Ack, kEmulatorPins ), 1 );
    sm_config_set_in_pins( &config, gpio_of( Signal::Cmd, kEmulatorPins ) );
    sm_config_set_out_shift( &config, true, false, kShiftBits );
    sm_config_set_in_shift( &config, true, false, kShiftBits );
    sm_config_set_clkdiv( &config,
                          static_cast<float>( clock_get_hz( clk_sys ) ) /
                              static_cast<float>( kDeviceCyclesPerUs * kUsPerSecond ) );
    pio_sm_init( device_pio, device_sm, device_offset, &config );
    device_restart();
}

void device_restart() {
    pio_sm_set_enabled( device_pio, device_sm, false );
    pio_sm_clear_fifos( device_pio, device_sm );
    pio_sm_restart( device_pio, device_sm );
    for ( const auto& pin : kEmulatorPins ) {
        if ( pin.drive == DriveMode::OpenDrainOutput ) {
            pio_sm_set_consecutive_pindirs( device_pio, device_sm, pin.gpio, 1, false );
        }
    }
    pio_sm_exec( device_pio, device_sm, pio_encode_jmp( device_offset ) );
    pio_sm_put( device_pio, device_sm, word_for( kReleasedByte, false ) );
    pio_sm_set_enabled( device_pio, device_sm, true );
}

std::optional<std::uint8_t> device_take_byte() {
    if ( pio_sm_is_rx_fifo_empty( device_pio, device_sm ) ) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>( pio_sm_get( device_pio, device_sm ) >> kRxByteShift );
}

void device_answer( std::uint8_t next, bool should_ack ) {
    pio_sm_put( device_pio, device_sm, word_for( next, should_ack ) );
}

bool device_att_is_high() {
    return gpio_get( gpio_of( Signal::Att, kEmulatorPins ) );
}

}  // namespace ps2
