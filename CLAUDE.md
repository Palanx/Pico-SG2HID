# pico-sg2hid

RP2040 firmware turning a wired PS2 Guitar Hero SG into a driverless USB HID gamepad on
macOS and Debian, with a real analog whammy axis.

Greenfield project run under the phase workflow. Every rule here is true in *every*
session; everything session-specific lives on disk behind the pointer table. Keep this
file under 150 lines — a line added here is a tax on every future session.

## How work happens here

One pipeline: `/plan-feature` → `/expand-phase <id>` → `/implement-phase <id>` →
`/validate-phase <id>` → repeat. Phase state lives in `docs/phases/PHASES.md`
(vocabulary: `pending | expanded | in-progress | blocked | superseded by <ids> | done`) —
trust the table, not memory. Never expand a phase whose dependencies aren't `done`. Never
mark `done` yourself; only `/validate-phase` does. A cut that turns out wrong is
superseded by new rows, never edited or deleted — same rule as an ADR.
A phase's diff always carries files the workflow wrote, not its plan — `docs/phases/<id>/spec.md`, `docs/phases/<id>/notes.md`, `docs/phases/PHASES.md`, `docs/index/`. They are never that phase escaping its scope.

## Session reading rule

Read this file + the current phase's directory
(`docs/phases/<id>/spec.md`, `notes.md`) + the files the spec points to. Nothing else
unless the spec proves insufficient — and then record the gap in the phase's `notes.md`.
For orientation beyond the phase, load ONE section of `docs/index/`, not the whole thing.

## Hardware safety — before any code that touches a pin

Real hardware is on the bench and one of the two devices is irreplaceable. These are not
advisory (full text and bindings in `docs/constraints.md` §Invariants, rationale in
ADR-0006):

- **R-SAFETY-01** — never configure `DATA` or `ACK` as a push-pull output. Both are
  open-drain, driven by the controller. Driving one is a short between output stages.
- **R-SAFETY-02** — every GPIO is declared once in `src/core/pins.h`. Only `src/hal/`
  calls `gpio_init` / `gpio_set_dir` / `gpio_pull_up`, and only by iterating that table.
- **R-SAFETY-04 / R-SAFETY-05** — the 7.6 V PS2 rail is never connected; the guitar is
  powered from 3V3 only, never VBUS or VSYS. The RP2040 is not 5 V tolerant.
- **R-SAFETY-08** — the order of first contact is fixed: host tests → loopback on one
  Pico → Pico against the emulator → the real guitar read-only through trace mode → the
  real guitar in full. Untested bus code never meets the guitar.

## Rules are bound to tests, in both directions

The operator's standing requirement, decided in ADR-0005. Every rule in
`docs/constraints.md` §Invariants has an id (`R-AREA-NN`) and exactly one binding:

- `test: <path>` — the file exists and carries a `RULE <id>` marker pointing back here.
- `manual: <reason>` — not machine-checkable; the reason is part of the rule.
- `planned: <phase-id>` — the test is owed. Tracked debt with an owning phase.

`tests/test_rule_traceability.py` fails the build on any drift between the two sides.
**When you change a rule, change its test in the same edit, and vice versa.** When you
add a rule, give it a binding — a rule with no binding is not a rule. `CLAUDE.md` may
mention ids but never declares bindings; `docs/constraints.md` is the only catalog.

## Teach, don't just deliver

The operator is not an electronics or driver specialist and has said so. Work is not
finished when it compiles — every phase ends with `docs/phases/<id>/verify.md` written
for a non-specialist: what was built, what it should do, and the exact physical steps
and expected readings that let the operator judge whether it is correct (R-PROC-02).
Explain the electronics and the protocol when they come up. Never assume a multimeter
procedure is obvious.

## Architecture

Dependencies point inward toward `core` (detail in `docs/constraints.md` §Layering,
rationale in ADR-0002, machine-enforced by `.claude/workflow/boundaries.rules`):

`app` → `core`, `hal`, `usb`; `emu` → `core`, `hal`; `hal` → `core`; `usb` → `core`;
`core` → nothing. Nothing imports upward.

`src/core/` is pure logic and data: no I/O, no clock, no allocation, and no Pico SDK,
TinyUSB or CMSIS header — that is what lets `make test` compile and run it on the
laptop. Timing-dependent behaviour is expressed as "given elapsed microseconds, decide",
with the caller supplying the clock.

Errors (ADR-0007, ADR-0009): pure decoding returns `[[nodiscard]] std::expected<T, Status>`
— never `.value()`, which aborts under `-fno-exceptions` (R-ERR-04); the link lifecycle is a state machine where a missing `ACK` is a
transition to `Absent`, not a failed call. No exceptions, and never a status returned
alongside a separate out-parameter (R-ERR-01..03).

`make` is the only entry point. `make test` needs a C++23 compiler and `python3` and
nothing else — no hardware, no network, no ARM toolchain. `make firmware` needs cmake,
`arm-none-eabi-gcc` and `PICO_SDK_PATH`; it is never a precondition for `make test`.

Enforcement is real, and its exact reach is recorded, not assumed: the hooks run whatever
`.claude/workflow/toolchain.json` configures for the file you touched, and print a loud
`workflow gap:` line naming any category that has no tool. Read that file's `gaps` to know
what is *not* checked here. Commits with secrets are blocked in every project. When a hook
reports a failure, fix it before doing anything else. Changing a boundary rule requires a
superseding ADR, never a silent edit.

## Conventions

- Commits: Conventional Commits (`feat:`, `fix:`, `chore:`, `docs:`, `refactor:`, `test:`), one phase's work per commit where practical.
- Code, identifiers, comments, commit messages and all documentation in this repo are English.
- Style is enforced, not advisory: `m_` on private/protected members, `CamelCase` types, `lower_case` functions and variables, `kCamelCase` compile-time constants, and spaces inside `( )` and `[ ]`. `make lint` is the authority (R-STYLE-01, R-STYLE-02); `make test` only skips the check when LLVM is absent.
- Expected protocol bytes in tests are hand-written literals under `tests/vectors/`, never generated by `core` or captured from the emulator (R-PROTO-05) — the emulator shares `core`, so a shared bug would otherwise be invisible.
- Every command that does work ends by writing its outcome to disk (notes, status). A session's undocumented knowledge is lost knowledge.
- Decisions that constrain the future get an ADR in `docs/adr/` before the code lands.
- A verified fact about how the code or the hardware *is* — where the obvious reading is wrong, established at real cost — is a **finding**, not an ADR: it goes to `docs/constraints.md` `§Observed conventions` with its reference file and the date it was checked. An ADR has a status and is immutable; a finding has neither and stops being true when the code changes.
- If a Belay hook or command misfires (false positive, wrong tool command, unhandled case) or a workflow step causes friction, say so and offer `/belay-feedback` — the only channel back to the workflow package.

## Pointer table — where everything else lives

| What you need | Where it is |
|---|---|
| What we're building & why | `docs/product/requirements.md` |
| Standing rules, the rule catalog & invariants | `docs/constraints.md` |
| Past decisions & rationale | `docs/adr/` (newest number wins; superseded ADRs say so) |
| Phase index & status | `docs/phases/PHASES.md` |
| Current phase spec / notes / operator verification | `docs/phases/<id>/spec.md`, `notes.md`, `verify.md` |
| Repo map & module symbols | `docs/index/_overview.md`, then one `docs/index/<module>.md` |
| Test/lint/typecheck commands | `.claude/workflow/toolchain.json` (+ `toolchain.manual.json`, which wins) |
| Layer boundary rules | `.claude/workflow/boundaries.rules` |
| Security review reports | `docs/security/` |
| Workflow templates | `docs/templates/` |
