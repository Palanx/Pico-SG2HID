#include "core/poll.h"

#include <algorithm>

namespace ps2 {

DecodeOutcome decode_poll( std::span<const WireByte> wire, std::size_t completed ) {
    // An analog frame plus its address byte: the most any declared id announces.
    constexpr std::size_t kLongestPoll = frame_len( ControllerId::Analog ) + 1;

    const std::size_t n = std::min( { completed, wire.size(), kLongestPoll } );

    std::array<std::uint8_t, kLongestPoll - 1> bytes{};
    std::size_t                                count = 0;
    for ( std::size_t i = 1; i < n; ++i ) {
        bytes[ count++ ] = wire[ i ].in;
    }
    return decode( std::span<const std::uint8_t>( bytes.data(), count ) );
}

void count_poll( PollTally& tally, LinkState now, const DecodeOutcome& outcome ) {
    ++tally.polls;
    if ( now == LinkState::Absent ) {
        ++tally.refused;
    }
    if ( now != LinkState::AnalogStreaming || !outcome.has_value() ) {
        return;
    }
    if ( tally.has_payload && outcome->payload != tally.last_payload ) {
        ++tally.payload_changes;
    }
    tally.last_payload = outcome->payload;
    tally.has_payload  = true;
}

}  // namespace ps2
