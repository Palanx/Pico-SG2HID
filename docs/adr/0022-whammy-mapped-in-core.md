# ADR-0022: The codec maps the SG's whammy byte to a 0-at-rest axis

- Status: accepted
- Date: 2026-10-12
- Narrows: ADR-0018 ("the whammy, passed straight through"; the neutral report's whammy at 0x80)

## Context

`09-guitar-observe` captured the real SG (`docs/phases/09-guitar-observe/captures/sg-analog.log`):
the whammy is payload byte 5 (`kWhammyIndex`, as assumed), but it rests at `0x7F`, not `0x80`, and
falls to `0x00` at the bar's stop. Passed straight through, as ADR-0018 has it, the host would see
the axis a step off its midpoint at rest, moving down to 0 when pushed, over half its range. That
was G7's stop condition, and the operator chose to map it in `core` (2026-10-12).

## Decision

`map_frame( )` reports the whammy as `( kWhammyAxisRest - byte ) * kWhammyAxisScale`, with
`kWhammyAxisRest = 0x7F` and `kWhammyAxisScale = 2`; a byte at or above `0x7F` reports rest.
So the host sees `0x00` with the bar untouched and `0xFE` at the stop. `kWhammyRest`, the value
the codec reports at rest and in the neutral report (R-HID-02), becomes `0x00`. The HID
descriptor is unchanged: Generic Desktop X, 0..255.

## Consequences

- **Easier.** The axis has the shape a PC guitar's whammy has: rest at one end, full at the
  other, the whole range used. The neutral report and the bar at rest are the same value.
- **Harder.** The reported value is no longer a wire byte, so a test cannot read it from a vector
  (R-PROTO-05). The full-scale value is hand-computed in `tests/ps2_codec_cases.cpp`
  (`kWhammyReportedFull`), as `kZeroFill` is; `tools/hil_digital.py` hand-writes its sweep pairs.
- **Harder.** The mapping is the SG's. A controller whose bar rests elsewhere needs its own
  constants; nothing here detects one.
- **Rejected: pass it raw and let the game calibrate.** Workable in Clone Hero, but the host would
  see a half-range axis resting off-centre, and every host-side tool would need the same fix.
- **Rejected: a rest-and-full calibration at power-up.** Nothing measured asks for one; the SG's
  rest byte was steady across every capture.
