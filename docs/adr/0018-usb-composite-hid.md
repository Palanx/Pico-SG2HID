# ADR-0018: The master is one composite CDC + HID device, its report descriptor lives in `core`

- Status: accepted
- Date: 2026-10-09

## Context

Phase `08-usb-hid` makes the master a USB HID gamepad (ADR-0003). Until now the master was a
CDC serial device only, through the Pico SDK's own descriptors, and the harness
(`tools/hil_digital.py`) reads its `hil:`, `link:` and `T1` lines from that port. Several shapes
were open:

- whether the HID gamepad replaces the serial port or sits beside it;
- where the HID report descriptor lives, given that the report layout is decided in
  `src/core/hid_report.h` and a descriptor that disagrees with it enumerates cleanly and reports
  the wrong control;
- what the host sees while the link is not streaming analog frames;
- which usage carries the whammy;
- when a report is sent;
- which Linux host stands in for Debian on the bench.

## Decision

- **One composite device, CDC + HID**, under ADR-0003's identity (VID `0x2E8A`, PID `0x000A`).
  The serial port keeps `make hil`, `link:` and `T1` trace working; the HID interface is the
  gamepad. Linking `tinyusb_device` turns off the SDK's descriptors, so `src/usb/` supplies the
  device, configuration, string and HID report descriptors itself.
- **The report descriptor is `core` data**, `kHidReportDescriptor` in
  `src/core/hid_descriptor.h`, built from `hid_report.h`'s constants. A host test compares it
  with a hand-written vector (R-HID-01), so the descriptor and the builder cannot drift apart
  without `make test` failing.
- **The neutral report while the link is not `AnalogStreaming`:** no button set, the whammy at
  `kWhammyRest` (R-HID-02). The host never sees a stale press held across a dropped link.
- **The whammy is Generic Desktop X**, 0..255. The first axis of a gamepad is the one every
  tester and mapper shows first.
- **A report each poll, when the interface is ready, dropped otherwise.** No queue: a report
  that missed its slot is already stale, and the next poll carries a newer one.
- **The Linux bench host is SteamOS on the operator's Steam Deck**, not Debian. The kernel
  path (`usbhid`, `hid-generic`) is the same; the distribution is not.

## Consequences

- **Easier.** The descriptor is checked on the laptop, byte for byte, against a literal. The
  harness keeps its port and gains the report bytes on its summary line.
- **Harder.** The master owns its USB descriptors now, including the CDC ones the SDK used to
  provide, and the SDK's USB reset interface (`picotool reboot` over USB) is gone: flashing is
  BOOTSEL by hand.
- **Debt taken knowingly.** Debian itself is not run on the bench; requirements R1 still names
  it. A report every millisecond whether it changed or not costs bus time nothing measured asks
  to save.
- **Rejected: HID only.** It would remove the harness's port, and with it every automated check
  on the two-Pico bench.
- **Rejected: writing the descriptor bytes in `src/usb/`.** It would be a second place the report
  layout is decided, and one no host test can read.
- **Rejected: passing the last good frame through while the link is down.** A press held when
  the guitar is unplugged would stay held on the host.
