#pragma once

// One PS2 frame on the bus: ATT asserted, every byte exchanged, ATT released.

#include "core/bus_trace.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ps2 {

// Exchanges `frame` byte by byte: byte i sends `frame[ i ].out`. Every attempted byte gets its
// `elapsed_us`, the failed one included; a completed byte also gets the byte shifted in, in
// `in`. A byte never sent is left as it was. Every byte but the last waits for ACK (R-PROTO-06).
// Stops at the first byte that did not complete, and releases ATT on every path (R-SAFETY-07).
// Returns how many bytes completed: `frame.size()` when the whole frame did. A short count is not
// an error here; the link layer decides what it means (ADR-0011).
[[nodiscard]] std::size_t exchange_frame( std::span<WireByte> frame );

}  // namespace ps2
