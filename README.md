# pico-sg2hid

Firmware for a Raspberry Pi Pico that turns a wired PlayStation 2 Guitar Hero SG into a USB
gamepad, with the whammy bar as a real analog axis.

## What it is

The Pico talks to the guitar over the PS2 controller bus, switches it into analog mode so the
whammy exists, and presents it to the computer as a standard USB HID gamepad: ten buttons (five
frets, strum up, strum down, start, select, tilt) and one axis (the whammy). It needs no driver
and no software on the computer. It is built and tested for two hosts: **macOS** and the
**Steam Deck** running SteamOS ([ADR-0019](docs/adr/0019-linux-target-steam-deck.md)).

Supported: the wired SG on an original Raspberry Pi Pico (RP2040). Not supported: the wireless
guitar and its dongle, any other PS2 controller, rumble, Windows, the Pico 2 — see
[`docs/product/requirements.md`](docs/product/requirements.md) §Non-goals.

The device has its own USB identity ("SG2HID Guitar") and does not pretend to be another
product, so games do not recognise it as a guitar on their own: Clone Hero needs a one-time
mapping, below ([ADR-0003](docs/adr/0003-generic-hid-gamepad-own-identity.md)).

## Safety

The RP2040 is **not 5 V tolerant**, and a guitar is hard to replace. These rules hold at all
times; the full set is in [`docs/constraints.md`](docs/constraints.md) §Invariants and
[ADR-0006](docs/adr/0006-fail-safe-hardware-policy.md):

- `DATA` and `ACK` are driven by the guitar. The Pico never drives them: doing so shorts two
  output stages together.
- The PS2 socket's 7.6 V pin (pin 3) is never connected to anything.
- The guitar is powered from the Pico's `3V3` pin, never `VBUS` or `VSYS` (both 5 V).
- Order of first contact is fixed: host tests → loopback on one Pico → Pico against the
  emulator → the real guitar observed read-only → the real guitar in full. Untested bus code
  never meets the guitar.
- **Never flash a Pico while the guitar is plugged into its socket.** Unplug the guitar first.

## Wire

Everything is in [`docs/wiring.md`](docs/wiring.md): the pin table, the parts (a PS2 female
socket breakout, five 330 Ω and two 10 kΩ resistors), the breadboard layout step by step, and
why each resistor is there. Read its section on breakouts that print pin numbers mirrored:
**wire by PS2 pin number**, not by what the breakout prints.

Before the guitar is ever plugged in, run the multimeter checklist in
[`docs/phases/02-wiring/verify.md`](docs/phases/02-wiring/verify.md). It is written for someone
who has not used a multimeter before. Its "socket pin" numbers are PS2 pin numbers too: find
each one on your breakout with `docs/wiring.md` first.

## Build

On a Mac with [Homebrew](https://brew.sh):

1. Install the build tools:

   ```sh
   brew install cmake picotool
   brew install --cask gcc-arm-embedded
   ```

   `picotool` turns the build into a `.uf2`; without it, the SDK downloads and builds its own copy
   during `make firmware`.

   Then run `arm-none-eabi-gcc --version`. If the shell says `command not found`, the cask put
   the compiler under `/Applications/ArmGNUToolchain/<version>/arm-none-eabi/bin`; add that
   directory to your `PATH` (for example in `~/.zshrc`) and open a new terminal.

2. Get the Pico SDK, at the exact version this firmware is pinned to
   ([ADR-0014](docs/adr/0014-pico-sdk-external-pinned.md)), anywhere outside this repository:

   ```sh
   git clone --branch 2.3.1 --depth 1 --recurse-submodules --shallow-submodules https://github.com/raspberrypi/pico-sdk.git
   export PICO_SDK_PATH="$PWD/pico-sdk"
   ```

   Put the `export` line, with the full path, in `~/.zshrc` so every new terminal has it.

3. In this repository:

   ```sh
   make firmware
   ```

   The file to flash is **`build/pico/sg2hid.uf2`**. The build also leaves `sg2hid_emu.uf2`,
   `sg2hid_loopback.uf2` and `sg2hid_observe.uf2`: those are development instruments, not the
   gamepad.

## Flash

Flashing is done from the Mac. Both the Mac and the Steam Deck play.

1. **Unplug the guitar from its socket.**
2. Hold the white **BOOTSEL** button on the Pico, plug its USB cable into the computer, then
   release the button. A drive called **`RPI-RP2`** appears.
3. In Finder, drag `sg2hid.uf2` onto `RPI-RP2`.
4. The Pico reboots by itself and the drive disappears. That is the flash finishing, not an
   error; the system may complain the drive was not ejected.

To plug the guitar back in: unplug the Pico's USB, plug the guitar into its socket, then plug
the Pico's USB back in.

## Use it on macOS

1. Plug the guitar into its socket, then the Pico into the Mac.
2. The first time, macOS may ask **"Allow accessory to connect?"**. Click **Allow**. If the
   gamepad does not show up after that, unplug the Pico and plug it back in.
3. First check: open a browser gamepad tester (any web page that shows the browser's gamepads)
   and **press a button on the guitar** — browsers list a gamepad only after its first press.
   It appears as `SG2HID Guitar`. Each fret, strum direction, start, select and tilting the
   neck lights one button; the whammy moves axis 0.

The tester shows the whammy as 0 (mid-travel) until the bar is first moved; after that it reads
-1 at rest and close to 1 at the stop. That is the browser waiting for the first change, not the
guitar.

## Use it on the Steam Deck

1. Switch to Desktop Mode (Steam button → Power → Switch to Desktop).
2. Plug the guitar into its socket, then the Pico into the Deck (a USB-C adapter or hub).
3. **The Steam client may hold the gamepad.** If a game or the gamepad tester sees nothing, quit
   Steam: its tray icon → Exit.
4. A browser gamepad tester shows the same buttons and axis as on the Mac, with one difference:
   Linux's joystick interface makes a small region in the middle of an axis read exactly 0, so
   around halfway down the whammy, the number stops moving for a short stretch and then moves on.
   That is how the browser reads the axis, not the device.

## Clone Hero

Get Clone Hero from [clonehero.net](https://clonehero.net/): the `.dmg` on the Mac, the Linux
`.tar.xz` on the Deck (extract it and run it in Desktop Mode, with Steam closed as above).

The guitar has to be mapped once per computer:

1. Plug the guitar in before starting Clone Hero.
2. In Clone Hero's main menu, press the **space bar**, or click the "press space to open the
   control remapper" message, to open the control mapper.
   Bind each action by choosing it, **Replace**, and pressing the matching control on the
   guitar: the five frets, strum up, strum down, start, select and star power (tilt the neck
   up). For the whammy, push the bar all the way to the stop: Clone Hero detects the axis only
   when it travels far enough.
3. Make sure the profile's instrument is a 5-fret guitar.
4. **Calibrate the whammy**, on every computer, or sustained notes keep wobbling after you let
   go. Push the bar once and let it go, so the computer sees its real resting value; then open
   **Calibrate** in the control mapper, choose the whammy axis, and set its **center to -1**. The
   whammy rests at one end of its axis (-1) and reaches the other (+1) at the stop; with the
   center at -1, Clone Hero reads rest as no whammy and the stop as full whammy.

## If it does not work

The Pico also opens a serial port and prints a summary line about once a second. Watching it
tells you whether the Pico sees the guitar:

1. Find the port with only this Pico plugged in:
   - **Mac:** `ls /dev/cu.usbmodem*` gives `/dev/cu.usbmodem<N>`.
   - **Steam Deck:** in Konsole, `ls /dev/ttyACM*` gives `/dev/ttyACM<N>`. Unplug the Pico and
     list again: the name that disappeared is the Pico's.
2. Print the lines: `cat /dev/cu.usbmodem<N>` (or `cat /dev/ttyACM<N>`), with your `<N>`.
   Ctrl-C stops it.
3. Each line starts with `hil:`. Read `state=`:
   - `state=analog` — the guitar is talking and in analog mode. `payload=` changes as you
     press frets and `report=` is what the computer gets. If this is what you see and the game
     still sees nothing, the problem is on the computer's side (Steam holding the device, the
     macOS accessory prompt, the Clone Hero mapping).
   - `state=absent` — the guitar is not answering. Check that it is plugged in all the way, and
     the wiring by PS2 pin number ([`docs/wiring.md`](docs/wiring.md)).
   - `state=negotiating` or `state=digital` that never becomes `analog` — the guitar answers but
     does not accept analog mode, and the computer gets a report with nothing pressed. Re-seat the plug; if it persists, open an issue with a few lines.

## Development

`make` is the only entry point.

```sh
make test       # host tests: logic only, no hardware, no network
make lint       # style gate: clang-format layout + clang-tidy naming
make typecheck  # each src/core header on its own; the firmware build when PICO_SDK_PATH is set
make firmware   # the UF2 images in build/pico/
make clean
```

`make test` needs a C++23 compiler, `python3` and bash >= 4 (`brew install bash`; macOS's
`/bin/bash` is 3.2). `make lint` also needs `clang-format` and `clang-tidy`
(`brew install clang-format llvm`): `make test` skips the style check when they are missing,
`make lint` fails. `make firmware` needs the toolchain and SDK from [Build](#build).

With the bench set up, `make hil MASTER=<port> EMU=<port>` runs the two-Pico hardware-in-the-loop
test against the emulator, and `make guitar MASTER=<port>` runs the real-guitar check
(`tools/hil_digital.py`).

```
src/core/    pure logic and data: PS2 codec, link state machine, HID mapping, pin table.
             No I/O, no SDK headers — this is what the host tests compile and run.
src/hal/     PIO programs and GPIO, for both the master and the emulator roles.
src/usb/     TinyUSB descriptors and HID report transport.
src/app/     main() for the gamepad, the loopback probe and the observe instrument.
src/emu/     second-Pico firmware that behaves as an SG on the bus.
tests/       host tests and repo-shape checks. `make test` runs everything here.
tools/       host-side development instruments (bus trace decoder, HIL harness).
```

Dependencies point inward toward `core`; the allowed edges are enforced on every edit from
`.claude/workflow/boundaries.rules`.

| What | Where |
|---|---|
| What is being built, and what is explicitly out of scope | [`docs/product/requirements.md`](docs/product/requirements.md) |
| Standing rules, the rule catalog, invariants | [`docs/constraints.md`](docs/constraints.md) |
| Decisions and their rationale | [`docs/adr/`](docs/adr/) |
| Phase index and status, with each phase's operator checks | [`docs/phases/PHASES.md`](docs/phases/PHASES.md) |

Every rule in the catalog carries an id and exactly one binding: a deterministic test, an
explicit `manual:` with the reason it cannot be machine-checked, or `planned:` naming the phase
that owes the test. Drift between a rule and its test fails the build. See
[ADR-0005](docs/adr/0005-rule-test-traceability.md).

## License

MIT. See [LICENSE](LICENSE).
