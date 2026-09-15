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

    // The length comes from the header, never from how many bytes happened to arrive, and it
    // is checked HERE — the first point at which it is knowable, immediately after the id
    // that announces it. Ordering, not taste: R-PROTO-02 says a frame the bus cut short
    // "reports the abort", so the abort has to win over every refusal that could still be
    // evaluated afterwards. Checking the ready byte first would report NotReady for a frame
    // that is both short and carrying 0xFF at the ready slot — and 0xFF there is an undriven
    // line, not a controller saying it is not ready. The vector covering that overlap lives
    // with the other refusal vectors; R-PROTO-05 is why it is not named here.
    //
    // The id check above cannot move below this one: frame_len needs the id.
    const std::size_t expected_len = frame_len( *id );
    if ( bytes.size() < expected_len ) {
        return std::unexpected( DecodeStatus::AckTimeout );
    }

    // Reached only by a frame of the announced length, so a wrong ready byte here is the
    // controller's own statement and not a side effect of the bytes stopping.
    if ( bytes[ kReadyIndex ] != kReadyByte ) {
        return std::unexpected( DecodeStatus::NotReady );
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
