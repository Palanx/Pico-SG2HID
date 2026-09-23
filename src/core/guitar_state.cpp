#include "core/guitar_state.h"

namespace ps2 {

namespace {

// Digital buttons are active low: pressed means the bit is CLEAR. Wrapping that in a named
// function is not ceremony — the bare `( byte & mask )` reads as "pressed" to everyone who
// has not just read the protocol, and it is the wrong answer.
[[nodiscard]] bool is_pressed( std::uint8_t buttons, std::uint8_t mask ) {
    return ( buttons & mask ) == 0;
}

// Does a frame carrying this id report controls at all? A closed positive set, so an id
// added to the table later reports nothing until someone says otherwise — the same
// refusing-by-default posture as id_from_byte.
//
// Config is the id that answers no today: its payload is padding, and because digital
// buttons are active low, reading it would report every control pressed — the worst answer
// from the most innocent-looking frame. The gate lives here and not in map_frame's caller
// on purpose: a core that is pure by construction cannot depend on every future caller
// checking LinkState first (ADR-0002, R-ARCH-01).
[[nodiscard]] bool reports_controls( ControllerId id ) {
    return id == ControllerId::Digital || id == ControllerId::Analog;
}

}  // namespace

GuitarState map_frame( const Ps2Frame& frame ) {
    GuitarState state{};

    if ( !reports_controls( frame.id ) ) {
        state.whammy = kWhammyRest;
        return state;
    }

    const std::uint8_t low  = frame.payload[ kButtonsLowIndex ];
    const std::uint8_t high = frame.payload[ kButtonsHighIndex ];

    state.is_fret_pressed[ static_cast<std::size_t>( Fret::Green ) ] =
        is_pressed( high, kMaskFretGreen );
    state.is_fret_pressed[ static_cast<std::size_t>( Fret::Red ) ] =
        is_pressed( high, kMaskFretRed );
    state.is_fret_pressed[ static_cast<std::size_t>( Fret::Yellow ) ] =
        is_pressed( high, kMaskFretYellow );
    state.is_fret_pressed[ static_cast<std::size_t>( Fret::Blue ) ] =
        is_pressed( high, kMaskFretBlue );
    state.is_fret_pressed[ static_cast<std::size_t>( Fret::Orange ) ] =
        is_pressed( high, kMaskFretOrange );

    state.is_strum_up   = is_pressed( low, kMaskStrumUp );
    state.is_strum_down = is_pressed( low, kMaskStrumDown );
    state.is_start      = is_pressed( low, kMaskStart );
    state.is_select     = is_pressed( low, kMaskSelect );
    state.is_tilt       = is_pressed( high, kMaskTilt );

    // The one branch R-PROTO-04 is about. The rest value is written by us; no digital payload
    // byte is ever reinterpreted as an axis, and there is no index arithmetic on this path
    // that could reach one by accident.
    if ( frame.id == ControllerId::Analog ) {
        state.whammy = frame.payload[ kWhammyIndex ];
    } else {
        state.whammy = kWhammyRest;
    }

    return state;
}

}  // namespace ps2
