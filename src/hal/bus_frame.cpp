#include "hal/bus_frame.h"

#include "hal/bus_port.h"

#include <optional>

namespace ps2 {

// One exit, so one release: a `return` inside the loop is the bug R-SAFETY-07 exists for.
std::size_t exchange_frame( std::span<std::uint8_t> frame ) {
    att_assert();
    std::size_t done = 0;
    while ( done < frame.size() ) {
        const bool                        should_wait_ack = done + 1 < frame.size();
        const std::optional<std::uint8_t> in = exchange_byte( frame[ done ], should_wait_ack );
        if ( !in ) {
            break;
        }
        frame[ done ] = *in;
        ++done;
    }
    att_release();
    return done;
}

}  // namespace ps2
