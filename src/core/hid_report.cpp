#include "core/hid_report.h"

namespace ps2 {

namespace {

// Set one HID button bit. Active high, so this only ever sets — the report starts zeroed and
// a released button is the absence of a bit, not a cleared one.
void set_button( HidReport& report, Button button, bool is_pressed ) {
    if ( !is_pressed ) {
        return;
    }
    const auto index = static_cast<std::size_t>( button );
    report[ index / kBitsPerByte ] |= static_cast<std::uint8_t>( 1U << ( index % kBitsPerByte ) );
}

[[nodiscard]] bool fret_of( const GuitarState& state, Fret fret ) {
    return state.is_fret_pressed[ static_cast<std::size_t>( fret ) ];
}

}  // namespace

HidReport build_report( const GuitarState& state ) {
    HidReport report{};

    set_button( report, Button::FretGreen, fret_of( state, Fret::Green ) );
    set_button( report, Button::FretRed, fret_of( state, Fret::Red ) );
    set_button( report, Button::FretYellow, fret_of( state, Fret::Yellow ) );
    set_button( report, Button::FretBlue, fret_of( state, Fret::Blue ) );
    set_button( report, Button::FretOrange, fret_of( state, Fret::Orange ) );

    set_button( report, Button::StrumUp, state.is_strum_up );
    set_button( report, Button::StrumDown, state.is_strum_down );
    set_button( report, Button::Start, state.is_start );
    set_button( report, Button::Select, state.is_select );
    set_button( report, Button::Tilt, state.is_tilt );

    // Straight through: the axis is one byte on the bus and one byte in the report, and
    // rescaling it here would be a second place the rest value is decided.
    report[ kWhammyOffset ] = state.whammy;

    return report;
}

}  // namespace ps2
