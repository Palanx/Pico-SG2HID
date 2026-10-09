#include "core/negotiation.h"

#include "core/poll.h"
#include "core/ps2_protocol.h"

#include <array>
#include <optional>

namespace ps2 {

namespace {

// The enter frame is a digital poll's length: the shortest answer any declared id gives, so it
// reaches a controller in whatever mode it is in. Every later frame is as long as an analog one.
constexpr std::size_t kEnterConfigLen = frame_len( ControllerId::Digital ) + 1;
constexpr std::size_t kLongFrameLen   = frame_len( ControllerId::Analog ) + 1;

// Frame positions shifted by one: wire byte 0 answers the address byte and carries nothing.
constexpr std::size_t kWireHeaderIndex = kHeaderIndex + 1;
constexpr std::size_t kWireReadyIndex  = kReadyIndex + 1;
constexpr std::size_t kWirePrefixLen   = kPrefixLen + 1;

constexpr std::array<std::uint8_t, kEnterConfigLen> kEnterConfig = {
    kFrameStart, kCmdConfig, kPadByte, kConfigEnter, kPadByte };

constexpr std::array<std::uint8_t, kLongFrameLen> kSetAnalog = { kFrameStart,
                                                                 kCmdSetMode,
                                                                 kPadByte,
                                                                 kModeAnalog,
                                                                 kModeLocked,
                                                                 kPadByte,
                                                                 kPadByte,
                                                                 kPadByte,
                                                                 kPadByte };

constexpr std::array<std::uint8_t, kLongFrameLen> kLeaveConfig = { kFrameStart,
                                                                   kCmdConfig,
                                                                   kPadByte,
                                                                   kConfigLeave,
                                                                   kPadByte,
                                                                   kPadByte,
                                                                   kPadByte,
                                                                   kPadByte,
                                                                   kPadByte };

constexpr std::array<std::uint8_t, kLongFrameLen> kAnalogPoll = {
    kFrameStart, kCmdPoll, kPadByte, kPadByte, kPadByte, kPadByte, kPadByte, kPadByte, kPadByte };

// The id the answer's prefix carries, or nothing when the prefix does not hold.
[[nodiscard]] std::optional<ControllerId> prefix_id( const Exchanged& exchanged ) {
    if ( exchanged.completed < kWirePrefixLen || exchanged.wire.size() < kWirePrefixLen ||
         exchanged.wire[ kWireReadyIndex ].in != kReadyByte ) {
        return std::nullopt;
    }
    return id_from_byte( exchanged.wire[ kWireHeaderIndex ].in );
}

// The id a stage after EnterConfig expects: config mode until it is left, then analog.
[[nodiscard]] ControllerId expected_id( NegotiationStage stage ) {
    return stage == NegotiationStage::Polling ? ControllerId::Analog : ControllerId::Config;
}

[[nodiscard]] NegotiationStage next_stage( NegotiationStage stage ) {
    switch ( stage ) {
    case NegotiationStage::EnterConfig:
        return NegotiationStage::SetAnalog;
    case NegotiationStage::SetAnalog:
        return NegotiationStage::LeaveConfig;
    case NegotiationStage::LeaveConfig:
    case NegotiationStage::Polling:
        return NegotiationStage::Polling;
    }
    return NegotiationStage::EnterConfig;
}

}  // namespace

std::span<const std::uint8_t> command_for( NegotiationStage stage ) {
    switch ( stage ) {
    case NegotiationStage::EnterConfig:
        return kEnterConfig;
    case NegotiationStage::SetAnalog:
        return kSetAnalog;
    case NegotiationStage::LeaveConfig:
        return kLeaveConfig;
    case NegotiationStage::Polling:
        return kAnalogPoll;
    }
    return kAnalogPoll;
}

LinkState advance( Master& master, const Exchanged& exchanged, std::uint32_t elapsed_us ) {
    const std::optional<ControllerId> id = prefix_id( exchanged );

    if ( master.stage == NegotiationStage::EnterConfig ) {
        if ( exchanged.completed >= kEnterConfigLen && id.has_value() ) {
            master.stage = next_stage( master.stage );
            return master.link.state;
        }
        return step( master.link, decode_poll( exchanged.wire, exchanged.completed ), elapsed_us );
    }

    if ( id.has_value() && *id != expected_id( master.stage ) ) {
        master.link = Link{
            .state = LinkState::Absent, .last_fault = FaultCause::Declined, .us_in_state = 0 };
        master.stage = NegotiationStage::EnterConfig;
        return master.link.state;
    }

    const LinkState now =
        step( master.link, decode_poll( exchanged.wire, exchanged.completed ), elapsed_us );
    master.stage =
        now == LinkState::Absent ? NegotiationStage::EnterConfig : next_stage( master.stage );
    return now;
}

}  // namespace ps2
