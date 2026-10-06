// Emulator firmware for phase 05-emulator: a second Pico that answers the master as a wired SG
// would (ADR-0004). While ATT is low it serves the frame one byte at a time from SgModel; while
// ATT is high it reads command lines from its own USB serial port (`mode`, `payload`, `fault`;
// grammar in docs/phases/05-emulator/spec.md §Goal) and answers each with `ok:` or `error:`.
// Configures no pin itself: device_init( ) in src/hal/ does, from kEmulatorPins (R-SAFETY-09,
// R-SAFETY-10).
#include "emu/sg_model.h"
#include "hal/device_port.h"
#include "pico/stdlib.h"

#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

static_assert( ps2::kDeviceAckPulseUs == ps2::kAckPulseUs,
               "the PIO program's ACK pulse is the model's" );

// One character more than the longest accepted line plus its '\r', so a line that is too long
// still reaches SgModel::apply( ) too long and is refused there.
constexpr std::size_t kLineBufferSize = ps2::kMaxLineLen + 2;

struct LineReader {
    char        text[ kLineBufferSize ];
    std::size_t len;
};

// Reads at most one waiting character; on '\n', applies the line and prints the answer.
void poll_command( LineReader& reader, ps2::SgModel& model ) {
    const int c = getchar_timeout_us( 0 );
    if ( c == PICO_ERROR_TIMEOUT ) {
        return;
    }
    if ( c != '\n' ) {
        if ( reader.len < kLineBufferSize ) {
            reader.text[ reader.len++ ] = static_cast<char>( c );
        }
        return;
    }
    std::string_view line( reader.text, reader.len );
    reader.len        = 0;
    const auto result = model.apply( line );
    if ( !line.empty() && line.back() == '\r' ) {
        line.remove_suffix( 1 );
    }
    if ( result.has_value() ) {
        std::printf( "ok: %.*s\n", static_cast<int>( line.size() ), line.data() );
    } else {
        std::printf(
            "error: %.*s\n", static_cast<int>( result.error().size() ), result.error().data() );
    }
}

// Waits `delay_us` before the ACK, unless the master gives up on the frame first.
[[nodiscard]] bool wait_unless_deselected( std::uint32_t delay_us ) {
    const std::uint32_t start = time_us_32();
    while ( time_us_32() - start < delay_us ) {
        if ( ps2::device_att_is_high() ) {
            return false;
        }
    }
    return true;
}

// One frame, from ATT falling to ATT rising.
void serve_frame( ps2::SgModel& model ) {
    model.reset();
    while ( !ps2::device_att_is_high() ) {
        const auto received = ps2::device_take_byte();
        if ( !received ) {
            continue;
        }
        const ps2::ByteAnswer answer = model.step( *received );
        if ( answer.should_ack && !wait_unless_deselected( answer.ack_delay_us ) ) {
            return;
        }
        ps2::device_answer( answer.next, answer.should_ack );
    }
}

}  // namespace

int main() {
    stdio_init_all();
    ps2::device_init();

    ps2::SgModel model;
    LineReader   reader{};
    while ( true ) {
        ps2::device_restart();
        while ( ps2::device_att_is_high() ) {
            poll_command( reader, model );
            // Clocks with ATT high are not for this controller (the master's boot probe makes
            // some); a byte they completed is dropped and the state machine re-armed.
            if ( ps2::device_take_byte() ) {
                ps2::device_restart();
            }
        }
        serve_frame( model );
    }
}
