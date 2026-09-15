# Hand-written protocol vectors

Every file here is a `constexpr std::uint8_t` array with one comment per byte, written by
hand from the PS2 controller protocol documentation (**R-PROTO-05**). Nothing here includes
`src/core/`, and no value here is produced by `core` or captured from the emulator — if it
were, a shared misreading of the protocol would be invisible, because the emulator shares
`core`.

Bytes are the controller's response with its first byte dropped: `[header][0x5A][payload…]`.
See `src/core/ps2_protocol.h` for why the dropped byte carries nothing.

Headers rather than hex text on purpose: a parser in the test is code that can be wrong
about the literal it reads, and then the vector is no longer the literal. `#embed` was
considered and rejected in the phase spec — it embeds a file verbatim, so the vectors would
have to become unreadable binary to benefit.

Digital buttons are **active low**: a bit is `1` when the button is released. A frame with
nothing pressed is therefore `0xFF 0xFF`, not `0x00 0x00`.
