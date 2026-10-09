#include "usb/usb_device.h"

#include "core/hid_descriptor.h"
#include "core/poll.h"
#include "pico/unique_id.h"
#include "tusb.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

static_assert( CFG_TUD_HID_EP_BUFSIZE >= ps2::kReportLen, "the HID endpoint must fit a report" );

// --- Identity (ADR-0003) ----------------------------------------------------------------

constexpr std::uint16_t kVendorId      = 0x2E8A;  // Raspberry Pi
constexpr std::uint16_t kProductId     = 0x000A;  // the Pico SDK's test product id
constexpr std::uint16_t kUsbVersion    = 0x0200;
constexpr std::uint16_t kDeviceVersion = 0x0100;

enum StringIndex : std::uint8_t {
    StringLanguage,
    StringManufacturer,
    StringProduct,
    StringSerial,
};

constexpr std::string_view kManufacturer = "pico-sg2hid";
constexpr std::string_view kProduct      = "SG2HID Guitar";
constexpr std::uint16_t    kLanguageEnUs = 0x0409;

// --- Configuration ----------------------------------------------------------------------

enum Interface : std::uint8_t {
    InterfaceCdc,
    InterfaceCdcData,
    InterfaceHid,
    InterfaceCount,
};

constexpr std::uint8_t  kEndpointCdcNotify = 0x81;
constexpr std::uint8_t  kEndpointCdcOut    = 0x02;
constexpr std::uint8_t  kEndpointCdcIn     = 0x82;
constexpr std::uint8_t  kEndpointHidIn     = 0x83;
constexpr std::uint8_t  kCdcNotifySize     = 8;
constexpr std::uint8_t  kCdcDataSize       = 64;
constexpr std::uint8_t  kHidPollIntervalMs = 1;
constexpr std::uint8_t  kConfigNumber      = 1;
constexpr std::uint8_t  kNoString          = 0;
constexpr std::uint8_t  kNoAttributes      = 0;  // bus-powered, no remote wakeup
constexpr std::uint16_t kMaxPowerMa        = 100;
constexpr std::uint16_t kConfigTotalLen = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN;

const tusb_desc_device_t kDeviceDescriptor = {
    .bLength            = sizeof( tusb_desc_device_t ),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = kUsbVersion,
    // Interface Association Descriptors, which a composite device with CDC needs.
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = kVendorId,
    .idProduct          = kProductId,
    .bcdDevice          = kDeviceVersion,
    .iManufacturer      = StringManufacturer,
    .iProduct           = StringProduct,
    .iSerialNumber      = StringSerial,
    .bNumConfigurations = 1,
};

const std::uint8_t kConfigDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(
        kConfigNumber, InterfaceCount, kNoString, kConfigTotalLen, kNoAttributes, kMaxPowerMa ),
    TUD_CDC_DESCRIPTOR( InterfaceCdc,
                        kNoString,
                        kEndpointCdcNotify,
                        kCdcNotifySize,
                        kEndpointCdcOut,
                        kEndpointCdcIn,
                        kCdcDataSize ),
    TUD_HID_DESCRIPTOR( InterfaceHid,
                        kNoString,
                        HID_ITF_PROTOCOL_NONE,
                        ps2::kHidReportDescriptor.size(),
                        kEndpointHidIn,
                        CFG_TUD_HID_EP_BUFSIZE,
                        kHidPollIntervalMs ),
};

// --- State ------------------------------------------------------------------------------

// The neutral report until the first poll stores one: no button set, the whammy at rest.
ps2::HidReport stored_report = ps2::neutral_report();

// A string descriptor: a header word, then UTF-16 code units. Long enough for the serial
// number, the longest string here.
constexpr std::size_t kMaxStringChars = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES;
std::array<std::uint16_t, kMaxStringChars + 1> string_buffer{};

constexpr unsigned kStringTypeShift = 8;

// Writes the header word for `count` code units already in string_buffer: the descriptor's length
// in bytes, then its type.
[[nodiscard]] const std::uint16_t* finish_string( std::size_t count ) {
    string_buffer[ 0 ] = static_cast<std::uint16_t>( ( TUSB_DESC_STRING << kStringTypeShift ) |
                                                     ( sizeof( std::uint16_t ) * ( count + 1 ) ) );
    return string_buffer.data();
}

[[nodiscard]] const std::uint16_t* string_descriptor( std::string_view text ) {
    const std::size_t count = text.size() < kMaxStringChars ? text.size() : kMaxStringChars;
    for ( std::size_t i = 0; i < count; ++i ) {
        string_buffer[ i + 1 ] = static_cast<std::uint8_t>( text[ i ] );
    }
    return finish_string( count );
}

}  // namespace

namespace usb {

void init() {
    tusb_init();
}

void task() {
    tud_task();
}

void send_report( const ps2::HidReport& report ) {
    stored_report = report;
    if ( tud_hid_ready() ) {
        tud_hid_report(
            0, stored_report.data(), static_cast<std::uint16_t>( stored_report.size() ) );
    }
}

}  // namespace usb

// --- TinyUSB callbacks ------------------------------------------------------------------
// Named and declared by TinyUSB, with C linkage; it calls them from tud_task( ).

extern "C" {

const std::uint8_t* tud_descriptor_device_cb() {
    return reinterpret_cast<const std::uint8_t*>( &kDeviceDescriptor );
}

const std::uint8_t* tud_descriptor_configuration_cb( std::uint8_t /*index*/ ) {
    return kConfigDescriptor;
}

const std::uint16_t* tud_descriptor_string_cb( std::uint8_t index, std::uint16_t /*langid*/ ) {
    switch ( index ) {
    case StringLanguage:
        string_buffer[ 1 ] = kLanguageEnUs;
        return finish_string( 1 );
    case StringManufacturer:
        return string_descriptor( kManufacturer );
    case StringProduct:
        return string_descriptor( kProduct );
    case StringSerial: {
        std::array<char, kMaxStringChars + 1> serial{};
        pico_get_unique_board_id_string( serial.data(), serial.size() );
        return string_descriptor( serial.data() );
    }
    default:
        return nullptr;
    }
}

const std::uint8_t* tud_hid_descriptor_report_cb( std::uint8_t /*instance*/ ) {
    return ps2::kHidReportDescriptor.data();
}

// NOLINTNEXTLINE(readability-function-size): TinyUSB fixes this five-parameter signature.
std::uint16_t tud_hid_get_report_cb( std::uint8_t /*instance*/,
                                     std::uint8_t /*report_id*/,
                                     hid_report_type_t /*report_type*/,
                                     std::uint8_t* buffer,
                                     std::uint16_t reqlen ) {
    const std::size_t count = reqlen < stored_report.size() ? reqlen : stored_report.size();
    for ( std::size_t i = 0; i < count; ++i ) {
        buffer[ i ] = stored_report[ i ];
    }
    return static_cast<std::uint16_t>( count );
}

// No OUT reports: nothing to set.
// NOLINTNEXTLINE(readability-function-size): TinyUSB fixes this five-parameter signature.
void tud_hid_set_report_cb( std::uint8_t /*instance*/,
                            std::uint8_t /*report_id*/,
                            hid_report_type_t /*report_type*/,
                            const std::uint8_t* /*buffer*/,
                            std::uint16_t /*bufsize*/ ) {}

}  // extern "C"
