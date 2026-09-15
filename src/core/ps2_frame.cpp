#include "core/ps2_frame.h"

namespace ps2 {

std::expected<Ps2Frame, DecodeStatus> decode( std::span<const std::uint8_t> bytes ) {
    // Nothing announced anything: there is no header to read, let alone a length to check.
    if ( bytes.size() < kPrefixLen ) {
        return std::unexpected( DecodeStatus::AckTimeout );
    }

    const auto id = id_from_byte( bytes[ kHeaderIndex ] );
    if ( !id.has_value() ) {
        return std::unexpected( DecodeStatus::UnknownId );
    }

    if ( bytes[ kReadyIndex ] != kReadyByte ) {
        return std::unexpected( DecodeStatus::NotReady );
    }

    // The length comes from the header, never from how many bytes happened to arrive. A
    // frame the bus cut short is refused here, which is the only place it can be: past this
    // point a partial payload would be indistinguishable from a complete one.
    const std::size_t expected_len = frame_len( *id );
    if ( bytes.size() < expected_len ) {
        return std::unexpected( DecodeStatus::AckTimeout );
    }

    // Built only once every refusal above has been passed, so no path can return a
    // half-filled frame (R-PROTO-02).
    Ps2Frame frame{ .id = *id, .payload = {} };

    const std::span<const std::uint8_t> arrived = bytes.subspan( kPayloadIndex );
    const std::size_t                   count   = payload_len( *id );

    // `count <= arrived.size()` is already guaranteed by the length refusal above, so this
    // second bound never fires in correct operation. It is written anyway, and not by accident:
    // it is what makes deleting that refusal produce a WRONG frame rather than a read past the
    // end of the buffer. R-PROTO-02's rejection case mutates exactly that refusal, and a
    // mutation whose observable effect is undefined behaviour proves nothing about the rule.
    const std::size_t copy = ( count < arrived.size() ) ? count : arrived.size();
    for ( std::size_t i = 0; i < copy; ++i ) {
        frame.payload[ i ] = arrived[ i ];
    }

    return frame;
}

}  // namespace ps2
