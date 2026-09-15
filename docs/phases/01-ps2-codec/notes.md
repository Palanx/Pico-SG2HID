# Phase 01-ps2-codec — notes

## Outcome

- base: working tree (the phase's work is uncommitted; `docs/phases/PHASES.md` was already
  modified and `docs/phases/01-ps2-codec/` already untracked when this session started).

`src/core/` now exists and holds the whole protocol brain, with no hardware anywhere in it.
Five header/implementation pairs, all compiling and running under `make test` on the laptop
with only a C++23 compiler and `python3`:

| file | what it adds |
|---|---|
| `src/core/ps2_protocol.h` | wire constants, `ControllerId`, `id_from_byte`, `payload_len`, `frame_len`. Header-only `constexpr`. |
| `src/core/ps2_frame.h/.cpp` | `Ps2Frame`, `DecodeStatus`, `DecodeOutcome`, `decode` returning `std::expected` |
| `src/core/guitar_state.h/.cpp` | `GuitarState`, `Fret`, `map_frame`, the active-low→active-high inversion |
| `src/core/link.h/.cpp` | `LinkState`, `FaultCause`, `Link`, `step` as a pure transition |
| `src/core/hid_report.h/.cpp` | `Button`, `HidReport`, `build_report`, and the byte layout `08-usb-hid` writes its descriptor from |

Tests: nine hand-written vector headers under `tests/vectors/` (plus a `README.md` there
explaining why they are hand-written), `tests/ps2_codec_cases.cpp` with 27 case functions and
3 rule functions (26 + 3 through round 4; round 7 added the link-uniformity case), and `tests/test_ps2_codec.py` as the single driver — it compiles the cases
against the real `src/core/`, runs them, forwards their lines, and carries three mutation
rejection cases.

Checks extended: `tests/test_repo_shape.sh` grew `find_err01`, `find_err02` and a
`core_headers` lister (eight checks → ten, wiring 8/8 → 10/10, accept floor 13 → 20);
`tests/test_style.sh` gained the two clang-tidy flags and now reports R-CLEAN-04;
`.clang-tidy` enabled `readability-magic-numbers` and narrowed `HeaderFilterRegex` to `src/.*`.

Rule work, all seven settled — `grep -c 'planned: 01-ps2-codec' docs/constraints.md` is **0**:

| rule | disposition |
|---|---|
| R-PROTO-01 | rebound `planned: 03-pio-bus` (bit order is a PIO property) |
| R-PROTO-02 | split; core clause → `test: tests/test_ps2_codec.py` |
| R-PROTO-06 | **new**, the bus clause of R-PROTO-02 → `planned: 03-pio-bus` |
| R-PROTO-03, R-PROTO-04 | → `test: tests/test_ps2_codec.py` |
| R-ERR-01 | narrowed in its own text, then → `test: tests/test_repo_shape.sh` |
| R-ERR-02 | → `test: tests/test_repo_shape.sh` |
| R-CLEAN-04 | → `test: tests/test_style.sh` |

New decision record: `docs/adr/0011-pure-link-step.md`. Operator document:
`docs/phases/01-ps2-codec/verify.md`.

Acceptance, measured: `make test` **OK**, `real` between **1:40 and 2:18** across this
session's runs and **2:11** in round 4 (cap 3m; it was 1:16 on this tree before the phase,
and the phase adds C++ compiles plus a third mutation-running check — the spread is the
thread pool, not drift). `make lint` **0**, `planned: 01-ps2-codec` **0**,
`planned: 03-pio-bus` **4**, 9 vector headers, 0 hits for `tests/vectors` under `src/`,
0 hits for `.value()`, 3 `TODO(09-guitar-observe)` markers, one ADR-0011 file, one ADR-0012
file, driver exit 0 with 31 `ok:` lines (30 through round 4), `accounting: test_ps2_codec.py 3 rule(s)`,
`accounting: test_style.sh 4 rule(s)`, `wiring cases: 10/10`, `neutered: 33/33`,
`alternations: 64/64`, `test_phase_docs.sh` 0. All four §Acceptance-criteria mutation blocks
(M1–M4) were run in scratch copies and each failed in the way the spec predicted; M4 produced
exactly `test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing
but its own cases — the real run is gone`.

## Deviations

- **§Goal's first measured fact was wrong in one clause, and step 2 hit it immediately.** The
  spec states `-xc++` is the only flag needed and that **no** sysroot flag is required,
  measured against a header including `<cstdint>` and `<expected>`. The `-xc++` half is
  correct. The "no sysroot" half is not: those two headers happen never to reach libc++'s
  platform layer, so they cannot detect a missing sysroot. `ps2_protocol.h` includes
  `<optional>`, which does, and `make lint` failed inside libc++ with *"We don't know how to
  get the definition of mbstate_t on your platform"* — as `clang-diagnostic-error`, therefore
  as a `FAIL: R-STYLE-02` line. Measured per header: `<array>`, `<optional>`, `<string_view>`,
  `<algorithm>`, `<functional>` and `<variant>` fail without `-isysroot`; `<cstdint>`,
  `<cstddef>`, `<span>`, `<bit>`, `<limits>`, `<type_traits>`, `<concepts>`, `<utility>`,
  `<tuple>` and `<expected>` do not. **Two flags, not three and not one.** The round-6 note was
  half right (sysroot yes, libc++ include path no) and the spec was half right (`-xc++` yes,
  sysroot no). Spec §Goal amended in place with the retraction spelled out; the finding in
  `docs/constraints.md` §Observed conventions records all three readings and what each got
  wrong; `docs/constraints.md` §Style and the `.clang-tidy` header both asserted the same
  fact and were reconciled in the same edit. **Checked for other assertions of it:**
  `tests/test_style.sh` (rewritten), `.clang-tidy` (rewritten), `docs/constraints.md` §Style
  (rewritten) and §Observed conventions (superseded entry replaced), spec §Goal (amended).
  ADR-0010's decision is untouched — only its flag count, which is a measurement.

- **Two acceptance counts in the spec were off by one; amended.** Step 1's check said
  `grep -c 'planned: 01-ps2-codec'` → `5`, but §Goal counts seven today and step 1 rebinds
  exactly one rule away, so six remain. The same slip propagated to step 8's `2`, which is
  `3`. Steps 9 (`1`) and 10 (`0`) were already consistent with six, and §Acceptance criteria's
  `0` — the number that actually closes the phase — was never affected. Both amended in
  `spec.md` with the reason inline; all four downstream counts then verified by running them.

- **ADR-0007's `step` signature could not be written, as the spec anticipated.** ADR-0011
  written, superseding only the signature. One choice the spec did not specify: the parameter
  is `elapsed_us`, not `now_us`, so the result depends only on the arguments and the 32-bit
  counter's ~71.6-minute wraparound stays `hal`'s subtraction rather than a latent bug
  reachable from every state in `core`. Recorded in the ADR with its rejected alternatives.

- **`id_from_byte` returns `std::optional<ControllerId>`, which the spec did not specify.**
  ADR-0009 rejected `std::optional` for `decode` because it loses the reason; `id_from_byte`
  has exactly one failure mode, and `DecodeStatus` belongs to the frame layer, which includes
  this header rather than the reverse. The reasoning is in the file. This is what pulled
  `<optional>` in and therefore what exposed the sysroot deviation above.

- **`R-ERR-02`'s check scans `src/core/*.h` only.** Not a shortcut: `[[nodiscard]]` belongs on
  the declaration, and C++ does not repeat it on the out-of-line definition — scanning `.cpp`
  reported `LinkState step( … ) {` in `src/core/link.cpp`, which is correct code. Found by
  running the check against the real tree, not by reasoning. Required a new `core_headers`
  lister. The residual gap (a function defined only in an anonymous namespace, with no
  declaration) is a `belay-debt:` comment in the check and an entry under §Debt.

- **`R-ERR-02`'s first pattern had a false positive on a struct member.** `LinkState state =
  LinkState::Absent;` in `Link` matched "line starts with LinkState followed by an
  identifier". Every alternative now ends in an identifier followed by `(`, which is what
  makes it a check on return types. Found by the real run, recorded in the check's comment.

- **`readability-magic-numbers` found nothing in `src/core/`,** so step 10's "fix whatever it
  finds" had nothing to fix — the code was written with named `constexpr` throughout. Because
  a check that finds nothing is indistinguishable from one that is disabled, it was proved
  live adversarially instead: replacing `kBitsPerByte` with `8` in `src/core/hid_report.cpp`
  makes `make lint` fail naming the literal. That probe is what uncovered the §Debt entry
  below.

- **The vector `analog_idle.h` was first written with a computed header byte (`0x41 + 0x32`)
  and corrected to the literal `0x73`** before anything ran. Recorded because it is precisely
  the failure R-PROTO-05 exists to prevent, committed inside the file whose only job is to be
  a literal. All eight vectors were then swept for arithmetic; none remains.

- **Two files the spec's Context pointers did not name were written:**
  `tests/vectors/README.md` (why the vectors are hand-written, and the frame shape) and
  `tests/ps2_codec_cases.cpp`'s dependency on `.clang-format`'s `InEmptyParentheses: false`
  (empty parens are `()`, not `( )`) — the latter cost three failed edit attempts against
  anchors that clang-format had rewritten. Not a spec gap so much as a house-style fact worth
  stating once: **anchors for any scripted edit must be read back from the formatted file.**

- **The driver leaked temp directories into the repository, and it was found by reading
  `git status` at close rather than by any check.** `build_and_run` put the compiled binary in
  `tempfile.mkdtemp( dir=tree )`; for the real run `tree` is the repo, so every `make test`
  left two `tmp*/` directories in the working tree — 25 of them by the end of the session.
  Harmless to the results (they are empty of `.cpp`/`.h`, so `test_style.sh`'s file list and
  the secrets scan saw nothing), but it is the same shape as the `mut_*.sh` orphans
  `tests/test_checks_are_live.py` has a `sweep_orphans` for. Fixed by building in the system
  temp directory inside a `TemporaryDirectory` context manager, so the exception paths clean
  up too and the tree cannot be polluted in the first place. The 25 were swept. Worth naming
  because **nothing in the suite noticed**: see §For later phases.

- **`docs/index/` was not regenerated.** The spec does not list it and `/refresh-index` is a
  separate command; `src/core/` and three test files are new, so the index is now stale.
  Named here rather than silently left.

### Round 2 — the five findings of validation round 1

- **`tests/vectors/README.md` is named in the spec; it was not deleted.** The reviewer read
  §Plan step 4's "(eight files)" as fixing the directory at eight and returned `contradicts`.
  The operator's decision is that the file is the provenance half of R-PROTO-05 and carries
  four facts no other file states — that no value comes from `core` or the emulator and why a
  shared misreading would otherwise be invisible; that the bytes arrive with the first one
  dropped; why headers and not hex text; and that digital buttons are active low, so nothing
  pressed is `0xFF 0xFF` and not `0x00 0x00`. The last one is the one that stops an inverted
  vector every assertion then agrees with. So the spec was wrong, not the file: §Plan step 4
  now reads "(nine header files plus `README.md`)" and §Files this phase writes gained a row
  for it. **Reconciled in the same edit:** §Plan step 4's count, §Files' `tests/vectors/*.h`
  row, and nothing else — checked §Goal ("no others exist", which is about *vectors*, and a
  README is not one), §Vectors' own count (a separate amendment below), and the acceptance
  criterion `ls tests/vectors/*.h | wc -l`, which globs `.h` and never saw the README.

- **A ninth vector, `not_ready.h`, and §Vectors stops saying "eight".** §Plan step 3 mandated
  refusing a bad ready byte and no vector exercised it, which the reviewer returned as
  `undecidable`: the spec did not say whether that clause was meant to be asserted. Decided:
  asserted. A refusal path with no coverage in a phase whose whole stated observable behaviour
  is "refusing by default" is the first thing to rot, and the mandate was already in the spec —
  what was missing was the vector. `not_ready.h` is a declared digital header with `0xFF` where
  `kReadyByte` belongs, full length, legitimate payload, so nothing but the ready byte can be
  the reason it is refused. `0xFF` is not an arbitrary wrong byte: `DATA` is open-drain with a
  pull-up, so it is what the master shifts in when the controller has stopped driving the line
  — a different failure from `truncated_ack.h`, where the bytes stop arriving at all.
  **Reconciled in the same edit — every statement that said eight:** spec §Goal, §Vectors' lead
  line and table, §Files' `tests/vectors/*.h` row, §Plan step 4, the acceptance criterion
  `ls tests/vectors/*.h | wc -l` (8 → 9), `verify.md`'s section heading, its intro sentence, its
  table lead-in, its table, and its §4 checklist expectation (`8` → `9`). **This list was wrong
  when first written: it missed §Plan step 11's "what the eight vectors are", which round 2's
  reviewer caught and which is exactly the failure the reconciliation requirement exists to
  prevent — a statement the amendment invalidated, sitting in the spec but not in the diff.
  Amended, and the miss is recorded rather than quietly corrected.** **Checked and left
  alone:** spec lines saying "the eight checks already there" and "the eight existing checks"
  (§Rule work and §Plan step 9) — those count `test_repo_shape.sh` checks, not vectors;
  `tests/vectors/README.md` and `tests/ps2_codec_cases.cpp`, which state no count;
  `verify.md`'s "roughly two dozen lines", which is a property and survives 28 → 29.

- **ADR-0012 written: `DecodeStatus` carries decode outcomes only.** The reasoning for dropping
  three of ADR-0007's five sketched members lived in a header comment, which is neither an ADR
  nor a finding — the reviewer's second `undecidable`. Classified as a **decision**: it
  constrains the future, because `07-analog-mode` is about to want a status and needs to know
  where it goes. A finding would have been wrong — a finding is a verified fact that stops
  being true when the code changes, and this is a rule meant to bind precisely when someone is
  about to change the code. ADR-0012 supersedes **only** that membership, the way ADR-0011
  superseded only the signature, and adds the membership rule: a status belongs in
  `DecodeStatus` if and only if it is decidable from the bytes of a single frame, with no
  history and no knowledge of what was asked. **Reconciled in the same edit:**
  `src/core/ps2_frame.h`'s comment no longer argues the case, it points at ADR-0012 and
  restates the rule; §Plan step 3 and §Files this phase writes name the ADR; §Acceptance
  criteria gained `ls docs/adr/0012-*.md`. ADR-0007's status line was left untouched — it
  already says the pure-decoding mechanism is superseded by ADR-0009, and ADR-0012 records its
  own supersession, which is the house pattern (an ADR is immutable).

- **The link's timeout: the transition stays here, the number goes to `07-analog-mode`.** The
  reviewer could not tell from the spec which states this phase owned. Decided and written into
  §Plan step 6: this phase owns all four `LinkState` members and every transition between them,
  including the exit from `Negotiating`, because the alternative is a trap state — a controller
  that keeps answering in config mode would hold the link there forever, contradicting
  §Error handling's "the firmware retries rather than stopping". The transition is decidable
  from `(state, outcome, elapsed_us)` with no clock read, which is `CLAUDE.md` §Architecture's
  own test for what belongs in `core`. The **budget** is not: 100 ms is a guess and nothing has
  timed a real negotiation, so tightening it is `07-analog-mode`'s, the first phase able to
  measure it. Same split as the `TODO(09-guitar-observe)` markers. **No code moved.**
  **Reconciled in the same edit:** §Out of scope gained the departing half, §Plan step 6's check
  line now names the `Negotiating` timeout drop, and the `belay-debt:` marker in
  `src/core/link.h` already said this and was left as written.

- **R-ERR-02's narrowing is now in the rule's own text; the check's scope is unchanged.** The
  reviewer's fourth `undecidable`: §Plan step 1 required R-ERR-01's equivalent narrowing to be
  recorded in the rule text, and the spec did not say whether R-ERR-02 owed the same. Decided:
  it does. `docs/constraints.md` R-ERR-02 now states that the check scans `src/core/*.h` only,
  why (`[[nodiscard]]` belongs on the declaration and C++ does not repeat it on the out-of-line
  definition, so scanning `.cpp` flags correct code), what is given up (a function declared in
  no header and defined only in an anonymous namespace), why the exposure is small, and that
  closing it needs `03-pio-bus`'s `clang-query` upgrade. The `belay-debt:` comment in the check
  stays: it names the ceiling where the code lives, the rule text names it where the rule lives.
  **Scope deliberately not widened** — that is new work, and a check whose scope grows during
  its own closing iteration is new work wearing a fix's clothes. §Out of scope says so and
  §Plan step 9 says so. **Reconciled in the same edit:** R-ERR-02's catalogue text, §Plan
  step 9, §Out of scope. R-ERR-01's text was checked and needed nothing — it already carried
  its narrowing.

- **The `grep` backreference criteria are an environment fact and the spec now says so.** Two
  acceptance criteria use an ERE backreference (`([0-9]+)/\1`), which is what makes the two
  sides *equal* rather than both merely present. Under an agent session `grep` is shadowed by
  `ugrep`, which rejects `\1` as `invalid escape`; under `/usr/bin/grep` both pass. Round 1
  spent a cycle diagnosing this. §Acceptance criteria now carries the note so the next round
  reads it instead of rediscovering it. No project defect and no criterion changed.


- **`verify.md`'s `make test` runtime was stale and told the operator a correct run was wrong.**
  It said "somewhere between one and two minutes"; this session measured 2:07.87 and 2:07, and
  §Outcome above already recorded 1:40–2:18. An operator document that calls a passing run
  suspicious is worse than one that says nothing. Corrected to "roughly two minutes … between
  1:40 and 2:20, and the phase's own limit is three". Found by round 2's reviewer as a gap it
  could not decide from the spec; it is not a spec gap, it is a measurement that drifted.


### Round 3 — two spec corrections, by operator decision. No code touched.

- **§Plan step 11 contradicted itself in one sentence; resolved as a boundary, not an
  exception.** It demanded "the exact commands to run and what each should print" and then
  "State properties of a run, not transcripts of it", so round 2's reviewer could only report
  the unambiguous half and returned `contradicts` against `verify.md` §3's six-line `FAIL`
  block. The operator's decision is to keep the block, and the rule is now written as the
  distinction it was always reaching for: **never transcribe what a normal run prints; always
  name what a deliberate exercise must produce.** The test is whether the document told the
  operator to *cause* that output. Incidental output drifts with the code and is what the
  "properties" instinct guards against; forced output is the result of the exercise, and
  without it the operator cannot tell that the cut landed rather than that something else
  broke. **Reconciled in the same edit:** §Plan step 11 itself — the one place in this spec
  that stated it. **Checked and left alone, with reasons:** `verify.md` §3's block (the thing
  the amendment exists to keep); `verify.md`'s "roughly two dozen lines" and "Expect two
  lines", which are property statements about normal runs and stay on the permitted side of
  the new line; `docs/phases/00-scaffold/spec.md` and its notes, which state the rule for
  *that* phase and are a closed phase's record — a closed phase is not edited, and its
  wording is its own.
  **Verified rather than assumed: nothing enforces this mechanically.** `00-scaffold`'s notes
  describe a "no-transcript grep", so before writing a clause that permits a fenced block I
  checked whether a live check would reject it. `tests/test_phase_docs.sh` only asserts that a
  `done` phase has a non-empty `verify.md`; no check under `tests/` reads `verify.md`'s
  content at all, and `make test` is green today with that block present. The amendment
  therefore changes a written rule and contradicts no gate.
  **Upstream:** `docs/templates/spec.md` line 19 ships the same instruction —
  *"properties, never transcripts"* — and it is a package file in `.claude/workflow/installed`.
  The distinction written here belongs there; `/belay-feedback` is the channel and this
  project's copy is a workaround until it lands.

- **`map_frame` gates the buttons on the id, as it already gates the whammy.** Round 2 proved
  a config-mode frame maps to all ten controls pressed (measured; the run is in that round's
  record). The operator's contract: finish the gate on the side it was left off. This is the
  option that makes the four documents already asserting it *true* without rewriting any of
  them, and it keeps the invariant inside `core`: the alternatives either move it to the
  caller — and a hardware-free `core` that depends on external discipline is no longer
  provable by `make test` on a laptop (ADR-0002, R-ARCH-01) — or rewrite four documents to
  land on a worse contract. Written into §Plan step 5 as a code step with its case.
  **No new vector, stated because the operator conditioned it:** `config_mode.h` is already
  the case and already the adversarial one — its all-`0x00` payload is exactly the value an
  ungated mapper reports as everything pressed. §Vectors stays at **nine**.
  **Reconciled in the same edit:** §Plan step 5 (the contract and the new case), §Vectors'
  `config_mode.h` row (its asserted outcome named only the decode; it now names the mapping
  too), and §Goal's observable-behaviour paragraph (which listed the refusals and the whammy
  rest and said nothing about config). **Checked and found already correct, needing no edit —
  which is the whole reason this contract was chosen:** `src/core/guitar_state.h`'s `map_frame`
  comment ("Config, which is a valid frame that reports no controls at all"),
  `tests/vectors/config_mode.h` ("reading buttons or a whammy out of it is what must not
  happen", "no button meaning"), and `verify.md`'s table ("Recognised, but it is not a button
  report"). All four become true when the code lands; none was reworded to fit.
  **Still open and owed to `/implement-phase`:** the code change itself and its case. Nothing
  under `src/` or `tests/` was touched in this pass. **Closed in round 4 below.**


### Round 4 — the code round 3 owed. One cut, one case, one enumeration.

- **`map_frame` now gates the buttons on the id, and the gate is a closed positive set.**
  Round 3 settled the contract and explicitly left the code for `/implement-phase`; this is
  that change and nothing else. `reports_controls( ControllerId )` in
  `src/core/guitar_state.cpp`'s anonymous namespace answers yes for `Digital` and `Analog`
  only, and `map_frame` returns a default `GuitarState` with `kWhammyRest` when it answers
  no. **Positive and not negative on purpose:** an id added to the table later reports
  nothing until someone decides otherwise, which is the same refusing-by-default posture as
  `id_from_byte`; a `!= Config` test would silently enrol the fourth id the SG might turn
  out to answer with (`09-guitar-observe` owns that question).
  The whammy's `if ( frame.id == ControllerId::Analog )` line was left byte-for-byte
  intact — it is the R-PROTO-04 mutation anchor in `tests/test_ps2_codec.py`, and the
  rejection cases still report 3/3, which is how that was verified rather than assumed.

- **The case, and the probe that proves it is live.** `case_config_mode_maps_to_nothing_pressed`
  asserts all five frets, both strums, start, select and tilt released on the `config_mode`
  vector. Because a case that passes is indistinguishable from one that asserts nothing, the
  gate was cut in a scratch copy (`reports_controls` returning true for every id) and the run
  read back: **exactly one line flipped** —
  `FAIL: config_mode: maps to every one of the ten controls released` — and **all three rule
  lines stayed `ok:`**. That is round 2's finding confirmed from the other side: R-PROTO-02,
  03 and 04 are structurally incapable of catching this, so this case is the only thing
  standing behind the button half of the gate. Anything that deletes it deletes the guarantee.
  Two earlier cuts were rejected by `-Werror` before they could run (`if ( false )` orphans
  the function, `return true;` orphans its parameter) — worth naming because a cut that does
  not compile is not evidence.

- **Generalised before fixing (command step 5): every payload read in `core` is gated on the
  id that announces it.** The finding was one instance; the property is the rule. Enumerated
  by grep over `src/core/`, all four reads of `frame.payload`, checked one by one:
  `guitar_state.cpp:37,38` (the two button bytes — the instance, now gated by
  `reports_controls`), `guitar_state.cpp:61` (the whammy — already gated on `Analog`), and
  `ps2_frame.cpp:42` (the decode copy — bounded by `payload_len( *id )`, which *is* the id's
  own announcement). `link.cpp` and `hid_report.cpp` read no payload at all: `step` reads the
  outcome's id and `build_report` reads a `GuitarState`. **The property holds everywhere; the
  fix was genuinely one instance.** Recorded as the enumeration, not the result, so the next
  round does not redo it.

- **Four counts in §Outcome above were stale and are corrected.** Round 2 added a ninth
  vector and reconciled the spec and `verify.md`, but not its own notes: §Outcome still said
  "eight hand-written vector headers", "8 vector headers", "24 case functions" and "28 `ok:`
  lines", and §For later phases still told `05-emulator` "the eight vectors". All measured
  fresh rather than incremented — 9 headers, 26 case functions, 30 `ok:` lines (29 case and
  rule lines plus the rejection-case line), 9 vectors — and ADR-0012, written in round 2, was
  missing from the acceptance list. This is the same class as round 2's own recorded
  reconciliation miss, one file further out: **a count in §Outcome is a measurement with a
  date, and an amendment that changes it has to re-measure it.**

- **No spec amendment this round.** Every statement in `spec.md` about this behaviour —
  §Goal's observable-behaviour paragraph, §Vectors' `config_mode.h` row, §Plan step 5 —
  already asserts the gate; round 3 wrote them that way deliberately so the code could land
  without rewriting them. Checked for others: `grep -rn 'map_frame' docs tests/vectors` finds
  only those, `src/core/guitar_state.h`'s contract comment (amended in this edit: it stated
  the whammy gate and then described the buttons as unconditional), `tests/vectors/config_mode.h`
  and `verify.md`'s table (both already correct, neither touched). `docs/index/src-core.md`
  does not mention `map_frame`.


### Round 5 — validation round 3 escaped the loop. No code finding; seven missing pointers.

- **The escape fired, and this is the reasoning.** Three validation rounds, no prior
  `escaped to /expand-phase` verdict, so the count is three against one spec. Every machine
  gate is green and has been for three rounds — criteria 22/22, `make test` and `make lint`
  pass, the boundary sweep is clean and was proved live, the index is fresh. What fails each
  round is the spec's descriptive completeness, and it is **not converging**: round 1 returned
  four `undecidable`, round 2 four, round 3 six. Each round closes the gaps its own diff
  touched and the next round finds a different set, which is the regeneration the escape
  exists to stop. `spec.md` is NOT patched here — the six findings are the input to
  `/expand-phase`, not six more amendments to a document that has been amended in every round
  so far. Status moved to `pending` by `/validate-phase`, which is the one time it writes that.

- **What the reviewer could not resolve — six, listed in the round-3 validation record below**
  and not restated here. Their shape is one thing: **the spec describes `decode` and `map_frame`
  in detail and describes `Link` and the frame's storage barely at all.** Four of the six are
  `Link` (`FaultCause`'s members, `last_fault`'s lifetime across a good frame, `us_in_state`'s
  accounting, the transition table quantified but never enumerated) or `Ps2Frame`'s fixed-width
  payload and its zero fill. That is where the re-expansion has to do its work.

- **A seventh, found here rather than by the reviewer, and invisible to it by construction.**
  §Context pointers line 114 names this phase as owner of `00-scaffold`'s `.py`-check debt.
  No Plan step closes it, no acceptance criterion covers it, and §Out of scope never releases
  it — while §Debt above records that the debt **grew**: `tests/test_ps2_codec.py` is now a
  second `.py` check held to the accounting property only. The reviewer cannot see this: the
  file that would carry the fix is not in the diff, because nothing touched it. **An inherited
  obligation that the spec neither schedules nor releases is a pointer missing in the one
  direction a diff review can never find.** `/expand-phase` owes it a decision either way.

- **One `contradicts` was returned and it is not a code finding.** The reviewer reported that
  §Plan step 11 says the step touches `notes.md` while the diff carries no `notes.md` hunk —
  and flagged its own caveat, that it could not tell a filtered diff from a missing write.
  `notes.md` is written; it was withheld because `/validate-phase` step 5 forbids it as an
  input. The command orders the diff "restricted to step 3's file set" and forbids `notes.md`
  in the same step, and the file set always contains `notes.md`. Any spec whose Plan says it
  writes notes — which the shipped template requires — produces this verdict forever, and no
  code change can clear it. Package defect, recorded on the `upstream:` line. **Amended in
  round 6:** the workaround is no longer this paragraph — it is `spec.md` §Notes to
  `/validate-phase`, note 1, which instructs the dispatcher and is read by the reviewer
  itself.


### Round 6 — a local patch for two Belay defects, written to survive the re-expansion.

**No code was touched in this pass.** The only files edited are `spec.md` and this one.

- **What was added, and where.** `spec.md` gained one new section, §Notes to
  `/validate-phase`, holding two notes: (1) when dispatching step 5's reviewer, name the
  paths held out of its diff — `notes.md` always, plus whatever the `.claude/workflow/installed`
  subtraction removed — and say their absence is not a finding; (2) `scripts/check.sh --files`
  at this version skips a non-file argument in silence and still prints `all gates passed`, so
  a zsh-concatenated argument list produces a sweep of zero files indistinguishable from a
  clean one. Both are Belay defects, **fixed upstream in 792e9d9**; this repo is pinned at
  **f001884** and takes the update when the phase closes. **Both notes are deleted then**, and
  they were put in one section so that removal is one cut.

- **One correction to the instruction as given, made because the note has to be runnable.**
  The operator's wording was to "confirm that the count of swept files matches the file set".
  There is no such count to read: `check.sh --files` prints nothing per file and only its
  closing line, which is exactly why the empty sweep is invisible. The note therefore puts the
  count on the **input** side (`wc -l` of the file set passed through `xargs`) and adds the
  defence that actually distinguishes the two cases — an adversarial probe, `#include
  "src/hal/bus_io.h"` in a `src/core/` header, which `deny core -> hal` must catch. It also
  records the probe *not* to use: an SDK include returns 0, because `boundaries.rules` says in
  its own header that R-ARCH-01 is not expressible as a layer rule and is enforced by the host
  build and by greps in `tests/` instead. A note that tells the next round to read a number
  that does not exist would have cost the cycle it was written to save.

- **`spec.md` is the wrong place for these to live, and that is why they are also here.**
  `/expand-phase` step 5 writes `docs/phases/$1/spec.md` from `docs/templates/spec.md` — a
  rewrite, not an amendment — while step 2 states that `notes.md` "is never rewritten here"
  and orders the re-expansion to derive the spec from the row's Goal and from these notes,
  never from the tree. So the section added to `spec.md` today does not survive
  `/expand-phase 01-ps2-codec`, which is the next command. **This entry is the durable copy.**

- **Owed to `/expand-phase`, explicitly:** re-emit both notes into the new `spec.md`, as their
  own section, with the same deletion trigger. They describe the tooling that will run against
  the next spec, not anything this cut decided, so they are the one part of this document that
  a re-expansion must carry forward verbatim rather than re-derive.

- **Reconciled in the same edit.** Round 5's `contradicts` paragraph closed with "the
  workaround is this paragraph"; that is now false and was amended to point at the spec note.
  **Checked and deliberately left alone:** the `upstream:` line of the round-3 validation
  record, and rounds 1 and 2 — a validation record is a dated account of what that run found,
  and `/belay-feedback` is still owed regardless of this local patch, which works around the
  defects without reporting them. **Checked and unaffected:** nothing under `src/` or `tests/`
  mentions either defect, and no check reads `spec.md` (`grep -rn 'spec\.md' tests/` is empty),
  so the new section changes no gate.


### Round 7 — the two items the re-expansion owed. One case, one criterion, two reconciliations.

- **`case_link_target_depends_on_the_outcome_alone`, and the cut that proves it.** §Plan step 10.
  Each existing `link:` case drives one outcome from one source state, so together they sample
  the transition table without ever asserting the property that makes it six rows instead of
  twenty-four. The new case builds the four source states by **driving** the link to each — a
  state the machine cannot reach is not one it has to be uniform from — then applies each of the
  six outcome vectors to a copy of all four and asserts one target per outcome.
  Proved live rather than assumed, in a scratch copy: making the error branch return
  `Negotiating` instead of `Absent` when `was == AnalogStreaming` flips **exactly one line** —
  `FAIL: link: the target of a transition depends on the outcome alone` — while **all three rule
  lines stay `ok:`**. Same shape as round 2's config-mode finding, confirmed from the other
  side: R-PROTO-02, 03 and 04 are structurally incapable of catching a source-state special
  case, so this case is the only thing standing behind the uniformity claim in §The link.

- **The stray-header criterion (§Plan step 11) needed no code.** `.clang-tidy`'s
  `HeaderFilterRegex` is `src/.*`, which stops clang-tidy diagnosing *every* header under
  `tests/`, not only the vectors — correct today only because the vectors are the only such
  headers, a claim about the tree that nothing checked. It is now a criterion:
  `find tests -name '*.h' -not -path 'tests/vectors/*' | wc -l` → `0`. Measured today: 0.

- **Deviation: the spec's negotiation bound said `reached`; the code is strictly `>`.** Found
  by reading `link.cpp` against §The link, not by a gate — no test pins the boundary, since the
  timeout case feeds `kNegotiationTimeoutUs` on top of an already non-zero accumulator. Nothing
  ever decided the inclusive/exclusive question; "has reached" was prose written during the
  re-expansion. The spec was amended to say the bound is exclusive rather than the code changed,
  because every other statement of it already agrees. **Enumerated rather than spot-fixed
  (command step 5): every place that states the bound.** `src/core/link.cpp:68` (`>`),
  `src/core/link.h:46` ("outlasted"), `tests/ps2_codec_cases.cpp` (the case label, "past"),
  `docs/constraints.md:240` (a mutation anecdote, not a statement of the bound). Four checked,
  three already correct, one — the spec — wrong and amended.

- **Deviation: `verify.md`'s "roughly two dozen lines" was drifting and this round made it
  worse.** Round 2's reviewer raised it as taste and it was left; the driver printed 30 lines
  then and 31 now. Rather than re-round the number, the count was **removed**: the sentence now
  reads "one line per case and per rule". That is `00-scaffold` §For later phases' rule at line
  3256 applied rather than restated — a number in prose has no check behind it. `verify.md`'s
  other count, §1's "Expect two lines", is a property of a two-rule check and was left alone.

- **`make test` failed once on R-STYLE-01 and the fix was the formatter, not the code.** The new
  case's initializer alignment and `&&` continuations differed from `.clang-format`; the gate
  named eight lines and `clang-format -i` settled it. Recorded because it is the third time this
  phase has hit the house rule that **anchors for a scripted edit must be read back from the
  formatted file** — the case was re-read after formatting before anything else was written.

- **§Outcome's counts re-measured, not incremented.** 27 case functions and 31 `ok:` lines,
  counted from the file and from a real run, with the round-4 values kept beside them. This is
  the discipline round 4 recorded after the same counts went stale once already.

- **Acceptance: all 19 criteria pass**, `make test` `OK` in **2:09** against the 3m cap,
  `make lint` 0. M1-M3 are the driver's rejection cases (`3/3`); **M4 was re-run by hand** in a
  scratch copy against today's tree and produced exactly
  `FAIL: test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing but
  its own cases — the real run is gone`, exit 1.


### Round 8 — what validation 2026-09-14 found. One contradicts, six undecidable, one self-found.

Recorded here because the closure test requires it; the fixes are `/implement-phase`'s, and
**none of them is applied yet.**

- **The `contradicts`, verified against the tree rather than taken on the reviewer's word.**
  `spec.md` §The frame says in bold that `Ps2Frame` stores the payload "alongside the announced
  length". `src/core/ps2_frame.h` has exactly two members — `ControllerId id` and the fixed
  `payload` array — and its comment argues the opposite deliberately: "carrying no length of its
  own: how many payload bytes are meaningful is `payload_len( id )`, so the two can never
  disagree." Two test hunks are built on the code's choice, recomputing from the id because no
  stored length exists. **The reading this round hands forward, which is not a decision this
  gate is allowed to make on its own: the code is right and the spec sentence is the defect.**
  A stored length that must always equal `payload_len( id )` is a second source of truth for one
  fact, and the phase's own §Error-handling posture is against that. The sentence was written
  during the 2026-09-14 re-expansion describing what its author believed the code did; the
  reviewer checked and it does not. Routing is unchanged regardless of which side is wrong: a
  `contradicts` fails the gate and goes to `/implement-phase`.

- **Six `undecidable`.** Each is a missing pointer, listed in the validation record below and
  not restated here. Two of them are, in substance, the same defect this phase keeps paying
  for — **a count with no enumeration behind it**:
  - the `TODO(09-guitar-observe)` markers: §Goal names four concerns, the tree carries three
    markers, the acceptance criterion says "1 or more", and `verify.md:165` tells the operator
    "Expect three of them". Confirmed by measurement: three markers, and concern two spans
    eleven `constexpr`s rather than the "one named `constexpr`" §Goal claims. Three documents,
    three different numbers, nothing reconciling them.
  - `tests/test_repo_shape.sh`'s accept floor moved 13 → 20 and the spec pins only the wiring
    count.
  - `kPadByte` verified present in `src/core/ps2_protocol.h:29` and referenced nowhere else;
    §Files enumerates that header's constants and does not include it, and never says whether
    the enumeration is exhaustive.

- **One this gate found on its own, of the same class, before the reviewer returned.** §Plan
  step 7's check reads "the four `hid:` cases are `ok:`" — a number whose set the spec never
  names. It belongs with the two above: fix it as a property ("every `hid:` case is `ok:`")
  rather than by writing a fourth enumeration that then has to be maintained.

- **M4's criterion was not runnable as written, and was fixed during this round** (§Plan
  step 1's route for an unrunnable criterion). Detail in the validation record; the reconciled
  twin in §For later phases was corrected in the same edit.


## Debt

- **`belay-debt:` in `tests/test_repo_shape.sh` (`core_headers`)** — R-ERR-02 cannot see a
  function that is defined only inside an anonymous namespace in a `.cpp` and never declared
  in a header. Those cannot be called by anyone who could ignore the result, so the exposure is
  small, but the gap is real. Upgrade path: the same `clang-query` + `compile_commands.json`
  that `03-pio-bus` already owes.
- **`belay-debt:` in `src/core/link.h` (`kNegotiationTimeoutUs`)** — 100 ms is a budget, not a
  measurement; nothing has yet timed a real negotiation. `07-analog-mode` drives the sequence
  for the first time and owns tightening it.
- **`readability-magic-numbers` ignores every literal inside a `const` or `constexpr`
  initializer.** Measured, and it matters here because `const bool is_ok = <expr>;` is this
  repo's dominant idiom in both `src/core/` and the cases file — so a magic number written
  inside one is not diagnosed. It *is* diagnosed in arithmetic, comparisons, subscripts, call
  arguments, `return` statements, shifts and non-`const` local initializers. Recorded in
  `docs/constraints.md` §Observed conventions and in R-CLEAN-04's own text. No cheap upgrade:
  `cppcoreguidelines-avoid-magic-numbers` is an alias of the same check and behaves
  identically, so closing it means a second checker or writing fewer `const` initializers.
  Declared rather than papered over.
- **The three mutation anchors in `tests/test_ps2_codec.py` are exact source lines**, so any
  reformatting of `ps2_protocol.h`, `guitar_state.cpp` or `ps2_frame.cpp` breaks them. That is
  survivable only because each mutation asserts the text changed before using it — the failure
  is a named `FAIL` line, never a silent pass. Hit twice during this phase, both times caught
  by that assert. Upgrade path: none proposed; the assert is the mitigation and it works.
- **The `.py`-check debt `00-scaffold` assigned to this phase is NOT closed, and is now
  explicitly released.** `tests/test_checks_are_live.py` still holds `.py` checks to the
  accounting property only — their internals are never mutated. `tests/test_ps2_codec.py` is a
  second `.py` check in that position, so the debt grew rather than shrank. The 2026-09-14
  re-expansion stopped leaving that implicit: `spec.md` §Out of scope releases it to no phase,
  with the reason, because naming this phase as owner while no step closed it is what made it
  undecidable in validation round 3. See §For later phases.

## For later phases

- **`03-pio-bus`** — owns four `planned:` rules now, up from two: R-SAFETY-07 and R-ERR-05 as
  before, plus **R-PROTO-01** (LSB-first, SPI mode 3) and **R-PROTO-06** (the master waits for
  `ACK` after every byte but the last). Both are properties of the wire that the loopback
  round-trip is the first thing able to observe. The traceability meta-test will not let that
  phase close with them still `planned:`.
- **`03-pio-bus`** — `decode` takes `std::span<const std::uint8_t>` holding
  `[header][ready][payload…]`, i.e. the controller's response **with its first byte dropped**.
  That first byte answers the address byte and carries nothing. Dropping it is `hal`'s job and
  is currently documented only in `src/core/ps2_protocol.h`'s header comment — if `hal` passes
  the raw shift-in buffer instead, every frame decodes as `UnknownId`.
- **`03-pio-bus`** — `step` takes **elapsed** microseconds, never a timestamp (ADR-0011). The
  32-bit wraparound subtraction is `hal`'s to get right, once. Nothing in `core` can detect a
  caller that passes a constant; that phase has the first real clock with which to be wrong.
- **`05-emulator`** — the emulator shares `core`, so it must **not** be used to produce or
  check expected bytes (R-PROTO-05). The nine vectors are the independent reference; a
  disagreement between the emulator and a vector is a finding about one of them, never a
  reason to regenerate the vector.
- **`07-analog-mode`** — the config-mode command bytes are declared in `ps2_protocol.h`
  (`kCmdConfig`, `kCmdSetMode`, `kConfigEnter/Leave`, `kModeDigital/Analog/Locked`) and nothing
  drives them yet. `FaultCause` has a `Negotiating` member for the timeout; that phase will
  want at least one more cause for "answered the config sequence, then declined analog mode".
- **`08-usb-hid`** — the HID descriptor must be written **from** `src/core/hid_report.h`, not
  independently: `kButtonCount`, `kBitsPerByte`, `kButtonBytes`, `kWhammyOffset` and
  `kReportLen` are derived from each other, and `Button`'s enumerators are the HID button
  indices in order. A descriptor that disagrees produces a device that enumerates cleanly and
  reports the wrong control — the worst kind to debug from the host side.
- **`09-guitar-observe`** — this is the phase's largest open item and it is entirely
  unverified. **Three `TODO(09-guitar-observe)` markers** cover: the three controller id bytes
  (`kIdDigital` 0x41, `kIdAnalog` 0x73, `kIdConfig` 0xF3); every button bit position and mask
  in `guitar_state.h`; and `kWhammyIndex` (5), `kWhammyRest` (0x80). All of it is read from
  protocol documentation and common descriptions of Guitar Hero controllers, none of it
  measured. The vectors assert the codec does what the codec says — **not that the SG agrees.**
  Note also that `unknown_id.h` deliberately uses `0x79`, the DualShock 2's real full-analog
  id; if that phase finds the SG uses it, the vector must be repointed at another undeclared
  byte rather than the id simply being added.
- **Any phase adding a `.py` check** — `00-scaffold` assigned this phase the debt of
  strengthening `tests/test_checks_are_live.py` beyond the accounting property for `.py`
  files. It was **not** closed here, and this phase added a second such file, so the exposure
  doubled. **Released to no phase on 2026-09-14** (`spec.md` §Out of scope): it is check-harness
  work, and it belongs with the `clang-query` + `compile_commands.json` upgrade `03-pio-bus`
  already owes and the "no new untracked paths after `make test`" check named below. Whoever
  next touches that harness inherits all three. `tests/test_ps2_codec.py`'s own internals are unmutated; what stands behind it is
  its three rejection cases and the four §Acceptance-criteria mutation blocks, which were run
  by hand. The honest statement is that the `.sh` checks are mutation-tested and the `.py`
  checks are not.
- **Any phase running a mutation in a scratch copy** — the copy must carry **both** the
  uncommitted work and `.git`, and only `cp -a` does. Copying without `.git` makes
  `tests/test_secrets.sh` fail its history clause (`has no .git — the history clause cannot be
  scanned here`), and anything running `test_checks_are_live.py` hits that too, because it runs
  every `test_*` file on the tree it is handed. **`git worktree` is the wrong tool here and the
  original version of this note recommended it** — it checks out HEAD, so for a phase whose work
  is uncommitted the copy contains none of that work and the mutation has no anchor to move.
  Corrected 2026-09-14 after validation ran it as written and measured both halves.
- **Any phase writing a scripted edit against C++ source** — `.clang-format` sets
  `InEmptyParentheses: false`, so `foo( )` becomes `foo()`, and it re-indents `switch` bodies.
  Read anchors back from the formatted file; three edits in this phase failed on anchors that
  had been rewritten. Every one was caught by an `assert`, which is the only reason they were
  cheap.
- **Any phase whose check writes files** — nothing in this suite notices a check that
  litters the working tree. This phase's driver left 25 stray directories in the repo root
  across the session and every gate stayed green; it was caught by reading `git status` by
  hand at close. `tests/test_checks_are_live.py` sweeps `mut_*.sh` specifically, which is a
  fix for one known instance rather than the property. A cheap general check — "the tree has
  no new untracked paths after `make test` that it did not have before" — does not exist and
  would have caught both. No phase owns it yet; it is a candidate for whichever phase next
  touches the check harness.

- **Whoever runs `/refresh-index`** — `docs/index/` predates `src/core/` entirely and is stale
  as of this phase.

## Validation — 2026-09-11
- criteria: 16 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap — `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean (21 source files; 13 active `deny` rules, and `src/core/` is a declared layer, so this is a real sweep and not an inert one)
- independent review: contradicts: `tests/vectors/README.md` is a ninth file in a directory §Plan step 4 fixes at "(eight files)" and §Files this phase writes covers only as `tests/vectors/*.h`. undecidable: (1) `kNegotiationTimeoutUs`, `Link::us_in_state` and `FaultCause::Negotiating` are a timeout *policy* the spec never assigns to this phase rather than to `07-analog-mode`; (2) `DecodeStatus` drops three of ADR-0007's five sketched members with the reasoning living in a header comment, which is neither an ADR nor a finding, while §Plan step 6 authorised ADR-0011 to supersede "only" the `step` signature; (3) §Plan step 3 mandates refusing a bad ready byte, but §Vectors fixes eight vectors and none exercises `NotReady`, so the spec does not say whether that clause was meant to be asserted; (4) `find_err02` scans `src/core/*.h` only and records the resulting hole as a `belay-debt:` comment, where §Plan step 1 required R-ERR-01's equivalent narrowing to be recorded in the rule's own text — the spec does not say whether R-ERR-02 owed the same.
- closure test: fail: `tests/vectors/README.md` is in the phase's file set and reachable from neither §Context pointers nor §Plan (notes.md §Deviations already reports it, which is itself a closure failure by the second-to-last bullet); the four `undecidable` findings are four more missing pointers
- upstream: none — no path in `.claude/workflow/installed` appears in the phase's file set
- verdict: returned to implementation

## Validation — 2026-09-11 (round 2)
- criteria: 18 passed / 0 failed (two new: `ls docs/adr/0012-*.md`, and the vector count now 9)
- project gates: test pass, lint pass, typecheck gap — same `workflow gap:` line as round 1, verbatim in that record
- boundary sweep: clean (22 source files; 13 active `deny` rules, `src/core/` is a declared layer)
- independent review: contradicts: `verify.md` §3 embeds a six-line verbatim `FAIL` transcript, which §Plan step 11's "State properties of a run, not transcripts of it" forbids — while the same step demands "the exact commands to run and what each should print". The spec contradicts itself and the reviewer could only report the half that is unambiguous. undecidable: (1) whether `map_frame` gates **buttons** on the controller id, not just the whammy — §Plan step 5 constrains the whammy alone; (2) whether `verify.md` may state counts in prose, given §Context pointers inherits "the rule that a count in prose has no check behind it" without stating it; (3) what `make test`'s post-phase runtime is, which the spec never records (fixed in `verify.md`, see §Deviations); (4) whether §Plan step 4's compiler-flag list is a floor or the exact set — the driver adds `-Og -g -UNDEBUG`.
- closure test: fail: finding (1) above is a missing pointer AND a live defect — see the entry below
- upstream: none — no path in `.claude/workflow/installed` is in the phase's file set
- verdict: returned to implementation

### The defect round 2 found, measured not argued

`map_frame` reads the two button bytes unconditionally. `config_mode.h`'s payload is six
`0x00` bytes, and buttons are active low, so **a config-mode frame maps to every one of the
ten controls pressed.** Compiled and run against the real `src/core/`:

```
frets pressed: 5/5
strum_up=1 strum_down=1 start=1 select=1 tilt=1 whammy=0x80
```

Only the whammy is right. Four places assert the opposite of the other ten controls:
`src/core/guitar_state.h`'s own `map_frame` comment ("including Config, which is a valid frame
that reports no controls at all"), `tests/vectors/config_mode.h` ("reading buttons or a whammy
out of it is what must not happen"), spec §Vectors ("a recognised id that is not a report"),
and `verify.md`'s table ("Recognised, but it is not a button report").

No case covers it: `case_config_mode_whammy_is_rest` asserts the whammy and stops. R-PROTO-04
is about the whammy, so its rule line is green and correct — the rule was never the thing that
would catch this.

**Not fixed here, and deliberately.** The fix is a cut, not a pointer: either `map_frame` gates
buttons on the id the way it already gates the whammy, or `Config` never reaches `map_frame`
because the caller checks `LinkState` first, or the three comments are wrong and a config
frame's buttons are simply meaningless data a later layer ignores. Those are three different
contracts for `07-analog-mode` to be written against. It is outside the five items this
iteration was authorised to change.

## Validation — 2026-09-11 (round 3)
- criteria: 22 passed / 0 failed — the 18 commands, plus the four liveness blocks. `time make test` → **2:11.56 real**, under the 3m cap. M1–M3 are automated inside the driver (§Plan step 8) and report `rejection cases: 3/3`; M4 was run by hand in a scratch copy — deleting `real_run()` makes `test_checks_are_live.py` exit 1 with `test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing but its own cases — the real run is gone`. The two backreference greps were run under `/usr/bin/grep`, as §Acceptance criteria's own note prescribes.
- project gates: test pass, lint pass, typecheck gap — `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean (34 files; 13 active `deny` rules, `src/core/` is a declared layer, so this is a real sweep). **Recorded because the first attempt was not a sweep at all:** `scripts/check.sh --files $FILES` under zsh passes ONE concatenated argument — zsh does not word-split unquoted expansions — and `gate_files`'s `[ -f "$abs" ] || continue` skips it in silence, so the vacuous run printed `check: all gates passed`. Re-run through `xargs`, then proved live adversarially: `#include "src/hal/bus_io.h"` added to `src/core/link.h` is caught as `deny core -> hal` with exit 2 (file restored byte-for-byte). A clean sweep and a sweep of nothing are the same output; the probe is what separates them.
- index: **STALE on entry** (`src/core/guitar_state.cpp`, `src/core/guitar_state.h`, `tests/ps2_codec_cases.cpp` changed after the last build), rebuilt with `scripts/build-index.sh`; fresh at stamp `24d489f`.
- independent review: contradicts: **none that survives** — the one returned (`notes.md` absent from the diff, against §Plan step 11) is an artifact of this command's own prescribed starvation; the reviewer flagged that caveat itself. See §Deviations round 5 and the `upstream:` line. undecidable: **6** —
  (1) **`FaultCause`'s membership, and that `last_fault` survives a good frame.** §Files names the enum, §Plan step 6 says only that `last_fault` carries the reason for trace mode. The code commits to five members and to seeding each step with the existing cause rather than `None`. Neither is mandated nor forbidden. The asymmetry is the point: step 3 required a whole ADR (0012) to fix `DecodeStatus`'s membership, and ADR-0012's own rule sends the *next* status to `FaultCause` — which the spec never scopes.
  (2) **`Link::us_in_state`'s accounting.** The member is not in the spec. It accumulates while the state is unchanged and resets to 0 on any change, which discards the `elapsed_us` of the step that *enters* `Negotiating`: the budget effectively starts at the second config frame. `case_link_negotiation_times_out` encodes that as correct. The spec says the timeout must exist and that the 100 ms number is `07-analog-mode`'s; it never says where the clock starts.
  (3) **The `LinkState` transition table is quantified but never enumerated.** §Plan step 6 claims "all four members and every transition between them" — a set the spec does not write down, so it cannot be computed from the spec. Concretely undecidable: `Config` → `Negotiating`, a mid-stream config frame pulling `DigitalStreaming` → `Negotiating`, and `NotReady` → `Absent` with `FaultCause::NotReady`.
  (4) **`Ps2Frame`'s fixed-width payload and its zero fill.** `std::array<std::uint8_t, kMaxPayloadLen>` with the unannounced tail zeroed, asserted by `case_digital_payload_is_zero_filled`. §Files names only `Ps2Frame`, `DecodeStatus` and `decode`. This one is load-bearing: with `kWhammyIndex = 5`, a naive analog read of a 2-byte digital payload never touches either byte §Vectors describes as chosen to trap it — **it reads the zero fill.** Verified during this validation rather than argued: the fill is `0x00`, `kWhammyRest` is `0x80`, so the trap does hold and M2 is live. What the spec never states is the mechanism its own §Vectors row credits to the wrong bytes.
  (5) **`kWhammyRest = 0x80`, and whether a centred analog stick equals rest.** §Vectors asserts "at rest" and "full deflection" and fixes no value. `case_analog_idle_whammy_is_rest` asserts `state.whammy == kWhammyRest` on a byte read *out of the payload*; it passes because the codec's rest value and `analog_idle.h`'s centred literal are the same byte. The spec neither mandates that coincidence nor forbids it. Related: `kWhammyRest` carries a `TODO(09-guitar-observe)` while §Goal's list of what gets a marker names only the whammy index, the fret bit positions and the controller id.
  (6) **`.clang-tidy`'s `HeaderFilterRegex` is narrowed wider than asked.** §Plan step 10 says narrow it "so the vector headers are not diagnosed"; the change is `(src|tests)/.*` → `src/.*`, which drops *every* header under `tests/`. The diff defends it with a claim about the tree ("the vectors are the only headers under `tests/`") that the spec never states and no criterion checks.
- closure test: **fail: seven missing pointers** — the six above plus one the reviewer cannot see by construction: §Context pointers line 114 names this phase as owner of `00-scaffold`'s `.py`-check debt, no step closes it, no criterion covers it, §Out of scope never releases it, and §Debt records that it grew. Everything else in the closure test passes: all four notes sections present and non-empty, every file in the 34-file set reachable from §Context pointers or §Files this phase writes (the four workflow-owned paths exempt), and no stray untracked path after repeated `make test` runs.
- upstream: `.claude/commands/validate-phase.md` — **/belay-feedback recommended.** Step 5 orders the reviewer's diff "restricted to step 3's file set" and forbids `notes.md` as an input in the same breath; the file set always contains `notes.md`. Any spec whose Plan says it writes notes — which the shipped `docs/templates/spec.md` requires — returns a `contradicts` no code change can clear. `docs/templates/spec.md` line 19's "properties, never transcripts", raised in round 3, is still open.
- verdict: **escaped to /expand-phase: spec re-expanded**

### Taste from validation 2026-09-14, recorded and not fixed (step 5)

- `src/core/guitar_state.h`'s `GuitarState` comment says "`frets` is indexed by
  `ControllerFret`"; the member is `is_fret_pressed` and the enum is `Fret`. Stale on two
  identifiers.
- `src/core/link.cpp`'s `state_for` falls through to `return LinkState::Absent` for an id
  outside its three-member switch — an unreachable drop that records no `FaultCause`, where
  §The link frames the enum as "one per way the link can drop".
- `src/core/hid_report.h`'s `kButtonCount = 10` is a written literal with nothing tying it to
  `guitar_state.h`'s `kFretCount` plus the five named bools; adding a control in one file
  cannot fail the other.
- `tests/ps2_codec_cases.cpp` aggregates verdicts as `is_ok = case_x() && is_ok;` once per
  case in `main`; a table of function pointers would make an omitted case visible, though the
  current form is deliberate about short-circuiting.

## Validation — 2026-09-14 (round 1 against the re-expanded spec)

Iteration 1: the count resets at the `escaped to /expand-phase` verdict of 2026-09-11, whose
output is the spec this round judges.

- criteria: **19 passed / 1 failed.** The nineteen commands all pass — `make test` **OK** in
  **2:06** against the 3m cap, `make lint` 0, `planned: 01-ps2-codec` 0, `planned: 03-pio-bus`
  4, 9 vectors, 0 stray `tests/` headers (the criterion §Plan step 11 added), 0 / 0 / 3 for the
  three greps, both ADRs present, driver exit 0, accounting 3 and 4 rule(s), `wiring cases:
  10/10`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0. The two
  backreference greps were run under `/usr/bin/grep`.
  **The failure is M4's instruction, not the code.** §Acceptance criteria said to run it in a
  `git worktree` rather than a `cp -a`. Run as written: `git worktree add` checks out HEAD, and
  this phase's work is uncommitted, so the copy contained neither `src/core/` nor
  `tests/test_ps2_codec.py` and the mutation had no anchor to move. Measured, not reasoned.
  The premise was then checked before rewriting: `tests/test_checks_are_live.py` runs **every**
  `test_*` file on the tree it is handed, `tests/test_secrets.sh` included, so the copy does
  need `.git` — and `cp -a` carries `.git` and the uncommitted work both. Criterion amended to
  say `cp -a` and to state both halves. **Reconciled in the same edit:** `notes.md` §For later
  phases, which carried the same advice the other way round ("Use `git worktree`") and is the
  section `/expand-phase` reads first. With `cp -a`, M4 produces exactly `FAIL:
  test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing but its
  own cases — the real run is gone`, exit 1. M1-M3 are the driver's rejection cases, `3/3`.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (34 files, 9 of them under `src/core/`; 13 active `deny` rules, so
  the rules are live and the set is covered). Passed one path per argument through `xargs`, per
  `spec.md` §Notes to `/validate-phase` note 2, and proved live before being recorded:
  `#include "src/hal/bus_io.h"` in `src/core/ps2_frame.h` is caught as `deny core -> hal`; the
  file was restored and `diff` reports it identical to the backup.
- index: **STALE on entry** (`tests/ps2_codec_cases.cpp` changed after the last build), rebuilt;
  fresh at stamp `24d489f`.
- independent review: **contradicts: 1** — §The frame mandates that `Ps2Frame` store the payload
  "alongside the announced length"; `src/core/ps2_frame.h` stores `id` and `payload` only and
  derives the length from `payload_len( id )` on purpose, and two test hunks depend on that
  choice. Verified against the file, not taken on the reviewer's word. **undecidable: 6** —
  (1) how many `TODO(09-guitar-observe)` markers the tree must carry: §Goal names four concerns,
  the tree has three markers, the criterion says "1 or more", `verify.md:165` says "Expect three
  of them", and concern two spans eleven `constexpr`s rather than one;
  (2) `us_in_state`'s overflow behaviour — `add_saturating` commits to saturating at
  `UINT32_MAX` with a 71.6-minute rationale, which §The link neither mandates nor forbids, and
  no case exercises it;
  (3) what `analog_idle` must assert about the whammy — §Vectors' row says "the byte at
  `kWhammyIndex`", the case asserts `== kWhammyRest`, the two are numerically identical and are
  different claims, and the paragraph below the table supports the weaker one;
  (4) `kPadByte` in `src/core/ps2_protocol.h`, absent from §Files' enumeration of that header's
  constants and referenced by nothing, with the spec never saying whether that list is
  exhaustive;
  (5) what §Plan step 1 does to `PHASES.md` — the diff edits the `01` row's acceptance text in
  place while `CLAUDE.md` says a cut is superseded by new rows, never edited;
  (6) `tests/test_style.sh`'s new `tidy_sysroot_flag()` degrading to a `note:` and an `ok:`
  verdict when `xcrun` names no SDK, where §Plan step 2 says two flags and §Acceptance requires
  exit 0. A seventh of the same class was found by this gate rather than the reviewer: §Plan
  step 7's "the four `hid:` cases" is a count whose set the spec never names.
  The four taste items are in §For later phases, unfixed, per step 5.
- closure test: **fail** — six `undecidable` findings are six missing pointers, plus the one
  above. Everything else in it passes: the four `notes.md` sections are present and non-empty,
  every file in the set is reachable from §Context pointers, §Files this phase writes or §Plan,
  and `git status -uall` shows no untracked path.
- upstream: **none** — no path in `.claude/workflow/installed` appears in the phase's file set.
  Note that the two package defects this spec works around (§Notes to `/validate-phase`) both
  behaved as documented this round: naming `notes.md` as held out of the diff produced no false
  `contradicts`, and the `xargs` form produced a real sweep. `/belay-feedback` is still owed.
- verdict: **returned to implementation**
