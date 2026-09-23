#pragma once

// A known controller id whose ready byte is not kReadyByte (0x5A).
//
// Every other byte of this frame is well-formed: the header is the real digital id, the
// length is exactly what that header announces, and both payload bytes are legitimate. The
// ready byte is the only thing wrong with it, so nothing else can be the reason it is
// refused — the same construction unknown_id.h uses.
//
// 0xFF is not an arbitrary wrong byte. `DATA` is open-drain with a pull-up, so 0xFF is what
// the master shifts in when the controller is not driving the line at all: the controller
// answered the header and then stopped talking, while the master kept clocking. That is a
// different failure from truncated_ack.h, where the bytes stop arriving altogether — here
// the bytes arrive and are the idle level. A decoder that checked only the length would
// accept this frame and hand on two payload bytes that are the bus at rest.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kNotReady[] = {
    0x41,  // header — digital; a declared id, so the refusal cannot be UnknownId
    0xFF,  // ready byte — should be 0x5A; this is the idle level of an undriven DATA line
    0xFF,  // payload 0 — legitimate; announced by the header and present
    0xFF,  // payload 1 — likewise, so the refusal cannot be AckTimeout either
};

}  // namespace vectors
