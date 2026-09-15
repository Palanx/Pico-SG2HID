# Phase 01-ps2-codec — Pure PS2 frame codec, id table, link state machine, button/axis map, HID report builder

<!-- Re-expanded 2026-09-14 after /validate-phase's iteration-3+ escape. The code from
     rounds 1-4 is on disk and green; this spec is derived from the PHASES.md row's Goal and
     from notes.md, never from that code. Where the two disagree, §Plan says so and the code
     is what changes. notes.md is the account of how we got here and is not rewritten. -->

## Goal

After this phase `src/core/` holds the whole protocol brain of the project with no hardware
anywhere near it: a decoder that turns the bytes of one PS2 poll frame into a typed frame or
a typed failure, a controller-id table, a link lifecycle state machine, a map from a decoded
frame to the guitar's controls, and a builder that packs those into the HID report bytes
`08-usb-hid` will hand to TinyUSB. All of it compiles and runs on the laptop under
`make test` with a C++23 compiler and `python3` and nothing else (R-PROC-04), because `core`
includes no SDK header and performs no I/O (ADR-0002, R-ARCH-01).

The observable behaviour is a decoder that is **refusing by default**. Each of the ten
vectors enumerated in §Vectors is fed to `decode`, and to `map_frame` where §Vectors names a
mapping, and produces exactly the outcome that table names:

- A header byte that is not a declared controller id produces `UnknownId` and no frame
  (R-PROTO-03).
- A declared id whose ready byte is not `kReadyByte` produces `NotReady` and no frame.
- A frame the bus cut short produces `AckTimeout` and no frame — never a frame with some
  fields filled in (R-PROTO-02).
- A digital-mode frame maps to a whammy axis at rest, a value the codec supplies and never a
  byte reinterpreted out of a digital payload (R-PROTO-04).
- A config-mode frame decodes and then reports *nothing*: all ten controls released and the
  whammy at rest. Its payload is all `0x00` and digital buttons are active low, so an ungated
  map would call that every control pressed.

The ten controls, named here because the rest of this document quantifies over them: five
frets (green, red, yellow, blue, orange), strum up, strum down, start, select, tilt. The
whammy is an axis, not a control, and is counted separately throughout.

**A buffer longer than the frame the header announces is accepted, and the bytes past the
announced length are ignored.** Chosen, not left to fall out of the code: the header is the
only authority on how long a frame is, so the span handed to `decode` is a *capacity* and
never a claim about the frame. `03-pio-bus` will hand `core` a fixed-size shift buffer, and
requiring `hal` to trim it first would put the length arithmetic `2 * (header & 0x0F)` on both
sides of the layer boundary — which is the duplication `core` exists to prevent. Refusing an
over-long buffer would also refuse the ordinary case on real hardware. Nothing asserts this
contract today: `notes.md` §Debt carries it.

**Where two refusals are both true, the order is fixed, and it is chosen rather than
inherited from the order somebody happened to write the checks in.** Each refusal is taken at
the first point where it is decidable, and the earliest decidable one wins:

1. fewer bytes than a header and a ready slot → `AckTimeout`. Nothing else is knowable yet.
2. the header is not a declared id → `UnknownId`. This one cannot move later: `frame_len`
   needs the id, so the length is not yet knowable above it.
3. fewer bytes than the id announces → `AckTimeout`.
4. the ready byte is not `kReadyByte` → `NotReady`.

**The consequence, because a real bus produces it:** a frame that is *both* cut short *and*
carrying `0xFF` at the ready slot reports `AckTimeout`, not `NotReady`. R-PROTO-02 says a
frame the bus cut short "reports the abort", so the abort outranks any refusal that is still
evaluable after it; and `0xFF` at the ready slot of a short frame is an undriven `DATA` line,
not a controller stating it is not ready — a controller that died mid-frame said nothing about
readiness at all. `NotReady` is for a frame that arrived **complete** and whose ready byte is
wrong. `tests/vectors/truncated_not_ready.h` is the overlap, and §Plan step 12 is the code
change that made the order this one.

R-PROTO-02, R-PROTO-03 and R-PROTO-04 are bound to `tests/test_ps2_codec.py`, and that check
is proven live in the sense `00-scaffold` established: mutating the codec so it stops
refusing makes the check fail and name the rule.

### What this phase does not prove, declared rather than discovered later

That any byte position in the SG's payload is the right one. Four concerns are written from
the documented protocol and none of them is measured: the three controller id bytes; the
button bit positions and masks; the whammy's payload index; and the whammy's rest value.

**They are covered by three `TODO(09-guitar-observe)` markers, not four, and the mapping is
written out here because three documents have disagreed about the number.** One marker in
`src/core/ps2_protocol.h` covers the id bytes; one in `src/core/guitar_state.h` covers the
whole block of button positions and masks — a block, not a single constant, which is why no
count of `constexpr`s appears in this paragraph; and a third, also in `guitar_state.h`, covers
the whammy's index and its rest value together. Three is the number §Acceptance criteria pins
and the number `verify.md` §1 tells the operator to expect. `09-guitar-observe` exists to
confront all four concerns with the real device. A vector here asserts that the codec does
what the codec says, not that the guitar agrees.

### Rule work — landed, stated so the next round does not re-do it

The seven rules that were `planned: 01-ps2-codec` are all settled and
`grep -c 'planned: 01-ps2-codec' docs/constraints.md` is **0**. R-PROTO-01 was rebound to
`03-pio-bus` (bit order is a PIO property); R-PROTO-02 was split, its bus clause becoming the
new R-PROTO-06, `planned: 03-pio-bus`; R-PROTO-03, R-PROTO-04 and the core clause of
R-PROTO-02 bind to `tests/test_ps2_codec.py`; R-ERR-01 and R-ERR-02 bind to
`tests/test_repo_shape.sh`, each with its narrowing recorded in the rule's own text;
R-CLEAN-04 binds to `tests/test_style.sh`. Nothing in this re-expansion moves a binding.

## Context pointers

- `CLAUDE.md` — the architecture paragraph, the hardware-safety rules, and the reading rule
  that scopes this list. `core` depends on nothing; `make` is the only entry point.
- `docs/phases/01-ps2-codec/notes.md` — the account of rounds 1-6 and of three validation
  rounds. §Deviations is where every decision this spec states was argued; §Debt and §For
  later phases are what this phase hands on. Read it before changing anything here.
- `docs/phases/00-scaffold/notes.md` — the dependency. §Debt, §For later phases and §Owed.
  Three patterns this phase inherits, at line 3256: amending a Goal clause creates
  reconciliation debt the reviewer cannot find; a fix written to close a finding tends to
  introduce a new unfounded claim; **a count written in prose has no check behind it.**
- `docs/constraints.md` — §Invariants (the rule catalogue and the seven rules above),
  §Layering, §Error handling, §Testing, and §Observed conventions, which carries this
  phase's clang-tidy findings.
- `docs/adr/0002-hardware-free-core.md` — why `core` has no SDK header and what that buys.
- `docs/adr/0007-error-model.md` — the link lifecycle as a state machine, still the decision
  of record; a missing `ACK` is a transition, not a failed call. Its `step` signature sketch
  is superseded by ADR-0011 and its `DecodeStatus` sketch by ADR-0012.
- `docs/adr/0009-std-expected.md` — pure decoding returns `std::expected<T, Status>`;
  `.value()` calls `abort` under `-fno-exceptions` and is forbidden (R-ERR-04).
- `docs/adr/0011-pure-link-step.md` — `step` is a pure transition over a decode outcome plus
  **elapsed** microseconds supplied by the caller. Never a timestamp: the 32-bit wraparound
  subtraction stays `hal`'s.
- `docs/adr/0012-decode-status-carries-decode-outcomes-only.md` — the membership rule: a
  status belongs in `DecodeStatus` if and only if it is decidable from the bytes of a single
  frame, with no history and no knowledge of what was asked. Anything needing history is a
  `FaultCause` on the link. This is what §Link below expands.
- `docs/adr/0003-generic-hid-gamepad-own-identity.md` — the control set the HID report
  carries.
- `docs/adr/0010-macos-only-development-host-agnostic-device.md` — development is macOS with
  Homebrew LLVM. Its flag prescription is superseded by a finding in §Observed conventions:
  `clang-tidy` on a header needs `-xc++` **and** `-isysroot "$(xcrun --show-sdk-path)"`, two
  flags, and not the libc++ include path.
- `src/core/` — the nine files this phase owns; `docs/index/src-core.md` locates them.
- `tests/test_repo_shape.sh` — the pattern for a grep-backed rule: header `RULE` markers, one
  `find_*` per rule, `report`, and a reject/accept/wiring case per rule. Read its header
  comment before extending it.
- `tests/test_style.sh` — the clang-tidy invocation and R-CLEAN-04's binding; `tidy_sources()`
  decides which files are checked.
- `tests/test_checks_are_live.py` — the accounting property every check file must satisfy:
  each rule id declared in a header produces an `ok:`/`FAIL:` line that a case line cannot
  produce. It picks up `test_*.sh` and `test_*.py` only, which is why the driver is Python and
  why the assertions it compiles are not named `test_*`.
- `tests/test_rule_traceability.py` — what fails the build on a `test:` binding whose file
  does not exist or carries no `RULE <id>` marker.
- `.clang-tidy` — naming rules, `readability-magic-numbers`, and the `HeaderFilterRegex`
  §Vectors depends on.
- `Makefile` — `CORE_SRC` is `$(wildcard src/core/*.cpp)`; `make test` runs every
  `tests/test_*.{cpp,sh,py}`. Nothing in this phase edits it. Its `CXXFLAGS` are
  `-std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc`, and `tests/test_ps2_codec.py`
  repeats that list by hand because it compiles the cases itself rather than through `make`.
  The duplicate was byte-for-byte identical when last measured (2026-09-15). It is a
  duplicate, which is why it is written here: if the two drift, the only gate that compiles
  `src/core/` stops compiling it the way the project does.
- `.claude/workflow/boundaries.rules` — the enforced layering. Read its header: R-ARCH-01 is
  **not** expressible as a layer rule, and is enforced by the host build and by greps in
  `tests/` instead.

## The frame: shape and storage

`decode` takes `std::span<const std::uint8_t>` holding `[header][ready][payload…]` — the
controller's response **with its first byte dropped**, because that byte answers the address
byte and carries nothing. Dropping it is `hal`'s job (`03-pio-bus`).

Payload length is `2 * (header & 0x0F)`, documented protocol, which makes a digital frame two
payload bytes and an analog frame six.

**`Ps2Frame` carries the controller id and the payload, and nothing else. The payload is a
fixed-width `std::array<std::uint8_t, kMaxPayloadLen>`, where `kMaxPayloadLen` is
`payload_len( ControllerId::Analog )`, and every byte past the length the id announces is
zero.** Fixed width because `core` allocates nothing; zero-filled because an out-of-range read
must be deterministic rather than garbage.

**It stores no length, deliberately.** How many payload bytes are meaningful is
`payload_len( id )`, so a stored length would be a second source of truth for one fact and the
only thing it could add is the possibility of disagreeing with the id. Everything that needs
the length recomputes it: `payload_matches` in the cases file loops to
`payload_len( frame.id )`, and the zero-fill case starts its scan at
`payload_len( ControllerId::Digital )`. Both read as redundant until you know there is no
member to read, which is why it is written here.

This is load-bearing and validation round 3 found the spec crediting the wrong mechanism for
it. `kWhammyIndex` is 5, so a *naive* analog read of a two-byte digital payload never touches
either byte `digital_whammy_absent.h` carries — **it reads the zero fill.** The vector traps
the bug because the fill is `0x00` and `kWhammyRest` is `0x80`, so an ungated read yields a
non-rest axis and the case fails. The two literal bytes make the vector realistic; the zero
fill is what makes it a trap. Both facts are stated in the vector's own comment and now here.

## The link: states, transitions, faults, and where the clock starts

`step` is a pure transition: given the current `Link`, a decode outcome, and the microseconds
**elapsed since the previous step**, it returns the next state (ADR-0011). It reads no clock.

**`LinkState` has exactly four members:** `Absent`, `Negotiating`, `DigitalStreaming`,
`AnalogStreaming`. This phase owns all four and every transition between them, including the
exit from `Negotiating` — the alternative is a trap state, since a controller that keeps
answering in config mode would hold the link there forever, contradicting §Error handling's
"the firmware retries rather than stopping".

**The transition rule, stated before the table it expands: the next state is a function of
the decode outcome alone, identical from every source state — except the negotiation timeout,
which is a function of the current state and `us_in_state`.** The table is that rule written
out, and it is the enumeration the quantifier above owes:

| decode outcome | next state, from any of the four | `last_fault` |
|---|---|---|
| ok, `ControllerId::Digital` | `DigitalStreaming` | unchanged |
| ok, `ControllerId::Analog` | `AnalogStreaming` | unchanged |
| ok, `ControllerId::Config` | `Negotiating` | unchanged |
| `DecodeStatus::UnknownId` | `Absent` | `FaultCause::UnknownId` |
| `DecodeStatus::NotReady` | `Absent` | `FaultCause::NotReady` |
| `DecodeStatus::AckTimeout` | `Absent` | `FaultCause::AckTimeout` |

Plus one rule the table cannot express, evaluated when the outcome above would leave the link
in `Negotiating`: if the accumulated time in that state has passed `kNegotiationTimeoutUs`
strictly — the bound is exclusive, and `link.h` and the timeout case both say "outlasted" and
"past" for the same reason — the next state is
`Absent` with `FaultCause::Negotiating`.

**`us_in_state` counts time since the transition *into* the current state.** The `elapsed_us`
handed to the step that *enters* a state is time that passed before that transition, so it is
attributed to the state being left, and the counter resets to zero on any change of state.
The consequence is stated because it reads like an off-by-one and is not: **the negotiation
timeout can only fire on the second consecutive step that leaves the link in `Negotiating`,
never on the one that enters it.**

**The accumulation saturates at `UINT32_MAX`; it does not wrap.** Wrapping would silently
reset the counter roughly every 71.6 minutes, which is the one behaviour that turns a
permanently stuck controller into one that looks fine on a schedule. Saturating is unreachable
in practice — it needs a caller that stops polling for over an hour and then resumes — and is
specified anyway, because "unreachable" is a claim about the caller and `core` does not get to
make claims about its callers. Nothing asserts it: see `notes.md` §Debt.

**`FaultCause` has exactly five members:** `None`, `AckTimeout`, `UnknownId`, `NotReady`,
`Negotiating` — one per way the link can drop, plus `None` for a link that has never dropped.
`last_fault` is **history, not current state**: it names the cause of the most recent drop and
is *not* cleared by a subsequent good frame. `LinkState` is where the current state lives;
trace mode (R5) prints the fault after recovery, which is precisely when clearing it would
have destroyed the thing being traced. `07-analog-mode` will add at least one cause
("answered the config sequence, then declined analog mode"), which ADR-0012's membership rule
sends here rather than to `DecodeStatus`.

**The budget is not owned here.** `kNegotiationTimeoutUs` is 100 ms, picked as a "something is
wrong" bound two orders of magnitude above a sequence of a few frames at ~1 ms. Nothing has
timed a real negotiation, so it is a guess carrying a `belay-debt:` marker that says so, and
tightening it is `07-analog-mode`'s — the first phase able to measure it. The structure is
decidable from `(state, outcome, elapsed_us)` and is therefore `core`'s; the number needs a
bus and is therefore not.

## Vectors

Ten files, hand-written literal `constexpr` byte arrays in C++ headers under
`tests/vectors/` (R-PROTO-05: written by hand from the protocol documentation, never generated
by `core`, never captured from the emulator), plus a `README.md` carrying the provenance half
of that rule. Headers rather than hex text because a parser in the test is code that can be
wrong about the literal it reads. `#embed` is **not** used: it embeds a file's bytes verbatim,
so it would require the vectors to be unreadable binary to be useful.

| file | frame shape | asserted outcome | rule |
|---|---|---|---|
| `digital_idle.h` | digital header, ready byte, 2 payload bytes, nothing pressed | `ControllerId::Digital`; maps to all ten controls released | — |
| `digital_pressed.h` | digital, one fret + one strum direction held | that fret and that strum pressed, the other eight released | — |
| `digital_whammy_absent.h` | digital, two payload bytes neither of which is `kWhammyRest` | whammy **at rest**; the trap is the zero fill at `kWhammyIndex`, see §The frame | R-PROTO-04 |
| `analog_idle.h` | analog header, ready byte, 6 payload bytes, sticks centred | `ControllerId::Analog`; whammy equals the byte at `kWhammyIndex` | — |
| `analog_whammy_full.h` | analog, whammy byte at full deflection | whammy reads full deflection — a value that is not `kWhammyRest`, which is what makes this the vector that proves the read path | R-PROTO-04 |
| `config_mode.h` | config-mode header, 6 payload bytes, all `0x00` | `ControllerId::Config` — recognised, not a report: it decodes, and `map_frame` yields all ten controls released and the whammy at rest, reading neither out of the payload | — |
| `not_ready.h` | a declared id whose ready byte is `0xFF`, the idle level of an undriven `DATA` line; length and payload well-formed | `DecodeStatus::NotReady`, no frame | — |
| `unknown_id.h` | header `0x79`, the DualShock 2's real full-analog id, deliberately undeclared here | `DecodeStatus::UnknownId`, no frame | R-PROTO-03 |
| `truncated_ack.h` | digital header, ready byte, payload cut short — the `ACK` never came | `DecodeStatus::AckTimeout`, no frame, and `step` moves the link to `Absent` | R-PROTO-02 |
| `truncated_not_ready.h` | digital header, `0xFF` at the ready slot, payload cut short — both faults at once | `DecodeStatus::AckTimeout`, no frame: the abort outranks the ready byte, per §Goal's precedence | R-PROTO-02 |

**`kWhammyRest` is `0x80`, and `analog_idle.h`'s centred axis byte is also `0x80`.** That is a
real coincidence and validation round 3 was right to ask: an assertion of
`whammy == kWhammyRest` on a value read *out of the payload* passes whether or not the read
path works. It is left as a coincidence rather than engineered away, because a centred axis
genuinely is rest, and the roles are split instead — stated here so no round asks again:
`analog_whammy_full.h` is what proves the payload is read, because its value is not
`kWhammyRest`; `digital_whammy_absent.h` and `config_mode.h` are what prove the rest default
is supplied by the codec; `analog_idle.h` proves neither on its own and is kept for the decode
half of its row.

**Which assertion its case writes, since the two readings are numerically identical and are
different claims:** `analog_idle`'s case asserts `whammy == kWhammyRest`. It is the weaker of
the two and the honest one — the vector's centred byte and the rest value are the same `0x80`,
so an assertion against `payload[ kWhammyIndex ]` would read as proof of the read path while
proving nothing the row above does not already cover. The table row says "the byte at
`kWhammyIndex`" because that is what the codec *does*; the case asserts the value, because
that is all this vector can honestly witness.

## Files this phase writes

| file | contents |
|---|---|
| `src/core/ps2_protocol.h` | wire constants — `kFrameStart`, `kCmdPoll`, `kReadyByte`, `kPadByte` (the filler the master sends in a slot whose value the controller ignores; declared here with the rest of the wire, used first by `03-pio-bus`, which is the phase that sends bytes), and the config-mode command bytes `kCmdConfig`, `kCmdSetMode`, `kConfigEnter`/`kConfigLeave`, `kModeDigital`/`kModeAnalog`/`kModeLocked` — plus `enum class ControllerId`, `id_from_byte` returning `std::optional<ControllerId>`, `payload_len`, `frame_len`. Header-only `constexpr`. **This list is illustrative, not exhaustive**: the header also carries the three id bytes and a frame-geometry block, and a reader who needs the full set reads the header. An earlier version of this row claimed to be exhaustive and was wrong the moment it was written — see §For later phases in `notes.md` for what it would take to make that claim mean something. |
| `src/core/ps2_frame.h` / `.cpp` | `Ps2Frame` as §The frame describes it, `enum class DecodeStatus`, `[[nodiscard]] std::expected<Ps2Frame, DecodeStatus> decode( … )` |
| `src/core/guitar_state.h` / `.cpp` | `GuitarState` (the ten controls plus the whammy), `Fret`, `map_frame`, the active-low→active-high inversion, and the id gate §Goal describes |
| `src/core/link.h` / `.cpp` | `enum class LinkState`, `enum class FaultCause`, `Link`, `step` — all as §The link fixes them |
| `src/core/hid_report.h` / `.cpp` | `Button`, `HidReport`, `build_report`, and the byte layout `08-usb-hid` writes its descriptor from: `kButtonCount`, `kBitsPerByte`, `kButtonBytes`, `kWhammyOffset`, `kReportLen`, derived from each other |
| `tests/vectors/` | the ten headers in §Vectors plus `README.md`: why the vectors are hand-written, the frame shape, and that digital buttons are active low — so nothing pressed is `0xFF 0xFF`, not `0x00 0x00`. A reader who does not know that writes an inverted vector every assertion then agrees with. |
| `tests/ps2_codec_cases.cpp` | the assertions. Deliberately **not** named `test_*`: the Makefile glob would build and run it a second time, and the driver is the single entry point. |
| `tests/test_ps2_codec.py` | the driver: compiles and runs the cases against the real `src/core/`, prints one `ok:`/`FAIL:` line per case and per rule, and carries the three rejection cases |
| `docs/adr/0011-*.md`, `docs/adr/0012-*.md` | the `step` signature and the `DecodeStatus` membership decisions |
| `docs/phases/01-ps2-codec/verify.md` | the operator procedure (R-PROC-02) |

## Plan

Rounds 1-4 landed steps 1-9; `make test` and `make lint` are green and three validation rounds
have run against them. **Steps 10 and 11 are what this re-expansion adds and are the only work
`/implement-phase` is owed.** Steps 1-9 are stated because a cold session must be able to
rebuild the phase, not because they are pending.

1. **Landed — catalogue surgery.** Touches `docs/constraints.md`, `docs/phases/PHASES.md`. The
   seven rules as §Rule work describes. The `PHASES.md` change is an **in-place edit of the
   `01` row's coarse acceptance text**, from "R-PROTO-01..04 move to `test:`" to
   "R-PROTO-02..04", plus the status transitions every command here makes. That is not the
   case `CLAUDE.md` reserves for a superseding row: a cut is superseded when the cut turns out
   wrong, and this cut is unchanged — one rule moved to another phase, so the row's one-line
   summary of the same cut stopped being true. `/expand-phase` names this edit as allowed in
   so many words ("the index stays shallow but must stay true"). — check:
   `grep -c 'planned: 01-ps2-codec' docs/constraints.md` → `0`.
2. **Landed — the clang-tidy invocation.** Touches `tests/test_style.sh`, `.clang-tidy`,
   `docs/constraints.md` §Observed conventions. Two flags, `-xc++` and `-isysroot`.
   **When `xcrun` names no SDK the sysroot flag is omitted, the check prints a `note:` and
   continues to its ordinary verdict — it does not fail.** The flag is macOS-specific and
   ADR-0010 makes macOS the development host; on a host where `xcrun` does not exist, libc++
   is found without it. The degradation cannot hide a naming violation, which is the thing
   that would make it dangerous: this phase measured that a missing sysroot on macOS makes
   the headers fail *inside libc++* as a `clang-diagnostic-error`, which surfaces as
   `FAIL: R-STYLE-02`. A wrongly omitted flag therefore fails loudly rather than passing
   quietly. — check: `make lint` → exit 0.
3. **Landed — `ps2_protocol.h` and `ps2_frame.h`/`.cpp`.** `decode` refuses per §Goal and
   stores per §The frame; ADR-0012 records the `DecodeStatus` membership rule. — check:
   `ls docs/adr/0012-*.md` → one file.
4. **Landed — the ten vectors, `README.md`, the cases file and the driver.** — check:
   `ls tests/vectors/*.h | wc -l` → `10`; `python3 tests/test_ps2_codec.py` → exit 0.
5. **Landed — `guitar_state.h`/`.cpp`.** `map_frame` gates **both** the buttons and the whammy
   on the id: `reports_controls()` answers yes for `Digital` and `Analog` only, and the gate is
   a closed positive set, so an id added later reports nothing until someone decides otherwise.
   — check: the `config_mode: maps to every one of the ten controls released` case is `ok:`.
6. **Landed — `link.h`/`.cpp` and ADR-0011.** — check: `ls docs/adr/0011-*.md` → one file.
7. **Landed — `hid_report.h`/`.cpp`.** — check: every `hid:` case is `ok:`. Stated as a
   property and not as a count on purpose: a number here would be a fourth set to enumerate
   and maintain, and this phase has already paid three rounds for counts with no set behind
   them.
8. **Landed — the three rejection cases in the driver.** One per rule, each copying the tree to
   a temp directory, mutating `src/core/`, and asserting the text actually changed before use.
   — check: `rejection cases: 3/3`.
9. **Landed — R-ERR-01, R-ERR-02 and R-CLEAN-04 checks.** Touches `tests/test_repo_shape.sh`,
   `tests/test_style.sh`, `.clang-tidy`. — check: `sh tests/test_repo_shape.sh | grep 'wiring
   cases'` → `10/10`.
10. **Owed — one case: the transition rule is a function of the outcome alone.** Touches
    `tests/ps2_codec_cases.cpp`. Each existing `link:` case drives one outcome from one source
    state, so together they sample the table in §The link but never assert the
    property that makes it a *table of six rows rather than twenty-four*: that the target is
    identical from every source state. Add one case that drives the same outcome from all four
    `LinkState` values and asserts the same target each time. Without it the uniformity claim
    in §The link is prose with no check behind it — the pattern `00-scaffold` §For later phases
    names at line 3256. — check: the new case's line is `ok:`, and cutting the uniformity (make
    one source state map an outcome elsewhere) turns that line, and no rule line, to `FAIL:`.
11. **Owed — one acceptance criterion: no non-vector header under `tests/`.** Touches
    §Acceptance criteria only. `.clang-tidy`'s `HeaderFilterRegex` is `src/.*`, which stops
    clang-tidy diagnosing every header under `tests/`, not only the vectors — correct today
    only because the vectors are the only such headers, which is a claim about the tree that
    nothing checks. Bind it. — check: the criterion below runs and reports `0`.

12. **Landed 2026-09-15 — `decode` checks the announced length before the ready byte.**
    Touches `src/core/ps2_frame.cpp`, `tests/vectors/truncated_not_ready.h` (new),
    `tests/ps2_codec_cases.cpp`. This is a code change and not a pointer: the previous order
    reported `NotReady` for a frame that was both cut short and carrying `0xFF` at the ready
    slot, which R-PROTO-02's own text rules out — a cut-short frame "reports the abort". The
    length is now taken at the first point it is knowable, immediately after the id that
    announces it; the id check cannot move below it, because `frame_len` needs the id.
    §Goal's precedence list is the contract. — check: the
    `truncated_not_ready: a cut-short frame reports the abort, not NotReady` case is `ok:`,
    and putting the ready-byte check back in front of the length check turns that line, and
    no rule line, to `FAIL:`. That the three rule lines stay green under the old order is the
    finding, not a detail: R-PROTO-02's own rule case exercises the two refusals only apart,
    so nothing but this case stands behind the order they are written in.

## Acceptance criteria

```
make test                                                      # expect: OK, exit 0
make lint                                                      # expect: exit 0
time make test                                                 # expect: real < 3m
grep -c 'planned: 01-ps2-codec' docs/constraints.md            # expect: 0
grep -c 'planned: 03-pio-bus' docs/constraints.md              # expect: 4
ls tests/vectors/*.h | wc -l                                   # expect: 10, the files in §Vectors
find tests -name '*.h' -not -path 'tests/vectors/*' | wc -l    # expect: 0 (step 11)
grep -rn 'tests/vectors' src/ | wc -l                          # expect: 0 (R-PROTO-05)
grep -rn '\.value( *)' src/ | wc -l                            # expect: 0 (R-ERR-04)
grep -rn 'TODO(09-guitar-observe)' src/core/ | wc -l           # expect: 3 (§Goal names which covers what)
ls docs/adr/0011-*.md                                          # expect: exactly one file
ls docs/adr/0012-*.md                                          # expect: exactly one file
python3 tests/test_ps2_codec.py                                # expect: exit 0
make test 2>&1 | grep -E 'accounting: test_ps2_codec.py'       # expect: a line reading 3 rule(s)
make test 2>&1 | grep -E 'accounting: test_style.sh'           # expect: a line reading 4 rule(s)
sh tests/test_repo_shape.sh | grep 'wiring cases'              # expect: 10/10
sh tests/test_repo_shape.sh | grep 'false-positive cases'       # expect: 20 (floor 20)
make test 2>&1 | grep -E 'neutered: ([0-9]+)/\1 caught'        # expect: one line — the two sides equal
make test 2>&1 | grep -E 'alternations: ([0-9]+)/\1 caught'    # expect: one line — the two sides equal
sh tests/test_phase_docs.sh                                    # expect: exit 0
```

**Two of those need the real `grep`.** The `neutered:` and `alternations:` lines use an ERE
backreference (`([0-9]+)/\1`), which is the point — it makes the two sides *equal* rather than
merely both present. Under an agent session `grep` is shadowed by `ugrep`, which rejects `\1`
as `invalid escape`; run those two under `/usr/bin/grep` and they pass. An environment fact,
not a project defect, recorded so no further round re-diagnoses it.

**The liveness evidence, and which of it is automated.** Validation round 3 established that
three of the four mutations below are not procedures anyone runs by hand — they are the
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
carries both the uncommitted work and `.git`. Both halves are load-bearing and validation
2026-09-14 measured each: a `git worktree` checks out HEAD, so while this phase is uncommitted
it contains neither `src/core/` nor the driver, and the mutation has no anchor to move; and the
copy needs `.git` because `test_checks_are_live.py` runs every `test_*` file on the tree it is
given, `tests/test_secrets.sh` included, and that one scans history. Write it as a `python3 -c`
block
that reads the file, replaces the anchor, **asserts the replacement changed the text**, writes
it back and runs the command — a block whose anchor has moved otherwise mutates nothing and
reports the exit 0 it was handed (`00-scaffold` §Owed).

## Notes to `/validate-phase` — two package workarounds, both temporary

Neither is a fact about this phase. Both are defects in the Belay commands this repo runs,
already fixed upstream in **792e9d9**; this repo is pinned at **f001884** and takes the update
when the phase closes. **Delete this whole section then** — it is one section precisely so that
removal is one cut. Carried forward from the previous spec at `notes.md` §Deviations round 6,
which requires this re-expansion to re-emit it.

**1. Tell step 5's reviewer which paths were held out of its diff.** When dispatching the
independent review, name in the prompt every path excluded from the diff it receives —
`notes.md` always, plus anything the `.claude/workflow/installed` subtraction removed — and say
that their absence is not a finding. Without it the reviewer reads in this spec that the phase
writes `notes.md`, fails to find it in the diff, and returns a `contradicts` that no change to
the code can clear: the command's step 5 orders the diff "restricted to step 3's file set" and
forbids `notes.md` as an input in the same breath, and the file set always contains it. Round 3
spent that verdict. This paragraph is also addressed to the reviewer itself, which receives this
file: **`notes.md` is written and is deliberately withheld from you.**

**2. A boundary sweep of zero files looks exactly like a clean one.** At this version
`scripts/check.sh --files` skips any argument that is not an existing file — silently, with
`continue` — and still prints `check: all gates passed`. Under zsh an unquoted expansion is not
word-split, so `check.sh --files $FILES` arrives as a single concatenated argument, which is a
path that does not exist, and the sweep covers nothing. Round 3 came one command away from
recording that empty run as clean. Two defences, both needed:

- Pass the paths one per argument — `xargs scripts/check.sh --files < <file-set>` — and check
  the input count (`wc -l`) against the file set. The count cannot be read off the output:
  `--files` prints **nothing** per file and only the closing line, so a silent skip leaves no
  trace. The input side is the only place the number exists.
- Prove the sweep live before recording it: add a violation an active rule must catch —
  `#include "src/hal/bus_io.h"` in any `src/core/` header is `deny core -> hal` — confirm the
  gate exits non-zero and names the rule, and restore the file. An SDK include is **not** the
  probe to use: `boundaries.rules` says in its own header that R-ARCH-01 is not expressible as
  a layer rule, and `boundary-check.sh` returns 0 on it.

**3. Check that the file set is not empty before trusting any gate.** Before running step 1,
confirm two things: that `notes.md` §Outcome's `- base:` line names a real ref rather than the
literal `working tree`, and that the resulting file set has the number of files this phase
actually touched. `/validate-phase` derives its file set from the working tree unless that line
overrides it, so the moment a phase's work is committed the default yields **nothing** — and a
sweep of nothing, a review of nothing and a closure test over nothing all report `pass`. That
happened on 2026-09-15: the line still read `working tree` from before this phase had a branch,
and it was corrected to `24d489f` before any gate ran. The three gates that silently depend on
it are the three that cost the most when they lie.

## Out of scope

- Anything that touches a GPIO, a PIO state machine, a clock or a timer — `03-pio-bus`. `core`
  supplies "given these bytes and this elapsed time, decide"; nothing here produces or consumes
  a real microsecond.
- `src/core/pins.h` and the pin table — `02-wiring`. R-SAFETY-01..03 stay `planned: 02-wiring`.
- Running the config-mode sequence, or verifying a controller answered analog mode on a real
  bus — `07-analog-mode`. This phase declares the command bytes and the states; driving them is
  that phase. **Tightening `kNegotiationTimeoutUs`** is that phase's too, per §The link.
- **Strengthening `tests/test_checks_are_live.py` beyond the accounting property for `.py`
  files — released here to no phase, deliberately.** `00-scaffold` named this phase as the
  owner, this phase did not close it, and it added a second `.py` check, so the exposure grew.
  Stated rather than left implied, because §Context pointers naming it as inherited while no
  step closed it is what made it undecidable in round 3. What stands behind
  `tests/test_ps2_codec.py` instead is its three rejection cases and M4. Closing it properly is
  check-harness work — the honest statement is that the `.sh` checks are mutation-tested and the
  `.py` checks are not — and it belongs to whichever phase next touches that harness, alongside
  the `clang-query` + `compile_commands.json` upgrade `03-pio-bus` already owes and the
  "no new untracked paths after `make test`" check that `notes.md` §For later phases names.
- **Widening R-ERR-02's check beyond `src/core/*.h`** — no phase yet; the narrowing is recorded
  in the rule's own text and the residual hole has no caller who could ignore a result.
- The TinyUSB HID descriptor — `08-usb-hid`. This phase fixes the report's byte layout so the
  descriptor can be written from it; it writes no descriptor and no USB code.
- The emulator — `05-emulator`. No file under `src/emu/`.
- Correcting any byte position against the real guitar — `09-guitar-observe`. Getting a
  `TODO(09-guitar-observe)` right early would be a guess wearing a measurement's clothes. Note
  that `unknown_id.h` uses `0x79` deliberately; if that phase finds the SG uses it, the vector
  is repointed at another undeclared byte rather than the id simply being added.
- A CMake or firmware build, and `-fno-exceptions -fno-rtti` — `03-pio-bus` owns R-ERR-05.
- `#embed` for the vectors — no phase; ruled out in §Vectors and not to be revisited without a
  reason that section does not already answer.
- Optimising `make test`'s runtime — no phase. The standing answer is to raise the cap in a
  re-expansion or cut mutants; §Acceptance criteria's cap is 3m.
