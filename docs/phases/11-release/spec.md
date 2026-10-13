# Phase 11-release — build, flash and play from the README

## Goal

After this phase, `README.md` is the one document a person needs to go from a clean clone of this
repository to the SG playing Clone Hero on a Mac and on the Steam Deck. It replaces today's README,
whose status, hardware and build sections predate the firmware.

The table fixes the README's `##` sections, their order, and where each section's facts come from.
It does not describe their wording.

| Section | Source |
|---|---|
| `## What it is` | `docs/product/requirements.md` R1 and §Non-goals; ADR-0019 (the two hosts); ADR-0003 (own USB identity) |
| `## Safety` | today's README §Safety; `docs/constraints.md` R-SAFETY-08 (never flash with the guitar connected) |
| `## Wire` | `docs/wiring.md`; `docs/phases/02-wiring/verify.md` Part C |
| `## Build` | ADR-0014 and `SG2HID_PICO_SDK_VERSION` in `CMakeLists.txt` (the SDK clone command, criterion 1); the `Makefile` header (`make firmware` and the UF2 files it writes); the bench (the Homebrew installs) |
| `## Flash` | `docs/phases/08-usb-hid/verify.md` §2 |
| `## Use it on macOS` | `docs/phases/08-usb-hid/verify.md` §2; `docs/phases/10-guitar-full/verify.md` §5 and `notes.md` §Bench readings |
| `## Use it on the Steam Deck` | `docs/phases/10-guitar-full/verify.md` §6; ADR-0019 |
| `## Clone Hero` | ADR-0003; the Clone Hero wiki (wiki.clonehero.net, "Controls" and "Installation"); the bench |
| `## If it does not work` | `state_name( )` in `src/app/console.cpp`; `docs/phases/10-guitar-full/verify.md` §What was built and §6, and `notes.md` §For later phases (port placeholders) |
| `## Development` | the `Makefile` header; today's README §Building, §Layout and §Documentation |
| `## License` | today's README §License |

"The bench" means this phase's operator run, recorded in `notes.md` §Bench readings.

Flashing is done from the Mac; both hosts play (operator, 2026-10-13). Nothing is published: no
GitHub Release, no tag, no UF2 committed (operator, 2026-10-13).

The phase also corrects the two documents `## Wire` points to, which name socket pins by the
numbers printed on the breakout (mirrored on this bench's breakout, `docs/wiring.md` §This bench's
breakout): both name them by PS2 pin number (operator, 2026-10-13).

The operator proves it on the bench: a clean clone builds `sg2hid.uf2` with a fresh pico-sdk clone
made by the README's command, the Mac flashes that UF2 through `## Flash`, and Clone Hero is played
on both hosts after only the README's steps.

## Context pointers

- `README.md` — today's README: the file this phase rewrites, and the source of `## Safety`,
  `## Development` and `## License`.
- `docs/phases/11-release/notes.md` §Deviations and §Bench readings — the operator's decisions and
  the bench run the README's `## Build` and `## Clone Hero` rest on.
- `docs/phases/10-guitar-full/notes.md` §For later phases and §Bench readings — port
  placeholders, the whammy reading 0 until first moved.
- `docs/phases/10-guitar-full/verify.md` §What was built, §5, §6 — the summary line, the gamepad
  tester on each host, the Deck in Desktop Mode, Steam holding the device.
- `docs/phases/08-usb-hid/verify.md` §2 — BOOTSEL flashing and the macOS accessory prompt.
- `docs/adr/0003-generic-hid-gamepad-own-identity.md` — own USB identity; the Clone Hero mapping
  is this phase's documented step.
- `docs/adr/0014-pico-sdk-external-pinned.md` — the SDK clone at tag 2.3.1, found through
  `PICO_SDK_PATH`.
- `docs/adr/0019-linux-target-steam-deck.md` — the two hosts; joydev correction on SteamOS.
- `CMakeLists.txt` — `SG2HID_PICO_SDK_VERSION`.
- `Makefile` — its header comment, the source of the targets the README names.
- `src/app/console.cpp` — `state_name( )`.
- `docs/wiring.md` and `docs/phases/02-wiring/verify.md` Part C — what `## Wire` points to, and
  the two documents Plan step 2 corrects.
- `docs/product/requirements.md` — R1 and §Non-goals.
- `docs/constraints.md` — R-SAFETY-08 and R-PROC-02 (the two headings `verify.md` needs).

## Plan

1. Rewrite `README.md` with the Goal table's sections, in order, from their sources — touches
   `README.md` — check: acceptance criteria 1–5 pass.
2. Name socket pins by PS2 pin number where the two `## Wire` documents named them by the printed
   numbers: the "Socket pin N" definition in `docs/phases/02-wiring/verify.md` Part C, and
   `docs/wiring.md` §Socket pin 3 and §Sources, whose stop for a breakout with no numbering stays —
   touches those two files — check: acceptance
   criterion 6 prints nothing.
3. Build from a clean clone with a fresh SDK made by the README's command — touches nothing in the
   repo (scratch directory) — check: acceptance criterion 7 prints `UF2`.
4. Write `docs/phases/11-release/verify.md` for the operator, with `## What was built` and
   `## Check it yourself`. The check section, in order: (a) on the Mac, follow the README's
   `## Build` in a fresh clone; (b) flash that clone's `sg2hid.uf2` from the Mac following
   `## Flash`; (c) reconnect the guitar following `## Flash`'s last paragraph; (d) on the Mac,
   follow `## Use it on macOS` and `## Clone Hero` and play a song; (e) the same on the Deck with
   `## Use it on the Steam Deck`; (f) note where a README section used in (a)–(e) was wrong,
   unclear or missing a step; (g) what to record — touches `docs/phases/11-release/verify.md` —
   check: acceptance criterion 8 prints `2`.
5. Apply what the operator reports in 4(f) to `README.md`, bring `verify.md` §What was built up
   to date with the bench run, and record that run in `docs/phases/11-release/notes.md` §Bench
   readings — touches `README.md`, `docs/phases/11-release/verify.md`, `notes.md` — check:
   criteria 1–5 and 8 still pass.

## Acceptance criteria

```
v=$(sed -n 's/.*SG2HID_PICO_SDK_VERSION "\([0-9.]*\)".*/\1/p' CMakeLists.txt); grep -cF "git clone --branch $v --depth 1 --recurse-submodules --shallow-submodules https://github.com/raspberrypi/pico-sdk.git" README.md
                                              # expect: 1 (the README's SDK clone matches the pin)
grep '^## ' README.md                         # expect exactly, in order: ## What it is, ## Safety, ## Wire, ## Build, ## Flash, ## Use it on macOS, ## Use it on the Steam Deck, ## Clone Hero, ## If it does not work, ## Development, ## License
grep -nE 'Les Paul|No firmware is implemented|not yet — no sources' README.md
                                              # expect: no output, exit 1
grep -nE 'ttyACM[0-9]|usbmodem[0-9]' README.md
                                              # expect: no output, exit 1 (ports are placeholders)
grep -c 'build/pico/sg2hid.uf2' README.md     # expect: 1 or more
grep -n 'is the number printed on the PS2 breakout' docs/phases/02-wiring/verify.md
                                              # expect: no output, exit 1
d=$(mktemp -d) && git clone -q --branch 2.3.1 --depth 1 --recurse-submodules --shallow-submodules https://github.com/raspberrypi/pico-sdk.git "$d/pico-sdk" && git clone -q . "$d/sg2hid" && PICO_SDK_PATH="$d/pico-sdk" make -C "$d/sg2hid" firmware >/dev/null && head -c 3 "$d/sg2hid/build/pico/sg2hid.uf2"; echo
                                              # expect: UF2 (needs network; clones HEAD)
grep -cE '^#+ What was built|^#+ .*[Cc]heck it' docs/phases/11-release/verify.md
                                              # expect: 2
make test                                     # expect: exit 0
make lint                                     # expect: exit 0
make typecheck                                # expect: exit 0
operator bench: verify.md §Check it yourself (a)–(e)
                                              # expect: the Mac flashed the clean-clone UF2; Clone Hero mapped and a song played with frets, strums and whammy on both hosts
```

## Out of scope

- A GitHub Release, a git tag, or a committed UF2 — not wanted (operator, 2026-10-13).
- A firmware change to the whammy's report — Clone Hero's calibration handles it (bench,
  2026-10-13); a different mapping is a new phase, operator's call.
- Flashing from the Steam Deck or any Linux host — a contribution, not this phase (operator,
  2026-10-13).
- Clone Hero in the Deck's Game Mode or through Steam Input; a PS2 emulator.
- Build instructions for Linux or Windows hosts — development is macOS-only (ADR-0010).
- Any change to `src/`, the USB identity or the HID descriptor.
- `docs/index/`, `CLAUDE.md`, `docs/product/requirements.md` — unchanged by this phase.
