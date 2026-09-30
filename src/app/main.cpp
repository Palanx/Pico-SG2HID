// Loopback firmware for phase 24-pio-bus. On one Pico with CMD jumpered to DATA and ATT to
// ACK, every frame should come back byte for byte; with the ACK jumper removed, every frame
// that waits for ACK should stop at its first byte with ATT released. Configures no pin itself:
// bus_init( ) in src/hal/ does, from src/core/pins.h (R-SAFETY-09, R-SAFETY-10).
#include "core/pins.h"
#include "hal/bus_frame.h"
#include "hal/bus_port.h"
#include "pico/stdlib.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

namespace {

constexpr std::uint32_t kPeriodMs   = 1000;
constexpr std::uint8_t  kProbeByte  = 0x01;
constexpr std::size_t   kLongestSeq = 9;

// Hand-written, in this order: a digital poll's master bytes; every bit pattern that would
// show a stuck or swapped line; one byte, which as the last byte waits for no ACK.
constexpr std::uint8_t kSeqPoll[]    = { 0x01, 0x42, 0x00, 0x00, 0x00 };
constexpr std::uint8_t kSeqPattern[] = { 0xFF, 0x00, 0xA5, 0x5A, 0x80, 0x01, 0xFE, 0x7F, 0x3C };
constexpr std::uint8_t kSeqSingle[]  = { 0x80 };

constexpr std::array<std::span<const std::uint8_t>, 3> kSequences = {
    kSeqPoll, kSeqPattern, kSeqSingle };

void print_probe() {
    const auto bits = ps2::probe_wire_bits( kProbeByte );
    if ( !bits ) {
        std::printf( "probe: timeout\n" );
        return;
    }
    std::printf( "probe: 0x%02X on the wire: ", kProbeByte );
    for ( std::size_t i = 0; i < bits->size(); ++i ) {
        std::printf( i == 0 ? "%d" : ",%d", ( *bits )[ i ] );
    }
    std::printf( "\n" );
}

void print_loopback( std::size_t index, std::span<const std::uint8_t> sent ) {
    std::array<std::uint8_t, kLongestSeq> frame{};
    std::copy( sent.begin(), sent.end(), frame.begin() );
    const std::uint32_t start = time_us_32();
    const std::size_t   count = ps2::exchange_frame( std::span( frame.data(), sent.size() ) );
    const std::uint32_t took  = time_us_32() - start;
    const bool          is_match =
        count == sent.size() && std::equal( sent.begin(), sent.end(), frame.begin() );
    const bool is_att_high = gpio_get( ps2::gpio_of( ps2::Signal::Att ) );
    std::printf( "loopback: seq=%zu bytes=%zu/%zu match=%s att=%s us=%lu\n",
                 index,
                 count,
                 sent.size(),
                 is_match ? "yes" : "no",
                 is_att_high ? "high" : "low",
                 static_cast<unsigned long>( took ) );
}

}  // namespace

int main() {
    stdio_init_all();
    ps2::bus_init();
    for ( ;; ) {
        print_probe();
        for ( std::size_t i = 0; i < kSequences.size(); ++i ) {
            print_loopback( i, kSequences[ i ] );
        }
        sleep_ms( kPeriodMs );
    }
}
