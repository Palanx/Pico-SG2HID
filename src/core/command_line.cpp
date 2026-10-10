#include "core/command_line.h"

namespace ps2 {

namespace {

[[nodiscard]] LineEvent judge( const CommandLine& line ) {
    std::string_view text( line.text.data(), line.len );
    if ( !text.empty() && text.back() == '\r' ) {
        text.remove_suffix( 1 );
    }
    if ( line.is_overlong ) {
        return LineEvent::Unknown;
    }
    if ( text.empty() ) {
        return LineEvent::Pending;
    }
    return text == kAnalogCommand ? LineEvent::Analog : LineEvent::Unknown;
}

}  // namespace

LineEvent take_char( CommandLine& line, char c ) {
    if ( c != '\n' ) {
        if ( line.len < line.text.size() ) {
            line.text[ line.len++ ] = c;
        } else {
            line.is_overlong = true;
        }
        return LineEvent::Pending;
    }
    // Judged first, cleared after: the 09-guitar-observe bug cleared the buffer under a view of
    // it, so every line compared as NUL bytes and `analog` was always refused.
    const LineEvent event = judge( line );
    line                  = CommandLine{};
    return event;
}

}  // namespace ps2
