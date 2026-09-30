# Phase 23-firmware-build — the firmware build exists before any bus code

## Goal

After this phase, `make firmware` turns the repository into a flashable
`build/pico/sg2hid.uf2`. It uses CMake and an external Pico SDK clone at tag **2.3.1**,
reached through `PICO_SDK_PATH`. Any other SDK version stops the configure step with a
message that names both versions. The image is built from three sources:

- every `src/core/*.cpp`, compiled for the RP2040 in C++23;
- every `src/hal/*.cpp` (none exists yet);
- `src/app/main.cpp`: a minimal entry point that configures no GPIO and prints one banner
  line over USB, once a second.

The three gaps the phase closes:

| Gap today | After this phase | Rule / source |
|---|---|---|
| `-fno-exceptions -fno-rtti` on firmware is a claim, since no firmware build exists | A check reads `build/pico/compile_commands.json` and fails when any compile command for a C++ file under `src/` does not end up with both flags in effect | R-ERR-05 → `test: tests/test_firmware_flags.sh` |
| A file under `src/hal/`, `src/usb/`, `src/app/` or `src/emu/` that includes an SDK header fails `make lint` with `file not found`, under a FAIL line naming R-STYLE-02 | `make lint` runs clang-tidy on those files through the CMake compile database. A naming violation there is reported as a naming violation. A missing database is its own FAIL line that says to run `make firmware` | `docs/phases/13-scaffold-check-gaps/notes.md` §For later phases |
| R-SAFETY-09's function names were never checked against real SDK headers | Every name, and every starred prefix, resolves to a declaration in the SDK 2.3.1 headers. | `docs/phases/02-wiring/notes.md` §For later phases |
| `make typecheck` covers `src/core/*.h` only | `make typecheck` also runs the firmware build when `PICO_SDK_PATH` is set. When it is unset, it prints a `skip: firmware typecheck` line and judges only the core headers | `.claude/rules/tech-debt.md` "No typecheck gate configured", `make firmware` bullet |

`make test` still needs only a C++23 compiler and `python3` (R-PROC-04).

## Context pointers

- `CLAUDE.md`: the hardware-safety rules. R-SAFETY-08 governs the flashing step in
  `verify.md`.
- `docs/adr/0001-cpp17-pico-sdk-pio-bus.md`: the Pico SDK, `make` as the only entry point,
  and CMake never invoked directly.
- `docs/adr/0008-cpp23.md`: C++23, and the SDK's C++17 default must be overridden
  explicitly (§Consequences). This ADR also records the ARM toolchain trap.
- `docs/adr/0010-macos-only-development-host-agnostic-device.md`: paths for macOS and
  Homebrew LLVM may be named directly.
- `docs/templates/adr.md`: the template for the new ADR in step 1.
- `docs/constraints.md`:
  - R-ERR-05, whose binding this phase changes.
  - R-SAFETY-09, whose name list and Scope clause change.
  - R-STYLE-02 and the §Style paragraph starting "R-STYLE-02's scope is every…", which says
    SDK files fail lint "until 03-pio-bus supplies its include flags".
  - §Observed conventions: the clang-tidy flags finding, and the finding "An SDK-including
    file anywhere under `src/` fails `make lint`".
  - R-PROC-04 and R-SAFETY-02 ("every GPIO used is declared").
- `.claude/rules/tech-debt.md`: the entry this phase partly closes. Read it, do not edit it
  (§Out of scope).
- `Makefile`: the targets `firmware`, `typecheck` and `lint`. `firmware` already refuses to
  run without cmake or `PICO_SDK_PATH`.
- `.gitignore`: already ignores `build/` and `*.uf2`.
- `.clang-tidy`: its header says SDK files fail "until 03-pio-bus", and that a compile
  database is the upgrade path. The header is rewritten; `CheckOptions` does not change.
- `tests/test_style.sh`: the clang-tidy invocation (`tidy_files( )`, `tidy_sources( )`,
  `tidy_lang( )`, `tidy_sysroot_flag( )`), the `OPTIONAL_TOOLS` skip-or-fail convention,
  and the header note on flags.
- `tests/test_repo_shape.sh`:
  - `find_safety09( )` and its reject and accept cases (search `R-SAFETY-09`);
  - `src_files( )`.
- `tests/test_tool_versions.sh`: how the ARM compiler is located and probed (`resolve( )`,
  `arm_compiles( )`).
- `tests/test_checks_are_live.py`:
  - every function in a `tests/test_*.sh` file is mutated, and neutering it must fail the
    file;
  - `# RULE` header lines are what the accounting reads;
  - real-run result lines must not carry case words (`CASE_LINE`).
- `tests/test_rule_traceability.py`: a `test:` binding needs the file to exist and carry a
  `RULE R-ERR-05` marker.
- `src/core/pins.h`: the only GPIOs the firmware may touch. GP0 and GP1, the SDK's default
  UART stdio pins, are not in it.
- `docs/phases/13-scaffold-check-gaps/notes.md` §For later phases: the measured lint failure
  on an SDK-including file.
- `docs/phases/02-wiring/notes.md` §For later phases, §Debt: R-SAFETY-09's names, and the
  fact that `.c` and `.pio` are not scanned.
- `docs/phases/22-scaffold-typecheck-transitive/notes.md` §For later phases: how
  `make typecheck` is built today.
- `docs/phases/00-scaffold/notes.md` §For later phases, the `03-pio-bus` toolchain bullet:
  the ARM compiler must stay the cask's `bin` on `PATH`. Do not relink it.
- `docs/phases/22-scaffold-typecheck-transitive/verify.md`: the house shape of a
  `verify.md`, with a `## What was built` and a `## …Check it…` heading (R-PROC-02).

### Facts measured while expanding (2026-09-30), not to be re-derived

- **The pin.** pico-sdk's latest release is **2.3.1**, published 2026-09-04 (the GitHub
  releases API). Its root `CMakeLists.txt` declares `cmake_minimum_required(VERSION
  3.13...3.27)`, and its `pico_sdk_version.cmake` builds `PICO_SDK_VERSION_STRING` as
  `2.3.1`.
- **Local tools.** This machine has CMake 4.4.3 and picotool v2.3.0. Whether SDK 2.3.1
  accepts picotool 2.3.0 is **not measured**: step 2 finds out.
- **The SDK's default C++ flags.** In SDK 2.3.1, `src/rp2_common/pico_cxx_options/CMakeLists.txt`
  adds `-fno-exceptions`, `-fno-unwind-tables` and `-fno-rtti` to C++ compiles unless
  `PICO_CXX_ENABLE_EXCEPTIONS` or `PICO_CXX_ENABLE_RTTI` is truthy. The flags therefore come
  from the SDK and not from this repo. That is exactly why the R-ERR-05 check reads the
  compile database instead of `CMakeLists.txt`.
- **clang-tidy over an ARM compile database.** Homebrew clang-tidy 23.1.0 was given a compile
  database whose `command` is the cask's `arm-none-eabi-g++ -mcpu=cortex-m0plus -mthumb
  -std=gnu++23 …`:
  - It infers the target from the driver's name (`Target: arm-unknown-none-eabi`), and on
    its own finds no standard header (`'cstddef' file not found`).
  - Passing each directory from `arm-none-eabi-g++ -mcpu=cortex-m0plus -mthumb -xc++ -E -v -
    </dev/null` (six directories under `…/arm-none-eabi/include/c++/15.3.1`,
    `…/lib/gcc/arm-none-eabi/15.3.1/include` and `…/arm-none-eabi/include`) as
    `--extra-arg=-isystem<dir>` fixes it. `src/core/link.cpp` then produces no diagnostic,
    and a probe holding `#include <optional>` and `int BadName;` produces exactly one:
    `invalid case style for variable 'BadName'`.
  - No `-nostdinc` was needed.
  - Not measured: a file that includes an SDK header, and a header (`.h`) that is absent
    from the database, which clang-tidy would have to interpolate.

## Plan

0. **The operator installs the SDK.** This is not the agent's job: it installs a dependency
   (R-PROC-05), and the operator approved the external clone on 2026-09-30.
   - Clone into a directory outside the repository, for example `~/pico/pico-sdk`, with
     `git clone --branch 2.3.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git`.
   - Inside the clone, run `git submodule update --init` (this fetches `lib/tinyusb`, which
     USB stdio needs).
   - Add `export PICO_SDK_PATH=<that path>` to `~/.zshrc`. An agent session opened before
     that edit does not see the variable: open a fresh session
     (`docs/phases/00-scaffold/notes.md`).
   - Check: `git -C "$PICO_SDK_PATH" describe --tags` → `2.3.1`.

1. **ADR-0014: where the SDK comes from and at which version**, from `docs/templates/adr.md`,
   status `accepted`.
   - Decision: an external clone at the exact tag, through `PICO_SDK_PATH`, with the tag
     pinned in `CMakeLists.txt` and a mismatch fatal at configure time.
   - Rejected, with reasons: a git submodule (clone size, and tinyusb nested inside it), and
     CMake `FetchContent` (a network fetch inside the build).
   - The ADR lands before any build file.
   - Touches: `docs/adr/0014-pico-sdk-external-pinned.md`.
   - Check: `grep -c '2.3.1' docs/adr/0014-*.md` ≥ 1.

2. **The build.**
   - Touches `CMakeLists.txt` (new, at the repository root), `src/app/main.cpp` (new) and
     `Makefile`.
   - `CMakeLists.txt`, before `pico_sdk_init.cmake` is included:
     - sets `CMAKE_CXX_STANDARD 23` and `CMAKE_CXX_STANDARD_REQUIRED ON`;
     - sets `CMAKE_EXPORT_COMPILE_COMMANDS ON`;
     - sets `PICO_CXX_ENABLE_EXCEPTIONS 0` and `PICO_CXX_ENABLE_RTTI 0` as normal variables,
       so a `-D` on the command line cannot turn them on.
   - It includes `$ENV{PICO_SDK_PATH}/pico_sdk_init.cmake`.
   - It holds `SG2HID_PICO_SDK_VERSION` as a CACHE STRING that defaults to `2.3.1`. After
     `pico_sdk_init( )`, a `PICO_SDK_VERSION_STRING` that differs is a `FATAL_ERROR` naming
     both values. The variable exists so the mismatch path can be exercised; the default is
     the pin.
   - It declares one executable, `sg2hid`, built from
     `file( GLOB CONFIGURE_DEPENDS src/core/*.cpp src/hal/*.cpp src/app/*.cpp )` and linked
     to `pico_stdlib`:
     - `-Wall -Wextra -Werror` on that target only (the SDK's sources are not ours to
       fix);
     - `pico_enable_stdio_usb( sg2hid 1 )` and `pico_enable_stdio_uart( sg2hid 0 )`. UART
       stdio would claim GP0 and GP1, which `src/core/pins.h` does not declare
       (R-SAFETY-02);
     - `pico_add_extra_outputs( sg2hid )`.
   - `src/app/main.cpp` calls `stdio_init_all( )`, then loops forever, printing one line
     that starts with `pico-sg2hid:` once a second. It calls no GPIO or PIO function.
   - The `Makefile`'s `firmware` target keeps its two guards and its two `cmake` commands.
     Only its comment and the header change, if a fact in them does.
   - Check: `make firmware && ls build/pico/sg2hid.uf2` → exit 0.
   - Check: `cmake -S . -B build/vcheck -DPICO_BOARD=pico -DSG2HID_PICO_SDK_VERSION=0.0.0`
     exits non-zero, and its output contains both `0.0.0` and `2.3.1`. Then
     `rm -rf build/vcheck`.
   - If picotool 2.3.0 is refused, stop and record it in `notes.md`. Upgrading picotool
     installs a dependency, which is the operator's call.

3. **R-ERR-05 becomes a check.**
   - Touches `tests/test_firmware_flags.sh` (new) and `docs/constraints.md` (R-ERR-05's
     line).
   - The file's header carries `# RULE R-ERR-05`. One check function (for example
     `check_err05( )`) takes a compile database path and prints one line per entry that
     fails. It considers only entries whose `file` is a `.cpp`, `.cc` or `.hpp` under this
     repository's `src/`, and fails when:
     - the `command` lacks `-fno-exceptions` or `-fno-rtti`; or
     - a later `-fexceptions` or `-frtti` overrides one of them (the last flag wins in GCC).
   - A database with **zero** such entries is itself a failure line, never a vacuous pass.
   - The real run reads `${COMPILE_DB:-build/pico/compile_commands.json}`. When that file is
     absent, it prints `skip:` under `OPTIONAL_TOOLS=1` and `FAIL` otherwise.
   - Rejection cases, always run, each a hand-written JSON fixture fed to the same function
     and each required to fail:
     - `-fno-rtti` missing;
     - `-fno-exceptions` followed by `-fexceptions`;
     - no entry under `src/`.
   - One accept case: an SDK-only entry lacking the flags, next to a compliant `src/` entry,
     must pass.
   - R-ERR-05's binding becomes `test: tests/test_firmware_flags.sh`. Its text gains a
     `**Scope, recorded <date>` clause saying:
     - it reads the compile database written by `make firmware`, so it judges the last
       configured build and not `CMakeLists.txt`;
     - it considers C++ entries under `src/` only;
     - it is skipped under `make test` when no build has been configured.
   - Check: `sh tests/test_firmware_flags.sh` → exit 0, with an `ok:` line for R-ERR-05 and
     every case passing.
   - Check: `python3 tests/test_checks_are_live.py` → exit 0.
   - Check: `python3 tests/test_rule_traceability.py` → exit 0.

4. **clang-tidy reaches the SDK layers.**
   - Touches `tests/test_style.sh`, `.clang-tidy` (header only) and `docs/constraints.md`
     (the §Style paragraph and both §Observed conventions entries named in Context pointers).
   - In `tidy_files( )`, a file under `src/core/` or `tests/` keeps today's host invocation,
     unchanged. A file under `src/hal/`, `src/usb/`, `src/app/` or `src/emu/` is run as
     `clang-tidy --quiet -p build/pico <arm -isystem extra args> <file>`, using the
     measured form in Context pointers.
   - The `-isystem` directories are derived at run time from the first `arm-none-eabi-g++`
     on `PATH`. They are never hardcoded.
   - With no `build/pico/compile_commands.json` and at least one SDK-layer file present, a
     distinct line is printed, `skip_or_fail`-style: `R-STYLE-02: no compile database for
     src/hal, src/usb, src/app, src/emu — run make firmware`. It never says `file not found`
     under the naming FAIL line. With the database present and no `arm-none-eabi-g++` on
     `PATH`, a second line of the same kind names the missing compiler.
   - New rejection case, run only when the ARM compiler and clang-tidy
     are both present, `skip:` otherwise:
     - a scratch compile database whose one entry compiles a scratch file with the ARM
       compiler;
     - that file holds `#include <optional>` and `int BadName;`;
     - it must report `readability-identifier-naming` and never `file not found`.
   - Measure, and record in `notes.md`, whether an SDK-layer `.h` that is absent from the
     database is linted correctly through interpolation. If it is not, that is a FAIL, never
     a silent skip, and the finding goes to §Observed conventions.
   - Rewrite, in the same edit, each text that says SDK files fail "until 03-pio-bus"
     (`.clang-tidy`'s header, `tests/test_style.sh`'s header, the §Style paragraph):
     - it now says how they are linted, and that lint needs a configured build;
     - the §Observed conventions finding "An SDK-including file … fails `make lint`" gains
       its resolution and date;
     - the clang-tidy flags finding gains the ARM-database form and its measurement.
   - Check: `make lint` → exit 0.
   - Check: `OPTIONAL_TOOLS=1 sh tests/test_style.sh` with `build/pico` moved away → exit 0
     and a `skip:` line naming the compile database.

5. **R-SAFETY-09 against the real headers.** Touches `docs/constraints.md` (R-SAFETY-09's
   line), and `tests/test_repo_shape.sh` only if the list changes.
   - Confirm each name in the rule resolves to a declaration under `$PICO_SDK_PATH/src`
     (`*.h`). For a starred name, at least one declaration must match the prefix.
   - Then list the functions in those headers whose names start with `gpio_set_`,
     `gpio_init`, `pio_gpio_` or `pio_sm_set_`, or which contain `pindir`. A function
     **joins the list** when it can change a pin's function, direction or output enable:
     function select, direction, an OE override, a pindir mask. Pull and input-buffer
     settings do not make a pin drive, so pulls stay as listed and nothing else of that
     kind is added.
   - Each added name gets a reject case in `tests/test_repo_shape.sh`, in the existing
     one-per-alternative shape.
   - Every considered and rejected candidate goes in `notes.md` with its reason.
   - The rule's sentence "not yet checked against installed SDK headers — `03-pio-bus` does
     that" is replaced by "checked against pico-sdk 2.3.1 headers on <date>", plus a sentence that
     SDK functions outside this search which configure pins internally (for example
     `stdio_uart_init_full`) are neither named nor matched. The operator kept that gap
     (2026-09-30).
   - Check: the name-resolution command in Acceptance criteria → no output.
   - Check: `sh tests/test_repo_shape.sh` → exit 0.

6. **`make typecheck` covers the SDK layers.**
   - Touches `Makefile` (the `typecheck` target and the header comment).
   - After the core-header loop:
     - with `PICO_SDK_PATH` set, it runs `$(MAKE) firmware`, and a failed build fails
       typecheck;
     - with it unset, it prints `skip: firmware typecheck: PICO_SDK_PATH unset` and the exit
       status is the core loop's.
   - `make test` still does not call it.
   - Check: `make typecheck` → exit 0.
   - Check: `env -u PICO_SDK_PATH make typecheck` → exit 0 and prints the `skip:` line.

7. **`docs/phases/23-firmware-build/verify.md`**, for a non-specialist, with the two
   R-PROC-02 headings.
   - What the SDK and a UF2 are.
   - Step 0's install.
   - These Acceptance criteria commands, with what each should print: `make firmware`, the
     `CMAKE_CXX_STANDARD` grep, the version-mismatch `cmake`, the `gpio_|pio_` grep of
     `src/app/main.cpp`, `sh tests/test_firmware_flags.sh` and its mutated-database run,
     `make lint`, the lint probe, the no-database `make lint`, `make typecheck` with and
     without `PICO_SDK_PATH`, and `make test`. The rest check the harness and the
     documents, not the firmware, and stay out of `verify.md`.
   - The flashing procedure:
     - **the guitar unplugged from the socket** (R-SAFETY-08);
     - hold BOOTSEL while plugging USB, copy `build/pico/sg2hid.uf2` to the `RPI-RP2`
       drive;
     - then `ls /dev/cu.usbmodem*`, and `cat` that device to see a `pico-sg2hid:` line
       every second; Ctrl-C to stop.
   - Why an unconfigured Pico is safe with the breadboard wiring left in place: every GPIO
     stays an input, as it is at reset.
   - Check: `sh tests/test_phase_docs.sh` → exit 0.

## Acceptance criteria

Run with the SDK installed, `PICO_SDK_PATH` exported and the cask's ARM `bin` on `PATH`, from
the repository root, in order.

```
git -C "$PICO_SDK_PATH" describe --tags                                  # expect: 2.3.1
grep -c '2\.3\.1' docs/adr/0014-*.md                                     # expect: >= 1
make firmware && test -f build/pico/sg2hid.uf2                           # expect: exit 0
grep -cE 'CMAKE_CXX_STANDARD[[:space:]]+23' CMakeLists.txt               # expect: 1
cmake -S . -B build/vcheck -DPICO_BOARD=pico -DSG2HID_PICO_SDK_VERSION=0.0.0 >build/vcheck.log 2>&1; echo $?; grep -c '0\.0\.0' build/vcheck.log; grep -c '2\.3\.1' build/vcheck.log; rm -rf build/vcheck build/vcheck.log   # expect: non-zero, then >= 1, then >= 1
grep -cE 'gpio_|pio_' src/app/main.cpp                                   # expect: 0
grep -c 'pico_enable_stdio_uart( *sg2hid *0 *)' CMakeLists.txt           # expect: 1
sh tests/test_firmware_flags.sh                                          # expect: exit 0, an `ok:` line naming R-ERR-05, every case ok
sed 's/-fno-rtti//' build/pico/compile_commands.json > build/mut_db.json; COMPILE_DB=build/mut_db.json sh tests/test_firmware_flags.sh; echo $?; rm build/mut_db.json   # expect: a FAIL line naming R-ERR-05, then 1
grep -cE '^- \*\*R-ERR-05\*\* .* — test: `tests/test_firmware_flags\.sh`$' docs/constraints.md   # expect: 1
make lint                                                                # expect: exit 0
mkdir -p src/hal && printf '#include "hardware/gpio.h"\nint BadName;\n' > src/hal/lint_probe.cpp && make firmware >/dev/null 2>&1; make lint > build/lint.log 2>&1; echo $?; grep -c "invalid case style for variable 'BadName'" build/lint.log; grep -c 'file not found' build/lint.log; rm src/hal/lint_probe.cpp; rmdir src/hal 2>/dev/null; make firmware >/dev/null 2>&1; rm build/lint.log   # expect: 2 (make's status for a failed recipe; amended, see notes.md §Deviations), then 1, then 0
mv build/pico build/pico.off; make lint 2>&1 | grep -c 'no compile database'; OPTIONAL_TOOLS=1 sh tests/test_style.sh >/dev/null; echo $?; mv build/pico.off build/pico   # expect: >= 1, then 0
for n in gpio_init gpio_set_dir gpio_set_function gpio_set_pulls gpio_pull_up gpio_pull_down gpio_disable_pulls gpio_set_oeover pio_gpio_init pio_sm_set_pindirs_with_mask pio_sm_set_consecutive_pindirs; do grep -rqE "\b${n}[a-z0-9_]*[[:space:]]*\(" "$PICO_SDK_PATH/src" --include='*.h' || echo "missing: $n"; done   # expect: no output
grep -c 'not yet checked against installed SDK headers' docs/constraints.md   # expect: 0
grep -c 'pico-sdk 2\.3\.1' docs/constraints.md                           # expect: >= 1
grep -c 'until 03-pio-bus' .clang-tidy tests/test_style.sh docs/constraints.md   # expect: 0 for each file
make typecheck                                                           # expect: exit 0
env -u PICO_SDK_PATH make typecheck 2>&1 | grep -c 'skip: firmware typecheck'   # expect: 1
printf 'int f( ) { return undeclared; }\n' > src/app/tc_probe.cpp; make typecheck >/dev/null 2>&1; echo $?; rm src/app/tc_probe.cpp; make firmware >/dev/null 2>&1   # expect: non-zero
scripts/check.sh typecheck                                               # expect: PASS, no `workflow gap:` line
python3 tests/test_rule_traceability.py                                  # expect: exit 0
python3 tests/test_checks_are_live.py                                    # expect: exit 0
sh tests/test_boundaries.sh                                              # expect: exit 0
mv build/pico build/pico.off; env -u PICO_SDK_PATH PATH=/opt/homebrew/opt/bash/bin:/usr/bin:/bin make test | tail -1; mv build/pico.off build/pico   # expect: OK — no SDK, no ARM compiler, no LLVM (bash >= 4 added to PATH; amended, see notes.md §Deviations)
make test                                                                # expect: last line OK
sh tests/test_phase_docs.sh                                              # expect: exit 0
```

If step 5 adds names to R-SAFETY-09, the name-resolution loop above gains them in the same
edit. That is the one criterion the implementation may extend.

## Out of scope

- Any PIO program, `.pio` file, bus code or pin configuration. `src/hal/` gains no committed
  file here. Owner: `24-pio-bus`, including adding `.pio` to `src_files( )`.
- The emulator binary and any second CMake target. Owner: `05-emulator`.
- USB HID and TinyUSB descriptors. USB stdio for the banner is the only USB. Owner:
  `08-usb-hid`.
- Upgrading the grep checks to `clang-query` with a compile database. It is check-harness
  work that the notes of 00, 01, 13, 15 and 16 attach to "03-pio-bus at the earliest", and
  it has no row. The `belay-debt:` comments in `tests/test_repo_shape.sh` that name
  `03-pio-bus` for it stay as they are. Repointing them is that row's job.
- Editing `.claude/rules/tech-debt.md`. After `/validate-phase` passes, propose the entry's
  update to the operator (both fix bullets are then done). Do not write it.
- A version floor for `cmake` or `picotool` in `tests/test_tool_versions.sh` (R-TOOL-01).
  None has been measured, and step 2 only learns whether the installed ones work.
- Scanning `.c` files under `src/`. No `.c` is added here, and the gap is recorded in
  R-CLEAN-03/05 and `docs/phases/15-scaffold-file-lists/notes.md`.
- Comments elsewhere that name `03-pio-bus` (`src/core/pins.h`, the `belay-debt:` headers).
  They stay, including in the files this phase edits, except in the passages Plan steps 4
  and 5 rewrite.
