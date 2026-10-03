#pragma once

// What the emulated SG answers on the bus, one byte at a time, and the line commands that change
// it. Pure logic: no SDK header, no clock, no pin. src/emu/main.cpp feeds it the bytes the PIO
// program shifted in and waits the delay it returns; tests/emulator_cases.cpp feeds it hand-
// written vectors (R-EMU-01, R-EMU-02).
//
// One poll frame, wire byte i, from the controller's side:
//
//     i       0      1     2      3 … frame_len( id )
//     sends   0xFF   id    0x5A   payload bytes
//     ACKs    when the master sent 0x01 at 0 and 0x42 at 1, then every byte but the last
//
// The answer to byte i is decided when byte i-1 arrives, because the PIO program shifts it out
// while the master's byte i is still coming in.

#include "core/ps2_protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>

namespace ps2 {

// How long after a byte's last rising CLK edge the ACK pulse starts, and how long it lasts.
// belay-debt: budgets, not measurements; 09-guitar-observe measures the real SG and replaces
// both. The pulse is kept short because the master never waits for ACK to rise: a pulse longer
// than the master's turnaround would overlap the next byte's first falling CLK edge.
constexpr std::uint32_t kAckDelayUs = 10;
constexpr std::uint32_t kAckPulseUs = 2;

// The longest command line accepted, terminator excluded.
constexpr std::size_t kMaxLineLen = 64;

// Payload bytes the model keeps: an analog frame's six. Digital sends the first two.
constexpr std::size_t kMaxPayload = 6;

// The first wire byte, before anything is known: also what a byte past the frame's end gets.
constexpr std::uint8_t kIdleByte = 0xFF;

// The default payload: every button released (active low), every axis centred.
constexpr std::uint8_t kButtonsReleased = 0xFF;
constexpr std::uint8_t kAxisCentred     = 0x80;

// What to do after the master's byte at the current wire index.
struct ByteAnswer {
    std::uint8_t  next;          // the byte to shift out for the next wire index
    bool          should_ack;    // pulse ACK for the byte just received
    std::uint32_t ack_delay_us;  // from the byte's last rising edge to the pulse
};

using Payload = std::array<std::uint8_t, kMaxPayload>;

enum class FaultKind : std::uint8_t { None, Ack, Late, Id };

struct Fault {
    FaultKind     kind;
    std::uint32_t value;  // the wire byte for Ack, µs for Late, the id byte for Id
};

class SgModel {
public:
    // The start of a frame: ATT fell. Wire byte 0's answer is kIdleByte.
    void reset();

    // The master sent `received` at the current wire index; advances to the next.
    [[nodiscard]] ByteAnswer step( std::uint8_t received );

    // One command line, without its terminator (a trailing '\r' is ignored). A refused line
    // changes nothing; the error is a fixed reason to print after `error: `.
    [[nodiscard]] std::expected<void, std::string_view> apply( std::string_view line );

private:
    [[nodiscard]] std::uint8_t byte_at( std::size_t index ) const;
    [[nodiscard]] std::size_t  last_index() const;

    bool        m_is_analog = false;
    Payload     m_payload   = { kButtonsReleased,
                                kButtonsReleased,
                                kAxisCentred,
                                kAxisCentred,
                                kAxisCentred,
                                kAxisCentred };
    Fault       m_fault     = { .kind = FaultKind::None, .value = 0 };
    std::size_t m_index     = 0;
    bool        m_is_broken = false;
};

}  // namespace ps2
