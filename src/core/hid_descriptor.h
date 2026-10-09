#pragma once

// The HID report descriptor: the bytes that tell the host how to read hid_report.h's report
// (08-usb-hid, ADR-0018). Built from that file's constants, so the descriptor and the builder
// cannot decide the layout twice. Pure data: src/usb/ hands it to TinyUSB, and the codec suite
// compares it with a hand-written vector (R-HID-01).
//
// Every item is a prefix byte followed by its data, little endian (HID 1.11 §6.2.2.2). The
// prefix encodes the item's tag, type and data size: 0x25 is Logical Maximum with one data
// byte, 0x26 the same item with two.

#include "core/hid_report.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ps2 {

// --- Item prefixes ----------------------------------------------------------------------

constexpr std::uint8_t kItemUsagePage       = 0x05;
constexpr std::uint8_t kItemUsage           = 0x09;
constexpr std::uint8_t kItemCollection      = 0xA1;
constexpr std::uint8_t kItemEndCollection   = 0xC0;
constexpr std::uint8_t kItemUsageMinimum    = 0x19;
constexpr std::uint8_t kItemUsageMaximum    = 0x29;
constexpr std::uint8_t kItemLogicalMinimum  = 0x15;
constexpr std::uint8_t kItemLogicalMaximum  = 0x25;  // one data byte, signed: at most 127
constexpr std::uint8_t kItemLogicalMaximum2 = 0x26;  // two data bytes, so 255 stays positive
constexpr std::uint8_t kItemReportSize      = 0x75;
constexpr std::uint8_t kItemReportCount     = 0x95;
constexpr std::uint8_t kItemInput           = 0x81;

// --- Item data --------------------------------------------------------------------------

constexpr std::uint8_t kUsagePageGenericDesktop = 0x01;
constexpr std::uint8_t kUsagePageButton         = 0x09;
constexpr std::uint8_t kUsageGamePad            = 0x05;
constexpr std::uint8_t kUsageX                  = 0x30;
constexpr std::uint8_t kCollectionApplication   = 0x01;
constexpr std::uint8_t kInputDataVarAbs         = 0x02;
constexpr std::uint8_t kInputConstVarAbs        = 0x03;
constexpr std::uint8_t kFirstButtonUsage        = 0x01;  // HID buttons count from 1
constexpr std::uint8_t kBitField                = 1;
constexpr std::uint8_t kAxisBits                = 8;
constexpr std::uint8_t kAxisMaximum             = 0xFF;

// The bits after the last button, up to the byte boundary.
constexpr std::size_t kPaddingBits = kButtonBytes * kBitsPerByte - kButtonCount;

// A Report Count of 0 is not "no padding", it is a malformed main item. A button count that
// fills its bytes exactly needs the padding items removed instead.
static_assert( kPaddingBits != 0, "remove the padding items: the buttons fill their bytes" );
static_assert( kButtonCount <= std::numeric_limits<std::uint8_t>::max(),
               "Usage Maximum and Report Count take one data byte" );

// One item per line: clang-format would otherwise split each prefix from its data.
// clang-format off
constexpr std::array<std::uint8_t, 44> kHidReportDescriptor = {
    kItemUsagePage, kUsagePageGenericDesktop,
    kItemUsage, kUsageGamePad,
    kItemCollection, kCollectionApplication,

    // The ten buttons, bit 0 first, active high (hid_report.h).
    kItemUsagePage, kUsagePageButton,
    kItemUsageMinimum, kFirstButtonUsage,
    kItemUsageMaximum, static_cast<std::uint8_t>( kButtonCount ),
    kItemLogicalMinimum, 0,
    kItemLogicalMaximum, 1,
    kItemReportSize, kBitField,
    kItemReportCount, static_cast<std::uint8_t>( kButtonCount ),
    kItemInput, kInputDataVarAbs,

    // Padding to the byte boundary, so the whammy starts at kWhammyOffset.
    kItemReportSize, kBitField,
    kItemReportCount, static_cast<std::uint8_t>( kPaddingBits ),
    kItemInput, kInputConstVarAbs,

    // The whammy, one byte, passed straight through.
    kItemUsagePage, kUsagePageGenericDesktop,
    kItemUsage, kUsageX,
    kItemLogicalMinimum, 0,
    kItemLogicalMaximum2, kAxisMaximum, 0,
    kItemReportSize, kAxisBits,
    kItemReportCount, 1,
    kItemInput, kInputDataVarAbs,

    kItemEndCollection,
};
// clang-format on

}  // namespace ps2
