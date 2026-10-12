// The firmware's bus_wait_us( ) (ADR-0020). Its own file so src/hal/pio_port.cpp, whose timed
// byte window the trace's calibration depends on (G11), stays unchanged.

#include "hal/bus_port.h"
#include "pico/time.h"

namespace ps2 {

void bus_wait_us( std::uint32_t us ) {
    busy_wait_us_32( us );
}

}  // namespace ps2
