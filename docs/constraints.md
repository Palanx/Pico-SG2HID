# Constraints — pico-sg2hid

> **Engineering guidelines: transcribed 2026-08-31.** The operator's general engineering
> guidelines were read and written out here in full, self-contained prose — see
> §Engineering guidelines. No session needs any skill installed to comply, and no rule in
> this repo is stated as "follow <skill name>". What was mapped to an existing rule, and
> what was deliberately dropped as inapplicable to bare-metal firmware, is recorded there
> rather than left silent.

<!-- Standing rules: true for EVERY phase, indefinitely, until an ADR changes them.
       One-time decision with rationale  -> docs/adr/
       True in every session AND needed before reading anything else -> CLAUDE.md
       Standing rule a session consults when relevant -> HERE. -->

## Layering

Set by ADR-0002. The table is the human-readable form;
`.claude/workflow/boundaries.rules` is the enforced form. Change **both**, and only via
a superseding ADR.

| Layer | Directory | May depend on |
|---|---|---|
| core | `src/core/` | nothing (freestanding C++ standard library only) |
| hal | `src/hal/` | core |
| usb | `src/usb/` | core |
| app | `src/app/` | core, hal, usb |
| emu | `src/emu/` | core, hal |

Dependency rule: dependencies point inward toward `core`. `core` is pure logic and data
— it performs no I/O, reads no clock, allocates nothing, and includes no Pico SDK,
TinyUSB or CMSIS header, which is what lets the host tests compile and run it. `hal`
and `usb` are the only layers that touch hardware. `app` wires them together and is the
only layer that may depend on all three. `emu` is a separate firmware binary for the
second Pico; it reuses `core` and `hal` and must never reach `usb` or `app`.

Tests (`tests/`) and host tools (`tools/`) are outside the layering: they may read
anything, and nothing under `src/` may depend on them.

## Invariants

Every rule below carries an id and exactly one binding, per the grammar fixed in
**ADR-0005**:

- `test:` — a deterministic automated check at that path, which must contain a
  `RULE <id>` marker comment pointing back here.
- `manual:` — cannot be machine-checked; the reason is part of the rule.
- `planned: <phase-id>` — the test is owed. Tracked debt with an owner. The phase must
  exist in `docs/phases/PHASES.md` and must not be `done`.

`tests/test_rule_traceability.py` enforces the grammar and both directions of the
binding. Adding a rule here without a binding fails the build.

### Hardware safety

Set by ADR-0006 and by the hard constraints in `docs/product/requirements.md`.

- **R-SAFETY-01** — The Pico must never configure `DATA` or `ACK` as a push-pull output; both are open-drain lines driven by the controller and read by us. — planned: 02-wiring
- **R-SAFETY-02** — Every GPIO used by this project is declared exactly once in `src/core/pins.h` with its signal name, direction and drive mode; no file outside `src/hal/` may call `gpio_init`, `gpio_set_dir` or `gpio_pull_up`, and `src/hal/` configures pins only by iterating that table. — planned: 02-wiring
- **R-SAFETY-03** — No entry in a pin table may use a GPIO the Pico board reserves (23, 24, 25, 29), and no two signals in one table may share a GPIO. — planned: 02-wiring
- **R-SAFETY-04** — The 7.6 V rail on the PS2 connector is never connected to anything. — manual: a property of physical wiring, not of any artifact the build can read; verified by the phase `02-wiring` multimeter checklist before power is ever applied.
- **R-SAFETY-05** — The guitar is powered only from the Pico's 3V3 pin, never from VBUS or VSYS. — manual: physical wiring; verified by the phase `02-wiring` checklist, which measures the rail before the guitar is connected.
- **R-SAFETY-06** — In any two-Pico setup, the master's pin table and the emulator's pin table must not both declare a push-pull output for the same signal. — planned: 05-emulator
- **R-SAFETY-07** — A bus frame always releases `ATT` when it ends, including on every error and timeout path; no code path may leave `ATT` asserted. — planned: 03-pio-bus
- **R-SAFETY-08** — Firmware is never flashed to a Pico while the guitar is connected, and the guitar is connected only after the firmware is running and has been verified against the emulator. — manual: an ordering of physical acts; enforced by the per-phase `verify.md` procedure, which the build cannot observe.

### PS2 protocol

- **R-PROTO-01** — All bytes on the PS2 bus are transferred LSB-first, SPI mode 3. Rebound from `01-ps2-codec` to `03-pio-bus` on 2026-09-11: bit order is a property of the PIO shift program, so no host-only test over decoded bytes can observe it — a codec handed already-assembled bytes behaves identically either way. The loopback round-trip in `03-pio-bus` is the first thing that can. — planned: 03-pio-bus
- **R-PROTO-02** — A frame the bus cut short never yields a partially decoded report: the decode reports the abort and the link transitions to `Absent`, and no field of a refused frame is readable as if it had been decoded. Split on 2026-09-11: this clause is decidable from the bytes alone, so it is testable on the host, while the timing clause it used to carry is not. The timing clause is now R-PROTO-06 — a binding covering both was a binding for neither, the same shape as R-ERR-05's split out of R-ERR-03. **Scope, recorded 2026-09-22, corrected 2026-09-22 after the first wording was measured false — this check is vector-driven, so its reach is its vector list, not a file list and a regex:** the check decodes **three** cut inputs, enumerated by grepping every truncated span in `tests/ps2_codec_cases.cpp`. Two are three bytes of a four-byte **digital** frame — `tests/vectors/truncated_ack.h` and `tests/vectors/truncated_not_ready.h`, differing only in the ready slot. The third is **one byte** (`kReadyIndex` is 1) of the twenty-byte undeclared-id frame `tests/vectors/unknown_id.h`, which is where §Goal's precedence item 1 is asserted: the abort outranks `UnknownId`. **Not exercised:** a cut in an **analog** (`0x73`) or **config-mode** (`0xF3`) frame, and every intermediate length — nothing between 1 and 3 bytes, and nothing between 3 and a whole frame. On the link side, `cut_drops_the_link_from_every_source` constructs **three** links (`DigitalStreaming`, `AnalogStreaming`, `Negotiating`); the fourth `LinkState` member, `Absent`, is covered elsewhere in `rule_proto02` by a freshly constructed `Link`, but not by that function. The over-long buffer is a separate unasserted behaviour, declared in `docs/phases/01-ps2-codec/spec.md` §Goal. — test: `tests/test_ps2_codec.py`
- **R-PROTO-03** — A frame whose header byte is not a known controller id is reported as unknown, never guessed at or decoded on a best-effort basis. **Scope, recorded 2026-09-22 — vector-driven, so the reach is the vector list:** this project declares three ids (`0x41`, `0x73`, `0xF3`), leaving 253 undeclared header bytes, and the check exercises exactly one of them — `0x79`, in `tests/vectors/unknown_id.h`, chosen as a real DualShock 2 id rather than an impossible byte so that the refusal is not an accident of the value. That one byte stands for the other 252; nothing measures them. — test: `tests/test_ps2_codec.py`
- **R-PROTO-04** — The whammy axis is read only from a frame that reported analog mode. A digital-mode frame yields the axis at rest, never a byte reinterpreted from a digital report. **Scope, recorded 2026-09-22 — vector-driven, so the reach is the vector list:** exercised are analog at rest and at full deflection (`analog_idle.h`, `analog_whammy_full.h`), a digital frame (`digital_whammy_absent.h`) and a config-mode frame (`config_mode.h`). Exactly **two** whammy bytes are exercised: `0x80`, which `kWhammyRest` names and `analog_idle.h` carries as a centred axis, and `0xFF` at full deflection. `0x80` is the **midpoint** of the `0x00..0xFF` range `src/core/guitar_state.h` documents, not an end of it — so the unexercised region is not merely the gap between the two values, it is every value below rest as well as every value between rest and full. No vector came off a real controller, which is `09-guitar-observe`'s work. — test: `tests/test_ps2_codec.py`
- **R-PROTO-05** — Every expected sequence of bytes **the protocol fixes** — what the controller puts on the wire — is a literal, stored under `tests/vectors/` and written by hand from the protocol documentation. No expected value is produced by `core` or captured from the emulator, and no file under `src/` may reference `tests/vectors/`. **"Reference" means any occurrence of the path, comments included**, and the check reads raw lines rather than comment-stripped ones to match: scope decided 2026-09-15 after the acceptance criterion (a plain `grep`) and the check (which stripped `//`) returned opposite verdicts on the same tree. The literal reading is chosen over "no code reference" because it leaves nothing to interpret — no one has to rule on whether a string literal or a macro counts. What is given up is stated rather than implied: a comment cannot create a dependency, so this check refuses some references that are harmless, and that over-strictness is the price of having no judgement in the rule. **It is also under-strict, recorded 2026-09-16, and the paragraph above stated only the first half:** an occurrence means the literal text `tests/vectors`. Measured one form each: `#include "vectors/a.h"` resolved through an `-Itests` include path, a path split across adjacent string literals (`"tests/" "vectors/a.h"`), a doubled separator (`tests//vectors`) and a backslash separator are not occurrences and are not reported; a relative `../../tests/vectors/a.h` is. The first is the one that matters, because it is a real dependency on test data containing no occurrence of the path. Nothing in the build passes `-Itests` today — `make test` and `tests/test_ps2_codec.py` both compile with `-Isrc` only. **What this rule does not reach, ruled 2026-09-17:** a value this project decides rather than the protocol. `decode`'s zero fill is the case — `tests/ps2_codec_cases.cpp` asserts the unannounced payload bytes against its own `kZeroFill`, and no vector could hold that byte, because the controller never sent it. Expected bytes that came off the wire stay under `tests/vectors/`. — test: `tests/test_repo_shape.sh`
- **R-PROTO-06** — After every byte of a frame except the last, the master waits for the controller's `ACK` within the timeout before shifting the next byte. Split out of R-PROTO-02 on 2026-09-11: this is a property of what the bus does between bytes, observable only by something that drives a real clock, and nothing in `src/core/` produces or consumes a microsecond. — planned: 03-pio-bus

### Style

Layout is `clang-format`, naming is `clang-tidy`; they are different tools because they
solve different problems, and neither does the other's job.

- **R-STYLE-01** — Every `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` tracked or newly added under this repo matches `.clang-format`. — test: `tests/test_style.sh`
- **R-STYLE-02** — Naming matches `.clang-tidy`: private and protected members carry the `m_` prefix, types are `CamelCase`, functions and variables are `lower_case`, compile-time constants are `kCamelCase`, macros are `UPPER_CASE`. **Scope, recorded 2026-09-23 because the check reads less than this text says:** the check runs `clang-tidy` over `tidy_sources()` only — `src/*.cpp`, `src/*.h`, `src/*.hpp`, `src/*.cc`, `src/*.inl` and `tests/*.cpp` (`tests/test_style.sh`), git pathspecs read recursively as R-CLEAN-04 records, so every layer under `src/` is reached — plus any header under `src/` or `tests/` that one of those files includes (`HeaderFilterRegex` in `.clang-tidy`). Under `src/` the only file extension left unchecked is `.c`; it, every C++ file outside `src/` other than `tests/*.cpp` and an included header, and every non-C++ file are unchecked: a name in those files that breaks this rule is reported by nothing. — test: `tests/test_style.sh`

Both are checked over `src/` and `tests/` by `make lint`, which fails if either tool is
missing. `make test` runs the same script with `STYLE_OPTIONAL=1`, which downgrades a
missing tool to a skip — that is what keeps R-PROC-04 true (host tests need only a C++23
compiler and `python3`), at the cost that a machine without LLVM installed does not
enforce style. `make lint` is the authority.

R-STYLE-02's scope is every `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` under `src/` plus
`tests/*.cpp`. clang-tidy
needs to know how to compile each file, so a file under `src/hal`, `src/usb`, `src/app` or
`src/emu` that includes a Pico SDK or TinyUSB header fails lint with
`clang-diagnostic-error` until `03-pio-bus` supplies its include flags — a loud failure, not
a skipped file. The upgrade path is in the header of `.clang-tidy`.

That scope is necessary but **not sufficient, and this paragraph used to imply it was**:
the invocation also needs `-xc++` on a `.h` or `.inl` and `-isysroot` on every file, or a file using
the standard library fails R-STYLE-02 with a diagnostic that is not about naming at all. Both
flags are in `tests/test_style.sh`, which is the file that fails when either is missing;
what each one fixes, and what the two earlier readings of this got wrong, is in §Observed
conventions, 2026-09-11 (superseding the 2026-09-02 entry).

### Architecture

- **R-ARCH-01** — `src/core/` contains no `#include` of a Pico SDK, TinyUSB, CMSIS or other hardware header, and no `#include <` of a hosted-only standard header. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_arch01( )` matches through `hits( )` over `core_files( )` (`tests/test_repo_shape.sh`), and its regex is a list of path prefixes (`pico/`, `hardware/`, `tusb`, `device/`, `class/`, `cmsis`, `core_cm`) and of named hosted headers, not a definition of "hardware" or "hosted-only". Measured one fixture line per form: `# include "pico/stdlib.h"` (space after `#`), `#include "pico.h"`, `#include "RP2040.h"`, `#include <stdio.h>`, `#include <cstdio>`, `#include <cstdlib>`, `#include <chrono>`, `#include <atomic>`, `#include <functional>` and `#include HEADER_MACRO` are all outside the binding and none is reported. Whether each of those headers is actually forbidden is a judgement the regex never makes: a header absent from the list passes whatever it is. — test: `tests/test_repo_shape.sh`
- **R-ARCH-02** — Every file under `src/` respects the dependency directions in §Layering, as encoded in `.claude/workflow/boundaries.rules`. **Scope, recorded 2026-09-24 because the check reads less than this text says:** the check is `sweep( )` in `tests/test_boundaries.sh`, which hands each path `find "$root/src" -type f` prints, one per line and unsplit, to `.claude/hooks/boundary-check.sh`. The hook flags a line only when it matches `IMPORT_RE` — a keyword such as `#include` at the start of the line followed by whitespace or `(` — and names the denied layer by its full prefix (`src/hal/`) or by its directory name preceded by `/`, `"`, `'` or whitespace (`"hal/`). Measured one fixture per form, in `src/core/x.cpp`, against the real hook and an extracted `sweep( )`: `#include <hal/bus.h>`, `# include "hal/bus.h"`, a tab between `#` and `include`, `#include"hal/bus.h"`, `#include_next "hal/bus.h"`, an include through a macro (`#define HAL_HEADER "hal/bus.h"`, then `#include HAL_HEADER`), an include split by a line continuation, a C++20 `import hal.bus;`, and a transitive include (`src/core/x.cpp` includes `common/y.h`, which includes `hal/bus.h`) are all outside the binding and none is reported. A file under `src/` in no declared layer, such as `src/common/y.h`, is never judged: the hook exits 0 before reading it. `sweep( )` reads `find`'s output one line per path, so a file whose name contains a newline is split into two paths that do not exist and is not reported (measured 2026-09-24 with `src/core/a<newline>b.cpp` including `hal/bus.h`). A layer directory that is a symlink is not descended by `find` and is not reported either. The hook also errs loudly: an include inside `#if 0` is reported. Closing these needs a preprocessor-aware include graph, which the hook's own `belay-debt:` header defers to a project-wide resolver. — test: `tests/test_boundaries.sh`
- **R-ARCH-03** — Firmware code performs no dynamic allocation: no `new`, `delete`, `malloc`, `free`, `std::vector`, `std::string` or `std::function` anywhere under `src/`. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_arch03( )` matches through `hits( )` over `src_files( )` (`tests/test_repo_shape.sh`) and recognises `new` or `delete` followed by whitespace and a name, `malloc(`, `free(`, `std::vector`, `std::string`, and `std::function`, one line at a time. Measured one fixture line per form: `delete[] p;`, a placement `new (buf) T`, `::operator new( 4 )`, `calloc( 1, 4 )`, `realloc( p, 8 )`, `std::make_unique<T>( )`, `std::make_shared<T>( )`, `std::wstring`, `std::u8string`, `std::map<int, int>`, and `new` with its type on the next line are all outside the binding and none is reported. `using std::vector; vector<int> v;` on one line is reported, but only because the `using` itself names `std::vector`. — test: `tests/test_repo_shape.sh`

### Error model

Set by ADR-0007.

- **R-ERR-01** — No function in `src/core/` returns a bare status enum: `DecodeStatus` and `FaultCause` appear only in `std::expected`'s error slot or as a member of `Link`, never as a return type of their own. Narrowed on 2026-09-11 from "reports through a result struct or through the link state machine, never a status alongside a separate out-parameter": the original text names a hazard whose subject is the *pairing* of a return with an out-parameter, and deciding that from source needs to know which parameter carries the value — which a grep cannot. The narrowed clause is the half that makes the hazard unwritable, because a status that is never returned alone cannot be returned beside anything. What is given up is recorded rather than implied: a `core` function taking a non-const reference and returning `void` is outside this binding, and R-CLEAN-07 (`manual:`) is what covers it. **A spelling narrowing, recorded 2026-09-16:** the check is a line-anchored grep, so it sees a status-enum return only when the line starts with the enum's unqualified name — optionally after `[[nodiscard]]` and `constexpr` — with the function name and `(` on the same line. Measured by feeding it one declaration per form: `static`, `inline`, `extern`, `friend` or `const` before the type, a qualified `ps2::FaultCause`, a trailing `-> FaultCause` return, and a return type on a line of its own are all outside this binding, and none is reported. None of those forms returns a status enum anywhere in `src/core/` today. Closing it needs the type information a grep does not have, which is the `clang-query` + `compile_commands.json` upgrade `03-pio-bus` already owes. — test: `tests/test_repo_shape.sh`
- **R-ERR-02** — Every function returning a result struct or a `LinkState` is marked `[[nodiscard]]`. The check behind this rule scans `.h` and `.hpp` under `src/core/` only, and the narrowing is recorded here rather than left in the check, the way R-ERR-01's is: `[[nodiscard]]` belongs on the declaration and C++ does not repeat it on the out-of-line definition, so scanning `.cpp` reports correct code — `LinkState step( … ) {` in `src/core/link.cpp` — as a violation. `.cc` and `.inl` are not scanned for the same reason as `.cpp`: both hold out-of-line definitions, so a declaration placed in a `.inl` is outside this binding. What is given up is stated rather than implied: a function defined only inside an anonymous namespace in a `.cpp`, declared in no header, is outside this binding. Such a function has no caller who could ignore its result, which is why the exposure is small and not zero; closing it needs the `clang-query` + `compile_commands.json` upgrade `03-pio-bus` already owes. Measured 2026-09-11 against the real tree, not reasoned. **A second narrowing, recorded 2026-09-15:** the check recognises three spellings of a return type — `std::expected<…>`, `DecodeOutcome` and `LinkState` — while this rule says "result struct", which is wider. A fallible return spelled any other way is outside the binding, and one exists: `id_from_byte` in `src/core/ps2_protocol.h` returns `std::optional<ControllerId>` and matches none of the three. It carries `[[nodiscard]]` today, so nothing is in violation — but the check would not notice if it stopped, which is precisely what a scope clause is for. Widening the pattern to cover every fallible return needs the type information a grep does not have, which is the same `clang-query` upgrade `03-pio-bus` already owes. **A third narrowing, recorded 2026-09-16, on the same axis as the second:** the three spellings are recognised only at the start of a line — optionally after `constexpr` — with the function name and `(` on the same line. Measured one declaration per form: `static`, `inline` or `const` before the type, a qualified `ps2::LinkState`, a trailing `-> LinkState` return, a return type on a line of its own, and a `std::expected<…>` split across lines are all outside the binding and none is reported. None of those forms returns one of the three types anywhere in `src/core/` today. The anchor also errs in the other direction: `[[nodiscard]]` on the line *before* a compliant declaration is reported as a violation. That one fails loudly rather than passing quietly, so it constrains how a declaration may be laid out rather than hiding one. Same upgrade path. — test: `tests/test_repo_shape.sh`
- **R-ERR-03** — No `throw`, `try` or `catch` anywhere under `src/`. **Scope, recorded 2026-09-23 because the check reads less than this text says:** `find_err03( )` matches through `hits( )` (`tests/test_repo_shape.sh`), which cuts each line at the first `//` outside a string or character literal (`strip_line_comments( )`) and strips nothing else — string contents are kept, not blanked. A `throw`, `try {` or `catch (` inside a `/* … */` comment or a string literal **is reported** — a loud false positive. `strip_line_comments( )` also takes every `'` as a character-literal quote, so after a C++14 digit separator (`1'000`) or an apostrophe in a `/* … */` comment it can cut a line at a `//` inside a real string and miss the code after it (`int n = 1'000; u = "a'"; v = "http://x"; throw E;` is not reported, measured 2026-09-23). **Its regex, measured 2026-09-24:** `find_err03( )` recognises `throw` as a word, `try` followed by `{` and `catch` followed by `(`, each on one line. Measured one fixture line per form: `try` with `{` on the next line, `catch` with `( ... )` on the next line, `std::rethrow_exception( e );` and `std::throw_with_nested( e );` are all outside the binding and none is reported. A function-try-block written `void f( ) try {` on one line is reported. — test: `tests/test_repo_shape.sh`
- **R-ERR-05** — Firmware builds pass `-fno-exceptions -fno-rtti`. Split out of R-ERR-03 on 2026-08-31: the source clause is a grep and the flags clause is a property of a build that does not exist yet, so one binding could not honestly cover both. — planned: 03-pio-bus
- **R-ERR-04** — No call to `.value()` on a `std::expected` anywhere under `src/`. Under `-fno-exceptions` it does not throw, it calls `abort` — the one outcome §Error handling rules out. Access goes through `has_value()` and `operator*`. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_err04( )` matches through `hits( )` over `src_files( )` (`tests/test_repo_shape.sh`) and reports `.value` then `(` on one line, whatever the object's type. Measured one fixture line per form: `r->value( )`, `r . value( )` (space before `value`) and `.value` with `( )` on the next line are all outside the binding and none is reported. The check has no type information, so the other direction also holds: `opt.value( )` on a `std::optional` is reported — a loud false positive — as is any `.value( )` on anything; `value_or( … )` is not. — test: `tests/test_repo_shape.sh`

### Toolchain

Set by ADR-0008. Versions are a rule here and not a README sentence because this project
has already been bitten twice by them: `.clang-format` keys were renamed in clang-format
17 and again in 23, and the ARM compiler decides which C++ standard is even available.

- **R-TOOL-01** — Every tool the checks depend on meets its minimum version when present: `clang-format` >= 23 and `clang-tidy` >= 23 (the config uses key shapes introduced in 23), `arm-none-eabi-g++` >= 12 (the release in which libstdc++ gained `<expected>`; only 15.3.1 is actually verified — see ADR-0008 §Verification), `python3` >= 3.8. A tool that is absent is skipped under `OPTIONAL_TOOLS`, never assumed to pass. **Scope, recorded 2026-09-24 because the check reads less than this text says:** the check is four `probe` calls through `check_version( )` in `tests/test_tool_versions.sh`, one per tool this text names. Tools that `make test` and the other checks also call and that no probe versions: the C++ compiler `$(CXX)` (default `c++`, which must accept `-std=c++23`), `git`, `bash` (the boundary hook's interpreter) and `gitleaks` — the last on purpose, because `tests/test_secrets.sh` probes its subcommands instead of a floor nobody has measured. `ver_num( )` reads, in the banner's first line, the first `N` or `N.N` after the first occurrence of the word `version` when the line contains it, and the first `N` or `N.N` in the line otherwise, as major*100+minor, so patch numbers and pre-release suffixes are ignored and a floor cannot carry a patch number. Measured with stub binaries: `Python 3.8.0rc1` against a floor of `3.8.1` and a tool that prints a passing banner and exits 1 both pass. `resolve( )` also looks in the keg-only LLVM prefixes, while R-STYLE-01's branch of `tests/test_style.sh` finds `clang-format` with `command -v` only: measured on this machine with `PATH=/usr/bin:/bin`, `check_version( )` passes the keg's clang-format 23.1.0, which R-STYLE-01's lookup does not find, so the binary whose version is checked is not always the one that runs. — test: `tests/test_tool_versions.sh`
- **R-TOOL-02** — The `arm-none-eabi-g++` first on `PATH` can compile a translation unit that includes `<cstdint>` for `cortex-m0plus`. A cross-compiler with no target C library looks installed and cannot build anything; the sudo-free Homebrew formula is exactly that, and it shadows the working cask. macOS is the only development platform (ADR-0010), so this names the trap that exists rather than one platform's among several. **Scope, recorded 2026-09-24 because the check reads less than this text says:** the check is `arm_compiles( )` in `tests/test_tool_versions.sh`, which compiles one fixed translation unit (`#include <cstdint>` and a one-line function) with `-mcpu=cortex-m0plus -mthumb -c` and takes the compiler's exit status as the verdict. It never links and never reads the object it wrote. Measured with stub compilers: one that succeeds on `-c` and fails any link, one that compiles `<cstdint>` and fails on `<expected>`, and one that exits 0 and writes an empty object whatever its flags are all pass. The probe passes no `-std=` on purpose — the file's comment explains that the spelling would fail a compliant GCC 12 — so whether the compiler provides `<expected>`, the reason R-TOOL-01's floor is 12, is not checked by this rule. — test: `tests/test_tool_versions.sh`

### Engineering guidelines

Transcribed from the operator's general engineering guidelines. Rules already implemented
above are **mapped**, not duplicated; guidelines that do not apply to this project are
**dropped by name** with the reason. Both lists follow the rules.

**Clean code**

- **R-CLEAN-01** — Names reveal intent and use domain language: what the thing is on the bus, not what it is in the abstract. Abbreviations only where universally known (`id`, `url`, `api`, `usb`, `pio`, `gpio`, `hid`, `ack`). A name that needs a comment to explain it is renamed instead. — manual: intent is a judgement about meaning; no checker can tell a good name from a bad one.
- **R-CLEAN-02** — A function is at most 60 lines, takes at most 3 parameters, and nests at most 4 deep. Past three parameters the arguments become a struct. **Scope, recorded 2026-09-23 because the check reads less than this text says:** the check is `readability-function-size` in the same `clang-tidy` run as R-STYLE-02, over `tidy_sources()` only — `src/*.cpp`, `src/*.h`, `src/*.hpp`, `src/*.cc`, `src/*.inl`, `tests/*.cpp` and the headers under `src/` or `tests/` they include (see R-STYLE-02 and R-CLEAN-04). Under `src/` the only file extension left unchecked is `.c`, and every function not written in C++ is unchecked too: shell functions in `tests/*.sh` and Python functions in `tests/*.py` are never measured for length, parameters or nesting. — test: `tests/test_style.sh`
- **R-CLEAN-03** — Boolean names are assertions: `is_`, `has_`, `can_`, `should_`. Never `flag`, `status`, `check`. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_clean03( )` matches through `hits( )` over `src_files( )` (`tests/test_repo_shape.sh`): a `bool` followed by one name and then `;` or `=`, with any line containing an `is_`/`has_`/`can_`/`should_` (optionally `m_`) bool dropped whole. The rule constrains every boolean name; the check sees only simple declarations of type `bool`. Measured one fixture line per form: `bool flag{ true };`, `bool flag( true );`, `bool a, flag;`, a parameter `void f( bool flag );`, a function `bool check( );`, `std::atomic<bool> flag;`, `auto flag = true;`, a bit-field `bool flag : 1;` and `bool is_ok = true; bool flag = false;` on one line are all outside the binding and none is reported. **File scope:** the rule is written for the whole repository and the check reads `src_files( )` only — `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` under `src/`. Everything outside `src/` and every `.c` under it is unchecked; measured, `bool flag;` in `tests/x.cpp` and in `src/core/x.c` is not reported. — test: `tests/test_repo_shape.sh`
- **R-CLEAN-04** — No magic numbers or strings in logic. Every protocol byte, timeout and threshold is a named `constexpr` in one place per concern. The literals under `tests/vectors/` are the sole exception, and being literal is their purpose (R-PROTO-05). Enforced by `readability-magic-numbers`, which does not see a literal inside a `const` or `constexpr` initializer — the gap is measured and recorded in §Observed conventions, 2026-09-11, rather than left for a reader to assume away. **Scope, recorded 2026-09-15 because the check reads less than this text says:** the rule is written for the whole repository, and the check runs `clang-tidy` over `tidy_sources()` only — `src/*.cpp`, `src/*.h`, `src/*.hpp`, `src/*.cc`, `src/*.inl` and `tests/*.cpp` (`tests/test_style.sh`; the three middle patterns added 2026-09-23). Anything outside those six patterns is unchecked; under `src/` that leaves `.c` as the only unchecked file extension. **They are git pathspecs, not shell globs** (recorded 2026-09-17): `git ls-files` lets `*` cross `/`, so each is read recursively — `tests/*.cpp` would pick up `tests/fixtures/x.cpp`. The set is therefore slightly wider than it reads, never narrower; measured with `git ls-files 'docs/*.md'`, which returns 38 files in subdirectories. There is no second narrowing from `.clang-tidy`: `HeaderFilterRegex` is `(src|tests)/.*`, so a header under `tests/` reached as an include is diagnosed. `01-ps2-codec` briefly narrowed it to `src/.*` and reverted that on 2026-09-17 — measured, the narrowing did not protect the vectors (their literals are `constexpr` initializers this check never sees, above) and it silenced two other phases' bindings and made lint's reach depend on the checkout path, the regex being unanchored and matched against the absolute path. **What this rule's `tests/vectors/` exception rests on is therefore the `constexpr` blindness measured above, not the header filter** — a non-`constexpr` magic number written into a vector would be diagnosed, and that is the correct outcome rather than a hole. — test: `tests/test_style.sh`
- **R-CLEAN-05** — Every `TODO` names the phase or issue that will close it: `// TODO(09-guitar-observe): confirm against the real controller`. A bare `TODO` is not allowed. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_clean05( )` matches through `raw_hits( )` over `src_files( )` (`tests/test_repo_shape.sh`) — raw lines, comments kept, because the subject is a comment — and reports a `TODO` not immediately followed by `(`. It checks that a parenthesis is present, not what it names. Measured one fixture line per form: `// TODO(): fix`, `// TODO(later): fix`, lower-case `// todo: fix` and `// FIXME: fix` are all outside the binding and none is reported. `// TODO (09-guitar-observe): x`, with a space before the parenthesis, is reported — loud, not quiet. **File scope:** the rule is written for the whole repository and the check reads `src_files( )` only — `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` under `src/`. Everything outside `src/` and every `.c` under it is unchecked; measured, a bare `TODO` in `tests/x.sh`, `tests/x.cpp` and `src/core/x.c` is not reported. — test: `tests/test_repo_shape.sh`
- **R-CLEAN-06** — Comments explain *why*, never *what*; a comment that no longer matches the code is deleted or corrected, never left standing. — manual: whether a comment is still true is exactly the judgement a checker cannot make.
- **R-CLEAN-07** — Command-Query Separation: a function returns a value or changes state, never both. — manual: distinguishing a query from a command requires knowing intent, not signature.
- **R-CLEAN-08** — Constructors only assign. No logic, no I/O, no side effects. — manual: "logic" has no syntactic definition; a grep would flag every non-trivial initialiser list.
- **R-CLEAN-09** — Composition over inheritance. `src/core/` uses no inheritance and no virtual dispatch at all: it is plain data plus free functions. **Scope, recorded 2026-09-24 because the check reads less than this text says:** `find_clean09( )` matches through `hits( )` over `core_files( )` (`tests/test_repo_shape.sh`): an access specifier after `:`, `struct`/`class` + one name + `:` on the same line, or the word `virtual`, with any line containing `enum class` dropped. Measured one fixture line per form: `struct A final : B { };`, `struct alignas( 4 ) A : B { };`, `struct [[nodiscard]] A : B { };`, `struct A` with `: B { };` on the next line, and a bare `void poll( ) override;` are all outside the binding and none is reported. `enum struct E : std::uint8_t { };` is reported although it is not inheritance — a loud false positive; `enum class` with the same base type is not. — test: `tests/test_repo_shape.sh`

**SOLID**

- **R-SOLID-01** — Every module has one reason to change. A design decision that violates a SOLID principle is flagged as a deliberate tradeoff with an alternative proposed, never taken silently. — manual: "one reason to change" is a claim about the future, not a property of the source.
- **R-SOLID-02** — Depend on abstractions, not concretions: where a layer needs something from a layer further out, the interface is owned by the inner layer and the dependency is injected, never constructed inside the consumer. — manual: the layering test proves the *direction* of every edge; whether the seam is an abstraction is a design judgement on top of that.

**Security**

- **R-SEC-01** — Zero secrets in the repository: no credentials, tokens or keys in source, config or history. **Scope, recorded 2026-09-24 because the check reads less than this text says:** the check is `scan( )` in `tests/test_secrets.sh`, which runs `gitleaks dir` over the working tree and `gitleaks git` over the history, both with gitleaks' default ruleset (measured with gitleaks 8.30.1), so "zero secrets" means "nothing a default gitleaks rule matches". Measured one fixture per form in a scratch repository, each token built at runtime: a low-entropy `password = "…"` assignment, a 16-character password that contains punctuation (`!`, `#`, `@`), a GitHub token split across two adjacent string literals assigned to a neutrally named variable, a token committed and then reset away so no ref reaches it, a token only in a commit message, and a token only in an annotated tag's message are all outside the binding and none is reported. The history scan does reach every ref — a token only in `git stash`, only on an unmerged branch or only in a `git notes` entry is reported — and the tree scan reads `.gitignore`d files. One reach item needs no fixture: without gitleaks on `PATH`, `make test` (which sets `OPTIONAL_TOOLS=1`) prints `skip: R-SEC-01: gitleaks not found` and exits 0, so on such a machine the rule is not checked at all; only running `sh tests/test_secrets.sh` directly fails. — test: `tests/test_secrets.sh`

**Process**

- **R-PROC-05** — Never commit, push, force-push, install a dependency, delete files, or change a public contract without explicit operator approval. — manual: a rule about what the agent does, observable only in the transcript, never in the tree.
- **R-PROC-06** — A decision with several valid paths — pattern choice, sync vs async, caching, error-handling approach — is surfaced with options and tradeoffs and waits for an answer. It is never picked silently. — manual: same; the artifact records the choice, not whether it was offered.
- **R-PROC-07** — Work happens on a prefixed branch (`feature/`, `fix/`, `refactor/`, `chore/`); never directly on `main` without explicit consent. — manual: the branch name is checkable, but a test asserting it would fail on every fresh clone and on `main` itself, which makes it a check that has to be ignored — worse than none.
- **R-TEST-01** — New behavior requires a new test. A phase does not close with behavior nothing exercises. — manual: "new behavior" is not derivable from a diff; the per-phase acceptance criteria carry this instead.

**Mapped, not duplicated**

| Guideline | Already carried by |
|---|---|
| Domain layer has zero framework imports | R-ARCH-01, ADR-0002 |
| Dependencies point inward only | R-ARCH-02, §Layering |
| Validate all inputs at system boundaries | R-PROTO-03 — the PS2 bus is this project's only trust boundary, and an unknown controller id is refused rather than guessed |
| Conventional Commits, authored messages | R-PROC-03 |
| Arrange-Act-Assert, one assertion concept per test | §Testing |
| Errors are domain citizens; never swallowed | §Error handling — `core` returns typed result values, a bus fault is a normal outcome, and no path is silent |
| Test pyramid: unit majority, integration at boundaries | §Testing — host tests are the unit tier, Pico-against-Pico is the integration tier, the real guitar is the E2E tier and is manual by design (ADR-0004) |

**Dropped, with the reason**

- **Repository pattern, Use Case / Interactor classes, DTOs at layer boundaries, Dependency Injection containers.** These solve the problem of an application with a data-access layer, a request lifecycle and swappable infrastructure. This is a single-binary bare-metal firmware with no database, no network, no request, and one hardware peripheral reached through one PIO program. Applying them here would produce ceremony with no seam behind it. The part of Clean Architecture that *does* apply — the dependency rule and a domain that knows nothing about the outside world — is ADR-0002 and is enforced.
- **Modular monolith with per-module data access and event-based cross-module communication.** Same reason: there are five layers and no modules to federate. The boundary rules already forbid every edge that would matter.
- **"Fail fast, fail loud in development; silence in production."** Inverted here on purpose, and it is worth being explicit: firmware that stops is worse than firmware that reports a wrong byte, because the host is left holding a HID device that silently stopped answering. Every error path returns to "controller absent, retry negotiation". See §Error handling.

### Process

- **R-PROC-01** — Every rule in this file has exactly one binding in ADR-0005's grammar; every `test:` file carries a matching `RULE <id>` marker; every marker names a declared id; `manual:` rules carry no marker; every `planned:` phase exists and is not `done`; every id `CLAUDE.md` mentions is declared here. **Scope, recorded 2026-09-24 because the check reads less than this text says:** the check is `check( )` in `tests/test_rule_traceability.py`. It sees a rule on any line matching `ANY_RULE_LINE` — `- **R-` or `* **R-` after optional spaces or tabs — and reports every such line `RULE_LINE` does not parse, so a rule written as `* **R-X-01**` or indented under another bullet is reported as an unparsable rule line rather than read. Measured one fixture per form through `make_repo( )` and the real `check( )`, each not reported: a `manual:` rule whose marker sits under `docs/`, `.claude/commands/`, `build/` or a `node_modules/` directory (`SKIP_PREFIXES`, `SKIP_DIRS`); a line carrying a second binding after its first, which the lazy `RULE_LINE` reads as one binding whose value swallows the second; a `planned:` phase whose status is spelled `Done`, since the status is compared exactly; and a `CLAUDE.md` mention in range notation (`R-X-01..03`), from which `RULE_ID` extracts only the first id. The check proves that the bound file exists and contains the marker text, not that `make test` runs that file or that it tests the rule. — test: `tests/test_rule_traceability.py`
- **R-PROC-02** — Every phase directory whose status is `done` contains a `verify.md` written for a non-specialist: what was built, what it should do, and the exact physical steps and expected readings that confirm it. It carries two headings: one matching `^#+ What was built` and one matching `^#+ .*[Cc]heck it` (e.g. `## Check it yourself`). **Scope, recorded 2026-09-23 because the check reads less than this text says:** the check proves the file and its two headings exist, not what is under them — for every `PHASES.md` row whose id is `NN-…` and whose status is exactly `done`, `docs/phases/<id>/verify.md` exists, is non-empty and has both heading lines (`missing_verify( )`, `tests/test_phase_docs.sh`). Whether the prose is written for a non-specialist, says what the phase should do, and gives exact physical steps and expected readings is judged by no check. — test: `tests/test_phase_docs.sh`
- **R-PROC-03** — Commit messages follow Conventional Commits (`feat:`, `fix:`, `chore:`, `docs:`, `refactor:`, `test:`), one phase's work per commit where practical. — manual: git history is not a build artifact; a check would either run only on the newest commit (trivially bypassed) or demand rewriting history, which costs more than the drift.
- **R-PROC-04** — Automated tests never require the real guitar, a network connection, or any toolchain beyond a C++23 compiler and `python3`. Hardware-in-the-loop means Pico against Pico. — manual: a property of the whole suite over time; the closest machine-checkable proxy would be a sandbox the harness does not have.

## Observed conventions

Written as chosen at bootstrap. Real reference files land as the first phases do, and
each entry gets one when it does.

- Header/implementation pairs are `.h` / `.cpp`; `core` headers are self-contained and
  include-what-they-use.
- `core` types are `constexpr`-friendly plain data with `enum class` for closed sets and
  `std::array` for fixed buffers. No inheritance, no virtual dispatch in `core`.
- Names say what the thing is on the bus, not what it is in the abstract:
  `Ps2Frame`, `ControllerId`, `AckTimeout`, `WhammyAxis`.
- Files, directories and identifiers are English, always, including comments.
- **clang-tidy needs exactly two extra flags, and the reason is two separate failures
  (measured 2026-09-11, Homebrew LLVM 23.1.0, reference file `tests/test_style.sh`).** The
  working invocation is `clang-tidy --quiet <file> -- [-xc++ if <file> is a .h or .inl] -std=c++23
  -Isrc -isysroot "$(xcrun --show-sdk-path)"`. Both failures it fixes arrive as
  `clang-diagnostic-error`, which `WarningsAsErrors: '*'` turns into a `FAIL: R-STYLE-02`
  line — so a broken invocation is indistinguishable from a naming violation until someone
  reads the diagnostic text. That is the expensive part, and it is why this entry exists.
  - `-xc++`, headers only: a `.h` with no compile database is compiled as **C**. The first
    diagnostic is `invalid argument '-std=c++23' not allowed with 'C'`, and `'cstdint' file
    not found` is its *consequence*, not a missing include path. A `.inl` with no `-x`
    fails differently, with
    `unable to handle compilation, expected exactly one compiler job`; `.hpp` and `.cc`
    are inferred as C++ and need no `-x` (measured 2026-09-23).
  - `-isysroot`, every file: without it Homebrew's libc++ finds no platform C library and
    dies inside its own headers on `"We don't know how to get the definition of mbstate_t on
    your platform"`. Measured per header — `<array>`, `<optional>`, `<string_view>`,
    `<algorithm>`, `<functional>` and `<variant>` fail; `<cstdint>`, `<cstddef>`, `<span>`,
    `<bit>`, `<limits>`, `<type_traits>`, `<concepts>`, `<utility>`, `<tuple>` and
    `<expected>` do not, because they never reach that platform layer.
  **This supersedes two earlier readings, and naming what each got wrong is the point.** The
  2026-09-02 entry this replaces prescribed three flags — `-isysroot` plus
  `-I/opt/homebrew/opt/llvm/include/c++/v1` — and attributed the `'cstdint' file not found`
  error to a missing libc++ include path. The include path is **not** needed; the error came
  from the file being compiled as C, which that measurement never tested for because it never
  passed `-xc++`. `01-ps2-codec`'s own spec then measured the opposite error and concluded
  **no** sysroot flag was needed; that probe included only `<cstdint>` and `<expected>`, the
  two headers above that happen not to need one, so the conclusion did not survive the first
  `<optional>`. ADR-0010's decision (macOS-only, Homebrew LLVM, explicit flags rather than a
  generated compile database) is untouched — only its count of the flags, which is a
  measurement and so lives here rather than in an ADR.
  A consequence worth stating: Apple's `c++`, which `make test` uses, compiles all of these
  with no flags at all. So `make test` cannot catch a broken clang-tidy invocation, and
  `make lint` is the only thing that can.
- Magic bytes from the PS2 protocol are named `constexpr` values in one place per
  concern, never inline literals in logic — except inside `tests/vectors/`, where being
  a literal is the point (R-PROTO-05).
- **`readability-magic-numbers` ignores every literal inside a `const` or `constexpr`
  variable's initializer (measured 2026-09-11, reference file `tests/test_style.sh`).** This
  is what R-CLEAN-04's binding does and does not reach, and it matters here more than it would
  in most codebases, because `const bool is_ok = <expression>;` is this repo's dominant idiom
  in both `src/core/` and the test cases — so a magic number written inside one is not
  diagnosed. Measured by feeding the check one literal per context: flagged in arithmetic
  (`n * 4096`), comparisons (`n > k`, `n == k`), subscripts, call arguments, `return`
  statements, shifts, and non-`const` local initializers; **not** flagged in a `const` local's
  initializer or a `constexpr` initializer. The last of those is correct and is the rule's
  own escape hatch — a named `constexpr` is exactly what R-CLEAN-04 asks for. The `const
  bool` case is the gap. It was found by mutating `kNegotiationTimeoutUs` to `100000` inside
  `const bool is_stuck = …` in `src/core/link.cpp` and watching `make lint` stay green; the
  mutation that does fail is a literal in `src/core/hid_report.cpp`'s compound assignment.
  Upgrade path: none cheap. `cppcoreguidelines-avoid-magic-numbers` is an alias of the same
  check and behaves identically. Closing it means either a second checker or writing fewer
  `const` initializers, and neither is worth it for a gap this shape — so it is declared here
  rather than papered over.
- Tables meant to be read as tables — the pin table above all — are plain C arrays,
  not `std::array`. `AlignArrayOfStructures` aligns the columns of a plain array and
  gives up on `std::array`'s doubled braces, and for `src/core/pins.h` a scannable
  column beats a bounds-checked type: a wrong drive mode has to be visible.
- Private and protected member state carries `m_`. Chosen over a trailing `_`, which
  disappears at the end of a long name, and over a leading `_`, which is legal for a
  class member but is one capital letter away from an identifier the standard reserves.
- One PIO program per bus role, each short enough to read in full, with the cycle
  budget written above it as a comment.
<!-- Findings — expensive, verified facts about how the code or the hardware actually
     behaves where the obvious reading is wrong — go here, each with its reference file
     and the date it was last checked. -->

- **An SDK-including file anywhere under `src/` fails `make lint`, and the FAIL line
  misnames the cause (measured 2026-09-23, Homebrew LLVM 23.1.0, reference file
  `tests/test_style.sh` `tidy_sources()`).** Since `13-scaffold-check-gaps`, clang-tidy reads
  every `.cpp`/`.h` under `src/`, not only `src/core/`. A one-line `src/hal/x.cpp` holding
  `#include "pico/stdlib.h"` produces `'pico/stdlib.h' file not found [clang-diagnostic-error]`
  under `FAIL: R-STYLE-02 / R-CLEAN-02 / R-CLEAN-04: naming, function-size or magic-number
  violations` — the line names three rules none of which is broken. The invocation passes only
  `-std=c++23 -Isrc -isysroot …`; nothing supplies the Pico SDK or TinyUSB include paths.
  **Consequence for `03-pio-bus`:** its first `src/hal/` file makes `make lint` red until that
  phase gives `tidy_sources()`'s invocation the SDK's include flags (the upgrade path in
  `.clang-tidy`). Skipping those files is not the fix — that was the silent hole 13 closed.

## Error handling

Decided in **ADR-0007**. Two mechanisms, at two levels, and the level decides which:
one input and one output is a result struct; anything that remembers what happened last
time is the link state machine.

- **Pure decoding returns `std::expected<T, Status>`** (ADR-0009) — value and reason in
  one object, so the value cannot be reached without the status that qualifies it. Access
  is `has_value()` and `operator*`; **never `.value()`**, which calls `abort` under
  `-fno-exceptions` (R-ERR-04).
- **The link lifecycle is a state machine** — `Absent | Negotiating | DigitalStreaming |
  AnalogStreaming`. A missing `ACK` is not a failed call, it is a transition to `Absent`.
  `Link::last_fault` carries why, because trace mode (R5) has to print it.
- **No exceptions anywhere.** The firmware builds with exceptions and RTTI off. A bus
  fault is a normal, expected outcome, not an exceptional one.
- **No status returned alongside a separate out-parameter** holding the value. That shape
  leaves the value readable when the status says it is meaningless, which is exactly what
  ADR-0007 buys protection from. `hal` sits against the Pico SDK, which does use that
  shape; the translation happens there and does not leak inward.
- **The firmware never stops.** Every error path returns to `Absent` and retries
  negotiation. A hang is a worse failure than a wrong report, because the host is left
  holding a HID device that has silently stopped answering. This deliberately inverts the
  "fail fast, fail loud" guideline; see §Engineering guidelines.
- **`assert` is for invariants the code guarantees**, and is compiled out of firmware
  builds. Anything that can happen because of the outside world is a value, never an
  assert and never control flow.

## Testing

- Tests live under `tests/`, one file per unit under test, named `test_<subject>.cpp`
  for host C++ tests and `test_<subject>.py` / `.sh` for repo-shape checks.
- `make test` runs everything and needs no hardware. Hardware-in-the-loop suites are
  separate targets, are always Pico-against-Pico, and are never a precondition for
  `make test` passing.
- Each test is arranged as **arrange / act / assert**, in that order, with the three
  visually separated. A test asserting several unrelated things is several tests.
- Every acceptance criterion in a phase spec has at least one automated check, or an
  explicit line in that phase's `verify.md` saying why it is operator-verified instead.
- Expected protocol bytes come from `tests/vectors/` and nowhere else (R-PROTO-05).
- A test named by a rule carries a `RULE <id>` marker comment referencing
  `docs/constraints.md` (R-PROC-01). When either side changes, the other is updated in
  the same change — the meta-test exists to make forgetting fail loudly.
