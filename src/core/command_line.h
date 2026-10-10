#pragma once

// The observe firmware's serial input (09-guitar-observe), one character at a time. It accepts
// exactly one line, `analog`, optionally followed by '\r'; every other non-empty line is
// unknown. A line longer than the buffer is unknown as a whole, never cut and read as a shorter
// command.
//
// Pure: the caller reads the characters and acts on the result. The line is judged before the
// reader is cleared for the next one, so no result ever refers to a buffer already reset.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ps2 {

constexpr std::string_view kAnalogCommand = "analog";

// The longest accepted line, its '\r', and one more, so a longer line is still refused whole.
constexpr std::size_t kCommandBufferSize = kAnalogCommand.size() + 2;

enum class LineEvent : std::uint8_t {
    Pending,  // no '\n' yet, or an empty line: nothing to answer
    Analog,   // the line `analog`
    Unknown,  // any other non-empty line, an overlong one included
};

// The line read so far.
struct CommandLine {
    std::array<char, kCommandBufferSize> text        = {};
    std::size_t                          len         = 0;
    bool                                 is_overlong = false;
};

// Takes one received character. On '\n' it judges the line and clears `line` for the next one.
[[nodiscard]] LineEvent take_char( CommandLine& line, char c );

}  // namespace ps2
