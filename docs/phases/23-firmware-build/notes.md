# Phase 23-firmware-build — notes

## Outcome

- base: cad63a6
- `docs/adr/0014-pico-sdk-external-pinned.md`: the SDK is an external clone at tag 2.3.1,
  reached through `PICO_SDK_PATH`. Submodule, `FetchContent` and `PICO_SDK_FETCH_FROM_GIT` were
  rejected.
- `CMakeLists.txt` (new, repo root), which:
  - sets C++23, exports the compile database, and pins `PICO_CXX_ENABLE_EXCEPTIONS` and
    `PICO_CXX_ENABLE_RTTI` to 0 as normal variables;
  - holds `SG2HID_PICO_SDK_VERSION` (cache, default `2.3.1`), and a mismatch is a
    `FATAL_ERROR` naming both versions;
  - declares one executable, `sg2hid`, from a `CONFIGURE_DEPENDS` glob of `src/core`,
    `src/hal` and `src/app` `*.cpp`;
  - enables USB stdio and disables UART stdio, and emits `build/pico/sg2hid.uf2` (43 520
    bytes).
- `src/app/main.cpp` (new): `stdio_init_all( )`, then prints `pico-sg2hid: firmware build
  alive` once a second. It calls no GPIO or PIO function. It is not flashed yet; flashing is
  in `verify.md` §2.
- `tests/test_firmware_flags.sh` (new): R-ERR-05, read from
  `${COMPILE_DB:-build/pico/compile_commands.json}`.
  - It judges only the C++ entries under the repo's `src/`, with the last flag winning, and a
    database with no such entry fails.
  - Three rejection cases and one accept case, with a counted floor (4/4).
  - R-ERR-05 is rebound from `planned: 23-firmware-build` to that file, with a Scope clause.
- `tests/test_style.sh`: `src/hal`, `src/usb`, `src/app` and `src/emu` are linted with
  `clang-tidy -p build/pico` plus the ARM compiler's `-isystem` directories, derived at run
  time (`arm_isystem_args( )`, `tidy_sdk( )`).
  - With no database, or no ARM compiler, those files are dropped from the run and a distinct
    `skip_or_fail` line names the cause.
  - New rejection case: a scratch ARM compile database, `#include <optional>` and
    `int BadName;`.
  - `.clang-tidy`'s header, `tests/test_style.sh`'s header, the §Style paragraph and both
    §Observed conventions entries are rewritten to match.
- `Makefile`: after the core-header loop, `make typecheck` runs `make firmware` when
  `PICO_SDK_PATH` is set, and prints `skip: firmware typecheck: PICO_SDK_PATH unset`
  otherwise. The header comment is updated.
- R-SAFETY-09 was checked against the pico-sdk 2.3.1 headers: all 11 names resolve, and none
  is added (enumeration below). Its "not yet checked" sentence is replaced.
- `docs/phases/23-firmware-build/verify.md`.

### Acceptance criteria (run 2026-09-30)

- All pass, including the amended `make lint` exit code (see Deviations).
- Specific measured values:
  - vcheck: exit 1, and the log names both `0.0.0` and `2.3.1`;
  - mutated database: a FAIL line naming R-ERR-05, and exit 1;
  - SDK-layer lint probe: 1 `BadName` diagnostic and 0 `file not found`;
  - typecheck probe (`src/app/tc_probe.cpp`): exit 2;
  - `scripts/check.sh typecheck`: PASS.
- Results from other tests:
  - `tests/test_checks_are_live.py`: 87/87 alternations, every neutering caught;
  - `test_rule_traceability.py`, `test_boundaries.sh`, `test_repo_shape.sh` and
    `test_phase_docs.sh`: exit 0.
- `make test` results:
  - full run: `OK`;
  - bare run, with `PICO_SDK_PATH` unset, `build/pico` moved away and
    `PATH=/opt/homebrew/opt/bash/bin:/usr/bin:/bin`: `OK`. The new checks skip:
    - `skip: R-ERR-05: no compile database …`;
    - `skip: R-STYLE-02: no compile database for src/hal, …`;
    - `skip: R-STYLE-02 rejection case: ARM compile database …`.

### R-SAFETY-09 enumeration (pico-sdk 2.3.1, `src/**/*.h`)

Candidates are the functions named `gpio_set_*`, `gpio_init*`, `pio_gpio_*` or
`pio_sm_set_*`, or containing `pindir`.

Already matched by a listed name or starred prefix:
- `gpio_init`, `gpio_init_mask`, `gpio_init_mask64`;
- `gpio_set_dir`, and `gpio_set_dir_*`: `all_bits`, `all_bits64`, `in_masked`,
  `in_masked64`, `masked`, `masked64`, `out_masked`, `out_masked64`;
- `gpio_set_function`, `gpio_set_function_masked`, `gpio_set_function_masked64`;
- `gpio_set_oeover`, `gpio_set_pulls`, `pio_gpio_init`;
- `pio_sm_set_pindirs_with_mask`, `pio_sm_set_pindirs_with_mask64`,
  `pio_sm_set_consecutive_pindirs`.

Considered and rejected. None of these changes function, direction or output enable:
- `gpio_set_outover`: overrides the output *value*. It drives nothing unless OE is already
  on, and OE is covered by `gpio_set_oeover`, `gpio_set_dir*` and `gpio_set_function*`.
- `gpio_set_mask`, `gpio_set_mask64`, `gpio_set_mask_n`: SIO output values. These are the
  same kind as `gpio_put`, which the rule deliberately does not name.
- `pio_sm_set_pins`, `pio_sm_set_pins64`, `pio_sm_set_pins_with_mask`,
  `pio_sm_set_pins_with_mask64`: PIO output values, not pindirs.
- `gpio_set_inover`, `gpio_set_input_enabled`, `gpio_set_input_hysteresis_enabled`: input
  path only.
- `gpio_set_irqover`, `gpio_set_irq_enabled`, `gpio_set_irq_enabled_with_callback`,
  `gpio_set_irq_callback`, `gpio_set_dormant_irq_enabled`: interrupts.
- `gpio_set_drive_strength`, `gpio_set_slew_rate`: pad electrical settings. They only matter
  once a pin already drives.
- `pio_sm_set_out_pins`, `pio_sm_set_set_pins`, `pio_sm_set_sideset_pins`,
  `pio_sm_set_in_pins`, `pio_sm_set_jmp_pin`, `pio_sm_set_config`: these choose *which* pins
  a state machine's instructions address, and do not change a pin's direction by themselves.
  The direction change happens through `pio_sm_set_*pindirs*` or a `set pindirs` / `out
  pindirs` instruction, which is already recorded as unscanned.
- `pio_sm_set_clkdiv*`, `pio_sm_set_enabled`, `pio_sm_set_wrap`: timing and state.

## Deviations

- **`-Wall -Wextra -Werror` scope.**
  - Spec: `target_compile_options` on `sg2hid`, "the SDK's sources are not ours to fix".
  - Done: `set_source_files_properties( ${SG2HID_SOURCES} PROPERTIES COMPILE_OPTIONS … )`.
  - Why: the SDK's libraries are INTERFACE libraries, so their `.c` files (tinyusb and
    others) compile *inside* the `sg2hid` target. Target options would have put `-Werror`
    on them, which contradicts the spec's own reason.
  - Measured in the compile database: `-Werror` is present on all 5 `src/` entries and
    absent on `tusb.c`.
  - Checked for the same fact elsewhere in the spec: only Plan step 2 states it.
- **Acceptance criterion amended: the `make lint` exit code in the SDK-layer lint probe.**
  - The spec expected `1`. `make` exits `2` whenever a recipe fails, so `1` is unreachable
    through `make lint`.
  - The spec's criterion now reads `2`, with a pointer here. The other two expectations of
    that line (1 `BadName`, 0 `file not found`) are unchanged and met.
- **`tests/test_firmware_flags.sh` has no `skip_or_fail( )` function.**
  - Spec: go "through the `skip_or_fail` convention of `tests/test_style.sh`".
  - Done: the convention is inlined, as two branches of the real run.
  - Why: `tests/test_checks_are_live.py` neuters every function in a check file, and a
    function that runs only when no build exists is never exercised while one does. The
    harness reported it uncaught.
  - For the same reason, the cases carry a counted floor (`4/4`). A neutered `err05_case( )`
    printed nothing and passed until then.
- **An extra `skip_or_fail` line in `tests/test_style.sh`.** The line is `arm-none-eabi-g++
  not on PATH — src/hal, src/usb, src/app, src/emu not linted`, for a database present with
  no ARM compiler. The spec named only the no-database line. Without it, that state would
  fall back to `file not found` under the naming FAIL line, which is the defect the phase
  closes.
- **R-SAFETY-09's replaced sentence is longer than the spec's text.**
  - Spec: "checked against pico-sdk 2.3.1 headers on <date>".
  - Done: that phrase, plus what "checked" covered, a pointer to this enumeration, and one
    sentence saying that SDK convenience functions which configure pins internally (for
    example `stdio_uart_init_full`, which calls `gpio_set_function`) are neither named nor
    matched.
  - Why: that is a true scope limit of the rule, and the house convention is that a Scope
    clause names what the check does not see.
- **`src/app/main.cpp` formatting.** `clang-format` writes `int main()` and
  `stdio_init_all();` without inner spaces. Empty parentheses are the formatter's call, and
  R-STYLE-01 is the authority.
- **Acceptance criterion amended: the bare `make test` run's PATH.**
  - The spec ran it with `PATH=/usr/bin:/bin`. There it fails one case: `R-ARCH-02 rejection
    case did not fire … include-check.sh run by a bash older than 4`. That PATH finds macOS
    `/bin/bash` 3.2, and phase 22 recorded that the transitive-include hook needs bash 4 or
    newer.
  - This phase did not cause it: `tests/test_boundaries.sh` and `.claude/hooks/` are
    unchanged since `cad63a6`, and the same case fails there on its own under that PATH.
  - The criterion now adds only `/opt/homebrew/opt/bash/bin` (bash 5.3.15), which still
    leaves out the SDK, the ARM compiler and LLVM, and it passes.
  - Judged against the Goal: the Goal's clause is "`make test` still needs only a C++23
    compiler and `python3`" — *still*, relative to before this phase, which already needed
    bash 4. So the Goal holds. The requirement is documented, not a rule violation: see the
    bash entry in §For later phases.
- **Spec pointers.** Every file touched is named in the Plan or the Context pointers. No
  missing pointer.

## Debt

- The ARM `-isystem` list is word-split (`belay-debt:` in `tests/test_style.sh`,
  `arm_isystem_args( )`).
  - Ceiling: an ARM toolchain installed under a path containing a space breaks SDK-layer lint.
  - Upgrade: pass the directories one argument each, for example through a generated
    argument file, if a toolchain under such a path is ever used.
- R-ERR-05 and SDK-layer lint judge the *last configured build*. A stale
  `build/pico/compile_commands.json` from an older `CMakeLists.txt` is what gets checked
  until `make firmware` runs again. This is recorded in R-ERR-05's Scope clause.
  - Upgrade: have `make lint` / the check re-run the configure step when `CMakeLists.txt` is
    newer than the database.
- `make typecheck` builds the whole firmware (≈15 s from clean) instead of a syntax-only
  pass. This is accepted: it is the only typecheck the SDK layers have, and it is not part of
  `make test`.

## For later phases

- **24-pio-bus: generated PIO init helpers configure pins outside the R-SAFETY-09 grep.**
  - The conventional `<prog>_program_init( )` helper is written in a `.pio` file's
    `% c-sdk { }` block, which `pioasm` copies into a generated header under `build/`. It
    calls `pio_gpio_init` and `pio_sm_set_consecutive_pindirs`.
  - A call to such a helper from `src/app/` configures pins without any listed name
    appearing in a scanned file under `src/`, so `find_safety09( )` never sees it.
  - Keep those calls in `src/hal/`, and consider adding `_program_init` to the rule's name
    list when `.pio` joins `src_files( )`.
- **24-pio-bus: how the target handles warnings and sources.**
  - `-Wall -Wextra -Werror` apply per source, through `SG2HID_SOURCES`, not per target.
  - A new source added outside that glob (for example a generated file) gets no warnings
    unless it is added to the list.
  - `src/hal/*.cpp` is already in the glob. `pico_generate_pio_header( )` is not wired yet.
- **picotool: measured.** SDK 2.3.1 accepts the Homebrew picotool v2.3.0 (`Using picotool
  from /opt/homebrew/bin/picotool`, UF2 produced). Nothing was built from source.
- **Lint on the SDK layers needs a configured build.**
  - After `make clean`, `make lint` fails with `no compile database … run make firmware`
    until the firmware is built again, and `make test` reports it as `skip:`.
  - Any phase whose `verify.md` runs `make lint` after `make clean` must run
    `make firmware` first.
- **`.claude/rules/tech-debt.md` "No typecheck gate configured".** Both fix bullets are now
  done (22 did the first, this phase the second). After `/validate-phase` passes, propose
  closing that entry to the operator. The phase must not write it (§Out of scope).
- **needs a row: 22-scaffold-typecheck-transitive — bash 4 or newer is a documented
  requirement of `make test` that no check probes up front.**
  - The requirement is already recorded, and it is not a violation of R-PROC-04. R-PROC-04
    limits *toolchains* (compilers and build chains) to a C++23 compiler and `python3`. The
    suite already calls unlisted programs such as `sh`, `make` and `git`, and
    `docs/constraints.md` records both bash ≥ 4 (R-ARCH-02's scope clause) and `bash` among
    the tools that no check versions.
  - The gap: on stock macOS (`/bin/bash` 3.2), the first sign of the requirement is the
    R-ARCH-02 transitive-include rejection case failing mid-suite. Its message does name the
    cause.
  - Operator's decision, 2026-09-30: add a bash probe with a floor of 4 to R-TOOL-01 in
    `tests/test_tool_versions.sh`, next to the four existing probes. It needs a `LIVE` label,
    and the `rejected` floor rises if a rejection stub is added. Take bash from `PATH`, the
    interpreter `include-check.sh` gets.
  - Also on that row: the loose prose that says `make test` needs "nothing else"
    (`CLAUDE.md`, the §Style paragraph's R-PROC-04 wording).
  - Scheduled 2026-09-30 as `25-scaffold-bash-floor` (`/plan-feature`). This phase does not do it, because a version floor in
    `tests/test_tool_versions.sh` is outside its scope (§Out of scope, R-TOOL-01 bullet).
- **Not done here, still unowned:** the `clang-query` + `compile_commands.json` upgrade of the
  grep checks. The compile database now exists at `build/pico/compile_commands.json`, so that
  row, once planned, has its input. The row has not been planned yet.
