#pragma once

// One decoded PS2 poll frame, and the decoder that refuses to produce a broken one.
//
// `decode` is a pure function of bytes: no clock, no pin, no retained state (ADR-0002). It
// is the project's only trust boundary — everything the guitar says arrives through it — so
// it is written to refuse rather than to cope (R-PROTO-02, R-PROTO-03).

#include "core/ps2_protocol.h"

#include <array>
#include <cstdint>
#include <expected>
#include <span>

namespace ps2 {

// Why a frame was refused. There is no `Ok` member: success is `has_value()` on the
// std::expected, so an "ok status" cannot be stored, compared, or accidentally ignored
// (ADR-0009).
//
// ADR-0007 sketched this list as `Ok | AckTimeout | UnknownId | NotAnalog | Absent`.
// ADR-0012 supersedes that membership and records why each of the three is absent, along
// with the rule that decides where the next status goes: a status belongs here if and only
// if it is decidable from the bytes of a single frame, with no history and no knowledge of
// what was asked. Anything needing history is a FaultCause; anything describing what the
// controller is to us is a LinkState.
enum class DecodeStatus : std::uint8_t {
    // The frame stopped before the header announced it would. On the real bus the cause is
    // an `ACK` that never came, which is why the name is the bus event and not "TooShort":
    // trace mode has to print why the link dropped, and "the bytes ran out" is not a cause
    // (R-PROTO-02).
    AckTimeout,

    // The header byte is not one of the declared controller ids (R-PROTO-03).
    UnknownId,

    // The header was a known id but the byte after it was not kReadyByte. Something is
    // talking on the bus and it is not answering the protocol.
    NotReady,
};

// Every declared id announces at most this many payload bytes. Derived from the table rather
// than written down, so adding an id with a longer payload cannot leave this behind.
constexpr std::size_t kMaxPayloadLen = payload_len( ControllerId::Analog );

// A frame that decoded. Plain data, comparable as a whole, and carrying no length of its
// own: how many payload bytes are meaningful is `payload_len( id )`, so the two can never
// disagree. Bytes past that point are zero, not whatever the buffer held.
struct Ps2Frame {
    ControllerId                             id;
    std::array<std::uint8_t, kMaxPayloadLen> payload;
};

// Decode one controller response: header byte, ready byte, then the payload the header
// announced. `bytes` is what `hal` shifted in, minus the leading byte that answers the
// address byte and carries nothing (see ps2_protocol.h).
//
// On any refusal the error is the reason and there is no frame at all — not a frame with
// some fields filled in (R-PROTO-02).
[[nodiscard]] std::expected<Ps2Frame, DecodeStatus> decode( std::span<const std::uint8_t> bytes );

// What one poll produced: a frame, or the reason there is none. Named because the link state
// machine takes it as a parameter (ADR-0011) and "std::expected<Ps2Frame, DecodeStatus>"
// spelled out at every such signature says less than the name does.
using DecodeOutcome = std::expected<Ps2Frame, DecodeStatus>;

}  // namespace ps2
