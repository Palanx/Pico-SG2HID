#pragma once

// The HID report descriptor the master sends the host (08-usb-hid, R-HID-01): ten buttons from
// bit 0, padding to the byte, then the whammy as Generic Desktop X, 0..255. Written by hand from
// the HID 1.11 item encoding (§6.2.2): a prefix byte naming the item and its data size, then
// the data, little endian.
//
// R-PROTO-05: hand-written literal.

#include <cstdint>

namespace vectors {

constexpr std::uint8_t kHidDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x0A,        //   Usage Maximum (10)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x0A,        //   Report Count (10) — the buttons, bits 0..9
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x06,        //   Report Count (6) — padding, bits 10..15
    0x81, 0x03,        //   Input (Constant, Variable, Absolute)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X) — the whammy
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255) — two data bytes, so it is not read as -1
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x01,        //   Report Count (1) — byte 2 of the report
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0xC0,              // End Collection
};

}  // namespace vectors
