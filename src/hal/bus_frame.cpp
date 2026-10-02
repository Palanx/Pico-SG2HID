#include "hal/bus_frame.h"

#include "hal/bus_port.h"

namespace ps2 {

// One exit, so one release: a `return` inside the loop is the bug R-SAFETY-07 exists for.
std::size_t exchange_frame( std::span<WireByte> frame ) {
    att_assert();
    std::size_t done = 0;
    while ( done < frame.size() ) {
        const bool         should_wait_ack = done + 1 < frame.size();
        const ByteExchange got             = exchange_byte( frame[ done ].out, should_wait_ack );
        frame[ done ].elapsed_us           = got.elapsed_us;
        if ( !got.in ) {
            break;
        }
        frame[ done ].in = *got.in;
        ++done;
    }
    att_release();
    return done;
}

}  // namespace ps2
