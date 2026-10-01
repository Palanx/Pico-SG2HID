// The firmware's side of src/hal/bus_port.h: the PS2 bus on a PIO state machine.
//
// The only file that configures a bus pin, and it does so in one loop over kMasterPins
// (R-SAFETY-10), with every configuring call on a line naming `pin.gpio`. DATA and ACK are
// never made outputs (R-SAFETY-01): they get an input with a pull-up and nothing else.

#include "hal/bus_port.h"

#include "core/pins.h"
#include "core/ps2_protocol.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "ps2_master.pio.h"

namespace ps2 {

namespace {

constexpr std::uint32_t kCyclesPerBit  = 4;   // two with CLK low, two high: see ps2_master.pio
constexpr std::uint32_t kUsPerSecond   = 1000000;
constexpr std::uint32_t kShiftBits     = 32;  // a whole FIFO word; no autopush or autopull
constexpr std::uint32_t kRxByteShift   = kShiftBits - kWireBitsPerByte;
constexpr std::uint32_t kWaitAckFlag   = 1U << kWireBitsPerByte;
constexpr std::uint32_t kSlowestDivInt = 65535;  // the largest integer PIO clock divider
constexpr std::uint32_t kProbeBudgetUs = kUsPerSecond;

// Eight bit periods, then kAckTimeoutUs for the controller's ACK.
constexpr std::uint32_t kByteBudgetUs =
    ( kWireBitsPerByte * kUsPerSecond / kBusClockHz ) + kAckTimeoutUs;

PIO   bus_pio    = pio0;
uint  bus_sm     = 0;
uint  bus_offset = 0;
float bus_div    = 1.0F;

[[nodiscard]] bool rx_within( std::uint32_t start, std::uint32_t budget_us ) {
    while ( pio_sm_is_rx_fifo_empty( bus_pio, bus_sm ) ) {
        if ( time_us_32() - start > budget_us ) {
            return false;
        }
    }
    return true;
}

// Stops a byte the controller never finished: the state machine is halted, emptied and put
// back at `pull`, with CLK high. ATT is the frame loop's to release.
void recover() {
    pio_sm_set_enabled( bus_pio, bus_sm, false );
    pio_sm_clear_fifos( bus_pio, bus_sm );
    pio_sm_restart( bus_pio, bus_sm );
    pio_sm_exec( bus_pio, bus_sm, pio_encode_jmp( bus_offset ) );
    pio_sm_set_enabled( bus_pio, bus_sm, true );
}

[[nodiscard]] bool clk_reaches( bool is_high, std::uint32_t start ) {
    while ( gpio_get( gpio_of( Signal::Clk ) ) != is_high ) {
        if ( time_us_32() - start > kProbeBudgetUs ) {
            return false;
        }
    }
    return true;
}

void set_divider( float div ) {
    pio_sm_set_clkdiv( bus_pio, bus_sm, div );
    pio_sm_clkdiv_restart( bus_pio, bus_sm );
}

}  // namespace

void bus_init() {
    bus_sm     = static_cast<uint>( pio_claim_unused_sm( bus_pio, true ) );
    bus_offset = static_cast<uint>( pio_add_program( bus_pio, &ps2_master_program ) );

    for ( const auto& pin : kMasterPins ) {
        const std::uint32_t mask = 1U << pin.gpio;
        if ( pin.drive == DriveMode::OpenDrainInputOnly ) {
            gpio_init( pin.gpio );
            gpio_pull_up( pin.gpio );
        } else if ( pin.signal == Signal::Att ) {
            gpio_init( pin.gpio );
            gpio_put( pin.gpio, true );  // released before it can drive
            gpio_set_dir( pin.gpio, GPIO_OUT );
        } else {
            pio_sm_set_pins_with_mask( bus_pio, bus_sm, mask, mask );  // idle high first
            pio_sm_set_consecutive_pindirs( bus_pio, bus_sm, pin.gpio, 1, true );
            pio_gpio_init( bus_pio, pin.gpio );
        }
    }

    pio_sm_config config = ps2_master_program_get_default_config( bus_offset );
    sm_config_set_out_pins( &config, gpio_of( Signal::Cmd ), 1 );
    sm_config_set_in_pins( &config, gpio_of( Signal::Data ) );
    sm_config_set_sideset_pins( &config, gpio_of( Signal::Clk ) );
    sm_config_set_jmp_pin( &config, gpio_of( Signal::Ack ) );
    sm_config_set_out_shift( &config, true, false, kShiftBits );
    sm_config_set_in_shift( &config, true, false, kShiftBits );
    bus_div = static_cast<float>( clock_get_hz( clk_sys ) ) /
              static_cast<float>( kCyclesPerBit * kBusClockHz );
    sm_config_set_clkdiv( &config, bus_div );
    pio_sm_init( bus_pio, bus_sm, bus_offset, &config );
    pio_sm_set_enabled( bus_pio, bus_sm, true );
}

void att_assert() {
    gpio_put( gpio_of( Signal::Att ), false );
}

void att_release() {
    gpio_put( gpio_of( Signal::Att ), true );
}

ByteExchange exchange_byte( std::uint8_t out, bool should_wait_ack ) {
    const std::uint32_t start = time_us_32();
    pio_sm_put( bus_pio, bus_sm, out | ( should_wait_ack ? kWaitAckFlag : 0U ) );
    const bool          is_complete = rx_within( start, kByteBudgetUs );
    const std::uint32_t elapsed_us  = time_us_32() - start;  // before recover( ) on a timeout
    if ( !is_complete ) {
        recover();
        return { .in = std::nullopt, .elapsed_us = elapsed_us };
    }
    return { .in = static_cast<std::uint8_t>( pio_sm_get( bus_pio, bus_sm ) >> kRxByteShift ),
             .elapsed_us = elapsed_us };
}

std::optional<std::array<std::uint8_t, kWireBitsPerByte>> probe_wire_bits( std::uint8_t byte ) {
    set_divider( static_cast<float>( kSlowestDivInt ) );
    std::array<std::uint8_t, kWireBitsPerByte> bits{};
    const std::uint32_t                        start = time_us_32();
    pio_sm_put( bus_pio, bus_sm, byte );
    bool is_complete = true;
    for ( auto& bit : bits ) {
        if ( !clk_reaches( false, start ) || !clk_reaches( true, start ) ) {
            is_complete = false;
            break;
        }
        bit = gpio_get( gpio_of( Signal::Cmd ) ) ? 1 : 0;
    }
    is_complete = is_complete && rx_within( start, kProbeBudgetUs );
    if ( is_complete ) {
        static_cast<void>( pio_sm_get( bus_pio, bus_sm ) );
    } else {
        recover();
    }
    set_divider( bus_div );
    if ( !is_complete ) {
        return std::nullopt;
    }
    return bits;
}

}  // namespace ps2
