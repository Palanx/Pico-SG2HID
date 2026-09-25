// Assertions over the pin table in src/core/pins.h (R-SAFETY-01, R-SAFETY-02, R-SAFETY-03).
//
// NOT named test_*.cpp on purpose: the Makefile builds and runs every tests/test_*.cpp, so that
// name would give this file a second entry point with no driver around it.
// tests/test_pin_table.py is the single entry point — it compiles this file, runs it, forwards
// the lines below, and checks the `pin:` lines against docs/wiring.md.
//
// One check function per rule, one aggregate that reports them. Every case goes THROUGH the
// aggregate's reporting, never around it: a case that called a check function directly would
// leave the verdict text and the exit code proven by nothing (00-scaffold notes, round 9).

#include "core/pins.h"

#include <cstdio>
#include <cstring>
#include <span>

namespace {

using Table = std::span<const ps2::PinAssignment>;

// GPIOs the Pico board wires to its own parts: 23 the regulator's power-save pin, 24 VBUS
// sense, 25 the LED, 29 the VSYS voltage divider. A test constant, not a `core` one — the
// firmware has no business knowing which pins it must not want.
constexpr std::uint8_t kReservedGpios[] = { 23, 24, 25, 29 };

constexpr ps2::Signal kAllSignals[] = {
    ps2::Signal::Data, ps2::Signal::Cmd, ps2::Signal::Att, ps2::Signal::Clk, ps2::Signal::Ack };

// Large enough for every line one aggregate run prints.
constexpr std::size_t kCaptureSize = 1024;

[[nodiscard]] const char* signal_name( ps2::Signal signal ) {
    switch ( signal ) {
    case ps2::Signal::Data:
        return "DATA";
    case ps2::Signal::Cmd:
        return "CMD";
    case ps2::Signal::Att:
        return "ATT";
    case ps2::Signal::Clk:
        return "CLK";
    case ps2::Signal::Ack:
        return "ACK";
    }
    return "?";
}

// --- one check per rule ----------------------------------------------------------------

// R-SAFETY-01: DATA and ACK are inputs and are never push-pull.
[[nodiscard]] bool check_bus_inputs_not_driven( Table table ) {
    for ( const ps2::PinAssignment& pin : table ) {
        const bool is_bus_input = pin.signal == ps2::Signal::Data || pin.signal == ps2::Signal::Ack;
        if ( is_bus_input &&
             ( pin.direction != ps2::Direction::Input || pin.drive == ps2::DriveMode::PushPull ) ) {
            return false;
        }
    }
    return true;
}

// R-SAFETY-02 (table): every bus signal is declared, and declared exactly once.
[[nodiscard]] bool check_each_signal_once( Table table ) {
    for ( const ps2::Signal signal : kAllSignals ) {
        int count = 0;
        for ( const ps2::PinAssignment& pin : table ) {
            if ( pin.signal == signal ) {
                ++count;
            }
        }
        if ( count != 1 ) {
            return false;
        }
    }
    return true;
}

// R-SAFETY-03: no reserved GPIO, and no GPIO shared by two entries.
[[nodiscard]] bool check_gpios_free_and_distinct( Table table ) {
    for ( std::size_t i = 0; i < table.size(); ++i ) {
        for ( const std::uint8_t reserved : kReservedGpios ) {
            if ( table[ i ].gpio == reserved ) {
                return false;
            }
        }
        for ( std::size_t j = i + 1; j < table.size(); ++j ) {
            if ( table[ i ].gpio == table[ j ].gpio ) {
                return false;
            }
        }
    }
    return true;
}

// --- the aggregate ---------------------------------------------------------------------

[[nodiscard]] bool report( std::FILE* out, bool is_ok, const char* label ) {
    std::fprintf( out, "  %s %s\n", is_ok ? "ok:  " : "FAIL:", label );
    return is_ok;
}

// One line per rule into `out`, and the verdict of all three as the return value. The real
// run, the rejection cases and the wiring cases all come through here.
[[nodiscard]] bool report_rules( Table table, std::FILE* out ) {
    bool is_ok = true;
    is_ok      = report( out,
                         check_bus_inputs_not_driven( table ),
                         "R-SAFETY-01 (DATA and ACK are inputs, never push-pull)" ) &&
                 is_ok;
    is_ok      = report( out,
                         check_each_signal_once( table ),
                         "R-SAFETY-02 (table): every bus signal declared exactly once" ) &&
                 is_ok;
    is_ok      = report( out,
                         check_gpios_free_and_distinct( table ),
                         "R-SAFETY-03 (no reserved GPIO, no GPIO shared)" ) &&
                 is_ok;
    return is_ok;
}

// Runs the aggregate on `table` with its lines captured instead of printed, so a case can
// read the verdict text the real run would have produced.
struct Capture {
    bool is_ok;
    char text[ kCaptureSize ];
};

[[nodiscard]] Capture capture_report( Table table ) {
    Capture    result{};
    std::FILE* scratch = std::tmpfile();
    if ( scratch == nullptr ) {
        return result;  // is_ok false and empty text: every case reading it fails loudly.
    }
    result.is_ok = report_rules( table, scratch );
    std::rewind( scratch );
    const std::size_t read = std::fread( result.text, 1, kCaptureSize - 1, scratch );
    result.text[ read ]    = '\0';
    std::fclose( scratch );
    return result;
}

// Does the captured output carry `FAIL:` on the line of `rule`?
[[nodiscard]] bool says_fail( const Capture& capture, const char* rule ) {
    const char* line = std::strstr( capture.text, rule );
    if ( line == nullptr ) {
        return false;
    }
    const char* start = line;
    while ( start > capture.text && start[ -1 ] != '\n' ) {
        --start;
    }
    const char* verdict = std::strstr( start, "FAIL:" );
    return verdict != nullptr && verdict < line;
}

// --- fixtures ----------------------------------------------------------------------------

// A copy of the good table that one fixture then breaks in one place.
struct Fixture {
    ps2::PinAssignment pins[ std::size( ps2::kMasterPins ) + 1 ];
    std::size_t        size;

    [[nodiscard]] Table table() const {
        return { pins, size };
    }
};

[[nodiscard]] Fixture good_fixture() {
    Fixture fixture{};
    for ( const ps2::PinAssignment& pin : ps2::kMasterPins ) {
        fixture.pins[ fixture.size++ ] = pin;
    }
    return fixture;
}

[[nodiscard]] ps2::PinAssignment& entry( Fixture& fixture, ps2::Signal signal ) {
    for ( std::size_t i = 0; i < fixture.size; ++i ) {
        if ( fixture.pins[ i ].signal == signal ) {
            return fixture.pins[ i ];
        }
    }
    return fixture.pins[ 0 ];  // unreachable for the master table, which has every signal.
}

void drop( Fixture& fixture, ps2::Signal signal ) {
    std::size_t kept = 0;
    for ( std::size_t i = 0; i < fixture.size; ++i ) {
        if ( fixture.pins[ i ].signal != signal ) {
            fixture.pins[ kept++ ] = fixture.pins[ i ];
        }
    }
    fixture.size = kept;
}

struct Rejection {
    const char* label;
    const char* rule;
    Fixture     fixture;
};

[[nodiscard]] Fixture data_push_pull() {
    Fixture fixture                           = good_fixture();
    entry( fixture, ps2::Signal::Data ).drive = ps2::DriveMode::PushPull;
    return fixture;
}

[[nodiscard]] Fixture ack_push_pull() {
    Fixture fixture                          = good_fixture();
    entry( fixture, ps2::Signal::Ack ).drive = ps2::DriveMode::PushPull;
    return fixture;
}

[[nodiscard]] Fixture data_as_output() {
    Fixture fixture                               = good_fixture();
    entry( fixture, ps2::Signal::Data ).direction = ps2::Direction::Output;
    return fixture;
}

[[nodiscard]] Fixture ack_as_output() {
    Fixture fixture                              = good_fixture();
    entry( fixture, ps2::Signal::Ack ).direction = ps2::Direction::Output;
    return fixture;
}

[[nodiscard]] Fixture missing_signal() {
    Fixture fixture = good_fixture();
    drop( fixture, ps2::Signal::Clk );
    return fixture;
}

// A second CMD entry on a GPIO nothing else uses, so only the duplicate is wrong.
[[nodiscard]] Fixture duplicated_signal() {
    constexpr std::uint8_t kUnusedGpio = 10;
    Fixture                fixture     = good_fixture();
    fixture.pins[ fixture.size ]       = entry( fixture, ps2::Signal::Cmd );
    fixture.pins[ fixture.size ].gpio  = kUnusedGpio;
    ++fixture.size;
    return fixture;
}

[[nodiscard]] Fixture clk_on( std::uint8_t gpio ) {
    Fixture fixture                         = good_fixture();
    entry( fixture, ps2::Signal::Clk ).gpio = gpio;
    return fixture;
}

[[nodiscard]] Fixture shared_gpio() {
    Fixture fixture                         = good_fixture();
    entry( fixture, ps2::Signal::Clk ).gpio = entry( fixture, ps2::Signal::Att ).gpio;
    return fixture;
}

// --- the real run ------------------------------------------------------------------------

[[nodiscard]] bool real_run() {
    for ( const ps2::PinAssignment& pin : ps2::kMasterPins ) {
        std::printf( "  pin: %s GP%d\n", signal_name( pin.signal ), static_cast<int>( pin.gpio ) );
    }
    return report_rules( ps2::kMasterPins, stdout );
}

// --- rejection cases: each bad table flips its own rule's line ---------------------------

[[nodiscard]] bool rejection_cases() {
    const Rejection cases[] = {
        {"DATA push-pull",          "R-SAFETY-01", data_push_pull()             },
        {"ACK push-pull",           "R-SAFETY-01", ack_push_pull()              },
        {"DATA as output",          "R-SAFETY-01", data_as_output()             },
        {"ACK as output",           "R-SAFETY-01", ack_as_output()              },
        {"CLK missing",             "R-SAFETY-02", missing_signal()             },
        {"CMD declared twice",      "R-SAFETY-02", duplicated_signal()          },
        {"CLK on GPIO 23",          "R-SAFETY-03", clk_on( kReservedGpios[ 0 ] )},
        {"CLK on GPIO 24",          "R-SAFETY-03", clk_on( kReservedGpios[ 1 ] )},
        {"CLK on GPIO 25",          "R-SAFETY-03", clk_on( kReservedGpios[ 2 ] )},
        {"CLK on GPIO 29",          "R-SAFETY-03", clk_on( kReservedGpios[ 3 ] )},
        {"CLK and ATT on one GPIO", "R-SAFETY-03", shared_gpio()                },
    };
    int passed = 0;
    for ( const Rejection& rejection : cases ) {
        const Capture capture = capture_report( rejection.fixture.table() );
        if ( says_fail( capture, rejection.rule ) ) {
            ++passed;
        } else {
            std::printf( "  FAIL: rejection case %s: %s's line did not say FAIL\n",
                         rejection.label,
                         rejection.rule );
        }
    }
    const int total = static_cast<int>( std::size( cases ) );
    std::printf( "  %s rejection cases: %d/%d (each bad table flips its own rule's line)\n",
                 passed == total ? "ok:  " : "FAIL:",
                 passed,
                 total );
    return passed == total;
}

// --- wiring cases: the aggregate's verdict reaches the return value ----------------------

[[nodiscard]] bool wiring_cases() {
    const Rejection cases[] = {
        {"DATA push-pull", "R-SAFETY-01", data_push_pull()             },
        {"CLK missing",    "R-SAFETY-02", missing_signal()             },
        {"CLK on GPIO 25", "R-SAFETY-03", clk_on( kReservedGpios[ 2 ] )},
    };
    int passed = 0;
    for ( const Rejection& rejection : cases ) {
        const Capture capture = capture_report( rejection.fixture.table() );
        if ( !capture.is_ok && std::strstr( capture.text, rejection.rule ) != nullptr ) {
            ++passed;
        } else {
            std::printf( "  FAIL: wiring case %s: the aggregate did not fail naming %s\n",
                         rejection.label,
                         rejection.rule );
        }
    }
    const int total = static_cast<int>( std::size( cases ) );
    std::printf( "  %s wiring cases: %d/%d (a bad table fails the aggregate)\n",
                 passed == total ? "ok:  " : "FAIL:",
                 passed,
                 total );
    return passed == total;
}

}  // namespace

int main() {
    bool is_ok = true;
    is_ok      = real_run() && is_ok;
    is_ok      = rejection_cases() && is_ok;
    is_ok      = wiring_cases() && is_ok;
    return is_ok ? 0 : 1;
}
