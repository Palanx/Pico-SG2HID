# Phase 01-ps2-codec — Pure PS2 frame codec, id table, link state machine, button/axis map, HID report builder

<!-- Re-expanded 2026-09-17, the third time, after /validate-phase's iteration-3+ escape.
     Derived from the PHASES.md row's Goal and from notes.md, never from the tree: the code is
     on disk and green, and a spec written to match it would pass review while proving nothing.
     Where the two disagree, §Plan says so and the code is what changes.
     Two rules this re-expansion was written under, both from the escape that produced it:
     §Plan carries only work that is NOT done — a landed step's result is in the tree and its
     history is in notes.md, and a Plan that logs what already happened duplicates notes.md and
     rots; and §Context pointers is the one section regenerated against the tree, because it is
     a list of files to open rather than a claim about what this phase decided.
     notes.md is the account of every implementation round and every validation. It is never
     rewritten, here or anywhere. -->

## Goal

After this phase `src/core/` holds the whole protocol brain of the project with no hardware
anywhere near it: a decoder that turns the bytes of one PS2 poll frame into a typed frame or a
typed failure, a controller-id table, a link lifecycle state machine, a map from a decoded frame
to the guitar's controls, and a builder that packs those into the HID report bytes `08-usb-hid`
will hand to TinyUSB. All of it compiles and runs on the laptop under `make test` with a C++23
compiler and `python3` and nothing else (R-PROC-04), because `core` includes no SDK header and
performs no I/O (ADR-0002, R-ARCH-01).

The observable behaviour is a decoder that is **refusing by default**. Each of the ten vectors
enumerated in §Vectors is fed to `decode`, and to `map_frame` where §Vectors names a mapping, and
produces exactly the outcome that table names:

- A header byte that is not a declared controller id produces `UnknownId` and no frame (R-PROTO-03).
- A declared id whose ready byte is not `kReadyByte` produces `NotReady` and no frame.
- A frame the bus cut short produces `AckTimeout` and no frame — never a frame with some fields
  filled in (R-PROTO-02).
- A digital-mode frame maps to a whammy axis at rest, a value the codec supplies and never a byte
  reinterpreted out of a digital payload (R-PROTO-04).
- A config-mode frame decodes and then reports *nothing*: all ten controls released and the whammy
  at rest. Its payload is all `0x00` and digital buttons are active low, so an ungated map would
  call that every control pressed.

The ten controls, named here because the rest of this document quantifies over them: five frets
(green, red, yellow, blue, orange), strum up, strum down, start, select, tilt. The whammy is an
axis, not a control, and is counted separately throughout.

**A buffer longer than the frame the header announces is accepted, and the bytes past the
announced length are ignored.** The header is the only authority on how long a frame is, so the
span handed to `decode` is a *capacity* and never a claim about the frame. Requiring the caller to
trim would put the arithmetic `2 * (header & 0x0F)` on both sides of the layer boundary — the
duplication `core` exists to prevent. What `03-pio-bus` will actually hand over is not recorded
anywhere yet, and this contract is deliberately the one that does not need it known. Nothing
asserts it: `notes.md` §Debt carries it with its owner.

**Where two refusals are both true, the order is fixed, and it is chosen rather than inherited
from the order somebody happened to write the checks in.** The list is evaluated top to bottom and
the first refusal that applies wins. No refusal sits above a point where it is decidable, but
decidability alone does not set the order — item 1 says why:

1. fewer bytes than a header and a ready slot → `AckTimeout`. A single byte would already decide
   the header, so this is a choice: the frame is cut short, and the abort outranks every refusal
   still evaluable (R-PROTO-02), an undeclared header that arrived alone included.
2. the header is not a declared id → `UnknownId`. This one cannot move later: `frame_len` needs
   the id, so the length is not yet knowable above it.
3. fewer bytes than the id announces → `AckTimeout`.
4. the ready byte is not `kReadyByte` → `NotReady`.

**The consequence, because a real bus produces it:** a frame that is *both* cut short *and*
carrying `0xFF` at the ready slot reports `AckTimeout`, not `NotReady`. `0xFF` at the ready slot of
a short frame is an undriven `DATA` line, not a controller stating it is not ready — a controller
that died mid-frame said nothing about readiness at all. `NotReady` is for a frame that arrived
**complete** and whose ready byte is wrong.

R-PROTO-02, R-PROTO-03 and R-PROTO-04 are bound to `tests/test_ps2_codec.py`, and that binding is
proven live in the sense `00-scaffold` established: mutating the codec so it stops refusing makes
the check fail and name the rule. The driver carries three such mutations as rejection cases; a
fourth (M4) is a by-hand procedure §Acceptance criteria describes and `/validate-phase` performs.

### What this phase does not prove, declared rather than discovered later

That any byte position in the SG's payload is the right one. Four concerns are written from the
documented protocol and none of them is measured: the three controller id bytes; the button bit
positions and masks; the whammy's payload index; and the whammy's rest value.

**They are covered by three `TODO(09-guitar-observe)` markers, not four.** One in
`src/core/ps2_protocol.h` covers the id bytes; one in `src/core/guitar_state.h` covers the whole
block of button positions and masks — a block, not a single constant; and a third, also in
`guitar_state.h`, covers the whammy's index and its rest value together. Three is the number
§Acceptance criteria pins and the number `verify.md`'s "What this phase does NOT prove" section
tells the operator to expect. A vector here asserts that the codec does what the codec says, not
that the guitar agrees.

## The frame: shape and storage

`decode` takes `std::span<const std::uint8_t>` holding `[header][ready][payload…]` — the
controller's response **with its first byte dropped**, because that byte answers the address byte
and carries nothing. Dropping it is `hal`'s job (`03-pio-bus`).

Payload length is `2 * (header & 0x0F)`, documented protocol, which makes a digital frame two
payload bytes and an analog frame six.

**`Ps2Frame` carries the controller id and the payload, and nothing else.** The payload is a
fixed-width `std::array<std::uint8_t, kMaxPayloadLen>`, where `kMaxPayloadLen` is
`payload_len( ControllerId::Analog )`, and every byte past the length the id announces is zero.
Fixed width because `core` allocates nothing; zero-filled because an out-of-range read must be
deterministic rather than garbage.

**It stores no length, deliberately.** How many payload bytes are meaningful is `payload_len( id )`,
so a stored length would be a second source of truth for one fact, and the only thing it could add
is the possibility of disagreeing with the id. Everything that needs the length recomputes it.

`kWhammyIndex` is 5, so a *naive* analog read of a two-byte digital payload never touches either
byte `digital_whammy_absent.h` carries — **it reads the zero fill.** The vector traps the bug
because the fill is `0x00` and `kWhammyRest` is `0x80`, so an ungated read yields a non-rest axis
and the case fails. The two literal bytes make the vector realistic; the zero fill is what makes it
a trap.

**The fill value is not a protocol byte and R-PROTO-05 does not reach it.** The cases file declares
its own `kZeroFill` and asserts the unannounced payload bytes against it, which reads like an
expected byte written outside `tests/vectors/`. It is not: the controller never sent those bytes.
The fill is this project's decision about what `decode` leaves in a buffer it did not fill, so no
vector could carry it — a vector holds what the bus put on the wire. R-PROTO-05 governs the bytes
the protocol fixes, which is how `CLAUDE.md` §Conventions states it and how the catalogue now
states it too.

## The link: states, transitions, faults, and where the clock starts

`step` is a pure transition: given the current `Link`, a decode outcome, and the microseconds
**elapsed since the previous step**, it returns the next state (ADR-0011). It reads no clock.

**`LinkState` has exactly four members:** `Absent`, `Negotiating`, `DigitalStreaming`,
`AnalogStreaming`. This phase owns all four and every transition between them, including the exit
from `Negotiating` — the alternative is a trap state, since a controller that keeps answering in
config mode would hold the link there forever, contradicting §Error handling's "the firmware
retries rather than stopping".

**The transition rule, stated before the table it expands: the next state is a function of the
decode outcome alone, identical from every source state — except the negotiation timeout, which is
a function of the current state and `us_in_state`.** The table is that rule written out, and it is
the enumeration the quantifier above owes:

| decode outcome | next state, from any of the four | `last_fault` |
|---|---|---|
| ok, `ControllerId::Digital` | `DigitalStreaming` | unchanged |
| ok, `ControllerId::Analog` | `AnalogStreaming` | unchanged |
| ok, `ControllerId::Config` | `Negotiating` | unchanged |
| `DecodeStatus::UnknownId` | `Absent` | `FaultCause::UnknownId` |
| `DecodeStatus::NotReady` | `Absent` | `FaultCause::NotReady` |
| `DecodeStatus::AckTimeout` | `Absent` | `FaultCause::AckTimeout` |

Plus one rule the table cannot express, evaluated when the outcome above would leave the link in
`Negotiating`: if the accumulated time in that state has passed `kNegotiationTimeoutUs` strictly —
the bound is exclusive — the next state is `Absent` with `FaultCause::Negotiating`.

**`us_in_state` counts time since the transition *into* the current state.** The `elapsed_us` handed
to the step that *enters* a state is time that passed before that transition, so it is attributed to
the state being left, and the counter resets to zero on any change of state. The consequence is
stated because it reads like an off-by-one and is not: **the negotiation timeout can only fire on
the second consecutive step that leaves the link in `Negotiating`, never on the one that enters it.**

**The accumulation saturates at `UINT32_MAX`; it does not wrap.** Wrapping would silently reset the
counter roughly every 71.6 minutes, which is the one behaviour that turns a permanently stuck
controller into one that looks fine on a schedule. Saturating is unreachable in practice — it needs
a caller that stops polling for over an hour and then resumes — and is specified anyway, because
"unreachable" is a claim about the caller and `core` does not get to make claims about its callers.
Nothing asserts it: `notes.md` §Debt carries it.

**`FaultCause` has exactly five members:** `None`, `AckTimeout`, `UnknownId`, `NotReady`,
`Negotiating` — one per way the link can drop, plus `None` for a link that has never dropped.
`last_fault` is **history, not current state**: it names the cause of the most recent drop and is
*not* cleared by a subsequent good frame. Trace mode (R5) prints the fault after recovery, which is
precisely when clearing it would have destroyed the thing being traced.

**The budget is not owned here.** `kNegotiationTimeoutUs` is 100 ms, picked as a "something is
wrong" bound two orders of magnitude above a sequence of a few frames at ~1 ms. Nothing has timed a
real negotiation, so it is a guess carrying a `belay-debt:` marker that says so, and tightening it
is `07-analog-mode`'s. The structure is decidable from `(state, outcome, elapsed_us)` and is
therefore `core`'s; the number needs a bus and is therefore not.

Four behaviours above are asserted by cases rather than left as prose: the exclusive bound, the
reset on entry, `last_fault` surviving a good frame, and the `NotReady` row. The saturation and the
over-long buffer are the two that are not, and both are declared here and in `notes.md` §Debt.

## Vectors

Ten files, hand-written literal `constexpr` byte arrays in C++ headers under `tests/vectors/`
(R-PROTO-05: written by hand from the protocol documentation, never generated by `core`, never
captured from the emulator), plus a `README.md` carrying the provenance half of that rule. Headers
rather than hex text because a parser in the test is code that can be wrong about the literal it
reads. `#embed` is **not** used: it embeds a file's bytes verbatim, so it would require the vectors
to be unreadable binary to be useful.

| file | frame shape | asserted outcome | rule |
|---|---|---|---|
| `digital_idle.h` | digital header, ready byte, 2 payload bytes, nothing pressed | `ControllerId::Digital`; maps to all ten controls released | — |
| `digital_pressed.h` | digital, one fret + one strum direction held | that fret and that strum pressed, the other eight released | — |
| `digital_whammy_absent.h` | digital, two payload bytes neither of which is `kWhammyRest` | whammy **at rest**; the trap is the zero fill at `kWhammyIndex`, see §The frame | R-PROTO-04 |
| `analog_idle.h` | analog header, ready byte, 6 payload bytes, sticks centred | `ControllerId::Analog`; its case asserts `whammy == kWhammyRest` | — |
| `analog_whammy_full.h` | analog, whammy byte at full deflection | whammy reads full deflection — a value that is not `kWhammyRest`, which is what makes this the vector that proves the read path | R-PROTO-04 |
| `config_mode.h` | config-mode header, 6 payload bytes, all `0x00` | `ControllerId::Config` — recognised, not a report: it decodes, and `map_frame` yields all ten controls released and the whammy at rest | — |
| `not_ready.h` | a declared id whose ready byte is `0xFF`, the idle level of an undriven `DATA` line; length and payload well-formed | `DecodeStatus::NotReady`, no frame, and `step` drops the link recording `FaultCause::NotReady` | — |
| `unknown_id.h` | header `0x79`, the DualShock 2's real full-analog id, deliberately undeclared here; **20 bytes — the whole frame `0x79` announces**, since `2 * (0x79 & 0x0F)` is 18 payload bytes. A shorter one would be cut short as well as undeclared | `DecodeStatus::UnknownId`, no frame. Its first byte alone is also decoded, and reports `AckTimeout` per §Goal's item 1 | R-PROTO-03 |
| `truncated_ack.h` | digital header, ready byte, payload cut short — the `ACK` never came | `DecodeStatus::AckTimeout`, no frame, and `step` moves the link to `Absent` from every source state | R-PROTO-02 |
| `truncated_not_ready.h` | digital header, `0xFF` at the ready slot, payload cut short — both faults at once | `DecodeStatus::AckTimeout`, no frame: the abort outranks the ready byte, per §Goal's precedence | R-PROTO-02 |

**`kWhammyRest` is `0x80`, and `analog_idle.h`'s centred axis byte is also `0x80`.** That is a real
coincidence, and the roles are split rather than engineered away: `analog_whammy_full.h` is what
proves the payload is read, because its value is not `kWhammyRest`; `digital_whammy_absent.h` and
`config_mode.h` are what prove the rest default is supplied by the codec; `analog_idle.h` proves
neither on its own and is kept for the decode half of its row. Its case asserts the value and not
`payload[ kWhammyIndex ]`, because the two readings are numerically identical here and only the
weaker one is honest.

## What a `test:` binding does and does not promise

A rule's check can print `ok:` while the rule is broken, and this phase measured that five times.
`tests/test_checks_are_live.py` does not catch it, because **that harness proves a check is
*wired*, not that it is *adequate*.** A check whose reach is narrower than its rule's text passes
it perfectly: it is alive, it just measures less than the sentence it is bound to.

The convention that closes the gap:

> **The phase that moves a rule from `planned:` to `test:` writes, in the rule's own text, what the
> check does not see.** A rule whose text states no scope asserts that its check covers it
> entirely, and that assertion is what a later reader is entitled to disbelieve and measure.

**Which clause a rule owes is derived from what its check is made of, and the parts are
independent.** Three earlier attempts derived it from a hand-kept list of rules and all three were
falsified; a fourth derived it from the scanner function alone and was falsified too, because a
grep check has three separable parts and that version conflated two of them:

| the part | what it decides | the clause it generates |
|---|---|---|
| the **file list** — `src_files`, `core_files`, `core_headers` in `tests/test_repo_shape.sh`; `sources()`, `tidy_sources()` in `tests/test_style.sh` | which files are read at all | a **file-scope clause**, whenever the rule's text is written wider than the list |
| the **scanner** — `hits()` strips `//` before grepping, `raw_hits()` does not | whether an occurrence inside a comment can be seen | a **comment clause**, and only if the rule's subject can occur in a comment |
| the **pattern** — each `find_*`'s own regex | which spellings of the subject match | a **spelling clause**, unconditionally: a regex always has forms it does not match |

The file list and the scanner are orthogonal: `find_clean05` pairs `raw_hits` with `src_files`,
`find_err02` pairs `hits` with `core_headers`. `tidy_sources()` is a file list and not a scanner,
which is why its obligation is unconditional — it narrows *whatever* the rule says — while a
scanner's depends on the rule's subject.

### The assignment, measured against the files rather than recalled

Produced 2026-09-17 by reading the `report R-` lines, the `find_*` definitions and the list
functions out of `tests/test_repo_shape.sh`, the source lists out of `tests/test_style.sh`, and each
rule's own text out of `docs/constraints.md`. Recompute with:

```
grep -E 'report R-' tests/test_repo_shape.sh                  # rule -> finder
grep -E '^find_[a-z0-9]+\( *\)' tests/test_repo_shape.sh      # finder -> scanner and list
grep -nE 'tidy_sources\(\)|sources\(\)' tests/test_style.sh   # the clang-tidy lists
grep -E '^- \*\*R-' docs/constraints.md                       # each rule's own text
```

| rule | list | scanner | file-scope clause | comment clause | spelling clause |
|---|---|---|---|---|---|
| R-ARCH-01 | `core_files` | `hits` | not owed — text says `src/core/` | not owed — subject is a directive | owed, **not written**: `00-scaffold`'s |
| R-ARCH-03 | `src_files` | `hits` | not owed — text says under `src/` | not owed — subject is a call | owed, **not written**: `00-scaffold`'s |
| R-CLEAN-03 | `src_files` | `hits` | **owed, not written** — text is unscoped; `00-scaffold`'s | not owed — subject is a declaration | owed, **not written**: `00-scaffold`'s |
| R-CLEAN-05 | `src_files` | `raw_hits` | **owed, not written** — text is unscoped; `00-scaffold`'s | not owed — `raw_hits` sees comments, and a comment is the subject | owed, **not written**: `00-scaffold`'s |
| R-CLEAN-09 | `core_files` | `hits` | not owed — text says `src/core/` | not owed — subject is inheritance | owed, **not written**: `00-scaffold`'s |
| R-ERR-01 | `core_files` | `hits` | not owed — text says `src/core/` | not owed — subject is a return type | **written** |
| R-ERR-02 | `core_headers` | `hits` | **written** — text is unscoped, check reads `src/core/*.h` | not owed — subject is an attribute | **written** (two: return-type spellings, and the line anchor) |
| R-ERR-03 | `src_files` | `hits` | not owed — text says under `src/` | not owed — subject is a keyword | owed, **not written**: `00-scaffold`'s |
| R-ERR-04 | `src_files` | `hits` | not owed — text says under `src/` | not owed — subject is a call | owed, **not written**: `00-scaffold`'s |
| R-PROTO-05 | `src_files` | `raw_hits` | not owed — text says under `src/` | **written** — the subject is a path, which can sit in a comment, and the text rules that it counts | **written** |
| R-STYLE-01 | `sources()` | — (clang-format) | not owed — text says the whole repo | — | — |
| R-STYLE-02 | `tidy_sources()` | — (clang-tidy) | **owed, not written**: `00-scaffold`'s | — | — |
| R-CLEAN-02 | `tidy_sources()` | — (clang-tidy) | **owed, not written**: `00-scaffold`'s | — | — |
| R-CLEAN-04 | `tidy_sources()` | — (clang-tidy) | **written** | — | — |

Every clause this phase owes is written; every one still owed belongs to a rule `00-scaffold`
bound, and §Out of scope releases them with their owner. The `tidy_sources()` patterns are **git
pathspecs, not shell globs** — `git ls-files` lets `*` cross `/`, so `src/core/*.h` and
`tests/*.cpp` read recursively (measured: `git ls-files 'docs/*.md'` returns 38 files in
subdirectories). The set is wider than it reads, never narrower.

The remaining `test:`-bound rules run no scanner this table covers: R-PROTO-02, R-PROTO-03 and
R-PROTO-04 (`tests/test_ps2_codec.py`, whose liveness is its three rejection cases and M4),
R-ARCH-02 (`tests/test_boundaries.sh`), R-SEC-01 (`tests/test_secrets.sh`), R-TOOL-01 and R-TOOL-02
(`tests/test_tool_versions.sh`), R-PROC-01 (`tests/test_rule_traceability.py`) and R-PROC-02
(`tests/test_phase_docs.sh`).

**One limit is not a scanner's and belongs beside them.** `tests/test_checks_are_live.py` holds
`NO_MUTATE = {"test_style.sh"}`, so of the six `.sh` checks in `tests/`, five are mutation-tested
and `tests/test_style.sh` is not. All four rules bound to that file — R-STYLE-01, R-STYLE-02,
R-CLEAN-02 and this phase's **R-CLEAN-04** — therefore stand on the accounting property alone:
their lines are proven to be *printed*, not proven to *fire*. §Out of scope releases the closing of
that gap, with the reason.

## clang-tidy's header filter

`.clang-tidy` sets `HeaderFilterRegex: '(src|tests)/.*'`, which is what `00-scaffold` set and what
R-STYLE-02 and R-CLEAN-02 are bound to. This phase narrowed it to `src/.*` when it enabled
`readability-magic-numbers`, to keep the literals under `tests/vectors/` — R-CLEAN-04's own stated
exception — out of reach, and **reverted that on 2026-09-17 having measured that the narrowing
bought nothing**: those literals sit in `constexpr` initializers, which `readability-magic-numbers`
never diagnoses either way. What it cost was real — it silenced naming and function-size
diagnostics for every header under `tests/`, which are two other phases' bindings, and it made
lint's reach depend on the checkout path, the regex being unanchored and matched against the
absolute path. The `tests/vectors/` exception therefore rests on the `constexpr` blindness recorded
in §Observed conventions, not on the filter. The cost of the revert is runtime: `make test` goes
from about 1:40 to about 2:30 against a 3m cap, because clang-tidy now analyses the headers under
`tests/` as includes.

## Files this phase writes

| file | contents |
|---|---|
| `src/core/ps2_protocol.h` | wire constants — `kFrameStart`, `kCmdPoll`, `kReadyByte`, `kPadByte`, and the config-mode command bytes `kCmdConfig`, `kCmdSetMode`, `kConfigEnter`/`kConfigLeave`, `kModeDigital`/`kModeAnalog`/`kModeLocked` — plus `enum class ControllerId`, `id_from_byte` returning `std::optional<ControllerId>`, `payload_len`, `frame_len`, the three id bytes and a frame-geometry block. Header-only `constexpr`. **This list is illustrative, not exhaustive**: a reader who needs the full set reads the header. |
| `src/core/ps2_frame.h` / `.cpp` | `Ps2Frame` as §The frame describes it, `enum class DecodeStatus`, `DecodeOutcome`, `[[nodiscard]] std::expected<Ps2Frame, DecodeStatus> decode( … )` |
| `src/core/guitar_state.h` / `.cpp` | `GuitarState` (the ten controls plus the whammy), `Fret`, `map_frame`, the active-low→active-high inversion, and the id gate §Goal describes |
| `src/core/link.h` / `.cpp` | `enum class LinkState`, `enum class FaultCause`, `Link`, `step` — all as §The link fixes them |
| `src/core/hid_report.h` / `.cpp` | `Button`, `HidReport`, `build_report`, and the byte layout `08-usb-hid` writes its descriptor from: `kButtonCount`, `kBitsPerByte`, `kButtonBytes`, `kWhammyOffset`, `kReportLen` |
| `tests/vectors/` | the ten headers in §Vectors plus `README.md`: why the vectors are hand-written, the frame shape, and that digital buttons are active low — so nothing pressed is `0xFF 0xFF`, not `0x00 0x00` |
| `tests/ps2_codec_cases.cpp` | the assertions. Deliberately **not** named `test_*`: the Makefile glob would build and run it a second time, and the driver is the single entry point |
| `tests/test_ps2_codec.py` | the driver: compiles and runs the cases against the real `src/core/`, prints one `ok:`/`FAIL:` line per case and per rule, and carries the three rejection cases |
| `tests/test_repo_shape.sh`, `tests/test_style.sh`, `.clang-tidy` | the checks behind R-ERR-01, R-ERR-02 and R-CLEAN-04, and the clang-tidy invocation they need |
| `docs/constraints.md` | the seven rules this phase settled, each with its binding and its scope clauses |
| `docs/adr/0011-*.md`, `docs/adr/0012-*.md` | the `step` signature and the `DecodeStatus` membership decisions |
| `docs/adr/0007-error-model.md` | **status line only**: it names the three later ADRs that supersede parts of it. An ADR's body is immutable; its status is the mutable part |
| `docs/phases/01-ps2-codec/verify.md` | the operator procedure (R-PROC-02) |
| `CLAUDE.md` | **not written by this phase.** The operator amended it mid-phase, adding the three pre-validation checks to §Conventions. It appears in the diff only because the base ref `24d489f` precedes that edit |

## Context pointers

Regenerated against the tree on 2026-09-17, which is what this section is for: it lists files to
open, so it must match the tree even though the Goal and the Plan must not be derived from it.

- `CLAUDE.md` — the architecture paragraph, the hardware-safety rules, the three pre-validation
  checks in §Conventions, and the reading rule that scopes this list.
- `docs/phases/01-ps2-codec/notes.md` — 2575 lines. `grep -c '^### Round ' ` returns **28** and
  `grep -c '^## Validation'` **13**; the first implementation round predates the heading style and
  sits unheaded at the top of §Deviations, so the rounds are 29. §Deviations is where every decision this spec states was argued; §Debt and
  §For later phases are what this phase hands on, including six taste items from the last review.
  Read it before changing anything here.
- `docs/phases/00-scaffold/notes.md` — the dependency. §Debt, §For later phases and §Owed. Three
  patterns this phase inherits: amending a Goal clause creates reconciliation debt the reviewer
  cannot find; a fix written to close a finding tends to introduce a new unfounded claim; a count
  written in prose has no check behind it.
- `docs/constraints.md` — §Invariants (the rule catalogue), §Layering, §Error handling, §Testing,
  and §Observed conventions, which carries this phase's clang-tidy findings and the
  `readability-magic-numbers` measurement the `tests/vectors/` exception rests on.
- `docs/adr/0002-hardware-free-core.md` — why `core` has no SDK header and what that buys.
- `docs/adr/0003-generic-hid-gamepad-own-identity.md` — the control set the HID report carries.
- `docs/adr/0007-error-model.md` — the link lifecycle as a state machine; a missing `ACK` is a
  transition, not a failed call. Its status line names what ADR-0009, ADR-0011 and ADR-0012
  superseded in it.
- `docs/adr/0009-std-expected.md` — pure decoding returns `std::expected<T, Status>`; `.value()`
  calls `abort` under `-fno-exceptions` and is forbidden (R-ERR-04).
- `docs/adr/0011-pure-link-step.md` — `step` is a pure transition over a decode outcome plus
  **elapsed** microseconds supplied by the caller.
- `docs/adr/0012-decode-status-carries-decode-outcomes-only.md` — a status belongs in
  `DecodeStatus` if and only if it is decidable from the bytes of a single frame.
- `docs/adr/0010-macos-only-development-host-agnostic-device.md` — development is macOS with
  Homebrew LLVM. Its flag prescription is superseded by a finding in §Observed conventions:
  `clang-tidy` needs `-xc++` on a header **and** `-isysroot "$(xcrun --show-sdk-path)"`.
- `src/core/` — the nine files this phase owns: `ps2_protocol.h`, `ps2_frame.h`/`.cpp`,
  `guitar_state.h`/`.cpp`, `link.h`/`.cpp`, `hid_report.h`/`.cpp`. `docs/index/src-core.md`
  locates them.
- `tests/vectors/` — the ten vector headers and `README.md`. `docs/index/tests.md` lists them.
- `tests/ps2_codec_cases.cpp` — 33 case functions and 3 rule functions, run by the driver below.
- `tests/test_ps2_codec.py` — the driver and its three rejection cases.
- `tests/test_repo_shape.sh` — the pattern for a grep-backed rule: header `RULE` markers, one
  `find_*` per rule, the `hits`/`raw_hits` scanners, the `src_files`/`core_files`/`core_headers`
  lists, `report`, and `reject`/`accept`/`wiring` cases. Read its header comment before extending
  it.
- `tests/test_style.sh` — the clang-tidy invocation, `sources()` and `tidy_sources()`, and the
  R-STYLE-01/R-STYLE-02/R-CLEAN-02/R-CLEAN-04 report lines.
- `tests/test_checks_are_live.py` — the accounting property every check file must satisfy, and
  `NO_MUTATE`, which is why `tests/test_style.sh` is not mutation-tested. It picks up `test_*.sh`
  and `test_*.py` only, which is why the driver is Python and why the cases file is not named
  `test_*`.
- `tests/test_rule_traceability.py` — what fails the build on a `test:` binding whose file does not
  exist or carries no `RULE <id>` marker.
- `.clang-tidy` — naming rules, `readability-magic-numbers`, and the `HeaderFilterRegex` that
  §clang-tidy's header filter explains.
- `Makefile` — `CORE_SRC` is `$(wildcard src/core/*.cpp)`; `make test` runs every
  `tests/test_*.{cpp,sh,py}`. Nothing in this phase edits it. Its `CXXFLAGS` are
  `-std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc`, and `tests/test_ps2_codec.py` repeats
  that list by hand because it compiles the cases itself rather than through `make`. The duplicate
  was byte-for-byte identical when last measured (2026-09-17); if the two drift, the only gate that
  compiles `src/core/` stops compiling it the way the project does.
- `.claude/workflow/boundaries.rules` — the enforced layering. Read its header: R-ARCH-01 is **not**
  expressible as a layer rule, and is enforced by the host build and by greps in `tests/` instead.

## Plan

**No work is outstanding.** Everything this phase set out to do is in the tree, and `notes.md`
is the account of how it got there — this section stays empty rather than logging it, because a
Plan that lists what already happened duplicates `notes.md` and goes stale against it. If a later
round finds work, it is written here and nowhere else.

## Acceptance criteria

```
make test                                                      # expect: OK, exit 0
make lint                                                      # expect: exit 0
time make test                                                 # expect: real < 3m
grep -c 'planned: 01-ps2-codec' docs/constraints.md            # expect: 0
grep -c 'planned: 03-pio-bus' docs/constraints.md              # expect: 4 — R-SAFETY-07, R-PROTO-01, R-PROTO-06, R-ERR-05
ls tests/vectors/*.h | wc -l                                   # expect: 10, the files in §Vectors
find tests -name '*.h' -not -path 'tests/vectors/*' | wc -l    # expect: 0
grep -rn 'tests/vectors' src/ | wc -l                          # expect: 0 (R-PROTO-05)
grep -rn '\.value( *)' src/ | wc -l                            # expect: 0 (R-ERR-04)
grep -rn 'TODO(09-guitar-observe)' src/core/ | wc -l           # expect: 3 (§Goal names which covers what)
grep -c "HeaderFilterRegex: '(src|tests)/.*'" .clang-tidy      # expect: 1 (§clang-tidy's header filter)
ls docs/adr/0011-*.md                                          # expect: exactly one file
ls docs/adr/0012-*.md                                          # expect: exactly one file
grep -c 'ADR-0011' docs/adr/0007-error-model.md                # expect: 1 or more
python3 tests/test_ps2_codec.py                                # expect: exit 0
make test 2>&1 | grep -E 'accounting: test_ps2_codec.py'       # expect: 3 rule(s) — R-PROTO-02, R-PROTO-03, R-PROTO-04
make test 2>&1 | grep -E 'accounting: test_style.sh'           # expect: 4 rule(s) — R-STYLE-01, R-STYLE-02, R-CLEAN-02, R-CLEAN-04
sh tests/test_repo_shape.sh | grep 'wiring cases'              # expect: 10/10 — R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-ERR-01, R-ERR-02, R-ERR-03, R-ERR-04, R-PROTO-05
sh tests/test_repo_shape.sh | grep 'false-positive cases'      # expect: 23 (floor 23) — see the derivation below
sh tests/test_repo_shape.sh | grep 'rejection cases'           # expect: 64 — one per `reject` line; see the derivation below
make test 2>&1 | grep -E 'neutered: ([0-9]+)/\1 caught'        # expect: one line — the two sides equal
make test 2>&1 | grep -E 'alternations: ([0-9]+)/\1 caught'    # expect: one line — the two sides equal
sh tests/test_phase_docs.sh                                    # expect: exit 0
```

**The two case counts are derived, not remembered.** `rejection cases` is one per `reject`
invocation in `tests/test_repo_shape.sh` and `false-positive cases` is one per `accept`
invocation, so both recompute from the file rather than having to be maintained against it:

```
grep -cE '^reject ' tests/test_repo_shape.sh                   # the 64
grep -cE '^accept ' tests/test_repo_shape.sh                   # the 23
grep -oE 'RULE R-[A-Z]+-[0-9]+' <check file> | sort -u          # the rule sets named above
```

The `rule(s)` and `wiring` counts recompute the same way: a check declares its rules with `RULE`
markers in its header, so the members of "3 rule(s)", "4 rule(s)" and "10/10" are whatever that
grep returns for `tests/test_ps2_codec.py`, `tests/test_style.sh` and `tests/test_repo_shape.sh`.

Measured 2026-09-17, stated as a distribution because a total with no members is the defect this
phase kept paying for. The 64 rejection cases: **31** R-ARCH-01, **8** R-ARCH-03, **7** R-CLEAN-09,
**4** each for R-ERR-01, R-ERR-02 and R-ERR-03, **2** each for R-PROTO-05 and R-CLEAN-05, **1**
each for R-ERR-04 and R-CLEAN-03. The 23 false-positive cases: **5** `find_arch01`, **4**
each for `find_err02` and `find_clean03`, **3** `find_err01`, **2** `find_arch03`, and **1** each
for `find_clean05`, `find_clean09`, `find_err03`, `find_err04` and `find_proto05` — one per finder,
which is the property the count is for. The floor equals the count, so it asserts "no accept case
was deleted" rather than "at least this many exist".

**Two of those need the real `grep`.** The `neutered:` and `alternations:` lines use an ERE
backreference (`([0-9]+)/\1`), which is the point — it makes the two sides *equal* rather than
merely both present. Under an agent session `grep` is shadowed by `ugrep`, which rejects `\1` as
`invalid escape`; run those two under `/usr/bin/grep`. An environment fact, not a project defect.
Run them as two commands and never as one `-E` with both patterns: capture-group numbers are global
to an expression, so the second backreference silently points at the first group and that line
stops matching anything.

**The liveness evidence, and which of it is automated.** Three of the four mutations below are the
driver's own rejection cases, and `rejection cases: 3/3` is their result:

```
# M1 — the decoder stops refusing an unknown id     → automated; R-PROTO-03 turns FAIL
# M2 — the digital path reads a whammy byte         → automated; R-PROTO-04 turns FAIL
# M3 — a truncated frame yields a frame             → automated; R-PROTO-02 turns FAIL
# M4 — delete the real_run() call in tests/test_ps2_codec.py
#      → NOT automated. Run by hand in a scratch copy: expect test_checks_are_live.py to exit
#        non-zero naming all three rules as "reported by nothing but its own cases".
```

M4 is the one a validator must perform. Run it in a **`cp -a` copy of the working tree**, which
carries both the uncommitted work and `.git`. Both halves are load-bearing: a `git worktree` checks
out HEAD, so uncommitted work is not in it and the mutation has no anchor to move; and the copy
needs `.git` because `test_checks_are_live.py` runs every `test_*` file on the tree it is given,
`tests/test_secrets.sh` included, and that one scans history. Write it as a `python3 -c` block that
reads the file, replaces the anchor, **asserts the replacement changed the text**, writes it back
and runs the command — a block whose anchor has moved otherwise mutates nothing and reports the
exit 0 it was handed.

## Notes to `/validate-phase` — three package workarounds, all temporary

None is a fact about this phase. All three are defects in the Belay commands this repo runs, and
they do not share an upstream state: **1 and 2 are fixed upstream in 792e9d9**, this repo is pinned
at **f001884** and takes the update when the phase closes; **3 is filed and not fixed** — the
`- base:` staleness entry dated 2026-09-15 in `~/.claude-belay/feedback/pico-sg2hid.md`. Delete each
when its fix lands.

**1. Tell step 5's reviewer which paths were held out of its diff.** When dispatching the
independent review, name in the prompt every path excluded from the diff it receives — `notes.md`
always, plus anything the `.claude/workflow/installed` subtraction removed — and say that their
absence is not a finding. Without it the reviewer reads in this spec that the phase writes
`notes.md`, fails to find it in the diff, and returns a `contradicts` that no change to the code can
clear. This paragraph is also addressed to the reviewer itself, which receives this file:
**`notes.md` is written and is deliberately withheld from you.**

**2. A boundary sweep of zero files looks exactly like a clean one.** At this version
`scripts/check.sh --files` skips any argument that is not an existing file — silently, with
`continue` — and still prints `check: all gates passed`. Under zsh an unquoted expansion is not
word-split, so `check.sh --files $FILES` arrives as a single concatenated argument, which is a path
that does not exist, and the sweep covers nothing. Two defences, both needed:

- Pass the paths one per argument — `xargs scripts/check.sh --files < <file-set>` — and check the
  input count (`wc -l`) against the file set. The count cannot be read off the output: `--files`
  prints **nothing** per file and only the closing line.
- Prove the sweep live before recording it: add a violation an active rule must catch —
  `#include "src/hal/bus_io.h"` in any `src/core/` file is `deny core -> hal` — confirm the gate
  exits non-zero and names the rule, and restore the file. An SDK include is **not** the probe to
  use: `boundaries.rules` says in its own header that R-ARCH-01 is not expressible as a layer rule.

**3. Check that the file set is not empty before trusting any gate.** Before running step 1, confirm
that `notes.md` §Outcome's `- base:` line names a real ref rather than the literal `working tree`,
and that the resulting file set has the number of files this phase actually touched.
`/validate-phase` derives its file set from the working tree unless that line overrides it, so the
moment a phase's work is committed the default yields **nothing** — and a sweep of nothing, a review
of nothing and a closure test over nothing all report `pass`.

## Out of scope

- Anything that touches a GPIO, a PIO state machine, a clock or a timer — `03-pio-bus`. `core`
  supplies "given these bytes and this elapsed time, decide".
- `src/core/pins.h` and the pin table — `02-wiring`. R-SAFETY-01..03 stay `planned: 02-wiring`.
- Running the config-mode sequence, or verifying a controller answered analog mode on a real bus —
  `07-analog-mode`. This phase declares the command bytes and the states; driving them is that
  phase. **Tightening `kNegotiationTimeoutUs`** is that phase's too.
- **Asserting the two behaviours §The link and §Goal declare unasserted** — `us_in_state`'s
  saturation and the over-long buffer. Both are in `notes.md` §Debt with the reason and an upgrade
  path; `03-pio-bus` is the natural owner of the second, being the first phase with a real caller.
- **Mutating `tests/test_style.sh`** — released to no phase. `tests/test_checks_are_live.py:49`
  holds `NO_MUTATE = {"test_style.sh"}`; closing it means mutating a clang-tidy invocation, which is
  tool configuration rather than a grep alternative, and that is harness work. Until then the four
  rules bound to that file stand on the accounting property.
- **Strengthening `tests/test_checks_are_live.py` beyond the accounting property for `.py` files** —
  released here to no phase, deliberately. `00-scaffold` named this phase as the owner, this phase
  did not close it, and it added a second `.py` check, so the exposure grew. What stands behind
  `tests/test_ps2_codec.py` instead is its three rejection cases and M4. It belongs to whichever
  phase next touches that harness, alongside the `clang-query` + `compile_commands.json` upgrade
  `03-pio-bus` already owes and the "no new untracked paths after `make test`" check `notes.md`
  §For later phases names.
- **Writing the clauses the assignment table marks "owed, not written"** — all of them belong to
  rules `00-scaffold` bound: the file-scope clause for R-CLEAN-03, R-CLEAN-05, R-STYLE-02 and
  R-CLEAN-02, and the spelling clause for those two plus R-ARCH-01, R-ARCH-03, R-CLEAN-09, R-ERR-03
  and R-ERR-04. None was measured here, so no form is claimed to escape any of them; the method is
  in `notes.md` round 28. This phase writes the clauses for R-ERR-01, R-ERR-02, R-PROTO-05 and
  R-CLEAN-04 only — the bindings it moved or re-scoped itself.
- **Widening R-ERR-02's check beyond `src/core/*.h`** — no phase yet; the narrowing is recorded in
  the rule's own text and the residual hole has no caller who could ignore a result.
- **Re-narrowing `HeaderFilterRegex`** — no phase. Ruled out in §clang-tidy's header filter and not
  to be revisited as a way of buying back `make test` runtime; the standing answer to the 3m cap is
  to raise it in a re-expansion or cut mutants.
- The TinyUSB HID descriptor — `08-usb-hid`. This phase fixes the report's byte layout so the
  descriptor can be written from it; it writes no descriptor and no USB code.
- The emulator — `05-emulator`. No file under `src/emu/`.
- Correcting any byte position against the real guitar — `09-guitar-observe`. Note that
  `unknown_id.h` uses `0x79` deliberately; if that phase finds the SG uses it, the vector is
  repointed at another undeclared byte rather than the id simply being added.
- A CMake or firmware build, and `-fno-exceptions -fno-rtti` — `03-pio-bus` owns R-ERR-05.
- `#embed` for the vectors — no phase; ruled out in §Vectors.
- The six taste items from the 2026-09-17 review, in `notes.md` §For later phases — including
  `case_report_is_wide_enough`, which asserts tautologies of the constants it restates. Recorded as
  debt rather than fixed in a closing round.
