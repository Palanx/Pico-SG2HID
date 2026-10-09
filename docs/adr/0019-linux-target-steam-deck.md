# ADR-0019: The Linux host the device targets is the Steam Deck (SteamOS), not Debian

- Status: accepted
- Date: 2026-10-09

## Context

`docs/product/requirements.md` R1 named macOS and Debian as the two hosts. Phase `08-usb-hid`
proved the Linux side on the operator's Steam Deck (SteamOS), and ADR-0018 recorded that as a
stand-in, with "Debian itself is not run on the bench" as debt. After the bench, the operator
stated that Debian was never the intended host: the device will only be used with a Mac and the
Steam Deck. Keeping Debian in R1 would leave a requirement no one intends to meet.

## Decision

The two hosts the device targets are **macOS** and **the Steam Deck running SteamOS**. R1, the
Users section of `docs/product/requirements.md`, `CLAUDE.md`'s summary line and `PHASES.md`'s
feature summary name SteamOS where they named Debian. The guarantee is unchanged in kind: the
device enumerates as a standard USB HID gamepad through the kernel's generic HID driver, with no
driver or host software. No layering or rule changes.

## Consequences

- **Easier.** The Linux host proved in `08-usb-hid` is the one the requirement names; ADR-0018's
  Debian debt is closed by this decision, not paid by a later bench run.
- **Harder.** Nothing guarantees behaviour on another distribution. The kernel path is the same
  (`usbhid`, `hid-generic`), so it is likely, not checked.
- **Noted.** On SteamOS a browser reads the whammy through the kernel's `joydev` correction (a
  dead zone around rest, a rescaled range outside it; `docs/phases/08-usb-hid/notes.md`). The
  report itself stays linear; a host-side reading must expect the corrected values.
- **Rejected: keeping Debian and running the bench on it once.** It would test a host the
  operator does not use.
- **Rejected: "any Linux" as the target.** Wider than anything the bench proves.
