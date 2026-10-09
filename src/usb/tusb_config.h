#pragma once

// TinyUSB's build configuration for the master (08-usb-hid, ADR-0018): device mode on the
// RP2040's one port, one CDC interface (the serial port stdio writes to) and one HID interface
// (the gamepad). TinyUSB reads these as preprocessor macros, from C sources too, so they cannot
// be constexpr. Found ahead of the SDK's own tusb_config.h because src/usb is a plain include
// directory and the SDK's is a system one.

#define CFG_TUSB_RHPORT0_MODE ( OPT_MODE_DEVICE )

#define CFG_TUD_CDC    ( 1 )
#define CFG_TUD_HID    ( 1 )
#define CFG_TUD_MSC    ( 0 )
#define CFG_TUD_MIDI   ( 0 )
#define CFG_TUD_VENDOR ( 0 )

// The SDK's sizes for its own CDC interface, kept so the serial port behaves as before.
#define CFG_TUD_CDC_RX_BUFSIZE ( 64 )
#define CFG_TUD_CDC_TX_BUFSIZE ( 64 )
#define CFG_TUD_CDC_EP_BUFSIZE ( 64 )

// At least ps2::kReportLen; usb_device.cpp asserts it.
#define CFG_TUD_HID_EP_BUFSIZE ( 8 )
