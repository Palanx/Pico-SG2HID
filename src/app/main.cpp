// Firmware entry point. Configures no GPIO: every pin stays an input, as at reset.
#include "pico/stdlib.h"

#include <cstdio>

namespace {

constexpr uint32_t kBannerPeriodMs = 1000;

}  // namespace

int main() {
    stdio_init_all();
    for ( ;; ) {
        std::printf( "pico-sg2hid: firmware build alive\n" );
        sleep_ms( kBannerPeriodMs );
    }
}
