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

- **R-PROTO-01** — All bytes on the PS2 bus are transferred LSB-first, SPI mode 3. — planned: 01-ps2-codec
- **R-PROTO-02** — After every byte of a frame except the last, the master waits for the controller's `ACK`. A missing `ACK` within the timeout aborts the frame and marks the controller absent; it never yields a partially decoded report. — planned: 01-ps2-codec
- **R-PROTO-03** — A frame whose header byte is not a known controller id is reported as unknown, never guessed at or decoded on a best-effort basis. — planned: 01-ps2-codec
- **R-PROTO-04** — The whammy axis is read only from a frame that reported analog mode. A digital-mode frame yields the axis at rest, never a byte reinterpreted from a digital report. — planned: 01-ps2-codec
- **R-PROTO-05** — Every expected byte sequence in the tests is a literal, stored under `tests/vectors/` and written by hand from the protocol documentation. No expected value is produced by `core` or captured from the emulator, and no file under `src/` may reference `tests/vectors/`. — test: `tests/test_repo_shape.sh`

### Style

Layout is `clang-format`, naming is `clang-tidy`; they are different tools because they
solve different problems, and neither does the other's job.

- **R-STYLE-01** — Every `.cpp` and `.h` tracked or newly added under this repo matches `.clang-format`. — test: `tests/test_style.sh`
- **R-STYLE-02** — Naming matches `.clang-tidy`: private and protected members carry the `m_` prefix, types are `CamelCase`, functions and variables are `lower_case`, compile-time constants are `kCamelCase`, macros are `UPPER_CASE`. — test: `tests/test_style.sh`

Both are checked over `src/` and `tests/` by `make lint`, which fails if either tool is
missing. `make test` runs the same script with `STYLE_OPTIONAL=1`, which downgrades a
missing tool to a skip — that is what keeps R-PROC-04 true (host tests need only a C++23
compiler and `python3`), at the cost that a machine without LLVM installed does not
enforce style. `make lint` is the authority.

R-STYLE-02's scope is `src/core/` and `tests/` only: clang-tidy needs to know how to
compile each file, and everything else pulls in Pico SDK or TinyUSB headers with no
`compile_commands.json` to describe them. The upgrade path is in the header of
`.clang-tidy`.

That scope is necessary but **not sufficient, and this paragraph used to imply it was**:
inside `src/core/` the invocation still resolves no standard header, so the first file
using the standard library fails R-STYLE-02 too. Measured by `tests/test_style.sh`, whose
clang-tidy invocation is the one that fails, while running phase `00-scaffold`'s adversarial
block against a scratch `src/core/x.h` containing `std::string_view sv;`. See §Observed
conventions, 2026-09-02.

### Architecture

- **R-ARCH-01** — `src/core/` contains no `#include` of a Pico SDK, TinyUSB, CMSIS or other hardware header, and no `#include <` of a hosted-only standard header. — test: `tests/test_repo_shape.sh`
- **R-ARCH-02** — Every file under `src/` respects the dependency directions in §Layering, as encoded in `.claude/workflow/boundaries.rules`. — test: `tests/test_boundaries.sh`
- **R-ARCH-03** — Firmware code performs no dynamic allocation: no `new`, `delete`, `malloc`, `free`, `std::vector`, `std::string` or `std::function` anywhere under `src/`. — test: `tests/test_repo_shape.sh`

### Error model

Set by ADR-0007.

- **R-ERR-01** — Every fallible function in `src/core/` reports through a result struct or through the link state machine. No function returns a status alongside a separate out-parameter carrying the value. — planned: 01-ps2-codec
- **R-ERR-02** — Every function returning a result struct or a `LinkState` is marked `[[nodiscard]]`. — planned: 01-ps2-codec
- **R-ERR-03** — No `throw`, `try` or `catch` anywhere under `src/`. — test: `tests/test_repo_shape.sh`
- **R-ERR-05** — Firmware builds pass `-fno-exceptions -fno-rtti`. Split out of R-ERR-03 on 2026-08-31: the source clause is a grep and the flags clause is a property of a build that does not exist yet, so one binding could not honestly cover both. — planned: 03-pio-bus
- **R-ERR-04** — No call to `.value()` on a `std::expected` anywhere under `src/`. Under `-fno-exceptions` it does not throw, it calls `abort` — the one outcome §Error handling rules out. Access goes through `has_value()` and `operator*`. — test: `tests/test_repo_shape.sh`

### Toolchain

Set by ADR-0008. Versions are a rule here and not a README sentence because this project
has already been bitten twice by them: `.clang-format` keys were renamed in clang-format
17 and again in 23, and the ARM compiler decides which C++ standard is even available.

- **R-TOOL-01** — Every tool the checks depend on meets its minimum version when present: `clang-format` >= 23 and `clang-tidy` >= 23 (the config uses key shapes introduced in 23), `arm-none-eabi-g++` >= 12 (the release in which libstdc++ gained `<expected>`; only 15.3.1 is actually verified — see ADR-0008 §Verification), `python3` >= 3.8. A tool that is absent is skipped under `OPTIONAL_TOOLS`, never assumed to pass. — test: `tests/test_tool_versions.sh`
- **R-TOOL-02** — The `arm-none-eabi-g++` first on `PATH` can compile a translation unit that includes `<cstdint>` for `cortex-m0plus`. A cross-compiler with no target C library looks installed and cannot build anything; the sudo-free Homebrew formula is exactly that, and it shadows the working cask. macOS is the only development platform (ADR-0010), so this names the trap that exists rather than one platform's among several. — test: `tests/test_tool_versions.sh`

### Engineering guidelines

Transcribed from the operator's general engineering guidelines. Rules already implemented
above are **mapped**, not duplicated; guidelines that do not apply to this project are
**dropped by name** with the reason. Both lists follow the rules.

**Clean code**

- **R-CLEAN-01** — Names reveal intent and use domain language: what the thing is on the bus, not what it is in the abstract. Abbreviations only where universally known (`id`, `url`, `api`, `usb`, `pio`, `gpio`, `hid`, `ack`). A name that needs a comment to explain it is renamed instead. — manual: intent is a judgement about meaning; no checker can tell a good name from a bad one.
- **R-CLEAN-02** — A function is at most 60 lines, takes at most 3 parameters, and nests at most 4 deep. Past three parameters the arguments become a struct. — test: `tests/test_style.sh`
- **R-CLEAN-03** — Boolean names are assertions: `is_`, `has_`, `can_`, `should_`. Never `flag`, `status`, `check`. — test: `tests/test_repo_shape.sh`
- **R-CLEAN-04** — No magic numbers or strings in logic. Every protocol byte, timeout and threshold is a named `constexpr` in one place per concern. The literals under `tests/vectors/` are the sole exception, and being literal is their purpose (R-PROTO-05). — planned: 01-ps2-codec
- **R-CLEAN-05** — Every `TODO` names the phase or issue that will close it: `// TODO(09-guitar-observe): confirm against the real controller`. A bare `TODO` is not allowed. — test: `tests/test_repo_shape.sh`
- **R-CLEAN-06** — Comments explain *why*, never *what*; a comment that no longer matches the code is deleted or corrected, never left standing. — manual: whether a comment is still true is exactly the judgement a checker cannot make.
- **R-CLEAN-07** — Command-Query Separation: a function returns a value or changes state, never both. — manual: distinguishing a query from a command requires knowing intent, not signature.
- **R-CLEAN-08** — Constructors only assign. No logic, no I/O, no side effects. — manual: "logic" has no syntactic definition; a grep would flag every non-trivial initialiser list.
- **R-CLEAN-09** — Composition over inheritance. `src/core/` uses no inheritance and no virtual dispatch at all: it is plain data plus free functions. — test: `tests/test_repo_shape.sh`

**SOLID**

- **R-SOLID-01** — Every module has one reason to change. A design decision that violates a SOLID principle is flagged as a deliberate tradeoff with an alternative proposed, never taken silently. — manual: "one reason to change" is a claim about the future, not a property of the source.
- **R-SOLID-02** — Depend on abstractions, not concretions: where a layer needs something from a layer further out, the interface is owned by the inner layer and the dependency is injected, never constructed inside the consumer. — manual: the layering test proves the *direction* of every edge; whether the seam is an abstraction is a design judgement on top of that.

**Security**

- **R-SEC-01** — Zero secrets in the repository: no credentials, tokens or keys in source, config or history. — test: `tests/test_secrets.sh`

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

- **R-PROC-01** — Every rule in this file has exactly one binding in ADR-0005's grammar; every `test:` file carries a matching `RULE <id>` marker; every marker names a declared id; `manual:` rules carry no marker; every `planned:` phase exists and is not `done`; every id `CLAUDE.md` mentions is declared here. — test: `tests/test_rule_traceability.py`
- **R-PROC-02** — Every phase directory whose status is `done` contains a `verify.md` written for a non-specialist: what was built, what it should do, and the exact physical steps and expected readings that confirm it. — test: `tests/test_phase_docs.sh`
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
- **clang-tidy resolves no standard header without a compile database (measured
  2026-09-02).** `clang-tidy --quiet <file> -- -std=c++23 -Isrc` fails on a file as small
  as `#include <cstdint>` + `using Byte = std::uint8_t;`, with `clang-diagnostic-error`,
  not a naming complaint — so the failure does not look like the rule it comes from.
  `-isysroot $(xcrun --show-sdk-path)` alone does not fix it; adding
  `-I/opt/homebrew/opt/llvm/include/c++/v1` does. That third flag is Homebrew-on-Apple-
  Silicon specific. That is a documented assumption rather than a portability violation:
  ADR-0010 fixes development to macOS with Homebrew LLVM and settles the choice as the three
  flags, so the first phase writing `src/core/` inherits an answer instead of a question.
- Magic bytes from the PS2 protocol are named `constexpr` values in one place per
  concern, never inline literals in logic — except inside `tests/vectors/`, where being
  a literal is the point (R-PROTO-05).
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
     and the date it was last checked. None yet; phase 09-guitar-observe is expected to
     produce the first ones. -->

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
