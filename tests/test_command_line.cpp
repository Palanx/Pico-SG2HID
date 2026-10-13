// Host test for src/core/command_line.{h,cpp}: the observe firmware's serial input
// (09-guitar-observe).
//
// Named test_*.cpp on purpose: it is its own entry point, built and run by `make test`'s
// tests/test_*.cpp wildcard. The lines are serial text, not protocol bytes, so nothing here
// comes from tests/vectors/ (R-PROTO-05). Prints one `FAIL: <case>` line per failed case and
// exits 0 only when every case passed.

#include "core/command_line.h"

#include <cstdio>
#include <string_view>

namespace {

[[nodiscard]] bool report( bool is_ok, const char* label ) {
    if ( !is_ok ) {
        std::printf( "FAIL: %s\n", label );
    }
    return is_ok;
}

// Feeds every character of `text` and returns the event of the last one.
[[nodiscard]] ps2::LineEvent feed( ps2::CommandLine& line, std::string_view text ) {
    ps2::LineEvent event = ps2::LineEvent::Pending;
    for ( const char c : text ) {
        event = ps2::take_char( line, c );
    }
    return event;
}

[[nodiscard]] bool case_analog_is_accepted() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "analog\n" ) == ps2::LineEvent::Analog;
    return report( is_ok, "(a) `analog` is accepted" );
}

[[nodiscard]] bool case_analog_with_cr_is_accepted() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "analog\r\n" ) == ps2::LineEvent::Analog;
    return report( is_ok, "(b) `analog\\r` is accepted" );
}

[[nodiscard]] bool case_nothing_before_newline() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "analog" ) == ps2::LineEvent::Pending;
    return report( is_ok, "(c) nothing is answered before '\\n'" );
}

[[nodiscard]] bool case_empty_line_is_pending() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "\n" ) == ps2::LineEvent::Pending &&
                             feed( line, "\r\n" ) == ps2::LineEvent::Pending;
    return report( is_ok, "(d) an empty line is not answered" );
}

[[nodiscard]] bool case_other_lines_are_unknown() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "analo\n" ) == ps2::LineEvent::Unknown &&
                             feed( line, "Analog\n" ) == ps2::LineEvent::Unknown &&
                             feed( line, "digital\n" ) == ps2::LineEvent::Unknown;
    return report( is_ok, "(e) a near miss or another word is unknown" );
}

[[nodiscard]] bool case_overlong_is_unknown() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "analogue\n" ) == ps2::LineEvent::Unknown &&
                             feed( line, "analog analog analog\n" ) == ps2::LineEvent::Unknown;
    return report( is_ok, "(f) a line longer than the buffer is unknown, never cut to `analog`" );
}

[[nodiscard]] bool case_next_line_starts_clean() {
    ps2::CommandLine line{};
    const bool       is_ok = feed( line, "xxxxxxxxxxxx\n" ) == ps2::LineEvent::Unknown &&
                             feed( line, "analog\n" ) == ps2::LineEvent::Analog &&
                             feed( line, "analog\n" ) == ps2::LineEvent::Analog;
    return report( is_ok, "(g) each line is judged on its own, after an overlong one too" );
}

}  // namespace

int main() {
    bool is_ok = case_analog_is_accepted();
    is_ok      = case_analog_with_cr_is_accepted() && is_ok;
    is_ok      = case_nothing_before_newline() && is_ok;
    is_ok      = case_empty_line_is_pending() && is_ok;
    is_ok      = case_other_lines_are_unknown() && is_ok;
    is_ok      = case_overlong_is_unknown() && is_ok;
    is_ok      = case_next_line_starts_clean() && is_ok;
    return is_ok ? 0 : 1;
}
