#pragma once

// The serial-port lines both master firmwares print, `sg2hid` (main.cpp) and `sg2hid_observe`
// (observe.cpp): a `link:` line on a link-state change and a frame's `T1` trace line (ADR-0015).
// tools/hil_digital.py and tools/trace_decode.py read them, so the two firmwares share one copy.

#include "core/bus_trace.h"
#include "core/link.h"

#include <cstddef>
#include <span>
#include <string_view>

namespace console {

// The names the `link:` and `hil:` lines use for a link state and a fault.
[[nodiscard]] std::string_view state_name( ps2::LinkState state );
[[nodiscard]] std::string_view fault_name( ps2::FaultCause fault );

// Prints `frame`'s `T1` line, of which the first `completed` bytes completed.
void print_trace( std::span<const ps2::WireByte> frame, std::size_t completed );

// Prints `link: <from> -> <to> fault=<fault>`.
void print_link( ps2::LinkState from, ps2::LinkState to, ps2::FaultCause fault );

}  // namespace console
