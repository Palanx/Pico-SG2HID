#pragma once

// Wire-level constants of the PlayStation 2 controller bus, and the controller-id table.
//
// This header is the one place a byte the protocol fixes is written down (R-CLEAN-04). It
// knows nothing about frames, links or reports: those layers include this one, never the
// reverse. Nothing here reads a clock or touches a pin (ADR-0002, R-ARCH-01).
//
// Shape of one poll exchange, master on top, controller below:
//
//     master      0x01  0x42  0x00  0x00  0x00  ...
//     controller  0xFF  id    0x5A  d0    d1    ...
//
// The controller's first byte answers the address byte and carries nothing, so `hal` drops
// it: what reaches `decode` starts at the id. That is why kHeaderIndex is 0 and not 1.

#include <cstddef>
#include <cstdint>
#include <optional>

namespace ps2 {

// --- Bytes the master sends -------------------------------------------------------------

constexpr std::uint8_t kFrameStart = 0x01;  // addresses the controller; opens every frame
constexpr std::uint8_t kCmdPoll    = 0x42;  // read buttons and axes
constexpr std::uint8_t kCmdConfig  = 0x43;  // enter config mode with 0x01, leave it with 0x00
constexpr std::uint8_t kCmdSetMode = 0x44;  // config mode only: choose digital or analog
constexpr std::uint8_t kPadByte    = 0x00;  // filler in a slot whose value the controller ignores

// Arguments to kCmdConfig and kCmdSetMode. Declared here, driven in 07-analog-mode.
constexpr std::uint8_t kConfigEnter = 0x01;
constexpr std::uint8_t kConfigLeave = 0x00;
constexpr std::uint8_t kModeDigital = 0x00;
constexpr std::uint8_t kModeAnalog  = 0x01;
constexpr std::uint8_t kModeLocked  = 0x03;  // keeps the mode across an ANALOG-button press

// --- Bytes the controller sends ---------------------------------------------------------

// The second response byte of every well-formed frame. A controller that is powered but not
// ready sends something else, and that is a refusal, not a frame.
constexpr std::uint8_t kReadyByte = 0x5A;

// Controller id bytes, in the header position of a response.
//
// TODO(09-guitar-observe): read from the protocol documentation, not measured. Which of
// these the SG actually answers with — and whether it answers a fourth — is what
// 09-guitar-observe establishes against the real device.
constexpr std::uint8_t kIdDigital = 0x41;
constexpr std::uint8_t kIdAnalog  = 0x73;
constexpr std::uint8_t kIdConfig  = 0xF3;

// A closed set on purpose: an id outside it is refused, never decoded best-effort
// (R-PROTO-03). The enumerator values are the wire bytes, so a ControllerId can be turned
// back into the byte it came from — which is what payload_len needs.
enum class ControllerId : std::uint8_t {
    Digital = kIdDigital,
    Analog  = kIdAnalog,
    Config  = kIdConfig,
};

// --- Frame geometry ---------------------------------------------------------------------

constexpr std::size_t kHeaderIndex  = 0;
constexpr std::size_t kReadyIndex   = 1;
constexpr std::size_t kPayloadIndex = 2;

// Header and ready byte. A frame shorter than this is cut short before the ready slot: its
// header may well be readable, and `decode` reports the abort anyway (R-PROTO-02).
constexpr std::size_t kPrefixLen = kPayloadIndex;

// The header's low nibble announces how many payload bytes follow, in pairs.
constexpr std::uint8_t kPayloadNibbleMask = 0x0F;
constexpr std::size_t  kBytesPerNibble    = 2;

// --- Lookups ----------------------------------------------------------------------------

// std::optional rather than std::expected, which ADR-0009 chose for decoding: there is
// exactly one way this fails and naming it needs no second type, and DecodeStatus belongs
// to the frame layer, which includes this header rather than being included by it.
[[nodiscard]] constexpr std::optional<ControllerId> id_from_byte( std::uint8_t header ) {
    switch ( header ) {
    case kIdDigital:
        return ControllerId::Digital;
    case kIdAnalog:
        return ControllerId::Analog;
    case kIdConfig:
        return ControllerId::Config;
    default:
        return std::nullopt;
    }
}

// Payload bytes this id announces: 2 for Digital, 6 for Analog and Config. Takes a
// ControllerId and not a raw byte, so the length of a header nobody validated cannot be
// asked for.
[[nodiscard]] constexpr std::size_t payload_len( ControllerId id ) {
    const auto header = static_cast<std::uint8_t>( id );
    return static_cast<std::size_t>( header & kPayloadNibbleMask ) * kBytesPerNibble;
}

// Total length of a well-formed frame carrying this id.
[[nodiscard]] constexpr std::size_t frame_len( ControllerId id ) {
    return kPrefixLen + payload_len( id );
}

}  // namespace ps2
