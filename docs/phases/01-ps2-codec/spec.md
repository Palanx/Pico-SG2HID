# Phase 01-ps2-codec — Pure PS2 frame codec, id table, link state machine, button/axis map, HID report builder

<!-- Re-expanded three times after /validate-phase's iteration-3+ escape: 2026-09-14, and
     twice on 2026-09-15 (c159aef, def180c). Each re-expansion was derived from the PHASES.md
     row's Goal and from notes.md, never from the code on disk; where the two disagree, §Plan
     says so and the code is what changes. Steps 18-28 were added in place on 2026-09-16 and
     2026-09-17 by implementation rounds, not by a re-expansion. notes.md is the account of how we got here and
     is not rewritten. -->

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
never a claim about the frame. Requiring the caller to trim to the announced length first
would put the arithmetic `2 * (header & 0x0F)` on both sides of the layer boundary — which is
the duplication `core` exists to prevent. **What `03-pio-bus` will actually hand over is not
recorded anywhere yet**, and this contract is deliberately the one that does not need it known:
a caller that trims and a caller that over-delivers decode identically, because `decode` reads
exactly the number of bytes the header announces either way. Nothing asserts this contract
today: `notes.md` §Debt carries it.

**Where two refusals are both true, the order is fixed, and it is chosen rather than
inherited from the order somebody happened to write the checks in.** The list is evaluated top
to bottom and the first refusal that applies wins. No refusal is placed above a point where it
is decidable, but decidability alone does not set the order — item 1 says why:

1. fewer bytes than a header and a ready slot → `AckTimeout`. A single byte would already
   decide the header, so this is a choice and not a necessity: the frame is cut short, and the
   abort outranks every refusal still evaluable (R-PROTO-02) — an undeclared header that
   arrived alone included. §Plan step 18 is what asserts it.
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
and the number `verify.md`'s "What this phase does NOT prove" section tells the operator to
expect. `09-guitar-observe` exists to confront all four concerns with the real device. A vector here asserts that the codec does
what the codec says, not that the guitar agrees.

### Rule work — landed, stated so the next round does not re-do it

The seven rules that were `planned: 01-ps2-codec` are all settled and
`grep -c 'planned: 01-ps2-codec' docs/constraints.md` is **0**. R-PROTO-01 was rebound to
`03-pio-bus` (bit order is a PIO property); R-PROTO-02 was split, its bus clause becoming the
new R-PROTO-06, `planned: 03-pio-bus`; R-PROTO-03, R-PROTO-04 and the core clause of
R-PROTO-02 bind to `tests/test_ps2_codec.py`; R-ERR-01 and R-ERR-02 bind to
`tests/test_repo_shape.sh`, each with its narrowing recorded in the rule's own text;
R-CLEAN-04 binds to `tests/test_style.sh`. Nothing in this re-expansion moves a binding.

## What a `test:` binding does and does not promise

Five times in this phase a rule's check printed `ok:` while the rule was broken — the
config-mode button gate, the link-transition uniformity, the refusal ordering, R-PROTO-05's
blindness to comments, and the first refusal in §Goal's order (§Plan step 18). All five survived `tests/test_checks_are_live.py`, because **that harness
proves a check is *wired*, not that it is *adequate*.** It mutates the code the check reads and
confirms the check reacts. A check whose reach is narrower than its rule's text passes that
perfectly: it is alive, it just measures less than the sentence it is bound to.

The convention that closes the gap:

> **The phase that moves a rule from `planned:` to `test:` writes, in the rule's own text, what
> the check does not see.** A rule whose text states no scope asserts that its check covers it
> entirely, and that assertion is what a later reader is entitled to disbelieve and measure.

**Which rules owe a clause is derived from what their check is made of, not from a list.**
Three earlier attempts answered it by enumerating rules by hand and all three were falsified —
the round-15 audit table, then R-ERR-02's return-type scope, then R-CLEAN-04's file scope. A
list has to be remembered; a function can be grepped, and adding a rule to one drags the
obligation along with it.

**The first version of this derivation was itself incomplete, and validation on 2026-09-16
found it.** It derived obligations from the scanner *function* only — which files it reads,
whether it strips comments — and missed the axis every grep-based check has regardless of
function: its own pattern. A regex matches the spellings it anchors and no others, so every
`find_*` owes a spelling clause, measured by feeding it one form per spelling. That is the last
row below, and it binds per finder rather than per function because each finder has its own
pattern.

| the check runs through | what that costs it | so the rule's text owes |
|---|---|---|
| `tidy_sources()` in `tests/test_style.sh` | passes `src/core/*.cpp`, `src/core/*.h` and `tests/*.cpp` to `git ls-files`, and nothing else, **whatever the rule says**. These are git pathspecs, not shell globs: `*` crosses `/`, so each prefix is read recursively — `tests/*.cpp` would include `tests/fixtures/x.cpp`. Measured: `git ls-files 'docs/*.md'` returns 38 files in subdirectories | a file-scope clause, unconditionally, in the notation the check actually uses |
| `hits()` in `tests/test_repo_shape.sh` | runs `sed 's\|//.*\|\|'` before grepping, so it cannot see an occurrence inside a comment | a comment clause **if the rule's subject can occur in a comment**; nothing if the subject is a code construct |
| `raw_hits()` in `tests/test_repo_shape.sh` | reads raw lines — the un-stripping counterpart | a statement that occurrences count, since choosing this scanner is itself a claim about the rule |
| `sources()` in `tests/test_style.sh` | every tracked-or-new `.cpp`/`.h` in the repo | nothing; it is the whole set already |
| the regex of any `find_*` in `tests/test_repo_shape.sh` | matches only the spellings it anchors — a qualifier before a type, a line break inside a declaration, a path assembled another way | a spelling clause, **unconditionally**, naming the forms measured as unmatched |

R-PROTO-05 is why the `hits`/`raw_hits` column is a claim and not a detail: its subject is a
*reference*, which can appear in a comment, so `hits()` made it narrower than its text — and
that is the defect that failed validation #3 on 2026-09-15.

### The assignment, measured against the files rather than recalled

Produced by reading the report lines and the finder definitions out of
`tests/test_repo_shape.sh` and the source lists out of `tests/test_style.sh`, 2026-09-15;
re-measured against the files on 2026-09-16, when the spelling axis was added and the "text
carries it" column was re-read out of `docs/constraints.md` rather than carried forward.
Recompute with:

```
grep -E 'report R-' tests/test_repo_shape.sh          # rule -> finder
grep -E '^find_[a-z0-9]+\( \)' tests/test_repo_shape.sh   # finder -> hits | raw_hits
grep -nE 'tidy_sources\(\)|sources\(\)' tests/test_style.sh
```

The "owes" column is the comment/occurrence/file axis first and the spelling axis second. A rule
whose check is not a `find_*` has no spelling axis.

| rule | runs through | owes | text carries it |
|---|---|---|---|
| R-ARCH-01 | `find_arch01` / `hits` | nothing on comments — subject is `#include`; a spelling clause | spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-ARCH-03 | `find_arch03` / `hits` | nothing on comments — subject is allocation calls; a spelling clause | spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-CLEAN-03 | `find_clean03` / `hits` | nothing on comments — subject is a declaration's name; a spelling clause | spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-CLEAN-09 | `find_clean09` / `hits` | nothing on comments — subject is inheritance and `virtual`; a spelling clause | spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-ERR-01 | `find_err01` / `hits` | nothing on comments — subject is a return type; a spelling clause | **yes** — §Plan step 19 |
| R-ERR-02 | `find_err02` / `hits` | nothing on comments; file scope; a spelling clause | **yes**, all three — file scope and two spelling narrowings (§Plan steps 16 and 19) |
| R-ERR-03 | `find_err03` / `hits` | nothing on comments — subject is `throw`/`try`/`catch`, though its text says "anywhere under `src/`", which reads wider than its subject; a spelling clause | **no** — not this phase's rule, `notes.md` §For later phases |
| R-ERR-04 | `find_err04` / `hits` | nothing on comments — subject is a call; a spelling clause | spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-CLEAN-05 | `find_clean05` / `raw_hits` | occurrences count; a spelling clause | occurrences: **yes** — it is a rule about comments. Spelling: **no** — not this phase's rule, `notes.md` §For later phases |
| R-PROTO-05 | `find_proto05` / `raw_hits` | occurrences count; a spelling clause | **yes**, both — §Plan steps 14 and 19 |
| R-STYLE-01 | `sources()` | nothing | — |
| R-STYLE-02 | `tidy_sources()` | file scope | **no** — not this phase's rule, `notes.md` §For later phases |
| R-CLEAN-02 | `tidy_sources()` | file scope | **no** — not this phase's rule, `notes.md` §For later phases |
| R-CLEAN-04 | `tidy_sources()` | file scope; and what the `tests/vectors/` exception actually rests on | **yes** — §Plan steps 17, 20 and 23 |

One limit is not a scanner's and belongs beside them: `tests/test_style.sh` is the one `.sh`
check `tests/test_checks_are_live.py` does not mutate (`NO_MUTATE`), so all four rules bound to
it — R-STYLE-01, R-STYLE-02, R-CLEAN-02 and **R-CLEAN-04**, the one this phase moved there — are
held to the accounting property only: their lines are proven to be printed, not proven to fire.
§Out of scope states it; nothing in this phase closes it.

The remaining `test:`-bound rules run no scanner this table covers and are unaffected by it:
R-PROTO-02, R-PROTO-03 and R-PROTO-04 (`tests/test_ps2_codec.py`, audited in round 12),
R-ARCH-02 (`tests/test_boundaries.sh`), R-SEC-01 (`tests/test_secrets.sh`), R-TOOL-01 and
R-TOOL-02 (`tests/test_tool_versions.sh`), R-PROC-01 (`tests/test_rule_traceability.py`) and
R-PROC-02 (`tests/test_phase_docs.sh`, whose text demands content no check reads —
`notes.md` §For later phases).

## Context pointers

- `CLAUDE.md` — the architecture paragraph, the hardware-safety rules, and the reading rule
  that scopes this list. `core` depends on nothing; `make` is the only entry point.
- `docs/phases/01-ps2-codec/notes.md` — the account of every implementation round and every
  validation round, each under its own heading. §Deviations is where every decision this spec states was argued; §Debt and §For
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

**The fill value itself is not a protocol byte, and R-PROTO-05 does not reach it** (ruled
2026-09-17, §Plan step 27). `tests/ps2_codec_cases.cpp` declares `kZeroFill` and asserts the
unannounced payload bytes against it, which reads like an expected byte written outside
`tests/vectors/`. It is not: the controller never sent those bytes. The fill is this project's
own decision about what `decode` leaves in a buffer it did not fill, so no vector could carry it
— a vector holds what the bus put on the wire. R-PROTO-05 governs the bytes the protocol fixes,
which is how `CLAUDE.md` §Conventions already states it, and the catalogue now says it too.

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
| `unknown_id.h` | header `0x79`, the DualShock 2's real full-analog id, deliberately undeclared here; **20 bytes — the whole frame `0x79` announces**, since `2 * (0x79 & 0x0F)` is 18 payload bytes. A shorter one would be cut short as well as undeclared, and either fault could then be the reason it was refused | `DecodeStatus::UnknownId`, no frame | R-PROTO-03 |
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
| `docs/adr/0007-error-model.md` | **status line only** (§Plan step 28): it names the three later ADRs that supersede parts of it. The body is untouched — an ADR is immutable except for its status. |
| `docs/phases/01-ps2-codec/verify.md` | the operator procedure (R-PROC-02) |
| `CLAUDE.md` | **not written by this phase.** The operator amended it mid-phase, adding the three pre-validation checks to §Conventions. It appears in this phase's diff only because the base ref `24d489f` precedes that edit. Listed here so the file is accounted for rather than read as scope this phase escaped into. |

## Plan

**Every step below is landed; none is owed.** Each carries a `Landed` marker, dated from step 10
on; steps 1-9 landed in rounds 1-4, before markers carried dates. They are all stated because a
cold session must be able to rebuild the phase, not because any is pending. Steps 18-22 are what
the 2026-09-16 round added after validation of the def180c spec returned five `contradicts` and
three `undecidable`; steps 23-28 are the 2026-09-17 round, after the next validation returned
three `contradicts` and five `undecidable`.

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
10. **Landed 2026-09-14 — one case: the transition rule is a function of the outcome alone.** Touches
    `tests/ps2_codec_cases.cpp`. Each existing `link:` case drives one outcome from one source
    state, so together they sample the table in §The link but never assert the
    property that makes it a *table of six rows rather than twenty-four*: that the target is
    identical from every source state. Add one case that drives the same outcome from all four
    `LinkState` values and asserts the same target each time. Without it the uniformity claim
    in §The link is prose with no check behind it — the pattern `00-scaffold` §For later phases
    names at line 3256. — check: the new case's line is `ok:`, and cutting the uniformity (make
    one source state map an outcome elsewhere) turns that line to `FAIL:` for **every** outcome
    and source.
    **Measured over all twenty-four, not argued from one (2026-09-16, re-measured 2026-09-17).** Each of the six outcomes,
    from each of the four source states, was sent to a wrong target in turn. The uniformity line
    failed 24 of 24. `R-PROTO-02` failed 7: the four `AckTimeout` mutants, and the three good
    frames sent astray from `Absent`, which break the set-up that line drives its source states
    with rather than its rule. It stayed `ok:` for the other 17, and correctly so: those are
    transitions on a good frame, `UnknownId` or `NotReady`, and R-PROTO-02's text is about
    frames the bus cut short. No rule's text covers them. §The link's table does, and what holds
    it is the uniformity case for "equal from every source" plus one `link:` case per outcome for
    "equal to what" — `NotReady`'s being §Plan step 21, because before it a `step` sending
    `NotReady` to the same wrong state from every source passed everything.
11. **Landed 2026-09-14 — one acceptance criterion: no non-vector header under `tests/`.** Touches
    §Acceptance criteria only. At the time, `.clang-tidy`'s `HeaderFilterRegex` was `src/.*`,
    which stopped clang-tidy diagnosing every header under `tests/`, not only the vectors —
    correct today
    only because the vectors are the only such headers, which is a claim about the tree that
    nothing checks. Bind it. — check: the criterion below runs and reports `0`.
    **The motive changed under it and the criterion stays (§Plan step 23).** With the filter back
    at `(src|tests)/.*` a stray header under `tests/` is diagnosed rather than silenced, so this
    no longer guards a silence. What it still asserts is that R-CLEAN-04's exception, which names
    `tests/vectors/` and nothing else, has no other header under `tests/` to be confused with.

12. **Landed 2026-09-15 — `decode` checks the announced length before the ready byte.**
    Touches `src/core/ps2_frame.cpp`, `tests/vectors/truncated_not_ready.h` (new),
    `tests/ps2_codec_cases.cpp`. This is a code change and not a pointer: the previous order
    reported `NotReady` for a frame that was both cut short and carrying `0xFF` at the ready
    slot, which R-PROTO-02's own text rules out — a cut-short frame "reports the abort". The
    length is now taken at the first point it is knowable, immediately after the id that
    announces it; the id check cannot move below it, because `frame_len` needs the id.
    §Goal's precedence list is the contract. — check: the
    `truncated_not_ready: a cut-short frame reports the abort, not NotReady` case is `ok:`,
    and putting the ready-byte check back in front of the length check turns **both** that
    line **and the `R-PROTO-02` rule line** to `FAIL:`.
    **The rule line is part of that check because of what the first probe found.** When this
    step first landed, reverting the order flipped only the case and left all three rule lines
    green — R-PROTO-02 says a cut-short frame "reports the abort" with no exception for frames
    that are also wrong some other way, and its rule line exercised truncation only where no
    other refusal competed. A rule line that stays green while its rule is broken makes §Goal's
    "mutating the codec so it stops refusing makes the check fail and name the rule" false, so
    `rule_proto02` now also decodes `truncated_not_ready` and asserts the abort there. §Plan
    step 13 is that widening.

13. **Landed 2026-09-15 — `rule_proto02` covers the overlap its rule quantifies over.**
    Touches `tests/ps2_codec_cases.cpp`. R-PROTO-02's line decoded `truncated_ack` only, which
    is truncation with a *good* ready byte — the one configuration where no other refusal
    competes — so the line could not see the ordering defect step 12 fixed. It now decodes
    `truncated_not_ready` as well and asserts `AckTimeout` and `Absent` there too.
    — check: restoring the old refusal order turns the `R-PROTO-02` rule line to `FAIL:`,
    not just the case.
    **The same question was asked of the other two rule lines and answered by measurement, not
    by reading.** `rule_proto03`: R-PROTO-03 is about never decoding an undeclared header on a
    best-effort basis, not about interaction, and every overlap with a refusal *below* it in
    §Goal's precedence still reports `UnknownId` — not an instance. The one refusal above it, a
    frame cut short before its ready slot, is R-PROTO-02's, and §Plan step 18 asserts it there. `rule_proto04`: a clause for `config_mode` was
    written, probed, and **reverted**. `map_frame` returns early for an id that reports no
    controls, so only `Digital` and `Analog` ever reach the whammy gate, which makes
    `id != Digital` an *equivalent* mutant rather than a violation — no single mutation can
    flip a Config clause, so it would have been green paint. The measurement is recorded in
    the rule function itself.

14. **Landed 2026-09-15 — R-PROTO-05 means every occurrence, comments included.** Touches
    `tests/test_repo_shape.sh`, `src/core/ps2_frame.cpp`, `docs/constraints.md`.
    Validation #3 failed because the acceptance criterion and the bound check disagreed about
    the rule: `grep -rn 'tests/vectors' src/` counts a comment, and `find_proto05` cannot,
    because it goes through `hits()`, which runs `sed 's|//.*||'` first. Both readings were
    defensible and the spec named neither, so `make test` and §Acceptance criteria could
    contradict each other on one tree.
    **The decision is the literal reading**, and the reason is that it leaves no judgement to
    interpret: check and criterion both become "no occurrence", and this phase's repeated
    failure mode is a check weaker than its rule. Three edits, one meaning:
    - `find_proto05` moves from `hits` to `raw_hits` — the scanner that does not strip
      comments, which `find_clean05` already uses for the same reason.
    - the comment at `src/core/ps2_frame.cpp:22` naming `tests/vectors/truncated_not_ready.h`
      is deleted. What it pointed at belongs in `tests/`, not in `core`.
    - R-PROTO-05's text gains the scope clause §What a `test:` binding promises requires:
      the check reads raw lines, so a mention in a comment is a violation, and that is
      deliberate rather than an accident of the scanner.
    — check: `grep -rn 'tests/vectors' src/ | wc -l` → `0`; `sh tests/test_repo_shape.sh`
    still reports `ok: R-PROTO-05`; and the rejection case proves the new scope — a scratch
    tree whose only reference is **inside a comment** must make R-PROTO-05 fire. Without that
    last part the change is untested and the old scanner would pass the suite equally well.

15. **Landed 2026-09-15 — `rule_proto02` asserts its target from every source state.**
    Touches `tests/ps2_codec_cases.cpp`. Found by running step 10's own liveness probe against
    the rule lines rather than only against its case: mutating `step` so the transition depends
    on the source state flipped the uniformity case and left `R-PROTO-02` green. R-PROTO-02's
    text says a cut-short frame's link "transitions to `Absent`" and does not qualify the
    source state, but every step in that rule line started from a fresh — therefore `Absent` —
    link, so a `step` that sent a cut-short frame elsewhere from one source state broke the
    rule with the line still printing `ok:`. The same shape as step 13, one axis over: that one
    was blind to a competing refusal, this one to where the link was standing.
    The line now drives a link to `DigitalStreaming`, `AnalogStreaming` and `Negotiating` and
    asserts `Absent` from each — the four `LinkState` members are the set §The link enumerates,
    and `Absent` is the fourth, already covered. **Not the uniformity case restated:** step 10
    asserts the four targets are *equal*, this asserts what they equal.
    — check: the same source-state mutation now turns **both** the uniformity case and the
    `R-PROTO-02` rule line to `FAIL:`.

16. **Landed 2026-09-15 — R-ERR-02 records its return-type scope as well as its file scope.**
    Touches `docs/constraints.md`. The check recognises `std::expected<…>`, `DecodeOutcome` and
    `LinkState`; the rule says "result struct", which is wider, and `id_from_byte` returns
    `std::optional<ControllerId>` and matches none of the three. It carries `[[nodiscard]]`
    today, so nothing is in violation — the check would simply not notice if that stopped.
    The clause is what §What a `test:` binding does and does not promise requires of a binding
    this phase owns, and closing the gap needs the type information a grep does not have, which
    is `03-pio-bus`'s `clang-query` upgrade. — check: `python3 tests/test_rule_traceability.py`
    → exit 0, and R-ERR-02's entry in `docs/constraints.md` names both narrowings.

17. **Landed 2026-09-15 — R-CLEAN-04 records the file scope its check has.** Touches `docs/constraints.md`.
    R-CLEAN-04 is bound to `tests/test_style.sh` and runs through `tidy_sources()`, which reads
    `src/core/*.cpp`, `src/core/*.h` and `tests/*.cpp` and nothing else; before this step the
    rule's text was unscoped and recorded only the `const`/`constexpr` initializer blindness. By the table above
    that is an unconditional obligation, and §Plan step 9 moved this binding from `planned:` to
    `test:`, so the phase owns it.
    **The clause covered two narrowings on that axis** when it was written: the file list, and
    `.clang-tidy`'s `HeaderFilterRegex` of `src/.*`. The second is gone — step 23 reverted that
    regex — and the clause now says what the `tests/vectors/` exception really rests on instead.
    The file list stands, in the pathspec notation the check uses (step 23).
    — check: `python3 tests/test_rule_traceability.py` → exit 0, and
    `grep -c 'tidy_sources\|src/core/\*' <(grep '^- \*\*R-CLEAN-04\*\*' docs/constraints.md)`
    → 1 or more.

18. **Landed 2026-09-16 — `rule_proto02` asserts the first refusal in §Goal's order.** Touches
    `tests/ps2_codec_cases.cpp`, and `src/core/ps2_frame.cpp` for one comment only: it said a
    frame shorter than the prefix has "no header to read", which is false for a single byte.
    §Goal's item 1 and step 13's `rule_proto03` sentence were reconciled in the same edit. Found by generalising a validation finding rather than fixing
    it: instead of mutating only the transition already known to be covered, every refusal in
    `decode`, every gate in `map_frame` and every transition in `step` was mutated in turn and
    each mutant's `FAIL:` lines recorded (`notes.md` §Deviations round 28 has the table).
    Deleting the size-below-prefix check survived every line, because no input was shorter than
    header plus ready slot: a lone undeclared header then reports `UnknownId` instead of the abort
    R-PROTO-02 requires. Fixed with one case and one clause in the rule line, both decoding
    `unknown_id`'s literal cut to its first byte (`.first( kReadyIndex )`). That is a prefix of a
    hand-written vector, not a new one and not a generated byte — the expected value is a status —
    so §Vectors' ten files and R-PROTO-05 are unchanged. — check: the `cut before the ready slot:
    reports the abort, even for an unknown id` case is `ok:`, and deleting the prefix check turns
    **both** that line **and the `R-PROTO-02` rule line** to `FAIL:`. Two mutants still survive
    everything, both already declared: `id != Digital` as the whammy gate (equivalent, see step 13)
    and a wrapping `us_in_state` (`notes.md` §Debt).

19. **Landed 2026-09-16 — the spelling axis: three rule texts record what their pattern does not
    match.** Touches `docs/constraints.md`. §What a `test:` binding does and does not promise now
    derives a spelling clause for every `find_*`; of the rules this phase bound or re-scoped,
    three run through one. Each clause names only forms measured by feeding the finder one line
    per form:
    - R-ERR-01 — `static`/`inline`/`extern`/`friend`/`const` before the type, `ps2::`
      qualification, a trailing return, a return type on its own line.
    - R-ERR-02 — the same prefixes and qualification, a trailing return, a return type on its own
      line, a `std::expected<…>` split across lines; plus the false positive on `[[nodiscard]]`
      written on the line before.
    - R-PROTO-05 — an include resolved through `-Itests`, a path split across string literals, a
      doubled or backslash separator. This clause also corrects R-PROTO-05's existing text, which
      stated only what the literal reading over-refuses.
    The other grep-bound rules owe the same clause and are not this phase's; §Out of scope
    releases them. — check: `python3 tests/test_rule_traceability.py` → exit 0, and
    `grep -c 'spelling narrowing\|third narrowing\|under-strict' docs/constraints.md` → `3`.

20. **Landed 2026-09-16, and superseded by step 23 the next day — R-CLEAN-04 records that its
    header filter depends on the checkout path.** The measurement stands; what changed is that
    the regex it measured is gone, so recording the dependence was the wrong fix and step 23 is
    the right one. Kept rather than deleted because step 23 is only legible next to it. Touches `docs/constraints.md`, `.clang-tidy` (comment only). Measured, not argued:
    the same naming violation planted in `tests/vectors/digital_idle.h` passes `make lint` in a
    clone whose path contains no `src/` and fails it in a clone under `…/src/pico-sg2hid`, because
    `HeaderFilterRegex: 'src/.*'` is unanchored and clang-tidy matches it against the absolute
    path. The unmodified tree is green in both. Steps 11 and 17 are reconciled to say "in a
    checkout whose path contains no `src/`". — check: `grep -c 'depends on where the repository is
    checked out' docs/constraints.md` → `1`.

21. **Landed 2026-09-16 — four `link:` cases for §The link behaviours nothing asserted.** Touches
    `tests/ps2_codec_cases.cpp`. The exclusive bound, the counter reset on entry, `last_fault`
    surviving a good frame, and the `NotReady` row. Each is proven by the mutant it exists for,
    and each mutant survived every line before the case landed: `>=` for `>`; the entering step's
    elapsed time counted toward the new state; a good frame clearing `last_fault`; `NotReady`
    recorded as `AckTimeout`. The saturation stays unasserted, as §The link says. — check: the
    four cases are `ok:`, and each mutant turns its own case to `FAIL:`.

22. **Landed 2026-09-16 — two comments that asserted what the code does not have.** Touches
    `src/core/guitar_state.h`, `src/core/ps2_frame.h`. `Fret`'s comment named `frets` and
    `ControllerFret`, neither of which exists, and credited the indexing with keeping the HID
    descriptor in step, which `Button` in `build_report` does. Both structs were called
    "comparable as a whole" and neither declares `operator==`. Generalised: every identifier
    named in a comment across `src/core/`, `tests/ps2_codec_cases.cpp` and `tests/vectors/` was
    checked against the code; the rest are bus line names (`ACK`, `DATA`), names ADR-0007 sketched
    and ADR-0011/0012 retired (`BusIo`, `NotAnalog`), a name `DecodeStatus`'s comment cites as
    rejected (`TooShort`), and proper nouns and file names. — check: `grep -rn 'ControllerFret\|comparable as a whole' src/` → no output.

23. **Landed 2026-09-17 — the `HeaderFilterRegex` narrowing is reverted, not owned.** Touches
    `.clang-tidy`, `docs/constraints.md`. No Plan step ever claimed
    `(src|tests)/.*` → `src/.*`; it arrived with step 2's clang-tidy work to keep the vectors'
    literals out of `readability-magic-numbers`. **Measured before deciding, in two clones:** with
    the regex reverted, `make lint` and `make test` are green at a path containing no `src/` and
    at one under `…/src/pico-sg2hid`, and a naming violation planted in
    `tests/vectors/digital_idle.h` is diagnosed in **both** — so the narrowing never protected the
    vectors (their literals are `constexpr` initializers the check cannot see, §Observed
    conventions) while it did silence naming and function-size diagnostics for every header under
    `tests/`. Those are R-STYLE-02's and R-CLEAN-02's reach, which are `00-scaffold`'s bindings and
    which §Out of scope says this phase does not touch — so the honest move is to put the regex
    back, not to write a clause owning a narrowing this phase had no business making. Reverting
    also removes the checkout-path dependence entirely, since both prefixes then match whatever
    the absolute path is. R-CLEAN-04's text now says its `tests/vectors/` exception rests on the
    `constexpr` blindness, and `tidy_sources()`'s three patterns are recorded as **git pathspecs**,
    where `*` crosses `/` — measured with `git ls-files 'docs/*.md'`, 38 files in subdirectories.
    — check: `grep -c "HeaderFilterRegex: '(src|tests)/.*'" .clang-tidy` → `1`; `make lint` → 0.

24. **Landed 2026-09-17 — the twin of a sentence step 18 deleted, in the other file of the pair.**
    Touches `src/core/ps2_protocol.h`. `kPrefixLen`'s comment still read "A frame shorter than this
    announced nothing at all" — the claim §Goal item 1 contradicts and step 18 removed from
    `src/core/ps2_frame.cpp`. **Generalised across files, since every earlier reconciliation this
    phase did was within one:** every comment line deleted from `src/` or `tests/` since the base
    ref was searched for in the current tree (two hits, both text that moved inside its own file);
    every comment sentence was compared against every other across files by word overlap; and the
    fact itself — what a frame shorter than the prefix means — was grepped tree-wide. That last one
    is what found it, and it is the method that generalises: match on the claim, not the wording.
    — check: `grep -rn 'announced nothing at all' src/` → no output.

25. **Landed 2026-09-17 — §Out of scope stops claiming mutation coverage `tests/test_style.sh`
    does not have.** Touches §Out of scope, and the table in §What a `test:` binding does and does
    not promise. `tests/test_checks_are_live.py:49` holds `NO_MUTATE = {"test_style.sh"}`, so that
    file is skipped by both the neutering and the alternation properties — and it is where §Plan
    step 9 bound R-CLEAN-04. The fix is the spec saying so, not mutating the file: the code's own
    comment records the exclusion as a decision, and mutating a clang-tidy invocation means
    mutating the tool's configuration, which is the harness work §Out of scope already releases.
    All four rules bound to that file stand on the accounting property alone. — check:
    `grep -n 'NO_MUTATE' tests/test_checks_are_live.py` names `test_style.sh`, which is the
    fact §Out of scope now states. The check reads that file and not this one on purpose: a
    `grep` of a phrase in `spec.md` counts the check's own line, which is how the first two
    attempts at this check reported 2 and 1 where they wanted 1 and 0.

26. **Landed 2026-09-17 — step 10's check keeps the measurement and drops the sentence that
    contradicted it.** Touches §Plan step 10. It claimed the uniformity mutation fails
    `R-PROTO-02` for the `AckTimeout` outcome and no other, eleven lines above its own table
    saying it failed 7 of 24. Re-measured on the current tree: **7** — the four `AckTimeout` mutants plus
    the three good frames sent astray from `Absent`, which break `cut_drops_the_link_from_every_source`'s
    set-up. The "only" sentence is deleted rather than reworded. — check: `grep -c 'only when the
    outcome is' docs/phases/01-ps2-codec/spec.md` → `0`.

27. **Landed 2026-09-17 — the zero fill is ruled not an expected protocol byte.** Touches
    §The frame, `docs/constraints.md`. `tests/ps2_codec_cases.cpp` writes
    `constexpr std::uint8_t kZeroFill = 0x00;` and asserts the unannounced payload bytes against
    it, while the cases file's own header says no expected byte is written there. The ruling:
    R-PROTO-05 governs bytes **the protocol** fixes — what the controller puts on the wire — and
    the fill is not one. It is `decode`'s own contract (§The frame), decided by this project, so a
    vector could not carry it: a vector holds what the bus sent, and the bus never sent those
    bytes. `CLAUDE.md` already scopes the rule that way ("expected protocol bytes"); R-PROTO-05's
    text in the catalogue did not, and now says it. — check: `grep -c 'protocol fixes' docs/constraints.md` → `1`.

28. **Landed 2026-09-17 — ADR-0007's status names both partial supersessions.** Touches
    `docs/adr/0007-error-model.md` (status line only). `CLAUDE.md` says superseded ADRs say so;
    ADR-0007's status already named ADR-0009 for its result struct, which is the precedent that
    the line is maintained, while ADR-0011 and ADR-0012 each say they supersede part of it and
    ADR-0007 said nothing back. Status lines are the one mutable part of an ADR; the body is
    untouched. — check: `grep -c 'ADR-0011' docs/adr/0007-error-model.md` → 1 or more.

## Acceptance criteria

```
make test                                                      # expect: OK, exit 0
make lint                                                      # expect: exit 0
time make test                                                 # expect: real < 3m
grep -c 'planned: 01-ps2-codec' docs/constraints.md            # expect: 0
grep -c 'planned: 03-pio-bus' docs/constraints.md              # expect: 4 — R-SAFETY-07, R-PROTO-01, R-PROTO-06, R-ERR-05
ls tests/vectors/*.h | wc -l                                   # expect: 10, the files in §Vectors
find tests -name '*.h' -not -path 'tests/vectors/*' | wc -l    # expect: 0 (step 11)
grep -rn 'tests/vectors' src/ | wc -l                          # expect: 0 (R-PROTO-05)
grep -rn '\.value( *)' src/ | wc -l                            # expect: 0 (R-ERR-04)
grep -rn 'TODO(09-guitar-observe)' src/core/ | wc -l           # expect: 3 (§Goal names which covers what)
ls docs/adr/0011-*.md                                          # expect: exactly one file
ls docs/adr/0012-*.md                                          # expect: exactly one file
python3 tests/test_ps2_codec.py                                # expect: exit 0
make test 2>&1 | grep -E 'accounting: test_ps2_codec.py'       # expect: 3 rule(s) — R-PROTO-02, R-PROTO-03, R-PROTO-04
make test 2>&1 | grep -E 'accounting: test_style.sh'           # expect: 4 rule(s) — R-STYLE-01, R-STYLE-02, R-CLEAN-02, R-CLEAN-04
sh tests/test_repo_shape.sh | grep 'wiring cases'              # expect: 10/10 — R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-ERR-01, R-ERR-02, R-ERR-03, R-ERR-04, R-PROTO-05
sh tests/test_repo_shape.sh | grep 'false-positive cases'       # expect: 20 (floor 20)
sh tests/test_repo_shape.sh | grep 'rejection cases'            # expect: 64 — one per `reject` line; see the derivation below
make test 2>&1 | grep -E 'neutered: ([0-9]+)/\1 caught'        # expect: one line — the two sides equal
make test 2>&1 | grep -E 'alternations: ([0-9]+)/\1 caught'    # expect: one line — the two sides equal
sh tests/test_phase_docs.sh                                    # expect: exit 0
```

**The two counts in that list are derived, not remembered.** `rejection cases` is one per
`reject` invocation in `tests/test_repo_shape.sh` and `false-positive cases` is one per
`accept` invocation, so both recompute from the file rather than having to be maintained
against it:

```
grep -cE '^reject ' tests/test_repo_shape.sh                   # the 64
grep -cE '^accept ' tests/test_repo_shape.sh                   # the 20
grep -oE 'RULE R-[A-Z]+-[0-9]+' <check file> | sort -u          # the rule sets named above
```

The `rule(s)` and `wiring` counts recompute the same way: a check declares its rules with
`RULE` markers in its header, so the members of "3 rule(s)", "4 rule(s)" and "10/10" are
whatever that grep returns for `tests/test_ps2_codec.py`, `tests/test_style.sh` and
`tests/test_repo_shape.sh`. They are spelled out in the criteria above so a reader can check
the number without running anything, and recomputable so the list cannot rot silently.

Measured 2026-09-15, stated as a distribution because a total with no members is the defect
this phase keeps paying for. The 64 rejection cases: **31** R-ARCH-01, **8**
R-ARCH-03, **7** R-CLEAN-09, **4** each for R-ERR-01, R-ERR-02 and R-ERR-03, **2** each for
R-PROTO-05 and R-CLEAN-05, **1** each for R-ERR-04 and R-CLEAN-03. The 20 false-positive
cases: **5** `find_arch01`, **4** each for `find_err02` and `find_clean03`, **3**
`find_err01`, **2** `find_arch03`, **1** each for `find_clean09` and `find_clean05`. The
floor is 20 and equals the count, so the floor asserts "no accept case was deleted" rather
than "at least this many exist".

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

## Notes to `/validate-phase` — three package workarounds, all temporary

None is a fact about this phase. All three are defects in the Belay commands this repo runs,
and they do not share an upstream state:

- **1 and 2 are fixed upstream in 792e9d9.** This repo is pinned at **f001884** and takes the
  update when the phase closes; delete both then. Carried forward from `notes.md` §Deviations
  round 6, which requires every re-expansion to re-emit them.
- **3 is filed and not fixed.** It is the `- base:` staleness entry dated 2026-09-15 in
  `~/.claude-belay/feedback/pico-sg2hid.md`, and no upstream commit is recorded as fixing it.
  Delete it when that fix lands — the same trigger as check 2 in `CLAUDE.md` §Conventions, which
  guards the same failure from the implementer's side.

They are one section so that removal stays a cut per item rather than a search.

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
  check-harness work — the honest statement is that four of the five `.sh` checks are
  mutation-tested, `tests/test_style.sh` and the `.py` checks are not — and it belongs to
  whichever phase next touches that harness, alongside
  the `clang-query` + `compile_commands.json` upgrade `03-pio-bus` already owes and the
  "no new untracked paths after `make test`" check that `notes.md` §For later phases names.
- **Mutating `tests/test_style.sh`** — released to no phase, and it is `tests/test_checks_are_live.py:49`'s
  `NO_MUTATE = {"test_style.sh"}` that this bullet exists to state rather than leave to a reader of
  that file. It is the one `.sh` check the neutering and alternation properties skip, so the
  binding this phase moved to it — R-CLEAN-04, §Plan step 9 — stands on the accounting property
  alone: its rules are proven to be *reported*, not proven to *fire*. The other three rules on
  that file (R-STYLE-01, R-STYLE-02, R-CLEAN-02) are `00-scaffold`'s and inherit the same limit.
  Not closed here because mutating a clang-tidy invocation means mutating the tool's
  configuration rather than a `grep` alternative, which is the harness work released above.
- **Reading R-PROTO-05 as "no *code* reference" rather than "no occurrence"** — no phase;
  considered on 2026-09-15 and rejected with the reason recorded, not merely dropped. It is the
  more honest reading semantically: a comment cannot make `core` depend on test data, which is
  what the rule exists to prevent. It was rejected because it obliges someone to define what
  counts as a reference — a string literal, a macro expansion, a path assembled from pieces —
  and that is new judgement surface in a phase that is closing, on the exact axis this phase
  has failed on four times. The literal reading needs no definition. `notes.md` §For later
  phases carries it for whoever wants to revisit it with budget to test the answer.
- **Writing the file-scope clause for R-STYLE-02 and R-CLEAN-02** — not this phase's
  bindings. Both run through `tidy_sources()` and owe it by the table in §What a `test:`
  binding does and does not promise; `00-scaffold` bound them and `notes.md` §For later phases
  carries them with an owner. This phase writes the clause for R-CLEAN-04 only, which is the
  one of the three it moved to `test:` itself. The header filter's dependence on the checkout
  path is gone: §Plan step 23 reverted the regex this phase had narrowed, which is what put
  those two bindings' reach back where `00-scaffold` set it. Their file-scope clause is still
  theirs to write.
- **Writing the spelling clause for R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09,
  R-ERR-03 and R-ERR-04** — not this phase's bindings; `00-scaffold` bound all seven. Each runs
  through a `find_*` and owes the clause by the last row of the table in §What a `test:` binding
  does and does not promise. None of the seven was measured on that axis here, so no form is
  claimed to escape them; `notes.md` §For later phases carries the obligation. This phase writes
  the clause for R-ERR-01, R-ERR-02 and R-PROTO-05 only (§Plan step 19).
- **Widening R-ERR-02's check beyond `src/core/*.h`** — no phase yet; the file-scope narrowing
  is recorded in the rule's own text and the residual hole has no caller who could ignore a
  result.
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
