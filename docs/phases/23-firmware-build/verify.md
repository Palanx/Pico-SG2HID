# Phase 23-firmware-build — how to check this yourself

Written for someone who does not write C++, CMake or firmware. Most of this is commands to
paste into a terminal. The only physical step is optional: flashing the Pico over USB to see
it print a line. **The guitar is not involved at any point, and must stay unplugged.**

## What was built

**The firmware build.** Until now this repository only built *host tests*: programs that run
on your Mac. This phase adds the other build, the one that produces the program that runs
on the Pico itself.

- **The Pico SDK** is Raspberry Pi's library for the RP2040 chip: the startup code, the USB
  stack, and the functions that talk to the chip's pins. It is not copied into this
  repository. It lives in its own folder (for example `~/pico/pico-sdk`), and the build finds
  it through the `PICO_SDK_PATH` environment variable. It is pinned to version **2.3.1**
  (decision recorded in `docs/adr/0014-pico-sdk-external-pinned.md`). Any other version stops
  the build straight away, with a message naming both versions, so the firmware can never
  quietly change because the SDK was updated.
- **A UF2 file** (`build/pico/sg2hid.uf2`) is the finished firmware in the format the Pico's
  built-in bootloader accepts. When you hold the BOOTSEL button while plugging the Pico in, it
  shows up as a USB drive called `RPI-RP2`. Copy a `.uf2` onto that drive and the Pico
  flashes itself and restarts.
- **The firmware itself** is deliberately minimal (`src/app/main.cpp`). It starts USB and
  prints `pico-sg2hid: firmware build alive` once a second. It **touches no pin**: every GPIO
  stays an input, exactly as it is at power-on. USB serial output is used instead of the
  chip's UART on purpose. The UART would take over GP0 and GP1, which the pin table in
  `src/core/pins.h` does not list.

Three automatic checks come with it:

1. **R-ERR-05: no exceptions, no RTTI.** These are two C++ features that cost code size and
   RAM on a microcontroller. The compiler flags `-fno-exceptions -fno-rtti` switch them off.
   The SDK adds those flags, not this repository, so `tests/test_firmware_flags.sh` reads the
   exact compiler commands the build used (`build/pico/compile_commands.json`) and fails if
   any of the project's own files was compiled without both flags.
2. **Style checks reach the hardware folders.** `make lint` checks naming rules with a tool
   called clang-tidy. That tool has to understand each file the way the compiler does.
   Before this phase, a file in `src/hal/` that used an SDK header failed with
   `file not found`, reported as if it were a naming mistake. Lint now reads the same
   compiler commands as check 1, so a real naming mistake is reported as one. If you have
   never run `make firmware`, lint says so on its own line.
3. **`make typecheck` covers the firmware.** It still checks each `src/core/` header on its
   own. With `PICO_SDK_PATH` set, it now also runs the firmware build, because compiling is
   the only way to check code that depends on the SDK. Without the SDK it prints
   `skip: firmware typecheck: PICO_SDK_PATH unset`.

The list of forbidden pin-configuring functions (rule R-SAFETY-09) was also checked against
the real SDK 2.3.1 headers. Every name exists, and the search for similarly named functions
found none that can make a pin drive, so the list did not change. One gap is kept on purpose:
SDK helpers that set up pins internally, such as `stdio_uart_init_full`, are not on the list.

`make test` still needs nothing but a C++ compiler and `python3`: no SDK, no ARM compiler.

## Check it yourself

### 0. Install the SDK (once per machine)

Done on 2026-09-30. Repeat only on a new machine.

```
git clone --branch 2.3.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git ~/pico/pico-sdk
cd ~/pico/pico-sdk && git submodule update --init && cd -
echo 'export PICO_SDK_PATH=$HOME/pico/pico-sdk' >> ~/.zshrc
```

Open a **new** terminal (the variable only exists in shells started after the edit), then:
`git -C "$PICO_SDK_PATH" describe --tags` → **Expected:** `2.3.1`.

### 1. The commands

Run these from the repository root, in order. Each one cleans up after itself. "Expected"
is what a correct build prints.

| Command | Expected |
|---|---|
| `make firmware && test -f build/pico/sg2hid.uf2; echo rc=$?` | many `Building …` lines, then `rc=0` |
| `grep -cE 'CMAKE_CXX_STANDARD[[:space:]]+23' CMakeLists.txt` | `1` |
| `cmake -S . -B build/vcheck -DPICO_BOARD=pico -DSG2HID_PICO_SDK_VERSION=0.0.0; echo rc=$?; rm -rf build/vcheck` | `CMake Error … pico-sdk 2.3.1 found … but this firmware is pinned to 0.0.0`, then a non-zero `rc` |
| `grep -cE 'gpio_\|pio_' src/app/main.cpp` | `0`: the firmware calls no pin function |
| `sh tests/test_firmware_flags.sh` | `ok:   R-ERR-05 (build/pico/compile_commands.json)`, then four `ok:` case lines and `ok:   R-ERR-05 rejection and accept cases: 4/4` |
| `sed 's/-fno-rtti//' build/pico/compile_commands.json > build/mut_db.json; COMPILE_DB=build/mut_db.json sh tests/test_firmware_flags.sh; echo rc=$?; rm build/mut_db.json` | `FAIL: R-ERR-05: …`, then one line per project file saying `no -fno-rtti`, then `rc=1`. This proves the check really fails when a flag goes missing. |
| `make lint; echo rc=$?` | only `ok:` lines, `rc=0` |
| `mkdir -p src/hal && printf '#include "hardware/gpio.h"\nint BadName;\n' > src/hal/lint_probe.cpp && make firmware >/dev/null 2>&1; make lint 2>&1 \| grep -E "BadName\|file not found"; rm src/hal/lint_probe.cpp; rmdir src/hal; make firmware >/dev/null 2>&1` | exactly one line: `… invalid case style for variable 'BadName' …`, and no `file not found` |
| `mv build/pico build/pico.off; make lint 2>&1 \| grep 'no compile database'; mv build/pico.off build/pico` | `FAIL: R-STYLE-02: no compile database for src/hal, src/usb, src/app, src/emu — run make firmware` |
| `make typecheck; echo rc=$?` | the firmware build output, then `rc=0` |
| `env -u PICO_SDK_PATH make typecheck` | `skip: firmware typecheck: PICO_SDK_PATH unset` |
| `make test \| tail -1` | `OK` |

### 2. Flash the Pico and see it alive (optional)

This is the first program of ours to run on the chip. It drives no pin, so it is safe with
the breadboard wiring from phase 02 left in place: every GPIO stays an input, as it is at
power-on, and an input neither pushes nor pulls current.

1. **Unplug the guitar from its socket** (R-SAFETY-08: no code has been tested against it
   yet). Leave the Pico's USB cable unplugged too.
2. Hold down the white **BOOTSEL** button on the Pico. While holding it, plug the USB cable
   into the Mac, then release it. A drive called `RPI-RP2` appears in Finder.
3. `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`. The drive disappears by itself within a
   second or two: the Pico has flashed and restarted. (macOS may warn that the disk was not
   ejected properly. That is expected.)
4. `ls /dev/cu.usbmodem*`. **Expected:** one device, for example `/dev/cu.usbmodem1101`.
5. `cat /dev/cu.usbmodem1101` (use the name from step 4). **Expected:** a new line
   `pico-sg2hid: firmware build alive` every second. Press Ctrl-C to stop.

If step 4 shows nothing, the firmware did not start USB. Unplug the Pico, repeat from
step 2, and keep the output of `make firmware` for the next session.
