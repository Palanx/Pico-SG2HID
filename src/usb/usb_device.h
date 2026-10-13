#pragma once

// The master's USB device (08-usb-hid, ADR-0018): one composite CDC + HID device under
// ADR-0003's identity. The CDC interface is the serial port stdio writes to; the HID interface
// is the gamepad, whose report descriptor is src/core/hid_descriptor.h's.

#include "core/hid_report.h"

namespace usb {

// Starts TinyUSB. Call before stdio_init_all( ), which asserts it has been done.
void init();

// Runs TinyUSB's device task. Call often: nothing else does, since this firmware owns TinyUSB.
void task();

// Stores `report` as the one a host GET_REPORT reads, and sends it when the HID interface is
// ready. A report that cannot be sent now is dropped, never queued: the next poll carries a
// newer one (ADR-0018).
void send_report( const ps2::HidReport& report );

// Whether the HID interface can take a report now: a send_report( ) while this is false sends
// nothing; the master counts those as `busy`.
[[nodiscard]] bool is_report_ready();

}  // namespace usb
