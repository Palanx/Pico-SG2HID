#include "core/trace_filter.h"

namespace ps2 {

namespace {

[[nodiscard]] bool is_out_changed( const TraceFilter& filter, const TracedFrame& frame ) {
    if ( frame.wire.size() != filter.len ) {
        return true;
    }
    for ( std::size_t i = 0; i < filter.len; ++i ) {
        if ( frame.wire[ i ].out != filter.out[ i ] ) {
            return true;
        }
    }
    return false;
}

// Called only once `out` matched, so `frame.wire` is `filter.len` bytes long.
[[nodiscard]] bool is_in_changed( const TraceFilter& filter, const TracedFrame& frame ) {
    if ( frame.completed != filter.completed ) {
        return true;
    }
    for ( std::size_t i = 0; i < filter.completed && i < filter.len; ++i ) {
        if ( frame.wire[ i ].in != filter.in[ i ] ) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool is_trace_due( const TraceFilter& filter, const TracedFrame& frame, std::uint32_t now_us ) {
    if ( !filter.has_printed || frame.wire.size() > kMaxTracedLen ) {
        return true;
    }
    if ( is_out_changed( filter, frame ) || frame.state != filter.state ) {
        return true;
    }
    return is_in_changed( filter, frame ) && now_us - filter.printed_at_us >= kTraceGapUs;
}

void note_printed( TraceFilter& filter, const TracedFrame& frame, std::uint32_t now_us ) {
    filter               = TraceFilter{};
    filter.len           = frame.wire.size() < kMaxTracedLen ? frame.wire.size() : kMaxTracedLen;
    filter.completed     = frame.completed;
    filter.state         = frame.state;
    filter.printed_at_us = now_us;
    filter.has_printed   = true;
    for ( std::size_t i = 0; i < filter.len; ++i ) {
        filter.out[ i ] = frame.wire[ i ].out;
        if ( i < frame.completed ) {
            filter.in[ i ] = frame.wire[ i ].in;
        }
    }
}

}  // namespace ps2
