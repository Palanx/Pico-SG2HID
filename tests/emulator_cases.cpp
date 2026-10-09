// Assertions over the emulator model in src/emu/sg_model.cpp (R-EMU-01, R-EMU-02).
//
// NOT named test_*.cpp on purpose: the Makefile builds and runs every tests/test_*.cpp, and this
// file needs src/emu/sg_model.cpp linked in. tests/test_emulator.py is the single entry point —
// it compiles this file with the model, runs it, forwards the lines below, and mutates the model
// to prove each line bites.
//
// Expected bytes come from tests/vectors/ only, never from src/core/ (ADR-0004, R-PROTO-05): the
// emulator shares core, so a core misreading of the protocol would otherwise agree with itself.

#include "emu/sg_model.h"
#include "vectors/analog_idle.h"
#include "vectors/config_mode.h"
#include "vectors/digital_idle.h"
#include "vectors/digital_pressed.h"
#include "vectors/negotiation.h"
#include "vectors/poll_exchange.h"

#include <cstdio>
#include <span>
#include <string_view>

namespace {

constexpr std::size_t kWireBytes = std::size( vectors::kPollCommand );

// The id `fault id 79` puts on wire byte 1: not one the protocol defines.
constexpr std::uint8_t kFaultedId = 0x79;

// One frame as the bus would show it: per wire byte, what the emulator sent, whether it ACKed
// the master's byte, and after how long.
struct Frame {
    std::uint8_t  sent[ kWireBytes ];
    bool          is_acked[ kWireBytes ];
    std::uint32_t delay_us[ kWireBytes ];
};

// Drives a whole frame of master bytes through the model, the way src/emu/main.cpp does: byte 0
// is the reset answer, and every later byte is the previous step's `next`.
[[nodiscard]] Frame drive( ps2::SgModel& model, std::span<const std::uint8_t> command ) {
    Frame frame{};
    model.reset();
    frame.sent[ 0 ] = ps2::kIdleByte;
    for ( std::size_t i = 0; i < command.size() && i < kWireBytes; ++i ) {
        const ps2::ByteAnswer answer = model.step( command[ i ] );
        frame.is_acked[ i ]          = answer.should_ack;
        frame.delay_us[ i ]          = answer.should_ack ? answer.ack_delay_us : 0;
        if ( i + 1 < kWireBytes ) {
            frame.sent[ i + 1 ] = answer.next;
        }
    }
    return frame;
}

[[nodiscard]] Frame poll( ps2::SgModel& model ) {
    return drive( model, vectors::kPollCommand );
}

// The frame a faithful SG sends for `response` (a vector: the reply with its first byte
// dropped): the address reply, the response, then released DATA; every byte but the response's
// last ACKed after kAckDelayUs.
[[nodiscard]] Frame faithful( std::span<const std::uint8_t> response ) {
    Frame frame{};
    frame.sent[ 0 ] = vectors::kAddressReply;
    for ( std::size_t i = 1; i < kWireBytes; ++i ) {
        frame.sent[ i ] = i - 1 < response.size() ? response[ i - 1 ] : vectors::kReleasedByte;
    }
    for ( std::size_t i = 0; i < response.size(); ++i ) {
        frame.is_acked[ i ] = true;
        frame.delay_us[ i ] = ps2::kAckDelayUs;
    }
    return frame;
}

[[nodiscard]] bool same( const Frame& got, const Frame& want ) {
    for ( std::size_t i = 0; i < kWireBytes; ++i ) {
        if ( got.sent[ i ] != want.sent[ i ] || got.is_acked[ i ] != want.is_acked[ i ] ||
             got.delay_us[ i ] != want.delay_us[ i ] ) {
            return false;
        }
    }
    return true;
}

// `got` against `want` over the first `driven` wire bytes: what was sent at each, and the ACK of
// each but the last, which the master never waits for. For a frame the master cuts short of the
// controller's answer, such as the 5-byte enter-config.
[[nodiscard]] bool same_prefix( const Frame& got, const Frame& want, std::size_t driven ) {
    for ( std::size_t i = 0; i < driven && i < kWireBytes; ++i ) {
        const bool is_waited_on = i + 1 < driven;
        if ( got.sent[ i ] != want.sent[ i ] ||
             ( is_waited_on && ( got.is_acked[ i ] != want.is_acked[ i ] ||
                                 got.delay_us[ i ] != want.delay_us[ i ] ) ) ) {
            return false;
        }
    }
    return true;
}

// Whether every byte the emulator sent after wire byte `broken`, up to the `driven` bytes the
// master clocked, is 0xFF: DATA released.
[[nodiscard]] bool released_after( const Frame& frame, std::size_t broken, std::size_t driven ) {
    for ( std::size_t i = broken + 1; i <= driven && i < kWireBytes; ++i ) {
        if ( frame.sent[ i ] != vectors::kReleasedByte ) {
            return false;
        }
    }
    return true;
}

// One named case: prints its own FAIL line and returns whether it held.
[[nodiscard]] bool expect( bool is_ok, const char* label ) {
    if ( !is_ok ) {
        std::printf( "  FAIL:   case: %s\n", label );
    }
    return is_ok;
}

[[nodiscard]] bool applied( ps2::SgModel& model, std::string_view line ) {
    return model.apply( line ).has_value();
}

// --- R-EMU-01: the faithful answer ---------------------------------------------------------

[[nodiscard]] bool faithful_answer() {
    bool is_ok = true;
    {
        ps2::SgModel model;
        is_ok = expect( same( poll( model ), faithful( vectors::kDigitalIdle ) ),
                        "default digital answers kDigitalIdle" ) &&
                is_ok;
    }
    {
        ps2::SgModel model;
        is_ok = expect( applied( model, "mode analog" ) &&
                            same( poll( model ), faithful( vectors::kAnalogIdle ) ),
                        "mode analog answers kAnalogIdle" ) &&
                is_ok;
    }
    {
        // The payload line is built from the vector's own payload bytes, plus four centred axes.
        char line[ ps2::kMaxLineLen + 1 ];
        std::snprintf( line,
                       sizeof( line ),
                       "payload %02X %02X 80 80 80 80",
                       vectors::kDigitalPressed[ 2 ],
                       vectors::kDigitalPressed[ 3 ] );
        ps2::SgModel model;
        is_ok = expect( applied( model, line ) &&
                            same( poll( model ), faithful( vectors::kDigitalPressed ) ),
                        "payload answers kDigitalPressed" ) &&
                is_ok;
    }
    {
        // Not addressed: no ACK at byte 0 or after it, and DATA released from byte 1 on.
        constexpr std::uint8_t kWrongStart[] = { 0x42, 0x42, 0x00, 0x00, 0x00 };
        ps2::SgModel           model;
        const Frame            frame   = drive( model, kWrongStart );
        bool                   is_none = true;
        for ( const bool is_acked : frame.is_acked ) {
            is_none = is_none && !is_acked;
        }
        is_ok = expect( is_none, "a frame starting 0x42 gets no ACK from byte 0 on" ) && is_ok;
        is_ok = expect( released_after( frame, 0, std::size( kWrongStart ) ),
                        "a frame starting 0x42 gets 0xFF after byte 0" ) &&
                is_ok;
    }
    {
        // Addressed, then set-mode outside config mode: ACK at 0 only.
        ps2::SgModel model;
        const Frame  frame   = drive( model, vectors::kSetAnalog );
        bool         is_none = true;
        for ( std::size_t i = 1; i < kWireBytes; ++i ) {
            is_none = is_none && !frame.is_acked[ i ];
        }
        is_ok = expect( frame.is_acked[ 0 ] && is_none,
                        "01 44 outside config mode gets no ACK from byte 1 on" ) &&
                is_ok;
        is_ok = expect( released_after( frame, 1, std::size( vectors::kSetAnalog ) ),
                        "01 44 outside config mode gets 0xFF after byte 1" ) &&
                is_ok;
    }
    return is_ok;
}

// --- R-EMU-01: config mode and the analog-mode sequence (07-analog-mode) -------------------

// Drives the three negotiation frames from tests/vectors/negotiation.h, then a poll, and returns
// the poll's frame. `answers` receives the three negotiation frames.
[[nodiscard]] Frame negotiate( ps2::SgModel& model, Frame ( &answers )[ 3 ] ) {
    answers[ 0 ] = drive( model, vectors::kEnterConfig );
    answers[ 1 ] = drive( model, vectors::kSetAnalog );
    answers[ 2 ] = drive( model, vectors::kLeaveConfig );
    return poll( model );
}

[[nodiscard]] bool config_mode() {
    bool        is_ok  = true;
    const Frame config = faithful( vectors::kConfigMode );
    {
        ps2::SgModel model;
        Frame        answers[ 3 ];
        const Frame  polled = negotiate( model, answers );
        is_ok               = expect( same_prefix( answers[ 0 ],
                                                   faithful( vectors::kDigitalIdle ),
                                                   std::size( vectors::kEnterConfig ) ),
                                      "from boot, the enter is answered as kDigitalIdle" ) &&
                              is_ok;
        is_ok               = expect( same( answers[ 1 ], config ),
                                      "set-analog is answered as kConfigMode, byte 8 not ACKed" ) &&
                              is_ok;
        is_ok = expect( same( answers[ 2 ], config ),
                        "leave-config is answered as kConfigMode, byte 8 not ACKed" ) &&
                is_ok;
        is_ok = expect( same( polled, faithful( vectors::kAnalogIdle ) ),
                        "the poll after the sequence is answered as kAnalogIdle" ) &&
                is_ok;
    }
    {
        constexpr std::size_t kAnsweredLen = std::size( vectors::kEnterConfig ) - 1;
        ps2::SgModel          model;
        const bool            is_applied = applied( model, "mode analog" );
        const Frame           entered    = drive( model, vectors::kEnterConfig );
        is_ok =
            expect( is_applied &&
                        same_prefix(
                            entered,
                            faithful( std::span( vectors::kAnalogIdle ).first( kAnsweredLen ) ),
                            std::size( vectors::kEnterConfig ) ),
                    "after mode analog, the enter is answered with kAnalogIdle's first 4 bytes" ) &&
            is_ok;
    }
    return is_ok;
}

// --- R-EMU-02: faults and commands ---------------------------------------------------------

[[nodiscard]] bool faults() {
    bool        is_ok = true;
    const Frame idle  = faithful( vectors::kDigitalIdle );
    {
        ps2::SgModel model;
        Frame        want  = idle;
        want.is_acked[ 2 ] = false;
        want.delay_us[ 2 ] = 0;
        is_ok              = expect( applied( model, "fault ack 2" ) && same( poll( model ), want ),
                                     "fault ack 2 withholds only byte 2's ACK" ) &&
                             is_ok;
    }
    {
        constexpr std::uint32_t kLateUs = 50;
        ps2::SgModel            model;
        Frame                   want = idle;
        for ( std::size_t i = 0; i < kWireBytes; ++i ) {
            want.delay_us[ i ] = want.is_acked[ i ] ? kLateUs : 0;
        }
        is_ok = expect( applied( model, "fault late 50" ) && same( poll( model ), want ),
                        "fault late 50 changes only the delay" ) &&
                is_ok;
    }
    {
        ps2::SgModel model;
        Frame        want = idle;
        want.sent[ 1 ]    = kFaultedId;  // sent[] is indexed by wire byte; byte 1 carries the id
        is_ok             = expect( applied( model, "fault id 79" ) && same( poll( model ), want ),
                                    "fault id 79 changes only byte 1" ) &&
                            is_ok;
    }
    {
        ps2::SgModel model;
        is_ok = expect( applied( model, "fault id 79" ) && applied( model, "fault none" ) &&
                            same( poll( model ), idle ),
                        "fault none restores the faithful answer" ) &&
                is_ok;
    }
    {
        ps2::SgModel model;
        Frame        answers[ 3 ];
        const bool   is_applied = applied( model, "fault decline" );
        is_ok = expect( is_applied && same( negotiate( model, answers ), idle ),
                        "fault decline: the poll after the sequence is still kDigitalIdle" ) &&
                is_ok;
    }
    {
        ps2::SgModel model;
        is_ok = expect( applied( model, "mode analog\r" ) &&
                            same( poll( model ), faithful( vectors::kAnalogIdle ) ),
                        "a trailing CR is ignored" ) &&
                is_ok;
    }
    return is_ok;
}

[[nodiscard]] bool refused_lines() {
    bool                       is_ok      = true;
    const Frame                idle       = faithful( vectors::kDigitalIdle );
    // Each refused line errors and leaves the next frame as it was. The long line would be a
    // valid `mode analog` but for its length, so only the length check refuses it.
    constexpr std::string_view kRefused[] = {
        "bogus",
        "payload 01 02 03 04 05",
        "payload 01 02 03 04 05 06 07",
        "payload 01 02 03 04 05 0G",
        "payload 01 02 03 04 05 6",
        "fault ack 8",
        "fault late 0",
        "fault late 10001",
        "fault decline x",
        "mode analog                                                      ",
    };
    for ( const std::string_view line : kRefused ) {
        ps2::SgModel model;
        const bool   is_refused = !applied( model, line );
        if ( !expect( is_refused && same( poll( model ), idle ), "a refused line" ) ) {
            std::printf(
                "          line: \"%.*s\"\n", static_cast<int>( line.size() ), line.data() );
            is_ok = false;
        }
    }
    return is_ok;
}

[[nodiscard]] bool report( bool is_ok, const char* label ) {
    std::printf( "  %s %s\n", is_ok ? "ok:  " : "FAIL:", label );
    return is_ok;
}

}  // namespace

int main() {
    bool       is_ok        = true;
    const bool is_config_ok = config_mode();
    is_ok = report( faithful_answer() && is_config_ok,
                    "R-EMU-01 (the emulator answers a poll as the vectors say)" ) &&
            is_ok;
    const bool is_faults_ok = faults();
    is_ok                   = report( refused_lines() && is_faults_ok,
                                      "R-EMU-02 (each fault and command changes only its part)" ) &&
                              is_ok;
    return is_ok ? 0 : 1;
}
