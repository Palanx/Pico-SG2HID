# Phase 01-ps2-codec — notes

## Outcome

- base: `24d489f` — **corrected 2026-09-15, and the correction is load-bearing.** This line
  read `working tree` through rounds 1-8, which was true while the phase had no branch. Round 9
  put the work on `feat/01-ps2-codec` and committed it, so the working tree went empty and
  `working tree` would have handed `/validate-phase` a file set of nothing: the boundary sweep,
  the independent review and the closure test would all have reported `pass` having examined
  zero files. `24d489f` is `chore(belay): update workflow package to f001884`, the commit the
  branch came off and the last one before any of this phase's work. `git diff 24d489f` against
  the working tree is the phase's diff — 34 files when this line was corrected, 36 as of
  2026-09-16 — and no path in `.claude/workflow/installed` falls inside it.

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

Tests: **ten** hand-written vector headers under `tests/vectors/` (plus a `README.md` there
explaining why they are hand-written), `tests/ps2_codec_cases.cpp` with **33** case functions
and 3 rule functions (26 + 3 through round 4; round 7 added the link-uniformity case, round 11
the refusal-precedence case, round 28 the cut-before-the-ready-slot case and four `link:` cases), and `tests/test_ps2_codec.py` as the single driver — it compiles the cases
against the real `src/core/`, runs them, forwards their lines, and carries three mutation
rejection cases.

Checks extended: `tests/test_repo_shape.sh` grew `find_err01`, `find_err02` and a
`core_headers` lister (eight checks → ten, wiring 8/8 → 10/10, accept floor 13 → 20, then 23 in round 30);
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

New decision records: `docs/adr/0011-pure-link-step.md` and
`docs/adr/0012-decode-status-carries-decode-outcomes-only.md`. Operator document:
`docs/phases/01-ps2-codec/verify.md`.

Acceptance, measured: `make test` **OK**, `real` between **1:40 and 2:18** across this
session's runs and **2:11** in round 4 (cap 3m; it was 1:16 on this tree before the phase,
and the phase adds C++ compiles plus a third mutation-running check — the spread is the
thread pool, not drift). `make lint` **0**, `planned: 01-ps2-codec` **0**,
`planned: 03-pio-bus` **4**, **10** vector headers, 0 hits for `tests/vectors` under `src/`,
0 hits for `.value()`, 3 `TODO(09-guitar-observe)` markers, one ADR-0011 file, one ADR-0012
file, driver exit 0 with **37** `ok:` lines (30 through round 4, 31 through round 7, 32 through round 27), `accounting: test_ps2_codec.py 3 rule(s)`,
`accounting: test_style.sh 4 rule(s)`, `wiring cases: 10/10`, `rejection cases: 64`,
`false-positive cases: 20 (floor 20)`, `neutered: 33/33`,
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


### Round 9 — a branch, the contradicts resolved against the spec, and eight pointers.

- **The phase has a branch and a rollback point, four rounds late.** `feat/01-ps2-codec` off
  `main` at `24d489f`, with everything to date in one commit, `b23b91a`, made **before** any of
  this round's edits. Its message says what is open rather than what is finished: the
  `contradicts`, the pointers and the released `.py`-check debt are all named in it, because a
  commit that reads as a success over code with a known open finding is how a later round comes
  to believe this one closed. `00-scaffold` had `chore/00-scaffold`; this phase had been running
  on `main` with 34 uncommitted files. One commit per round from here.

- **The `contradicts` was resolved by deleting the spec's sentence, not by adding a member to
  `Ps2Frame`.** §The frame claimed the struct stores the payload "alongside the announced
  length". It does not, deliberately: `payload_len( id )` is where the length comes from, so a
  stored length would be a second source of truth for one fact whose only possible contribution
  is disagreeing with the id. The sentence was written during the 2026-09-14 re-expansion by an
  author describing what he believed the code did — the spec is the side that moved, and the
  remedy follows the measurement rather than the routing. **Belay routes every `contradicts` to
  `/implement-phase` on the assumption that the code is wrong; the reviewer can only see that
  the two disagree, never which side moved.** That is filed as a package defect, and obeying it
  here would have added a redundant member to satisfy a sentence nobody decided.
  §The frame now says the struct stores no length and names the two hunks that recompute —
  `payload_matches` looping to `payload_len( frame.id )` and the zero-fill case starting at
  `payload_len( ControllerId::Digital )` — because both read as redundant until you know there
  is no member to read.
  **Enumerated before fixing (command step 5): every statement of the stored-length fact.**
  `grep` over the spec, `verify.md`, `src/core/` and the tests finds exactly one assertion of a
  *stored* length — the spec sentence, now gone. Everything else states that the *header
  announces* the length, which is true and untouched: `verify.md:49` and `:77`,
  `ps2_protocol.h:71` and `:93`, `ps2_frame.h:43`, and four comments in the cases file.
  `ps2_frame.h:48` already argued the case against storing it and needed no edit — it was right
  before the spec was.
  **The same property elsewhere, checked:** `hid_report.h`'s `kButtonCount = 10` is the one
  other place a fact has two independent sources (it is not derived from `kFretCount` plus the
  five named bools). It was raised as taste by the reviewer, stays taste, and is left in §For
  later phases — fixing it is a code change nobody asked for, and the point of enumerating was
  to learn whether the spec defect was an instance of something wider. It was not; it was
  singular.

- **The seven pointers the reviewer named, and the eighth this gate found.** Two were counts
  with no enumerated set, the class this phase has now paid four rounds for:
  - **The `TODO(09-guitar-observe)` markers: measured, then made to agree in three places.**
    The tree carries **three** markers covering **four** concerns — `ps2_protocol.h` over the id
    bytes, `guitar_state.h` over the block of button positions and masks, and a third over the
    whammy's index and rest value together. §Goal now states that mapping and deliberately
    prints no count of `constexpr`s (the mask block has twelve; the old text claimed each
    concern was "one named `constexpr`"). The criterion moved from "1 or more" to **3**, which
    is what `verify.md` §1 already told the operator to expect. **Reconciled in the same edit:**
    §Goal, the criterion, and `verify.md:165` — checked, already correct, untouched.
    **One code change came out of it:** `guitar_state.h`'s whammy marker said "and so are the
    two values below", which reads as `kWhammyRest` **and `kFretCount`** — and five frets is
    ADR-0003's control set, not a guess about this guitar. The marker now names `kWhammyRest`
    and says `kFretCount` is outside it. Round 2 recorded this as taste; it stopped being taste
    the moment the marker's scope became load-bearing for a pinned count.
  - **The accept floor is pinned with a criterion**, the way the wiring count already was:
    `sh tests/test_repo_shape.sh | grep 'false-positive cases'` → `20 (floor 20)`. The
    enumeration behind it is the `accept` block in that file.
  - **§Plan step 7's "the four `hid:` cases" became a property** — "every `hid:` case is `ok:`".
    Writing a fourth enumeration would have created a set to maintain in order to close a
    finding about unmaintained sets.
  The other five are direct pointers, all in `spec.md`: `us_in_state` saturates rather than
  wraps, and why; which assertion `analog_idle`'s case writes and why it is the weaker and more
  honest one; `kPadByte` added to §Files' list of `ps2_protocol.h` constants, with that list
  declared **exhaustive** so the next constant is a finding rather than a detail; what §Plan
  step 1 does to `PHASES.md` and why an in-place acceptance-text edit is not the case
  `CLAUDE.md` reserves for a superseding row; and that `tests/test_style.sh` omits `-isysroot`
  with a `note:` when `xcrun` names no SDK, which cannot hide a violation because a wrongly
  omitted flag fails inside libc++ as `FAIL: R-STYLE-02` — measured in round 1 of this phase.

- **The round-8 validation record said six `undecidable`; the reviewer returned seven.** The
  accept floor was described in §Deviations round 8 but never numbered in the record, and the
  miscount was repeated to the operator, who planned this round from it. Corrected in place in
  that record, with the correction visible. The number of findings is itself a count with a set
  behind it, which is the joke this phase has now told on itself twice.


### Round 10 — what validation 2026-09-15 found. The fix from round 9 introduced the finding.

- **The `contradicts` is round 9's own repair, and it is the pattern `00-scaffold` warned
  about by name.** Closing the `kPadByte` pointer, round 9 added to §Files' `ps2_protocol.h`
  row: "**This list is exhaustive:** a constant in that header and not in this row is a
  finding, not a detail." The claim was never checked against the header. Measured now:
  **the header declares twenty `constexpr`s and the row names eleven.** The nine missing are
  `kIdDigital`, `kIdAnalog`, `kIdConfig`, and the whole frame-geometry block `kHeaderIndex`,
  `kReadyIndex`, `kPayloadIndex`, `kPrefixLen`, `kPayloadNibbleMask`, `kBytesPerNibble` — not
  incidental ones: `kPrefixLen`, `kHeaderIndex` and `kReadyIndex` carry `ps2_frame.cpp`, and
  `kPayloadIndex` is used throughout the cases file. By the sentence's own terms that is a
  finding, so the sentence convicts itself.
  This is `docs/phases/00-scaffold/notes.md` line 3256, second bullet, verbatim: *"a fix
  written to close a finding tends to introduce a new claim with no founding … after writing a
  fix, read it as a reviewer who has never seen the finding, and ask what founds each new
  sentence. If the answer is 'the fix I just wrote', delete the sentence rather than founding
  it."* That rule is cited in this spec's own §Context pointers. It was read, quoted, and then
  not applied to the fix being written in the same edit.
  **Not fixed here.** Two ways out and they are different contracts: name all twenty constants
  in the row, or delete the exhaustiveness sentence and let the row stay illustrative. The
  first is a set that must be maintained against the header forever — the thing this phase has
  spent four rounds learning to stop writing. The second gives up the property the sentence was
  added to buy. That is an operator decision, not a gate's.

- **Three `undecidable`, each verified against the file rather than taken on the reviewer's
  word.** Listed in the validation record below; two are real gaps in what `decode` promises:
  - an **over-long buffer is accepted**: `ps2_frame.cpp` refuses only `bytes.size() <
    expected_len`, so trailing bytes past `frame_len( id )` decode normally and are ignored.
    §Goal's "refusing by default" does not say whether an over-long response is well-formed
    input — and it plausibly is, since `hal` handing over a fixed-size shift buffer produces
    exactly that. No vector exercises it.
  - the **precedence of two §Goal bullets is unstated**: `decode` checks the ready byte before
    the announced length, so a frame that is both cut short *and* carries a wrong ready byte
    reports `NotReady`, and the link records `FaultCause::NotReady` rather than
    `AckTimeout`. Both §Goal bullets are written unconditionally and the overlap is real on a
    bus — `not_ready.h` describes an undriven `DATA` line, `truncated_ack.h` describes the
    bytes stopping, and a controller that does both is covered by neither. The distinction is
    what trace mode prints.
  - the driver's `CXXFLAGS` duplicates the Makefile's by hand and the spec never states them.


### Taste from validation 2026-09-15, recorded and not fixed (step 5)

- Several `map_frame` cases in `tests/ps2_codec_cases.cpp` evaluate `ps2::map_frame( *frame )`
  on the line above the `frame.has_value()` term in `is_ok`, so the guard is checked after the
  dereference and a regression in `decode` turns the case into UB rather than a clean `FAIL:`
  line. Raised in a different form by round 3's reviewer too; still taste, still unfixed.
- `case_report_gives_every_button_its_own_bit` declares a local `const ps2::HidReport report`
  that shadows the file's `report()` helper four lines away.
- `state_for` in `src/core/link.cpp` ends with an unreachable `return LinkState::Absent;` after
  a switch covering every enumerator — deliberate and commented, but `-Wswitch` already
  protects it.

### Round 11 — the exhaustiveness sentence deleted, two decode contracts decided, one code change.

- **The `contradicts` is gone by deletion, which is what the rule it broke prescribes.**
  §Files' `ps2_protocol.h` row no longer claims to be exhaustive; it says it is illustrative
  and that the header is where the full set lives. The property the sentence bought — "no
  constant escapes the row" — was held up by nothing: it is an assertion with no mechanism,
  the same class as everything else this phase has spent rounds deleting. Enumerating all
  twenty constants would have bought the property back at the price of a hand-maintained set,
  which is the thing four rounds of findings taught this phase not to write.
  **The third way out is named and deliberately not taken:** make the property executable — a
  check that greps `^constexpr` out of the header and compares it with the row. That is a real
  check and it would work. It is also new work in a phase that is closing, and a check added
  during a closing round is untested machinery arriving at the moment nobody has budget to
  test it. Recorded in §For later phases for whoever next touches the check harness.
  **Applied to this edit too, which is the whole point:** the replacement row asserts only
  what the header itself shows, and the one new sentence it carries ("a reader who needs the
  full set reads the header") is founded on the header existing, not on the fix.

- **Over-long buffers: accepted, and now decided rather than emergent.** `decode` refuses only
  `bytes.size() < expected_len`, so trailing bytes past `frame_len( id )` are ignored. §Goal
  now states that as the contract with its reason: the header is the only authority on frame
  length, so the span is a capacity and never a claim. The alternative — refusing — would put
  `2 * (header & 0x0F)` on both sides of the `hal`/`core` boundary, because `03-pio-bus` hands
  over a fixed-size shift buffer and would have to trim it first. **No code change**: the code
  already did the right thing for a reason nobody had written down. Nothing asserts it; see
  §Debt.

- **Refusal precedence: written as a chosen order, and the code moved to match.** This one was
  a real defect, not a missing pointer. `decode` checked the ready byte before the announced
  length, so a frame that was both cut short and carrying `0xFF` at the ready slot reported
  `NotReady` — and R-PROTO-02's own text says a frame the bus cut short "reports the abort".
  The rule was being violated by the order of two `if`s.
  §Goal now carries the precedence as a four-step list, each refusal taken at the first point
  it is decidable. **The one ordering that is forced rather than chosen is stated as such:**
  `UnknownId` cannot move below the length check, because `frame_len` needs the id.
  `src/core/ps2_frame.cpp` reordered, `tests/vectors/truncated_not_ready.h` added as the tenth
  vector, and `case_truncated_and_not_ready_reports_the_abort` added.
  **Proved live, and the probe found something worth recording:** putting the old order back
  in a scratch copy flips exactly that one case to `FAIL:` and **leaves all three rule lines
  `ok:`** — R-PROTO-02's own rule case exercises the two refusals only apart, so it is
  structurally blind to the order they are written in. The third time this phase has measured
  that a rule line stays green while the rule is wrong.
  **Generalised before fixing (command step 5): every pair of refusals that can be true at
  once.** Three pairs exist. *Short + no prefix* → `AckTimeout`, the only thing knowable.
  *Short + unknown id* → `UnknownId`, and that order is forced, not chosen. *Short + bad ready
  byte* → was `NotReady`, now `AckTimeout`, the instance fixed here. The property is
  "the earliest decidable refusal wins, and the abort outranks anything still evaluable after
  it"; it holds in all three after this change.
  **Reconciled in the same edit — every statement of the vector count, following the list
  round 2 wrote when it went 8 → 9:** spec §Goal's opening line, §Vectors' lead line and table,
  §Files' `tests/vectors/` row, §Plan step 4 and its check, the acceptance criterion;
  `verify.md`'s section heading, its "there are nine small files", its "The nine:" table
  lead-in, the table itself (a row added), its "last three are the ones that matter most"
  (now four), and its §4 `Expect \`9\`` (now `10`); and in this file §Outcome's measured list
  and §For later phases' note to `05-emulator`. **Checked and deliberately left alone:** the
  `9 vectors` inside the round-4 Deviations entry and the two earlier validation records —
  those are dated accounts of what was true when they were written.
  **Re-measured, not incremented:** 10 vectors, 28 case functions, 32 `ok:` lines.

- **The `CXXFLAGS` duplicate is now stated where a reader will hit it.** §Context pointers'
  `Makefile` line carries the flag list and says the driver repeats it by hand because it
  compiles the cases itself rather than through `make`. Measured 2026-09-15: the two lists are
  byte-for-byte identical. Written as a duplicate rather than as a fact, because the risk is
  drift and a reader who does not know it is a copy cannot watch for it.

- **The `- base:` line, and why the old value was right when it was written.** Through rounds
  1-8 the phase had no branch and nothing was committed, so `working tree` was not a
  placeholder — it was the accurate answer, and `/validate-phase` used it to build a 34-file
  set every round. Round 9 put the work on `feat/01-ps2-codec` and committed it, and the line
  became false in a way that fails silently: the working tree went empty, so the file set would
  have been empty, and the boundary sweep, the independent review and the closure test would
  each have reported `pass` having examined zero files. Corrected to `24d489f` on 2026-09-15
  before any gate ran. **The trap is that nothing in the loop notices**: the line is written by
  `/implement-phase --implemented`, read by `/validate-phase`, and invalidated by an ordinary
  `git commit` that neither command is involved in. Filed as a package defect, and §Notes to
  `/validate-phase` note 3 in `spec.md` is the local workaround — check the `- base:` line names
  a real ref and the file set is non-empty, before trusting any gate.


### Round 12 — the blind rule line, and the second one that turned out not to be blind.

The Goal this phase inherits from `00-scaffold` is that mutating the codec so it stops
refusing "makes the check fail **and name the rule**". Round 11 measured that false for
R-PROTO-02 and wrote it down. This round fixes it. No step of the Plan named the work — the
Plan enumerates the work, the Goal defines what finished means.

- **Generalised first, over all three rule lines, and the answers differ.** The shape being
  hunted: a rule line that exercises the refusals *separately* when the rule is about their
  interaction, so the line stays green while the rule is broken.
  - **R-PROTO-02 — an instance, fixed.** The rule says a cut-short frame "reports the abort",
    with no exception for a frame that is also wrong some other way. `rule_proto02` decoded
    `truncated_ack` only, which is truncation with a *good* ready byte: the one configuration
    where nothing competes with the abort. It now also decodes `truncated_not_ready` and
    asserts `AckTimeout` and `Absent` there. **Verified by mutation, on the rule line and not
    the case:** restoring the old order of the two `if`s makes
    `FAIL: R-PROTO-02 (a cut-short frame yields no frame and the link goes Absent)` print.
    Re-confirmed against the final tree after the revert below, so the probe describes what
    shipped and not an intermediate state.
  - **R-PROTO-04 — looked like an instance, measured, and was NOT.** "The whammy is read only
    from a frame that reported analog mode" quantifies over every non-analog id, and there are
    two, `Digital` and `Config`; the line tested `Digital` alone. A `Config` clause was written
    and probed with a `id != ControllerId::Digital` gate — and the line **stayed green**,
    correctly. `map_frame` returns early when `reports_controls( id )` is false, so only
    `Digital` and `Analog` ever reach the whammy gate, which makes `!= Digital` an
    **equivalent mutant** rather than a violation. No single mutation can flip a Config clause,
    so the clause would have been an assertion nothing could falsify — green paint, the exact
    thing this round exists to remove. **Reverted**, and the measurement is recorded in the
    rule function itself so the next reader does not re-add it. `case_config_mode_whammy_is_rest`
    already covers the composite behaviour as a case, which is the right place for it.
  - **R-PROTO-03 — not an instance.** Its text is about never decoding an undeclared header on
    a best-effort basis, not about interaction, and under §Goal's precedence every overlap
    involving an undeclared header still reports `UnknownId`. The sub-prefix case (a buffer
    shorter than a header and a ready slot) reports `AckTimeout`, and §Goal step 1 already
    decides that explicitly, so there is no undecided overlap for the line to be blind to.

- **The probe that failed is the useful half of this round.** Writing the R-PROTO-04 clause,
  running it, and finding it unfalsifiable is `00-scaffold`'s rule applied to a test instead of
  a sentence: *after writing a fix, ask what founds it; if the answer is the fix itself, delete
  it.* A clause that cannot go red founds nothing. It would have passed every future validation
  and read, to a later session, as coverage.

- **Reconciled in the same edit.** `spec.md` §Plan step 12's check said reverting the order
  turns the case "and no rule line" to `FAIL:` — true when written, false the moment this round
  landed. It now says both, and carries why the rule line is part of the check. §Plan step 13
  is the widening itself. **Checked and left alone:** §Goal's liveness sentence, which is what
  the fix restores rather than changes, and round 11's Deviations entry above, which is a dated
  account of what was true then and is superseded by this one rather than edited.

- **Not touched, deliberately:** the two mandated behaviours nothing asserts — `us_in_state`'s
  saturation and the acceptance of over-long buffers. They stay in §Debt with owners. The Goal
  restored here is that every *rule* has a case that bites, not that every contract does; if
  the reviewer wants them as pointers, that is its call to make.


### Round 13 — the founding audit, and one sentence it deleted.

The round-12 fix was re-verified from scratch rather than re-read: the cut applied in a scratch
copy, the whole rule block printed, and **`FAIL: R-PROTO-02 (a cut-short frame yields no frame
and the link goes Absent)`** produced with the driver exiting non-zero. R-PROTO-03 and
R-PROTO-04 stay `ok:` under that cut, which is correct — neither is about the refusal order.
Nothing needed fixing; the audit below is what this round is.

- **Every sentence added to `spec.md` in rounds 11 and 12 was read back against the question
  "what founds this?", and one failed.** The over-long-buffer paragraph asserted
  *"`03-pio-bus` will hand `core` a fixed-size shift buffer"* and then, resting entirely on it,
  *"Refusing an over-long buffer would also refuse the ordinary case on real hardware."*
  **Measured:** `grep` over `docs/`, `src/` and `tests/` finds no statement about the shape of
  `hal`'s buffer that predates this session — no ADR, no dependency note, nothing in
  `00-scaffold`. It was a prediction about a phase nobody has written, and the second sentence
  was founded on nothing but the first. Both deleted. The decision stands on the two arguments
  that were already founded: the header is the sole authority on frame length, and trimming in
  the caller would put `2 * (header & 0x0F)` on both sides of the layer boundary. The
  replacement says what is actually known — that the caller's shape is unrecorded, and that
  this contract is the one that does not need it, because a caller that trims and one that
  over-delivers decode identically.
  **Reconciled in the same edit:** §Debt's owner line for that item, which justified
  `03-pio-bus` by the same unfounded prediction and now justifies it by the thing that is true
  — it is the first phase with a real caller. **Checked and left alone:** the same prediction
  inside the round-10 and round-11 Deviations entries, which are dated accounts of the
  reasoning as it stood; this entry supersedes them rather than editing them.
  Every other added sentence traced to something outside its own fix: the precedence list to
  the code and to R-PROTO-02's text, the `0xFF`-is-an-undriven-line claim to the open-drain
  fact already recorded in `not_ready.h`, the `CXXFLAGS` line to a byte-for-byte comparison,
  the exhaustiveness deletion to the twenty-versus-eleven measurement, and steps 12 and 13 to
  the mutation probes.

- **The state handed to validation, verified rather than assumed, because this phase committed
  three more times since the last check.** `- base:` names `24d489f`, which `git cat-file -t`
  confirms is a real commit (`chore(belay): update workflow package to f001884`). The file set
  it produces against the working tree is **35 files**, up one from 34 with
  `tests/vectors/truncated_not_ready.h`. Not empty, so the sweep, the review and the closure
  test will each look at something.

- **Not touched, as instructed and for the reason given:** `us_in_state`'s saturation and the
  over-long acceptance. Both state the behaviour *and* state that nothing asserts it, so the
  claim is complete and there is no conflict for a reviewer to find; neither is bound to an
  `R-*` rule; both sit in §Debt with an owner.


### Round 14 — validation #3 failed on a criterion, and the escape fired.

- **The failing criterion, and it is not a typo.** `grep -rn 'tests/vectors' src/ | wc -l`
  returns **1**, against `expect: 0 (R-PROTO-05)`. The hit is
  `src/core/ps2_frame.cpp:22`, a comment added in round 11 that reads
  *"See tests/vectors/truncated_not_ready.h."* — written to point a reader at the vector that
  covers the refusal order.

- **`make test` is green while that criterion fails, and the reason is the rule's own check.**
  `find_proto05` calls `hits`, and `hits` runs `sed 's|//.*||'` over each file before grepping.
  **R-PROTO-05's bound check is structurally incapable of seeing a reference inside a
  comment.** The same file already contains the counterpart — `raw_hits`, which exists because
  R-CLEAN-05 is *about* comments and the stripping would delete the text it hunts. So the two
  helpers encode two different answers and R-PROTO-05 was given the stripping one.

- **The two sides disagree about what the rule forbids, and that is the finding.**
  `docs/constraints.md` R-PROTO-05 says "no file under `src/` may reference `tests/vectors/`".
  The acceptance criterion reads that literally: any occurrence, comment or not. The bound
  check reads it as "no *code* dependency", which is defensible — a comment cannot make `core`
  depend on test data, and the rule's purpose is independence of the test data from the code.
  Both readings are reasonable and the spec does not say which one R-PROTO-05 means. Until it
  does, `make test` and §Acceptance criteria can disagree on the same tree, which is what just
  happened. **This is the fourth time in this phase that a rule's own check has been green
  while something else said the rule was broken** — after the config-mode gate (round 2), the
  link-transition uniformity (round 7) and the refusal ordering (round 11).

- **Not fixed here, and the fix is not obvious, which is why the escape is right.** Three
  different repairs, three different meanings for the rule: delete the comment and keep both
  readings apart; give `find_proto05` `raw_hits` so the check matches the criterion; or amend
  R-PROTO-05 and the criterion to say "no `#include` and no code reference", which makes the
  comment legal and the criterion wrong. Choosing is a spec decision.

- **The escape fired and this command moved the status.** Three `## Validation` sections now
  follow the 2026-09-11 `escaped to /expand-phase` verdict, and this third one failed a gate.
  `docs/phases/PHASES.md` row `01-ps2-codec` set to `pending` by `/validate-phase`, which is
  the one time it moves a phase backwards. The next command is `/expand-phase 01-ps2-codec`.


### Round 15 — the re-expansion. One decision, one new section, one audit.

- **`spec.md` was amended, not rewritten from the template, and that is a deviation from
  `/expand-phase` step 5.** Stated rather than hidden: the spec already follows the template's
  sections, and six rounds of reconciliation are embedded in its counts, its enumerations and
  its precedence list. A wholesale rewrite would have re-derived text that took four validation
  rounds to get right, on the chance of reintroducing what they removed. The substantive
  changes are the ones the escape asked for: a new §What a `test:` binding does and does not
  promise, §Plan step 14, its criterion, and the rejected reading in §Out of scope.

- **The decision: R-PROTO-05 means every occurrence, comments included.** §Plan step 14 carries
  it with its three edits. The rejected reading — "no *code* reference" — is in §Out of scope
  with its reason rather than dropped: it is semantically the more honest one, and it was
  rejected because it obliges someone to define what counts as a reference, which is new
  judgement surface on the exact axis this phase has failed on four times.

- **The audit the escape actually existed for, and what it found.** `tests/test_checks_are_live.py`
  proves a check is *wired* — it mutates the code and confirms the check reacts. It cannot
  prove the check's pattern covers what the rule's text says, and all four of this phase's
  "green check, broken rule" instances were live and narrower. There is no machine-checkable
  form of "this regex covers this English sentence", so the spec now states the binding as a
  convention with a named owner: **the phase that moves a rule from `planned:` to `test:`
  writes, in the rule's own text, what the check does not see.** That is not an invention —
  R-ARCH-01, R-ERR-01, R-ERR-02 and R-CLEAN-04 already do it, which is why it is written as a
  rule rather than proposed as one.
  **All 23 `test:`-bound rules were checked, and the full result is in the spec's table** so
  the next reader does not redo the search. Four gaps found that are undeclared and are **not**
  this phase's rules: R-STYLE-02 and R-CLEAN-02 (`tidy_sources()` covers `src/core/*` and
  `tests/*.cpp` only, while both texts are unscoped), R-PROC-02 (the check asserts a non-empty
  `verify.md` exists; the text demands it be written for a non-specialist with exact steps and
  readings, which nothing reads), and R-ERR-03 (the weakest: "anywhere under `src/`" against a
  comment-stripping scanner). Nine were checked and found sound, four already declare their
  narrowing, and the three R-PROTO rules were audited in round 12.

- **Reconciled in the same edit.** The new criterion pins `rejection cases: 64`, one above the
  63 measured today, because step 14 owes a case proving the new scope. **Checked and left
  alone:** §Acceptance criteria's `grep -rn 'tests/vectors' src/` line, which already expects
  `0` and becomes true rather than changing; `verify.md`, which never mentions R-PROTO-05's
  scanner; and `tests/vectors/README.md`, which states the provenance half of R-PROTO-05 and
  says nothing about `src/`.

## For later phases — added by the round-15 audit

- **Whoever owns `00-scaffold`'s bindings next** — four rules assert more than their checks
  measure and their texts do not say so: **R-STYLE-02**, **R-CLEAN-02** (both scoped by
  `tidy_sources()` to `src/core/*` and `tests/*.cpp`), **R-PROC-02** (existence versus "written
  for a non-specialist"), **R-ERR-03** ("anywhere under `src/`" versus comment stripping). Each
  needs a scope clause in its own text, per `spec.md` §What a `test:` binding does and does not
  promise. `01-ps2-codec` did not write them because it does not own those bindings and
  widening another phase's check during a closing round is new work wearing a fix's clothes.
- **Whoever revisits R-PROTO-05** — the "no code reference" reading is the semantically honest
  one and is rejected here only because defining "reference" is judgement surface this phase
  could not afford to add. Revisit it with budget to test the answer.


### Round 16 — §Plan step 14. Three edits, one meaning, and the case that holds it there.

- **`find_proto05` moved from `hits` to `raw_hits`.** The comment above it now states that the
  scanner choice *is* the rule's meaning rather than a detail: a comment naming a vector is a
  reference, so the check must read raw lines. `find_clean05` already used `raw_hits` for the
  mirror-image reason, which is why this is a correction and not an invention.

- **The path is out of `src/core/ps2_frame.cpp`.** The comment still explains why `0xFF` at the
  ready slot of a short frame is an undriven line rather than a controller statement — that
  reasoning is what a reader of `decode` needs. What it no longer does is name the file under
  `tests/` that covers the overlap: it says the vector lives with the other refusal vectors and
  that R-PROTO-05 is why it is not named. The pointer that was deleted pointed *out of* `core`,
  which is the direction the rule exists to forbid.

- **R-PROTO-05's text gained its scope clause**, which is what §What a `test:` binding does and
  does not promise requires of any binding this phase owns. It states the reading ("any
  occurrence of the path, comments included"), that the check reads raw lines to match, why the
  literal reading was chosen over "no code reference" (nothing left to interpret), and **what is
  given up**: a comment cannot create a dependency, so the check now refuses some references
  that are harmless, and that over-strictness is the price of having no judgement in the rule.

- **The rejection case that makes the scope real, and the probe that proves it.** A second
  R-PROTO-05 case was added whose bad tree's only reference is *inside a comment*. Without it
  the scanner could revert and the suite would not notice: measured, not assumed — putting
  `hits` back in a scratch copy produces
  `FAIL: R-PROTO-05 rejection case did not fire on: // see tests/vectors/digital.h for the bytes`,
  drops the count to 63 and exits non-zero. The count moving 63 → 64 is pinned by the criterion
  §Plan step 14 added.

- **Generalised, and the enumeration was already done rather than redone.** The re-expansion
  audited all 23 `test:`-bound rules and its table is in `spec.md`. What this round added is the
  narrow check that the scanner change introduces no new failure of its own: `grep -rnE 'tests/'
  src/` now returns nothing at all, so no other file was relying on the stripping. The two
  scanners are now assigned by meaning across the whole file — `raw_hits` for `find_clean05`
  (a rule about comments) and `find_proto05` (a rule about any occurrence), `hits` for
  `find_clean03`, `find_clean09`, `find_err01` and `find_err02`, whose subjects are code
  constructs a comment does not contain. R-ERR-03 is the one remaining candidate and it is not
  this phase's rule; §For later phases owns it.

- **Acceptance: all 21 criteria pass**, including the one that failed validation #3 —
  `grep -rn 'tests/vectors' src/ | wc -l` → **0** — and the new `rejection cases: 64`.
  `make test` **OK** in **1:38** against the 3m cap, `make lint` 0. M1-M3 are the driver's
  rejection cases (`3/3`); **M4 re-run by hand** in a `cp -a` copy and produced exit 1 with
  `FAIL: test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing but
  its own cases — the real run is gone`.


### Round 17 — stale markers, and the fifth instance the marker audit uncovered.

- **Premise corrected before doing the work: steps 10 and 11 were not owed.** Both landed in
  round 7 on 2026-09-14 — `case_link_target_depends_on_the_outcome_alone` is in the cases file
  and registered in `main`, and the stray-header criterion is in §Acceptance criteria. What was
  wrong was their marker, the same defect as step 14's: **a Plan that says it owes work already
  delivered is the spec failing to describe the change that happened**, which is what the
  closure test measures. Three markers corrected — 10 and 11 to `Landed 2026-09-14`, 14 to
  `Landed 2026-09-15`.

- **Audited in both directions, per step, against the artefact each names.** Nothing is marked
  `Landed` that is not: `planned: 01-ps2-codec` is 0 (step 1), `make lint` exits 0 (step 2),
  both ADRs exist (steps 3, 6), 10 vectors (step 4), the config-mode controls case exists
  (step 5), four `hid:` cases (step 7), three driver rejection cases (step 8),
  `wiring cases: 10/10` (step 9), the length check precedes the ready check at lines 26 and 33
  of `ps2_frame.cpp` (step 12), `rule_proto02` reads `kTruncatedNotReady` (step 13), and
  `find_proto05` uses `raw_hits` (step 14). The only drift was the three `Owed` markers.

- **Step 10's liveness probe, re-run against the rule lines rather than only its case, found a
  fifth instance of the phase's recurring defect.** Mutating `step` so the transition depends
  on the source state flips the uniformity case — and left `R-PROTO-02` **green**. R-PROTO-02's
  text says a cut-short frame's link "transitions to `Absent`" with no qualification of the
  source state, while every step in that rule line started from a fresh, therefore already
  `Absent`, link. So a `step` that sent a cut-short frame elsewhere from one source state broke
  the rule with its own line printing `ok:`. Same shape as round 12's fix, one axis over: that
  one was blind to a competing refusal, this one to where the link was standing.
  **Fixed:** `rule_proto02` now drives a link to `DigitalStreaming`, `AnalogStreaming` and
  `Negotiating` and asserts `Absent` from each; `Absent` as a source is already covered by the
  clauses above it, so the claim is asserted over all four members §The link enumerates.
  **Not the uniformity case restated** — step 10 asserts the four targets are *equal*, this
  asserts what they equal, and a mutation sending all four to the same wrong state is caught by
  the former and by the fresh-link clause, not by this one.
  **Proved live:** the same source-state mutation now turns **both** the uniformity case and
  the `R-PROTO-02` rule line to `FAIL:`. Recorded as §Plan step 15.

- **Step 11 verified in both directions rather than only forward**, which is what the operator
  asked and what the earlier rounds of this phase kept not doing: on the real tree
  `find tests -name '*.h' -not -path 'tests/vectors/*' | wc -l` → **0**; on a scratch copy with
  one `tests/helper.h` added → **1**, naming the file. A criterion that only ever passes is
  indistinguishable from one that cannot fail.

- **One date corrected in the same breath as it was written.** The new clause's comment was
  first dated 2026-09-16, a day ahead; caught on read-back and fixed to 2026-09-15 before the
  gates ran. Recorded because the phase's own standard is that a measurement carries the date
  it was taken, and a wrong date is a wrong measurement.

- **Counts re-measured, not incremented:** 28 case functions and 32 `ok:` lines — both
  unchanged, because this round widened an existing rule function rather than adding a case.


### Round 18 — five `undecidable`, no `contradicts`, and three of the five are mine from the re-expansion.

Every finding verified against the tree rather than taken on the reviewer's word.

- **1. `find_err02`'s return-type enumeration is narrower than R-ERR-02's text, and undeclared.**
  The check recognises three spellings — `std::expected<…>`, `DecodeOutcome`, `LinkState` — while
  the rule says "Every function returning a **result struct** or a `LinkState`", and the spec
  defines "result struct" nowhere. **Measured:** `src/core/ps2_protocol.h:80` declares
  `[[nodiscard]] constexpr std::optional<ControllerId> id_from_byte( std::uint8_t )`, a fallible
  return in a `src/core/*.h` that matches none of the three alternatives. No violation exists —
  the declaration already carries `[[nodiscard]]` — but the check could not catch it if it
  stopped. R-ERR-02's clause records the **file** scope (`src/core/*.h`) and says nothing about
  the **type** enumeration, so by this phase's own §What a `test:` binding does and does not
  promise it owes a second clause. This one is squarely this phase's: round 1 wrote that binding.

- **2. `grep -c 'planned: 03-pio-bus' … # expect: 4` lost its enumeration in the re-expansion.**
  The sibling criterion (`planned: 01-ps2-codec` → 0) has its seven rules enumerated in §Rule
  work; this one has nothing. The old spec carried the members inline and the 2026-09-15
  re-expansion shortened the line. **Measured — the four are R-SAFETY-07, R-PROTO-01,
  R-PROTO-06 and R-ERR-05.** A regression introduced while closing other findings, which is the
  pattern `00-scaffold` line 3256 names.

- **3. `rejection cases: 64` and `false-positive cases: 20 (floor 20)` are counts with no set.**
  Neither base total appears in the spec, so neither target is computable from spec + diff. The
  `64` is the worse of the two and it is mine: §Plan step 14 defines it as "one more than before
  step 14" — self-referential against a number the spec never states. The diff adds nine
  `reject` invocations, so a reader cannot even reconcile the delta of one without knowing the
  counter counts firings rather than lines. **This is the exact failure this phase has fixed
  four times elsewhere and I committed it while writing the fix for another one.**

- **4. `tests/vectors/unknown_id.h` is also cut short, and its comment says it is not.**
  **Measured:** header `0x79` announces `2 * (0x79 & 0x0F)` = 18 payload bytes, so a whole frame
  is 20 bytes; the vector carries 4. It is therefore truncated as well as undeclared, while its
  own comment claims "The rest of this frame IS well-formed, so nothing but the header can be
  the reason it is refused." The asserted outcome is unaffected — §Goal's precedence puts the id
  check above the length check, and that was true before round 11 as well — but the spec fixes
  only the header byte and the outcome for this vector and never its length. **The choice is
  real and is not a gate's to make:** lengthen the vector to the 20 bytes `0x79` announces, so
  the comment becomes true and the vector is undeclared-only; or keep 4 bytes and rewrite the
  comment to say the vector is doubly faulty and that §Goal's precedence is what makes the id
  the reason. The first is the one `not_ready.h` already models ("length and payload
  well-formed"); the second is cheaper and documents the precedence a second time.

- **5. §Plan step 10's check clause still says "and no rule line".** Step 15 made that false,
  and step 12's identical sentence was reconciled in round 12 while step 10's was missed —
  the same sentence, in the same file, one step apart. Both clauses now stand and a validator
  running step 10's check cannot tell which prediction to expect.

**Not fixed here.** Findings 1, 2, 3 and 5 are mechanical; 4 carries a choice. All five are
spec bugs, which the command routes to a `spec.md` amendment plus this entry rather than to a
code change.

### Taste from validation 2026-09-15 (round 4), recorded and not fixed

- Several cases dereference the `std::expected` before the `has_value()` guard is evaluated —
  `case_digital_idle_maps_to_nothing_pressed`, `case_analog_whammy_full`,
  `case_report_carries_the_whammy_end_to_end` and `rule_proto04`. Raised in three separate
  review rounds now. Latent today because no mutation makes `decode` refuse those vectors, but
  it is the hazard `src/core/ps2_frame.cpp`'s own comment argues against: a mutation whose
  observable effect is undefined behaviour proves nothing about the rule.
- `case_report_gives_every_button_its_own_bit` declares a local `const ps2::HidReport report`
  shadowing the file's `report()` reporter.
- `docs/constraints.md` §Style was rewritten while §Plan step 2 names only §Observed conventions
  for that file.


### Round 19 — the five amendments, and the check at writing time caught one in this very edit.

- **1. `planned: 03-pio-bus → 4` has its members back.** R-SAFETY-07, R-PROTO-01, R-PROTO-06,
  R-ERR-05 — measured off the catalogue, not remembered. The enumeration was inline in the
  pre-escape spec and I dropped it when shortening the line during the re-expansion.

- **2. The two counts are now derived rather than remembered, which is stronger than
  enumerating them.** `rejection cases` is one per `reject` line in `tests/test_repo_shape.sh`
  and `false-positive cases` one per `accept` line, so §Acceptance criteria carries the two
  greps that recompute them alongside the measured distribution — 31 R-ARCH-01, 8 R-ARCH-03,
  7 R-CLEAN-09, 4 each for R-ERR-01/02/03, 2 each for R-PROTO-05 and R-CLEAN-05, 1 each for
  R-ERR-04 and R-CLEAN-03; and 5 `find_arch01`, 4 each for `find_err02` and `find_clean03`,
  3 `find_err01`, 2 `find_arch03`, 1 each for `find_clean09` and `find_clean05`. A frozen list
  would have to be maintained by hand against the file, which is the trap this phase has been
  removing everywhere else; a derivation cannot rot. Step 14's self-referential definition
  ("one more than before step 14") is gone.
  **Also measured and worth naming: two different checks print the string `rejection cases`** —
  `tests/test_ps2_codec.py` prints `3/3` for the driver's mutations and
  `tests/test_repo_shape.sh` prints `64`. Both criteria scope by file (`sh <file> | grep`),
  so neither is ambiguous, but `make test 2>&1 | grep 'rejection cases'` would catch both.
  **Reconciled in the same edit:** `verify.md` was checked — its two mentions of
  `rejection cases` are the driver's `3/3`, a different counter, and needed no change.

- **3. §Plan step 10's prediction reconciled with step 15**, and **the third instance the
  operator asked about does not exist**: `grep -nE 'rule line'` over the spec returns step 10
  (stale, now fixed), step 12 (reconciled in round 12), step 13 and step 15 (both written
  after the widening and correct). Checked rather than assumed, and `verify.md` mentions rule
  lines nowhere.

- **4. R-ERR-02's second scope clause.** The binding already recorded its file scope; it now
  records the type scope too — the check recognises `std::expected<…>`, `DecodeOutcome` and
  `LinkState`, while the rule says "result struct", which is wider. The instance is named:
  `id_from_byte` returns `std::optional<ControllerId>` and matches none of the three. It
  carries `[[nodiscard]]` today so nothing is in violation, and the clause says exactly that —
  the check would not notice if it stopped. Closing it needs the type information a grep does
  not have, the same `clang-query` upgrade `03-pio-bus` already owes.

- **5. `unknown_id.h` lengthened to the 20 bytes its header announces**, `0x79` announcing
  `9 * 2 = 18` payload bytes. Two button bytes, four centred axis bytes and twelve zero
  pressure bytes, each commented. The vector was 4 bytes and therefore cut short as well as
  undeclared, which meant either fault could have caused the refusal — an outcome that agrees
  with §Goal's precedence is not a test of the precedence, only a failure to contradict it.
  `truncated_not_ready.h` is the vector for the overlap; this one now measures one thing.
  The comment that already claimed "the rest of this frame IS well-formed" became true instead
  of being reworded.
  **Checked for knock-on effects before editing:** `kUnknownId` is read by `case_unknown_id`,
  `case_link_unknown_id_drops`, `rule_proto03` and the uniformity case, and
  `case_announced_lengths` compares `frame_len` against vector sizes for the three **declared**
  ids only, so it never looked at this one. All four still pass.

- **Generalised before closing, and the sweep found two more of the same class.** Every number
  in §Acceptance criteria was read against "does its set appear?". Two did not:
  `accounting: test_style.sh 4 rule(s)` and `wiring cases: 10/10` were bare counts. Both now
  name their members — R-STYLE-01, R-STYLE-02, R-CLEAN-02, R-CLEAN-04 for the first; the ten
  `test_repo_shape.sh` rules for the second — and both recompute with
  `grep -oE 'RULE R-[A-Z]+-[0-9]+' <check file> | sort -u`, which is how the sets were measured
  rather than recalled. `3 rule(s)` was already enumerated in §Goal and is now spelled out at
  the criterion too.

- **The founding check caught one sentence inside this edit, which is the point of moving it
  to writing time.** The derivation paragraph first read "a total with no members is the defect
  this phase has now paid for **five times**" — a count in prose with no enumeration behind it,
  written into the fix for counts in prose with no enumeration behind them. Deleted before the
  gates ran. Nothing founds that number except the fix that wrote it.


### Round 20 — three `undecidable`, no `contradicts`. Two are round 19's reconciliation misses.

Second consecutive round with no `contradicts`: the reviewer walked all fifteen Plan steps and
found a faithful hunk for each, and reconciled the derived counts against the diff itself —
the accept floor 13 → 20 with exactly seven new `accept` lines, wiring 8 → 10, and the
64-rejection distribution summing to 64 with the nine new `reject` lines landing in the
R-ERR-01, R-ERR-02 and R-PROTO-05 buckets §Acceptance criteria names. The derivation added in
round 19 is doing the work it was added for.

- **1. `CLAUDE.md` is in the phase's diff and the spec never says this phase writes it.**
  **Measured:** `git diff 24d489f --stat -- CLAUDE.md` → 16 insertions, the three pre-validation
  checks. The spec names `CLAUDE.md` in §Context pointers as a file to **read**; it appears in
  no Plan step and in no row of §Files this phase writes, and it is not one of the four
  workflow-written paths the closure test exempts (`spec.md`, `notes.md`, `PHASES.md`,
  `docs/index/`). The mechanical reachability check passes it — it is "named in Context
  pointers" — which is exactly why the reviewer catches what that check cannot: being listed as
  a file to read does not authorise writing it.
  **This one is not a defect in the work; it is a question about whose change it is.** The
  block was added by the operator, outside the phase's Plan, and lands in the phase's diff only
  because the diff runs from `24d489f`. Three ways out and they are different claims: give
  §Files this phase writes a row recording that the operator amended the root instruction file
  mid-phase and why it appears here; move the edit to its own commit outside the phase's range;
  or leave it and accept that this phase's diff carries a change it did not make. The first is
  the only one that keeps the diff and the spec agreeing without rewriting history.

- **2. R-ERR-02's second scope clause is in `docs/constraints.md` and no Plan step names it.**
  Round 19 added it and did not add the step that describes it — the same shape as the stale
  markers of round 17, one file over. Worse, two statements went stale in the same edit and
  were not reconciled: §What a `test:` binding does and does not promise lists R-ERR-02 among
  the four rules that **already** record their scope and then says "This phase owes that clause
  for **R-PROTO-05**, which had none", and the audit table's row classifies R-ERR-02 as
  already-declared. Both were true when written and describe a tree that no longer exists.
  §Out of scope's "the narrowing is recorded in the rule's own text" is singular and now
  undercounts.

- **3. §Vectors fixes no length for `unknown_id.h`, and round 19 changed exactly that.**
  The other nine rows pin the shape — "2 payload bytes", "6 payload bytes", "payload cut
  short" — while this row says only "header `0x79`, the DualShock 2's real full-analog id,
  deliberately undeclared here". The ten-line rationale for the 20-byte length lives in the
  vector file and nowhere in the spec, so a reader working from the spec alone would write the
  obvious 4-byte frame, satisfy the asserted outcome, and have no way to know it was wrong.
  Round 19 made the change and recorded the reasoning in the file instead of in the table that
  fixes every other vector's shape.

**All three are spec bugs and none needs a code change.** Two and three are mine: an amendment
that changes the tree owes the Plan step and the table row that describe it, and round 19
delivered the amendments without them. Three taste items recorded below.

### Taste from validation 2026-09-15 (round 5), recorded and not fixed

- The deref-before-guard hazard, now named in **four** separate review rounds and enumerated
  this time: `case_digital_idle_maps_to_nothing_pressed`,
  `case_digital_pressed_maps_one_fret_and_one_strum`, `case_analog_whammy_full`,
  `case_analog_idle_whammy_is_rest`, `case_config_mode_whammy_is_rest`,
  `case_config_mode_maps_to_nothing_pressed`, `case_report_carries_the_whammy_end_to_end` and
  `rule_proto04`. Still latent — no current mutant makes those vectors refuse — but it is the
  hazard `src/core/ps2_frame.cpp`'s own comment argues against.
- `src/core/guitar_state.h`'s `GuitarState` comment names `frets` and `ControllerFret`;
  neither identifier exists (`is_fret_pressed`, `Fret`). Raised in round 4's taste list too.
- §Plan step 2 says the missing-SDK path "prints a `note:`"; `tests/test_style.sh` prints two.


### Round 21 — three accounting pointers. Nothing added that was not asked for.

- **1. `CLAUDE.md` has a row in §Files this phase writes, worded as explanation.** It records
  that the operator amended the file mid-phase with the three pre-validation checks, and that
  it appears in this phase's diff only because the base ref `24d489f` precedes that edit. The
  row does not claim the phase wrote it, because the phase did not. **Measured:**
  `git diff 24d489f --stat -- CLAUDE.md` → 16 insertions.

- **2. §Plan step 16 describes R-ERR-02's return-type clause, and three statements were
  reconciled — one more than the operator named, because the third needed a word.**
  - §What a `test:` binding does and does not promise listed R-ERR-02 as "(records that it
    scans `src/core/*.h` only, and why)" — stale the moment round 19 added the second
    narrowing. Now: "records both that it scans `src/core/*.h` only and which return-type
    spellings its pattern knows".
  - The audit table kept R-ERR-02 in the "text says so" row, which is still the right
    classification, but the row now records that **the audit did not find the second
    narrowing — the independent review did.** A dated audit that silently absorbs a later
    finding reads as if it had found it.
  - §Out of scope said "the narrowing is recorded in the rule's own text"; with two narrowings
    that is ambiguous, so it now says "the file-scope narrowing". One word, and it is the
    narrowing that entry is actually about.

- **3. §Vectors' `unknown_id.h` row states its shape, like the other nine.** 20 bytes, the
  whole frame `0x79` announces, since `2 * (0x79 & 0x0F)` is 18 payload bytes — and why a
  shorter one would not do: it would be cut short as well as undeclared, so either fault could
  be the reason for the refusal. This was the most expensive of the three to leave: the
  reasoning existed only inside the vector file, so a reader working from the spec would have
  written the obvious 4-byte frame, satisfied the asserted outcome, and had no way to know.

- **The founding check caught one clause inside this edit.** The `CLAUDE.md` row first read
  that the file appears in the diff "the same way `docs/phases/PHASES.md` and `docs/index/`
  appear without this phase authoring their content". That analogy is not accurate: the
  workflow commands **do** write `PHASES.md`'s status cell, so the phase does author content
  there. Deleted rather than reworded — the two measured facts in the row stand without it.

- **Nothing else was added.** No new note, clause or section beyond the three items: the
  operator's standing rule for this round, and the reason is on the record — the temporary
  workaround notes have themselves produced a finding.


### Round 22 — one `contradicts`, zero `undecidable`, and the escape fired on the threshold.

- **The finding, verified against both files.** §Goal says the marker count is "the number
  `verify.md` **§1** tells the operator to expect". `verify.md` §1 is `## 1. Everything passes`
  at line 172 and is about `make test` and `make lint`. The count — `grep -rn
  'TODO(09-guitar-observe)' src/core/` followed by "Expect three of them" — is at line 166,
  under the unnumbered `## What this phase does NOT prove` at line 143, before the numbered
  "Check it yourself" block begins. **The number is right and `verify.md` is right**; the
  spec's pointer to where it lives is wrong. One token: `§1` names a section that does not
  contain what the sentence says it contains.
  Mine, written during the 2026-09-15 re-expansion while closing the marker-count finding —
  the same class as rounds 19 and 20, an amendment that asserts something about another file
  without checking the other file.

- **Nothing else.** `undecidable: none`, and the reviewer said so plainly after checking every
  quantified claim the spec makes against the diff: the ten vectors and their mappings, four
  `LinkState` members and five `FaultCause`, the six-row transition table as a function of the
  outcome alone, the strict `>` on the timeout, the saturating accumulation declared as
  unasserted, the four-step refusal precedence in the order §Goal fixes, `Ps2Frame` carrying no
  length with both readers recomputing, three markers over four concerns, wiring 8 → 10, the
  floor 13 → 20 with seven new `accept` lines, nine new `reject` lines consistent with the
  64/20 distributions, `unknown_id.h` at 20 bytes, the seven rebound rules, and the driver's
  `CXXFLAGS` byte-identical to the list §Context pointers records. All 35 files accounted for,
  `CLAUDE.md` explicitly.

- **The escape fired and this command moved the status.** Three `## Validation` sections now
  follow the 2026-09-15 `escaped to /expand-phase` verdict and this third one failed a gate,
  so the iteration-3+ rule applies. `docs/phases/PHASES.md` row `01-ps2-codec` set to
  `pending` by `/validate-phase`. **Not overridden, and the operator's standing instruction was
  explicit that it should not be.**

- **The trend, recorded because the re-expansion will want it and it is not an argument against
  the escape.** Findings since the second escape: validation #1 returned 5 `undecidable` and 0
  `contradicts`; #2 returned 3 and 0; #3 returned 0 `undecidable` and 1 `contradicts`, that one
  being a section reference. Whether a one-token cross-reference error is proportionate to a
  re-expansion is the operator's call, not this gate's — the gate's job was to fail and route,
  and it did both.


### Round 23 — the token, its generalisation, and the override.

- **The pointer now names the heading, and the heading was read out of the file rather than
  typed from memory.** §Goal said "the number `verify.md` §1 tells the operator to expect";
  `verify.md` §1 is `## 1. Everything passes` and is about `make test` and `make lint`. The
  fix names the section that actually carries the count — "What this phase does NOT prove" —
  and the replacement text was produced by reading `verify.md`, finding the line with the
  count, and walking back to the nearest `##`. The finding was an amendment asserting something
  about another file without opening it; retyping the heading from memory would have been the
  same move again.

- **Generalised: every cross-file reference in the spec was checked against the file it
  names, not just the ones about `verify.md`.** This is the third time in the phase that an
  amendment claimed something about another file without verifying it, so the sweep covered the
  class rather than the instance. Eleven references, all verified:
  `verify.md` — three mentions, only one of which names a section, the broken one now fixed;
  `notes.md` §Debt, §Deviations and §For later phases — all three headings exist;
  `docs/phases/00-scaffold/notes.md` §Debt, §For later phases and §Owed — all present, and
  **line 3256 is exactly the heading the spec says it is**,
  `## For later phases (added round 13, at close — three patterns \`01-ps2-codec\` inherits)`;
  `docs/constraints.md` §Invariants, §Layering, §Error handling, §Testing and §Observed
  conventions — all five exist, at lines 39, 15, 262, 289 and 185. **Exactly one was wrong.**

- **The escape was overridden by the operator and the override is recorded under the
  validation #3 record**, with the trend table and the reason, so a later session reading only
  `notes.md` sees a decision rather than a missing gate. The status was returned to
  `in-progress` by that decision, not by this command's own rule.

- **Nothing else was touched.** One token, its generalisation, and the record.


### Round 24 — one `contradicts`, structural, and it meets the condition the override wrote for itself.

- **The finding, verified in four places.** §What a `test:` binding does and does not promise
  states the convention — *the phase that moves a rule from `planned:` to `test:` writes, in
  the rule's own text, what the check does not see* — and its audit table puts **R-CLEAN-04**
  in the row "**Narrower than its text, and the text says so**". Measured:
  1. R-CLEAN-04's text in `docs/constraints.md` records exactly one gap, the `const`/`constexpr`
     initializer blindness, plus the `tests/vectors/` exception to the *rule*. **It records no
     file scope.**
  2. `tests/test_style.sh:68` — `tidy_sources() { $LS 'src/core/*.cpp' 'src/core/*.h'
     'tests/*.cpp'; }`.
  3. `tests/test_style.sh:110-115` — R-STYLE-02, R-CLEAN-02 **and R-CLEAN-04** all run through
     that same function.
  4. The spec's own row 1 faults R-STYLE-02 and R-CLEAN-02 for precisely this: "`tidy_sources()`
     is `src/core/*.cpp`, `src/core/*.h`, `tests/*.cpp`, while both rule texts are unscoped."
  R-CLEAN-04's text is unscoped in the identical way and runs through the identical function,
  so by the spec's own row-1 criterion it belongs in row 1 — and the table asserts row 2. The
  spec contradicts itself, and R-CLEAN-04 is a binding **this phase owns**: §Plan step 9 moved
  it from `planned:` to `test:`, so the convention obliges the clause.
  The reviewer also names a second undeclared narrowing on the same rule: `.clang-tidy`'s
  `HeaderFilterRegex` moved `(src|tests)/.*` → `src/.*`, so no header under `tests/` is
  diagnosed at all. The spec binds the *consequence* through step 11's criterion and never the
  rule text.

- **This is not a typo, and that matters for what happens next.** It is the third time the
  audit table written in round 15 has itself been wrong: R-ERR-02's return-type narrowing
  (found by review in round 20), and now R-CLEAN-04's file scope. The audit's own conclusion —
  which rules declare their scope — is the thing that keeps being falsified, and it is the
  class this phase has paid for repeatedly.

- **The escape condition is met again, and the override wrote the test for itself.** The
  2026-09-15 override is recorded above with this sentence: *"si la ronda que viene falla con
  algo estructural, esta anotación es lo que le dice a la próxima sesión que la anulación fue
  un error."* The next round failed on something structural. That is the operator's own
  criterion, not this gate reinterpreting it, and it is why the escape is fired rather than
  reported: the command mandates it at iteration 3+, and the recorded condition for not
  overriding it a second time is satisfied.
  **Iteration count, stated because the mechanical count is misleading here.** The rule resets
  the counter at an `escaped to /expand-phase` verdict "because the escape's own output is a
  new spec". The 2026-09-15 escape produced no new spec — it was overridden — so the reset's
  justification does not hold and this is **iteration 4 against the same spec**, not iteration 1.
  Status moved `in-progress` → `pending` by `/validate-phase`.
  The operator may override again; the point of this entry is that the second override would
  be made against a structural finding rather than a section reference, which is a different
  decision from the first.


### Round 25 — the re-expansion. The obligation moves from a list to the function.

- **What was wrong was the method, not the instance.** Three times the same conclusion was
  falsified — the round-15 audit table, R-ERR-02's return-type scope in round 20, R-CLEAN-04's
  file scope in round 24 — because the spec answered "which rules declare their scope" by
  **enumerating rules by hand**. §What a `test:` binding does and does not promise now derives
  it from the function the check runs through: `tidy_sources()` costs a file scope
  unconditionally, `hits()` costs comment blindness, `raw_hits()` is the counterpart and costs
  nothing. Adding a rule to one of those functions drags the obligation along, and checking the
  conclusion is three greps rather than a memory.

- **The derivation is narrower than the obvious version, and the measurement is why.** Measured
  first: **eight** rules run through `hits()` and **none** of their texts says anything about
  comments. The naive rule — "runs through `hits()` ⇒ owes a comment clause" — would have
  created eight obligations. It is wrong for all eight: `hits()` strips `//` before grepping, so
  it narrows a rule only if that rule's **subject can occur in a comment**, and the eight
  subjects are `#include`, allocation calls, a declaration's name, inheritance, return types,
  `[[nodiscard]]`, `throw`/`try`/`catch` and a `.value()` call — a comment contains none of
  them. Writing the naive version would have been the fourth falsification, produced by the fix
  for the third. `tidy_sources()` is different and that difference is the point: it narrows
  **whatever the rule says**, so its obligation is unconditional and mechanical.

- **The assignment table was produced by reading the files, not by copying the old table** —
  the old one is the artefact that was falsified three times. Rule → finder came from the
  `report R-` lines, finder → scanner from the `find_*` definitions, and the source lists from
  `tests/test_style.sh`. The three greps that reproduce it are in the section.

- **What the measurement found.** Three rules run through `tidy_sources()` and **none** declares
  the file scope: R-STYLE-02, R-CLEAN-02 and R-CLEAN-04. The first two are `00-scaffold`'s
  bindings and stay released to §For later phases, now also stated in §Out of scope so the split
  is visible where scope is decided. **R-CLEAN-04 is this phase's** — §Plan step 9 moved it to
  `test:` — so §Plan step 17 owes its clause, covering both narrowings on that axis: the file
  list, and `.clang-tidy`'s `HeaderFilterRegex` of `src/.*`, which the spec previously bound
  only through step 11's criterion.

- **The override of 2026-09-15 is marked superseded in place, under its own record**, with the
  reason its own test came due and with the iteration count corrected: the reset is tied to an
  escape producing a new spec, and the overridden one produced none, so those rounds were
  iterations 4 and not 1. Not deleted — the record of a decision that turned out wrong is worth
  more than its absence.


### Round 26 — §Plan step 17. One clause, one file, measured before and after.

- **The check was run before as well as after, which is what makes it a check.**
  `grep -c 'tidy_sources\|src/core/\*'` over R-CLEAN-04's line returned **0** before the edit
  and **1** after. A criterion only ever run after the fix cannot distinguish a fix from a rule
  that already satisfied it.

- **Both facts were read out of the files being described, not recalled.**
  `tests/test_style.sh:68` is `tidy_sources() { $LS 'src/core/*.cpp' 'src/core/*.h'
  'tests/*.cpp'; }`, and `.clang-tidy:33` is `HeaderFilterRegex: 'src/.*'`.

- **Reading rather than recalling changed what the clause says.** `.clang-tidy`'s own comment
  states that the regex governs the headers an analysed file *includes* and that a file passed
  as the main file is always diagnosed. Written from memory the clause would have said "no
  header under `tests/` is diagnosed" as a bare fact; what is true is narrower and needs both
  files: `tidy_sources()` passes `tests/*.cpp` and never `tests/*.h`, so headers under `tests/`
  are only ever seen as includes, and the regex is what drops them. The clause says it that way.

- **Founding, checked clause by clause.** The glob list traces to `tests/test_style.sh:68`; the
  regex to `.clang-tidy:33`; the include-versus-main-file behaviour to `.clang-tidy`'s own
  comment; "the vectors are the deliberate case" to R-CLEAN-04's pre-existing text, which
  already names them as the rule's sole exception; and "which is why `01-ps2-codec` binds a
  criterion asserting there are none" to §Plan step 11's criterion. Nothing rests on the
  amendment that wrote it.

- **Nothing else was touched.** `docs/constraints.md` only, one rule line. `make test` **OK**
  in **2:38**, `make lint` 0, `python3 tests/test_rule_traceability.py` exit 0, and
  `accounting: test_style.sh` still reports its four rules.


### Round 27 — the stale marker, the audit in both directions, and two false alarms of my own.

- **Step 17's marker was `Owed` with the work landed; it now reads `Landed 2026-09-15`,** the
  same form as steps 12-16. No `Owed` markers remain.

- **All seventeen steps audited in both directions against the artefact each names.** Nothing
  is marked `Landed` whose work is undone: `planned: 01-ps2-codec` 0 (step 1), `make lint` 0
  (2), both ADRs (3, 6), 10 vectors (4), the config-controls case (5), four `hid:` cases (7),
  three driver mutations (8), `wiring cases: 10/10` (9), the uniformity case (10), the
  stray-header criterion (11), the length check at line 26 above the ready check at 33 (12),
  `rule_proto02` reading `kTruncatedNotReady` (13), `find_proto05` on `raw_hits` (14),
  `drops_from_every_source` (15), R-ERR-02's second narrowing (16), R-CLEAN-04's scope clause
  (17). The only drift was step 17's marker.

- **Two of those came back wrong on the first pass and both were my grep, not the tree.**
  `find_proto05` looked unmatched because the pattern expected a space before `{` that the file
  does not have, and R-ERR-02's clause looked absent because the pattern was "return-type
  spellings" while the text reads "three spellings of a return type". Recorded rather than
  quietly corrected: I was one step from reporting two findings that did not exist, and the
  mechanism is the same one that produced this phase's three real falsifications — asserting
  something about a file from a pattern instead of reading the file. Both were resolved by
  opening the lines.

- **Generalised: the cause of the stale marker is named in §For later phases.** It is the second
  time a step landed and kept its `Owed`, and both were caught by the operator rather than by a
  gate. The marker lives in `spec.md` §Plan while the work lands elsewhere, and
  `/implement-phase`'s final step writes `notes.md` and `PHASES.md` and never the Plan — so the
  step's check runs and its marker does not. Two upgrade paths recorded there, one upstream and
  one local.


### Round 28 — five `contradicts`, three `undecidable`: two generalised, one measured, one discarded.

Input: the independent review of 2026-09-16 (recorded under that validation below). The same
session that produced the review ran this round, so it cannot be the next reviewer; see the
validation addendum.

- **Contradicts 1 — the assignment table read "no" for R-CLEAN-04 after step 17 landed the
  clause.** Fixed in the table (now `yes`, steps 17 and 20). **Same fact, checked before closing:**
  step 17's prose said "the rule's text is unscoped" in the present tense — now "before this step
  … was". §Out of scope's R-STYLE-02/R-CLEAN-02 bullet and §What a `test:` binding promise are
  still true. The whole "text carries it" column was then re-read out of `docs/constraints.md`
  instead of carried forward, which is what the table claims to be.

- **Contradicts 2 — "Steps 10 and 11 are the only work owed" with steps 12-17 landed.** Rewritten:
  every step is landed, 18-22 added by this round. **Same fact, checked before closing:** the same
  paragraph's "three validation rounds have run" (dropped, not recounted — a count of rounds in
  the spec is one more thing to go stale); the header comment's "Re-expanded 2026-09-14 … code
  from rounds 1-4" (now names all three re-expansions, dates from `git log`); §Context pointers'
  "the account of rounds 1-6 and of three validation rounds" (now uncounted).

- **Contradicts 3 — "two package workarounds" titling three.** Retitled and split by upstream
  state, read from sources rather than assumed: 1 and 2 are fixed in 792e9d9 (round 6 above); 3
  is the `- base:` staleness entry dated 2026-09-15 in `~/.claude-belay/feedback/pico-sg2hid.md`
  with no fix recorded. **Same fact, checked:** "Neither is a fact … Both are defects … already
  fixed upstream" and "re-emit it" were in the same section and are rewritten with it; nothing
  else in the spec, `CLAUDE.md` or `verify.md` counts the workarounds.

- **Contradicts 4 — step 10's check claimed any source-state mutation fails `R-PROTO-02`.**
  **Generalised before fixing, and the measurement changed the diagnosis.** Property: *a rule line
  stays green only while its rule holds*. Enumeration — every mutation point behind the three rule
  lines, each applied alone in a scratch copy, `FAIL:` lines recorded:

  | mutant | lines that failed before this round |
  |---|---|
  | each of 6 outcomes × 4 sources sent to a wrong target (24) | uniformity 24/24; `R-PROTO-02` 7 (4 `AckTimeout`, 3 good frames from `Absent` via its set-up); 17 fail nothing else |
  | each outcome from **every** source at once (6) | `NotReady` **survived everything**; the others hit a per-outcome case |
  | delete the size-below-prefix check | **survived everything** |
  | delete the ready check | `not_ready` case |
  | length check uses `kPrefixLen` (M3) | `R-PROTO-02` + 3 cases |
  | unknown id → `Analog`; → `Config` | `R-PROTO-03` + 2 cases, each |
  | controls gate admits `Config` | `config_mode` controls case |
  | whammy gate `id != Digital` | survived — equivalent, recorded in round 12 / step 13 |
  | timeout `>=`; entering elapsed counted; good frame clears `last_fault`; `NotReady` recorded as `AckTimeout` | **survived everything**, all four |
  | counter wraps | survived — §Debt, unasserted by decision |

  **What that says about the finding, stated as a correction:** the step-10 sentence was an
  overclaim, not a green rule line over a broken rule. The 17 transitions it wrongly credited to
  `R-PROTO-02` are on a good frame, `UnknownId` or `NotReady`, and no rule's text covers them —
  §The link's table does, and the uniformity case caught all 17. The spec sentence is fixed with
  the measured table (step 10). **The real instance of the class was elsewhere and the enumeration
  found it:** deleting the prefix check made a lone undeclared header report `UnknownId` instead of
  the abort, every line green — that one is the fifth, and step 18 fixes it with a case and a
  `rule_proto02` clause over `kUnknownId`'s first byte. The spec's "Four times" became "Five times".
  `CLAUDE.md`'s "measured four times" is the operator's text and was **not** edited.
  **Same fact, checked:** §Goal's precedence item 1 said "Nothing else is knowable yet", false for
  one byte — rewritten as a choice; the sentence introducing the list said "the earliest decidable
  one wins", which item 1 contradicts — rewritten; step 13's "every overlap still reports
  `UnknownId`" — scoped to refusals below it; `src/core/ps2_frame.cpp`'s "there is no header to
  read" — rewritten. `verify.md` §3's six predicted `FAIL:` lines for M1 were re-run in a `cp -a`
  copy and are unchanged, so it was not touched. After the round every mutant in the table fails
  at least one line except the two already declared.
  **Deviation:** `rule_proto02` then exceeded R-CLEAN-02's 60 lines (`make test` failed, the edit
  hook had passed it); its source-state clause moved to `cut_drops_the_link_from_every_source`,
  re-measured live afterwards.

- **Contradicts 5 — `find_err01` misses `static`/`inline` returns.** **Generalised:** the scanner
  table derived obligations from the scanner *function* and never from the pattern, and every
  regex has spellings it does not match — the fourth falsification of "which rules owe a clause",
  and the first of the function-derived method. The table gained a row: every `find_*` owes a
  spelling clause, unconditionally. Enumeration over all ten finders: seven are `00-scaffold`'s
  bindings (R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-ERR-03, R-ERR-04) —
  released in §Out of scope, **not measured**, so no form is claimed for them; three are this
  phase's and were measured one line per form: R-ERR-01 (10 forms missed), R-ERR-02 (9 missed plus
  a false positive on `[[nodiscard]]` on the preceding line), R-PROTO-05 (4 missed, a relative
  `../../tests/vectors` caught). R-PROTO-05's existing clause said it stated "what is given up"
  and gave only the over-strict half. **Checked before writing "none occurs today":** a grep for
  every missed form over `src/` returns a parameter, a local and a `.cpp` definition — no return;
  and nothing passes `-Itests`.

- **Undecidable 1 — four §The link behaviours asserted by nothing.** By operator decision, four
  cases (step 21), each killed its mutant from the table above. The spec's saturation "Nothing
  asserts it" is still true and unchanged.

- **Undecidable 2 — claims about `notes.md` and `00-scaffold/notes.md`.** **Discarded, and not
  as a spec gap:** the reviewer was denied those files by construction (`/validate-phase` step 5),
  so "could not open the file I was forbidden to open" is the starvation working, not a missing
  pointer. The pointers exist; a reviewer with the files could check them.

- **Undecidable 3 — `HeaderFilterRegex` unanchored.** **Measured, not argued:** two clones of
  HEAD, one at a path with no `src/`, one under `…/scratchpad/src/pico-sg2hid`. Unmodified, both
  `make lint` 0 — which proves nothing, since the vectors' literals are `constexpr` and not
  diagnosed anyway. With `inline int BadProbeName = 0;` planted in
  `tests/vectors/digital_idle.h`: exit 0 at the plain path, exit 2 under `src/` with
  `invalid case style for variable 'BadProbeName'` on the absolute path. **Real.** R-CLEAN-04's
  text records it (step 20). **Same fact, checked:** `.clang-tidy`'s comment ("naming src/
  excludes them") — qualified; spec steps 11 and 17 — qualified. `verify.md` does not state it.
  `ExcludeHeaderFilterRegex` exists in the installed clang-tidy (`--help`, LLVM 23.1.0) and would
  close it; not applied, it was not asked for — §For later phases.

- **Aside — comments asserting code that does not exist.** `Fret`'s comment (`frets`,
  `ControllerFret`, and the claim that the indexing keeps the HID descriptor in step) and
  "comparable as a whole" on `GuitarState`/`Ps2Frame`. **Generalised:** every backticked and every
  CamelCase/snake_case identifier in comments across `src/core/`, `tests/ps2_codec_cases.cpp`,
  `tests/test_ps2_codec.py` and `tests/vectors/` checked against the code by script; the remaining
  unmatched names are bus lines, ADR-retired sketches, a cited rejected name and proper nouns.
  Taste items from the review are in §For later phases, untouched.

- **Acceptance after the round:** all 21 criteria pass; `make test` **OK** in **1:39**, `make lint`
  0, driver 37 `ok:` lines, 3/3 rejection cases. Steps 18-22's own checks pass. M4 not re-run
  here — it is the validator's.


### Round 29 — three `contradicts` closed by measurement, four pointers decided, one discarded.

Input: the independent review of 2026-09-17 (dispatched to a fresh agent by `/validate-phase`,
recorded below). Iteration 3 against the def180c spec, so the next failure fires the escape.

- **Contradicts 1 — `ps2_protocol.h:68` kept the sentence step 18 deleted from `ps2_frame.cpp`.**
  Rewritten to say what is true: a frame shorter than the prefix is cut short before the ready
  slot, its header may well be readable, and the abort is reported anyway.
  **Generalised, because every reconciliation this phase has done was inside one file and this one
  crossed a pair.** Three passes: (a) every comment line deleted from `src/` or `tests/` since
  `24d489f` grepped for in the current tree — two hits, both text that moved within its own file;
  (b) every comment sentence compared with every other across files by word overlap ≥ 0.45 — four
  hits, all legitimate restatements (a vector header echoing `DecodeStatus`'s wording, three
  vectors sharing a byte-comment shape); (c) the *fact* grepped tree-wide rather than the wording.
  Only (c) found it, because the twin was worded differently — **match on the claim, not the
  string**, and that is the transferable part. The same pass over the other facts fixed this
  phase (the zero fill, "no length of its own") found no second survivor.

- **Contradicts 2 — §Out of scope claimed mutation coverage `tests/test_style.sh` does not have.**
  `tests/test_checks_are_live.py:49` is `NO_MUTATE = {"test_style.sh"}`, skipped by both the
  neutering and alternation properties (lines 398 and 451). Fixed as instructed, in the spec and
  not in the harness: §Out of scope now says four of the five `.sh` checks are mutation-tested,
  adds a bullet releasing the mutation of that file with the reason the code's own comment gives,
  and the assignment table carries a paragraph saying all four rules bound to it — R-STYLE-01,
  R-STYLE-02, R-CLEAN-02 and this phase's R-CLEAN-04 — stand on the accounting property alone.

- **Contradicts 3 — §Plan step 10 asserted two different answers eleven lines apart.**
  Re-measured on the current tree: the uniformity mutation fails `R-PROTO-02` in **7** of 24 —
  the four `AckTimeout` mutants plus the three good frames sent astray from `Absent`, which break
  `cut_drops_the_link_from_every_source`'s set-up. The "only when the outcome is `AckTimeout`"
  sentence is **deleted**, not reworded; the measured table stands as written.

- **Pointer — `tidy_sources()`'s notation.** They are git pathspecs, where `*` crosses `/`, so
  the three patterns read recursively. Verified: `git ls-files 'docs/*.md'` returns 38 files in
  subdirectories. The set is wider than a shell-glob reading, never narrower, and today no `.cpp`
  lives below `tests/` at all. Written into both the axis table and R-CLEAN-04's text.

- **Decision — the `HeaderFilterRegex` narrowing is reverted, not owned (§Plan step 23).**
  Measured in two clones before deciding, one at a path containing `src/`: with
  `(src|tests)/.*` restored, `make lint` and `make test` are green in both, and a naming probe
  planted in `tests/vectors/digital_idle.h` is diagnosed in both. So the narrowing protected
  nothing — the vectors' literals are `constexpr` initializers `readability-magic-numbers` never
  sees — while it silenced naming and function-size diagnostics for every header under `tests/`,
  which is R-STYLE-02's and R-CLEAN-02's reach and `00-scaffold`'s to set. Reverting also deletes
  the checkout-path dependence rather than documenting it, so round 28's step 20 is marked
  superseded in place and R-CLEAN-04's clause now records what the exception really rests on.
  **Cost, measured:** `make test` goes from **1:41** to **2:37** against a 3m cap, because
  clang-tidy now analyses the headers under `tests/` as includes. That is the price of the
  coverage those two bindings are supposed to have; if the cap is ever hit, §Out of scope's
  standing answer is to raise it in a re-expansion, not to re-narrow a filter.

- **Decision — `kZeroFill` is not an expected protocol byte (§Plan step 27).** R-PROTO-05 governs
  what the controller puts on the wire; the fill is `decode`'s own contract, so no vector could
  carry it. `CLAUDE.md` already scoped the rule that way; the catalogue now does too.

- **Decision — ADR-0007's status names all three partial supersessions (§Plan step 28).** Its
  status line already named ADR-0009, which is the precedent that the line is maintained; it now
  names ADR-0011 and ADR-0012 as well. Body untouched — the status is the mutable part.

- **Discarded — the fifth `undecidable`.** It quantifies `docs/phases/00-scaffold/notes.md` and
  this file's own §Deviations, both denied to the reviewer by `/validate-phase` step 5. A finding
  that the starved reviewer could not open the files it was starved of is the deprivation working
  as designed, not a missing pointer: a reader with those files can check both claims, and the
  spec names where each lives. Recorded rather than silently dropped.

- **Two checks I wrote this round were wrong before they were right, and the cause is worth
  keeping:** a `grep -c '<phrase>' spec.md` counts the check's own line, because the check lives
  in the file it greps. The first attempt reported 2 where it wanted 1, the second 1 where it
  wanted 0. Step 25's check now reads `tests/test_checks_are_live.py` instead, and step 26's is an
  absence whose phrase its own text no longer contains.

- **Acceptance after the round:** 21/21, `make test` **OK** in **2:37**, `make lint` 0, driver 37
  `ok:` lines, `rejection cases: 3/3`, `neutered: 33/33`, `alternations: 64/64`. Steps 23-28's own
  checks pass.

- **Taste from the 2026-09-17 review is in §For later phases, untouched** — six items, one of
  which (`case_report_is_wide_enough` asserting tautologies of its own constants) looks like real
  debt and is recorded as such rather than fixed in a closing round.


### Round 30 — the one item the re-expanded Plan carried, and its liveness measured both ways.

- **What the spec asked for and what landed.** Three `accept` cases in `tests/test_repo_shape.sh`,
  one each for R-PROTO-05, R-ERR-03 and R-ERR-04 — the three finders that had none — and the
  floor raised 20 → 23. Nothing else was touched.

- **Generalised, since "three rules have no accept case" is an instance of a property.** The
  property: *every finder has at least one case proving it stays silent on the legitimate
  neighbour of the violation it hunts.* Enumerated over all ten finders after the edit, by
  counting `^accept ` lines per finder: `find_arch01` 5, `find_err02` 4, `find_clean03` 4,
  `find_err01` 3, `find_arch03` 2, and 1 each for `find_clean05`, `find_clean09`, `find_err03`,
  `find_err04` and `find_proto05` — 23, and no finder left at zero. That is the whole set; there
  is no eleventh finder, because `run_all` reports exactly ten rules.

- **Liveness measured in both directions, six probes in scratch copies.** Deleting each of the
  three new lines makes the file report `false-positive cases: 22 (floor 23)` and `FAIL` — so each
  is load-bearing rather than decorative. And loosening each finder's pattern by one character —
  `\bthrow\b` → `throw`, `\.value[[:space:]]*\(` → `\.value`, `tests/vectors` → `tests/vector` —
  leaves `rejection cases: 64` fully green and fails exactly the new accept case. That second half
  is the one worth keeping: it is the measurement behind the comment written above the three
  lines, and it shows what a rejection case structurally cannot catch.

- **Reconciled in the same edit, because the fix invalidated a sentence.** §Acceptance criteria
  said the false-positive cases were "**20** today … and **23** once §Plan step 1 adds one each";
  that is now false in both halves, and it is replaced by the measured 23-case distribution. The
  §Plan section is left **empty** rather than carrying the step with a `Landed` marker: the
  re-expansion's own rule is that the Plan holds work that is not done, and a step marked landed is
  precisely the shape that rotted across rounds 28 and 29. `grep -n '§Plan' spec.md` returns only
  the two references in the header comment, so nothing points at a step number that no longer
  exists. §Outcome's "accept floor 13 → 20" is updated to 23.

- **Acceptance: 23 of 23 pass.** `make test` **OK** in **2:29** (cap 3m), `make lint` 0, driver
  exit 0, `rejection cases: 64`, `false-positive cases: 23 (floor 23)`, `wiring cases: 10/10`,
  `neutered: 33/33`, `alternations: 64/64`, `accounting` 3 and 4 rule(s), `test_phase_docs.sh` 0,
  and the four criteria added by the re-expansion (`HeaderFilterRegex`, ADR-0007's status, both
  ADR files). M4 is the validator's and was not run here.


### Round 31 — the prose count deleted, and the two others it generalised to.

- **The finding.** §Context pointers said `grep -c '^### Round ' notes.md` returns **28** and
  `grep -c '^## Validation'` **13**, and derived "the rounds are 29" from the first. The file
  returned 29 and 13 when validation 2026-09-21 measured it, so the sentence was false. The fix is
  the deletion the validation prescribed, not a re-measurement: the pointer now names the file, its
  sections and its heading conventions, and measures nothing. The line count went with it for the
  same reason.

- **Generalised, because "a prose count of another file with no check behind it" is a property and
  `00-scaffold` §For later phases already named it.** The property as applied: *a count of a tree
  artifact, stated in prose in a document this phase owns, must either be recomputed by an
  acceptance criterion or have its members enumerated in place; otherwise the number goes.*
  Enumerated over `spec.md` and `verify.md` by grepping every numeral and number-word followed by a
  countable noun. Seven candidates, three violate:

  | where | the count | verdict |
  |---|---|---|
  | §Context pointers, `notes.md` | 2575 lines, 28 rounds, 13 validations, "so 29" | **violates** — deleted |
  | §Context pointers, `ps2_codec_cases.cpp` | "33 case functions and 3 rule functions" | **violates** in its first half — the 33 has no check; the 3 is checked by `accounting: test_ps2_codec.py`. Replaced by "the `case_*` functions and the three `rule_*` functions, each called once from `main`" |
  | §Files this phase writes, `docs/constraints.md` | "the seven rules this phase settled" | **violates** — no check, no members named; the number deleted |
  | §Context pointers, `src/core/` | "the nine files this phase owns" | holds — all nine are named in the same sentence |
  | §Goal, §Vectors, §Files, §Context pointers | "the ten vectors"/"ten headers" | holds — `ls tests/vectors/*.h \| wc -l` is an acceptance criterion |
  | §Acceptance criteria | the 64/23 distributions | holds — both totals are acceptance criteria and every member is named; the distribution exists *because* a total with no members was this phase's recurring defect |
  | §clang-tidy's header filter | "returns 38 files in subdirectories" | holds — a dated measurement carrying its own recompute command, evidence for a property rather than an inventory |

  `verify.md` carries no violating count: its "ten vectors" and its three `R-PROTO-*` lines are both
  checked.

- **The 33 was accurate when deleted.** Counted in `main`: 11 decode, 6 map, 12 link, 4 report. It
  was removed for being uncheckable, not for being wrong — which is the point, since the notes.md
  count was accurate when it was written too.

- **Nothing outside the three sentences changed.** No source file, no test, no rule. Every
  replacement sentence restates something the spec already says elsewhere (its own header comment
  for what `notes.md` is; `main`'s structure for the cases file) rather than being founded on this
  round's fix.

- **Acceptance: 23 of 23 pass.** `make test` **OK** in **2:04** (cap 3m), `make lint` 0, driver
  exit 0, `rejection cases: 64`, `false-positive cases: 23 (floor 23)`, `wiring cases: 10/10`,
  `neutered: 33/33`, `alternations: 64/64`, `accounting` 3 and 4 rule(s), `test_phase_docs.sh` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers, 0
  `tests/vectors` references under `src/`, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1, both ADR
  files, ADR-0007 naming ADR-0011. Both derivations recompute (64 `reject`, 23 `accept`). M4 is the
  validator's and was not run here.

- **The three pre-validation checks (`CLAUDE.md` §Conventions).** (1) Nothing found this round
  leaves the Goal unmet — the defect was a sentence about `notes.md`, and the codec is untouched and
  green. (2) `- base:` names `24d489f`, `git cat-file -t` says `commit`, and `git diff --name-only
  24d489f` yields **37** files. (3) Checked sentence by sentence; see the fourth bullet.

### Round 32 — one measurement reconciled, and two undecidable questions decided.

Closes validation 2026-09-22 (iteration 2): one `contradicts`, two `undecidable`.

- **The `contradicts` was a measurement, and re-measuring it falsified the fix first written for
  it.** Three fresh `make test` runs this round: **2:13**, **2:05**, **1:41**. The first two agreed
  with the four already on record (2:29 round 30, 2:14 validation 2026-09-21, 2:03/2:04 round 31),
  so the first version of this fix wrote "**2:00 to 2:30**" into both files and asserted that no
  post-revert run had produced 1:40. The third run produced 1:41 — on the same tree, with `src/`
  untouched — and the assertion was false within minutes of being written.

  **So the reconciliation is not a wider band, it is the deletion of the band.** The fastest and
  slowest post-revert runs differ by 48s and by CPU utilisation (420% against 339%): the spread
  measures machine load, not what the header filter costs, and stating it as a normal range invites
  an operator to read a slow run as a fault. Both files now give the seven measured times as
  *evidence that the number moves*, and point at the **3m cap** as the only timing with a check
  behind it. This is round 31's property again — a prose number with no check — arriving as a range
  rather than a count, and it was caught by the acceptance run rather than by review.

- **`undecidable` (1) — what a non-grep check owes.** Decided and written as a new subsection,
  §A check that is not a grep owes a clause too. The three-part derivation (file list × scanner ×
  pattern) is a property of **grep** checks; the *obligation* is not, because it comes from the
  section's own convention — a rule whose text states no scope asserts its check covers it
  entirely — which every `test:` binding makes regardless of what the check is built from. What
  varies is where the reach comes from: for a vector-driven check it is the **vector list**.

  That decision creates work this phase owed and had not done, and the spec's own sentence
  excluded R-PROTO-02..04 from its "bindings it moved or re-scoped itself" list inconsistently —
  this phase moved all three. **Three scope clauses written into `docs/constraints.md`, from the
  vector files and not from memory:** R-PROTO-02 cuts only a **digital** frame and only one byte
  short (both truncation vectors are 3 bytes of 4, differing in the ready slot); R-PROTO-03
  exercises **one** of the 253 undeclared header bytes (`0x79`, against the three declared ids
  `0x41`/`0x73`/`0xF3`); R-PROTO-04 exercises the two **ends** of the whammy axis and no value
  between them.

- **`undecidable` (2) — does releasing a clause release compliance? No.** Written into §Out of
  scope. A scope clause records what a check does not see: it is documentation, and releasing it
  releases the writing. The rule's *text* is the rule and binds every file it reads on. Two
  corollaries had to be ruled on explicitly, because both were live in the diff: R-CLEAN-02 and
  R-CLEAN-03 name no language, so they bind the `.py` and `.sh` files here; and where a rule's
  suggested remedy has no equivalent in the language at hand ("the arguments become a struct"),
  the limit still binds and the remedy is translated.

- **Generalised, because "a rule binding code no check reads" is a property, not two instances.**
  The reviewer could only report the two it was shown. Enumerated over the whole 37-file set:
  the four unscoped-text rules whose checks are narrower than their text are R-CLEAN-02
  (clang-tidy over `tidy_sources()` — no `.py`, no `.sh`), R-CLEAN-03 and R-CLEAN-05 (`src_files`
  — nothing under `tests/`), and R-STYLE-02 (text names C++ constructs only, so it reaches
  nothing outside them). In the unread region:

  | instance | rule | written by | disposition |
  |---|---|---|---|
  | `sources_reached`, `tests/ps2_codec_cases.cpp:670` | R-CLEAN-03 | this phase | fixed — `has_reached_every_source` |
  | `reject( rule, label, rel, anchor, replacement )`, `tests/test_ps2_codec.py` | R-CLEAN-02 (5 params) | this phase (`b23b91a`) | fixed — a `Mutation` namedtuple, one parameter |
  | `reject( )`, `tests/test_repo_shape.sh` (4 positional) | R-CLEAN-02 | `00-scaffold` (`c07f525`) | **not fixed** — another phase's function in a file this phase only modified; named in §Out of scope and below |
  | `accept( )`, `tests/test_repo_shape.sh` (4 positional) | R-CLEAN-02 | `00-scaffold` (`c07f525`) | same |
  | function length / nesting in `.py` and `.sh` | R-CLEAN-02 | — | clean — longest is `real_run` at 42 against a cap of 60 |
  | bare `TODO` outside `src/` | R-CLEAN-05 | — | clean — every hit is a rejection-case string literal or the rule's own text |
  | bool declarations outside `src/` | R-CLEAN-03 | — | `sources_reached` was the only one; `find_clean03`'s pattern is `bool NAME[;=]`, so the `rule_*` function definitions are not subjects |

- **Two taste items from the 2026-09-22 review, handled rather than only recorded.** §The link's
  closing sentence claimed both unasserted behaviours were declared there; the over-long buffer is
  declared in §Goal, and the sentence now says so. The dangling ADR-0011 → §Plan pointer stays: an
  ADR body is immutable here. §Out of scope now names both instead of only the six from
  2026-09-17.

- **Acceptance: 23 of 23 pass.** `make test` **OK** in **2:05** (cap 3m), `make lint` two `ok:`
  lines, driver exit 0, `rejection cases: 64`, `false-positive cases: 23 (floor 23)`,
  `wiring cases: 10/10`, `neutered: 33/33`, `alternations: 64/64`, `accounting` 3 and 4 rule(s),
  `test_phase_docs.sh` 0, `test_rule_traceability.py` green after the three clause edits,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers, 0
  `tests/vectors` references under `src/`, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1, both ADR
  files, ADR-0007 naming ADR-0011. Both derivations recompute (64 `reject`, 23 `accept`). M4 is the
  validator's and was not run here.

- **The three pre-validation checks (`CLAUDE.md` §Conventions).** (1) Nothing found this round
  leaves the Goal unmet — the two fixes are in `tests/`, `src/core/` is untouched, and the suite is
  green. (2) `- base:` names `24d489f`, `git cat-file -t` says `commit`, `git diff --name-only
  24d489f` yields **37** files and none is under `.claude/workflow/installed`. (3) Each new sentence
  is founded on something other than the fix that wrote it: the runtime spread on five timed runs,
  four of which predate this round; the non-grep decision on the convention already at the top of
  that section; the three clauses on the vector files; the compliance decision on that same
  convention plus `CLAUDE.md`'s scoping of an order to what it names.


### Round 33 — four spec-side contradicts corrected, and the routing deviation that allowed it.

Closes validation 2026-09-22 (iteration 3): 4 `contradicts`, 1 `undecidable`. **Every finding was
introduced by round 32** — the round written to close round 31's findings.

- **Routing deviation, decided by the operator and recorded because nothing else would.**
  `/validate-phase` defines `contradicts` as an implementation failure and routes it to
  `/implement-phase`; and its iteration-3+ escape fired on the round count, moving the phase to
  `pending`. Both were overridden this round:
  1. **All four `contradicts` are spec-side.** The tree is correct; four sentences the phase wrote
     about the tree are false. The remedy applied is the one an `undecidable` gets — fix `spec.md`
     (and `docs/constraints.md`) plus this entry — never editing working code to satisfy a false
     sentence. `src/core/` is untouched this round.
  2. **The escape was reverted.** Status went back `pending` → `in-progress`. The escape counts
     rounds and reads nothing else, so it cannot distinguish structural failure from convergence on
     discrete typos. Findings here are spec-side, falling (5 → 4+1), and each is one false sentence.
  Both overrides are defects already logged in `~/.claude-belay/feedback/pico-sg2hid.md`, both
  `status: open`, both dated 2026-09-15 — the `contradicts` routing entry and the iteration-3+
  escape entry. A third, the step-5 `notes.md` contradiction, is logged `resolved (f001884)` yet is
  present in the installed text and was hit live this round. **No local patch was applied to
  `.claude/commands/validate-phase.md`**: belay is frozen here, and every local workaround this
  phase carries must be removed before the package can be updated, so adding another was refused.
  The correct route was executed by hand instead and documented here.

- **C1 + C2 — R-PROTO-02's scope clause was false in both halves, and is now measured.** The first
  wording claimed the check cuts "only a digital frame and only one byte short", and that
  `cut_drops_the_link_from_every_source` asserts "all four `LinkState` members". Enumerated by
  grepping every truncated span in `tests/ps2_codec_cases.cpp`: the check decodes **three** cut
  inputs — two at three bytes of a four-byte digital frame (`truncated_ack.h`,
  `truncated_not_ready.h`), and one at **one byte** (`kReadyIndex` is 1) of the twenty-byte
  undeclared-id frame `unknown_id.h`, which is where §Goal's precedence item 1 is asserted. The
  named function constructs **three** links, not four; `Absent` is covered elsewhere in
  `rule_proto02` by a fresh `Link`. Not exercised: a cut in an analog or config-mode frame, and
  every intermediate length.

  **The false half of C2 came from a code comment, which is why it survived.** The comment above
  `cut_drops_the_link_from_every_source` said "The four source states are the four LinkState
  members, so the claim is asserted over all of them". Round 32 copied that sentence into the
  clause instead of counting the constructors — in a spec paragraph asserting the clauses "were
  written from the vector files, not recalled". The comment is corrected in the same edit
  (R-CLEAN-06: a comment that no longer matches the code is corrected, never left standing).

- **C3 — "longest is `real_run` at 42 lines" was false; `real_run` is 8.** 42 was a `def`-to-`def`
  distance produced by a careless `awk`, not a function length. Re-measured by parsing the files:
  longest `.py` function is `reject` at **35** lines (`build_and_run` 32, `rule_verdicts` 14,
  `run_rejection_cases` 9, `real_run` 8, `main` 4, `fail` 3); longest shell function is `wiring( )`
  at **13**. The verdict the row carries — clean against a cap of 60 — survives; the measurement
  behind it did not.

- **C4 — off-by-one.** "Four violations … and one more class was checked and is clean" preceded a
  table with **two** clean rows. The sentence now refers to the table's rows rather than counting.

- **The `undecidable` — analog button indices — is a real gap and got a spec sentence.**
  `map_frame` gates on `reports_controls( id ) → Digital || Analog` and then reads
  `kButtonsLowIndex`/`kButtonsHighIndex` for both, but no spec sentence and no vector pinned analog
  button positions: `analog_idle.h` and `analog_whammy_full.h` assert the id and the whammy only.
  The fact existed solely as a code comment in `guitar_state.h`. §Goal now states it, and states
  that it is an unmeasured protocol assumption under the same `TODO(09-guitar-observe)` block as
  every other byte position.

- **Reconciliation owed by these amendments, and what was checked.** C1's fact is asserted in two
  places: R-PROTO-02's clause in `docs/constraints.md` and its restatement in spec.md §A check that
  is not a grep owes a clause too. Both were corrected in the same edit. Searched for further
  restatements of the same four facts across `spec.md` and `verify.md`: §Vectors' `unknown_id.h`
  row already described the one-byte cut correctly and needed no change; no other statement asserts
  a function length, a clean-row count, or analog button positions.

- **Acceptance: 23 of 23 pass.** `make test` **OK**, `make lint` exit 0, traceability exit 0,
  `test_phase_docs.sh` 0, driver exit 0, `rejection cases: 64`, `false-positive cases: 23
  (floor 23)`, `wiring cases: 10/10`, `neutered: 33/33`, `alternations: 64/64`, accounting 3 and 4
  rule(s), `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/`
  headers, 0 `tests/vectors` refs under `src/`, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1,
  both ADRs, ADR-0007 naming ADR-0011. Both derivations recompute (64 `reject`, 23 `accept`).

- **The three pre-validation checks (`CLAUDE.md` §Conventions).** (1) Nothing found this round
  leaves the Goal unmet — `src/core/` is untouched and green; the only non-doc edit is a corrected
  comment in a test file. (2) `- base:` names `24d489f`, `git cat-file -t` says `commit`,
  `git diff --name-only 24d489f` yields **37** files, none under `.claude/workflow/installed`.
  (3) Every replacement sentence is founded on a measurement taken this round by parsing or
  grepping the tree — the cut enumeration, the link-constructor count, the function-length table —
  and not on the fix that wrote it. This is the check round 32 claimed to perform and did not.


### Round 34 — the four findings round 33 created, and the rule adopted for the next one.

Validation 2026-09-22 (iteration 4) returned **2 `contradicts` + 2 `undecidable`**, and confirmed
all four of round 32's corrections as `measured clean` by independent re-measurement. **Three of
the four new findings were created by rounds 32 and 33's own fixes.**

- **The pattern, stated with the numbers rather than as a feeling.** Findings per iteration against
  the 90d7564 spec: **1 → 3 → 5 → 4**. It is not converging; it is rotating. Every finding in all
  four iterations was about a document this phase writes, and `src/core/` has not been touched
  since 2026-09-17. The loop is validating its own prose. This is the defect logged
  `status: open` on 2026-09-22 in `~/.claude-belay/feedback/pico-sg2hid.md`, measured there on this
  very phase — and this round is a fifth data point for it.

- **The rule adopted for this round and the next, an extension of round 31's property.** Round 31
  ruled that *a count* of a tree artifact stated in prose must be recomputed by a criterion or have
  its members enumerated, or the number goes. Generalised now to any assertion: **a factual claim
  about the tree that no check recomputes is preferentially deleted rather than corrected.**
  `/validate-phase` says the same in its routing — "deleting a statement the amendment leaves
  unfounded is a legitimate outcome and often the right one". Applied below: C2 was deleted, not
  re-measured.

- **C1 — "§Out of scope releases their clauses with their owner" asserted a release that did not
  exist.** Round 32's ruling (a non-grep binding owes a scope clause like any other) reaches six
  rules beyond the assignment table — R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02, R-PROC-01,
  R-PROC-02. Measured: none appears in the table's 14 rows, none appears in any §Out of scope
  bullet, and none carries scope text in `docs/constraints.md`. So round 32 created six unowned
  obligations and then claimed they were released. **Fixed by making the release real**: a new
  §Out of scope bullet naming all six and releasing them to `00-scaffold`'s owner. Not deletable —
  the obligation genuinely follows from a ruling that answered a real question.

- **C2 — a parenthetical written in round 33, false within the hour.** §Goal said
  "`analog_idle.h` and `analog_whammy_full.h` assert the id and the whammy only". They also assert
  `payload_matches` over all six payload bytes, and `case_announced_lengths` asserts
  `frame_len( Analog )`. **Deleted, not corrected**, under the rule above: the load-bearing claim —
  no case calls `map_frame` on an analog frame and asserts a button — is true, checkable, and
  stands without the parenthetical propping it up.

- **U1 — the shared-index assumption was a fifth concern counted as one of four.** §What this phase
  does not prove enumerated four unmeasured byte-position concerns; round 33 added a fifth to §Goal
  and claimed the existing `TODO(09-guitar-observe)` block covered it. Measured: the
  `guitar_state.h` marker scopes "every position and mask *in this section*" — the constants block
  — while the assumption lives in `map_frame`'s doc comment below it. It is also not subsumed by
  "button bit positions and masks": those are *which bits within a byte*, this is *which bytes, for
  which id*. Now counted as five, with the marker gap stated. **The marker count stays three** —
  it is an acceptance criterion and the three are correct as scoped; see §Debt.

- **U2 — an unusable SDK had a behaviour and no ruling.** `tests/test_style.sh` prints
  `note: no macOS SDK from xcrun…` and runs clang-tidy anyway, which fails R-STYLE-02, R-CLEAN-02
  and R-CLEAN-04 through `clang-diagnostic-error`. The script is this phase's (`b23b91a`), so the
  decision is this phase's and was never written. New subsection §An unusable SDK is a failure, not
  a skip: it is deliberately not a skip, because `OPTIONAL_TOOLS` exists for a *missing* tool while
  this is a *broken invocation*, and skipping would leave three rules unchecked on a machine that
  looks fully equipped — the green-line-over-a-violated-rule failure this phase has measured four
  times. No code changed.

- **Reconciliation.** C1's fact is asserted in §A check that is not a grep owes a clause too and in
  §Out of scope; both were edited together. U1's count is asserted in §What this phase does not
  prove and referenced by §Acceptance criteria and `verify.md` — checked both: they pin the
  **marker** count (three), not the concern count, so neither needed changing, and that was
  verified rather than assumed.

- **Acceptance: 23 of 23 pass** after the fixes. `make test` **OK**, `make lint` 0, traceability 0,
  `test_phase_docs.sh` 0, driver 0, and every count unchanged. Boundary sweep clean over 37 files.

### Round 35 — deletion as the fix, and a reconciliation I claimed and did not do.

Validation 2026-09-22 (iteration 5) returned **3 `contradicts` + 2 `undecidable`**. Findings per
iteration against the 90d7564 spec are now **1 → 3 → 5 → 4 → 5**.

- **A failure of mine, recorded because the record is what the next session trusts.** Round 34's
  entry contains the sentence: "U1's count is asserted in §What this phase does not prove and
  referenced by §Acceptance criteria and `verify.md` — checked both: they pin the **marker** count
  (three), not the concern count, so neither needed changing, and that was verified rather than
  assumed." **That verification did not happen for the statement that mattered.** Round 33 had
  written into §Goal that the shared-index assumption "is covered by the same
  `TODO(09-guitar-observe)` block"; round 34 then edited §What this phase does not prove to say the
  opposite — that no marker reaches it — and never returned to §Goal. The reviewer found it as
  `contradicts` 1. `verify.md:152` carried the same stale universal ("Every one of those spots in
  the code is marked `TODO(09-guitar-observe)`") and was missed for the same reason. Writing "that
  was verified rather than assumed" about an unperformed check is worse than omitting the sentence:
  it spends the next session's trust. The two statements are reconciled now, in the same edit.

- **The rule from round 34 was applied properly this time: three of the five were fixed by
  deletion.**
  - **C1 (§Goal vs §What this phase does not prove)** — the false half of the §Goal sentence
    deleted; what remains points at the section that counts the concern and records the marker gap.
  - **C2 (`verify.md` over-reaching universal)** — narrowed to the four spots it had just
    enumerated, with the fifth named as carrying no marker.
  - **C3 (the compliance table's last row) and U1 (its completeness claim)** — **the whole table is
    deleted.** Its last row claimed "every hit is a rejection-case string literal or the rule's own
    text", and by then the table's own line in `spec.md` was itself a hit: the sentence had become
    false by being written. That is the clearest possible instance of the class — a claim about the
    tree, kept in a document that is part of the tree, falsified by its own presence. What the spec
    keeps is the **ruling** (a released clause releases documentation, never compliance) and the
    disposition of what is still owed; the measured enumeration lives in round 32 above, recomputes
    from the tree, and is not copied here again.
  - **U1's other half** — R-CLEAN-03's subject (bool variables, or also bool-returning function
    names?) is genuinely undefined, so no completeness claim is made over that region at all. The
    open question is in §Debt with its upgrade path.
  - **U2 ("`make test` is unaffected")** — false, and corrected rather than deleted, because the
    surrounding ruling needs it: `tests/test_style.sh:133` sets `fail=1` unconditionally on a
    clang-tidy error and `make test` runs it with `OPTIONAL_TOOLS=1` (`Makefile:28`), which
    downgrades only a *missing* tool. What survives is R-PROC-04's promise, and the spec now says
    exactly that and no more.

- **`spec.md` shrank.** First decrease in this phase not produced by a re-expansion: the deletion
  removed the table and its measurement paragraph, and the replacements are shorter than what they
  replaced. Worth noting against the feedback entry that measures this phase's spec growing
  30KB → 74KB → 45KB → climbing.

- **Acceptance: 23 of 23 pass.** `make test` **OK**, `make lint` 0, traceability 0,
  `test_phase_docs.sh` 0, driver 0, markers 3, boundary sweep clean over 37 files, index rebuilt.

- **The three pre-validation checks.** (1) Nothing found this round leaves the Goal unmet —
  `src/core/` untouched, suite green. (2) `- base:` names `24d489f`, a real commit, 37 files, none
  package-owned. (3) Each replacement sentence is founded on a measurement taken this round —
  the marker scopes read out of `guitar_state.h`, `fail=1` read out of `tests/test_style.sh:133`,
  `OPTIONAL_TOOLS=1` read out of `Makefile:28` — and the deletions are founded on nothing, which is
  the point of deleting them.

### Round 36 — a taste item that was a defect, and three explanatory sentences deleted.

Validation 2026-09-22 (iteration 6) returned **3 `contradicts` + 1 `undecidable` + 1 taste**.
Findings per iteration: **1 → 3 → 5 → 4 → 5 → 4**.

- **A second failure of mine, of a different kind from round 34's.** C2 this round —
  R-PROTO-04's clause calling `0x80` "an end of the axis" — was reported to me in the **previous**
  round as a *taste* item, and I left it. It was never taste: `0x80` is the midpoint of the
  `0x00..0xFF` range `guitar_state.h` documents, §Vectors calls that same byte "centred", and the
  clause therefore contradicted another section of this phase's own spec while understating the
  gap it exists to state. **A finding filed as taste is still a measurement, and this one was
  checkable.** The lesson worth keeping: taste is a verdict about whether something *blocks*, not
  about whether it is *true*.

  Corrected rather than deleted, because a scope clause's whole job is to name the unseen region:
  it now says two bytes are exercised, `0x80` and `0xFF`, and that the unexercised region is every
  value below rest as well as every value between rest and full — which is strictly more than
  "between the two ends".

- **C1 — the justification was false; the conclusion was not.** §What this phase does not prove
  argued the fifth concern escapes the `guitar_state.h` marker because "those are which bits within
  a byte, this is which bytes". Measured: that section *opens* with two byte-index constants
  (`kButtonsLowIndex`, `kButtonsHighIndex`), so "which bytes" is inside the marker. What actually
  puts the assumption outside it is the section header — "Where each control lives in a **digital**
  payload" — so the byte indices are marked and their *analog* applicability is not. The false
  half is deleted and the true reason stated. Third time this phase has written an explanatory
  clause that was itself wrong while the claim it explained was right.

- **C3 — `docs/index/tests.md` does not list `README.md`.** §Context pointers said it did. The
  generator indexes code files only. §Context pointers is the one section the spec declares
  regenerated against the tree, so it is the one place this class of error is least excusable.
  Corrected to say the index carries the ten headers and not the README, and why.

- **U1 — narrowing the PHASES.md acceptance column: bookkeeping or an edit of a cut?** The row went
  from `R-PROTO-01..04 move to test:` to `R-PROTO-02..04`, and `CLAUDE.md` says a wrong cut is
  superseded by new rows and never edited — but never says whether the acceptance column is part of
  the cut. Ruled in §Out of scope: **bookkeeping.** The row's *goal* defines the cut and is
  untouched; R-PROTO-01 left the column because it was rebound to `03-pio-bus`, where it always
  belonged (LSB-first and SPI mode 3 are properties of how the bus shifts bits, and nothing in
  `src/core/` drives a clock). No work moved between phases and no goal changed, so there is no
  superseded cut to record. A narrowing of the *goal* would be a re-cut and would go to
  `/plan-feature`.

- **Taste closed rather than deferred**, this time deliberately: `verify.md` said "each of the four
  spots is marked" eight lines above "expect three of them". Both were true — the whammy's index
  and its rest value share a marker — and it read as a mismatch to an operator. One clause added.

- **What the reviewer measured clean is worth recording, because it is the part that is not
  rotating.** It reproduced `verify.md` §3 end to end: copied the tree, applied the
  `std::nullopt → ControllerId::Digital` mutation, ran the driver, and got exactly the six FAIL
  lines in the order and wording `verify.md` predicts, ending `rejection cases: 2/3`, exit 1. It
  also re-derived both case distributions member by member, all 14 assignment-table rows, the
  refusal order against `ps2_frame.cpp`, both enum memberships, and every acceptance number. The
  codec and its harness have been stable for six iterations; every finding in all six was about
  prose.

- **Acceptance: 23 of 23 pass.** `make test` **OK**, `make lint` 0, traceability 0, driver 0,
  `test_phase_docs.sh` 0, markers 3, sweep clean over 37 files.

## Debt

- **R-CLEAN-03's subject is undefined: bool variables, or also bool-returning function names?**
  The rule's text is "Boolean names are assertions: `is_`, `has_`, `can_`, `should_`". Its check,
  `find_clean03`, matches `bool NAME[;=]` — variables only. Under the wider reading,
  `payload_matches` and `report` in `tests/ps2_codec_cases.cpp`, and `reports_controls` in
  `src/core/guitar_state.cpp`, are bool-returning names with no assertion prefix. Raised by the
  independent review on 2026-09-22 as an `undecidable`. Not closed here: deciding it either
  narrows a rule `00-scaffold` bound, or renames three functions across two layers to satisfy a
  reading nobody has chosen. `spec.md` §Out of scope therefore makes no completeness claim over
  that region. Upgrade path: whichever phase next edits R-CLEAN-03's text settles the subject in
  the same edit, and the spelling clause that rule already owes is the natural place for it.

- **The fifth unmeasured concern has no `TODO(09-guitar-observe)` marker.** That a digital and an
  analog frame carry the buttons at the same two payload indices is an unmeasured protocol
  assumption (spec §What this phase does not prove), and none of the three markers covers it: the
  `guitar_state.h` constants marker scopes "every position and mask in this section" and the
  assumption is in `map_frame`'s doc comment below that section. Not closed here because the marker
  count is an acceptance criterion (`expect: 3`) and all three are correctly scoped as written;
  moving or widening one to reach the doc comment would change a checked number to fix an unchecked
  gap. Upgrade path: `09-guitar-observe` confronts the assumption with the real device like every
  other byte position, and whichever phase next edits `guitar_state.h` can add a fourth marker and
  raise the criterion in the same edit.

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
- **Two behaviours the spec now mandates are asserted by nothing, and that is a pattern
  rather than two accidents.** Both arrived the same way: a validation round asked what the
  code does in a case no vector covers, the answer was written into the spec as a contract,
  and no case followed it. A mandate with no assertion is exactly what this phase keeps
  discovering one round later, so they are named together:
  1. `us_in_state` saturates at `UINT32_MAX` (below).
  2. **An over-long buffer is accepted and its extra bytes ignored** (§Goal). Asserting it
     needs an input longer than any vector, and every vector is a literal by R-PROTO-05, so
     the case would have to build its input from a vector plus padding — defensible, since the
     *expected* bytes would still come from the vector, but it is a new shape of test and this
     round was scoped to contracts. `03-pio-bus` is the natural owner because it is the first
     phase with a real caller — not because the buffer's shape is known, which it is not.

- **`us_in_state`'s saturation is specified and asserted by nothing.** `spec.md` §The link now
  mandates that the accumulator saturate at `UINT32_MAX` rather than wrap, and `add_saturating`
  in `src/core/link.cpp` implements it, but no case exercises the boundary — reaching it needs a
  caller that stops polling for over 71.6 minutes and then resumes. Declared rather than papered
  over, because an unasserted mandate is exactly what this phase has spent rounds discovering.
  Upgrade path: one case that steps twice with `elapsed_us` at `UINT32_MAX` and asserts the
  counter did not roll; cheap, and deliberately not taken this round, which was scoped to
  pointers.
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

### Owed by `00-scaffold`, found 2026-09-22 by round 32's enumeration

`reject( )` and `accept( )` in `tests/test_repo_shape.sh` each take four positional parameters
against R-CLEAN-02's "at most 3". Both were written by `00-scaffold` (`c07f525`); R-CLEAN-02's
check is clang-tidy, which reads no shell, so nothing will ever report them. They are real
violations under the rule decided in round 32 — a released clause releases documentation, not
compliance — and they belong to whichever phase next owns that file, not to this one. The remedy
in shell is not a struct: it is one variable assignment per case before the call, or a here-doc.

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
  check expected bytes (R-PROTO-05). The ten vectors are the independent reference; a
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
- **Whoever owns the phase workflow, and `/belay-feedback`** — a Plan step that lands keeps its
  `Owed` marker, and nothing in the loop notices. It happened twice in this phase: step 14 in
  round 17 and step 17 in round 26, both found by the operator reading the spec, neither by a
  gate. **The cause is structural, not forgetfulness: the marker lives in `spec.md` §Plan while
  the work lands in another file, and `/implement-phase`'s mandatory final step writes
  `notes.md` and `PHASES.md`'s status and never revisits the Plan.** The step's own *check* does
  get run — it is the criterion — but the marker is prose beside the check, not the check. Two
  upgrade paths, one cheap and one mechanical: have the command's final step name the marker
  alongside the notes it already writes, or bind a check that greps `spec.md` for `Owed` steps
  whose check command passes. The first is `/implement-phase`'s and therefore upstream; the
  second is this repo's and belongs with the other check-harness work below.

- **Whoever next touches the check harness** — make the "no constant escapes the spec's list"
  property executable, if it is wanted. A check that greps `^constexpr` out of
  `src/core/ps2_protocol.h` and compares the names against the row in that phase's `spec.md`
  would turn a sentence nobody can enforce into a gate. It was considered and rejected **in**
  `01-ps2-codec` on 2026-09-15 for one reason only: adding a check during a phase's closing
  round means new machinery arriving when there is no budget left to prove it live, and this
  repo's standard is that a check which has not been mutated is not known to work. The phase
  deleted the claim instead. Bundle it with the `clang-query` upgrade and the "no new untracked
  paths after `make test`" check named below, and with the `.py`-mutation debt: four pieces of
  harness work with one owner between them.

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

- **Whoever next touches `tests/test_repo_shape.sh` or `00-scaffold`'s bindings** — seven
  grep-bound rules owe a spelling clause by `spec.md` §What a `test:` binding does and does not
  promise and were **not measured** in `01-ps2-codec`: R-ARCH-01, R-ARCH-03, R-CLEAN-03,
  R-CLEAN-05, R-CLEAN-09, R-ERR-03, R-ERR-04. The method is cheap and is in round 28: extract
  the finder, feed it one line per spelling, record what it misses in the rule's own text.

- **Whoever next touches `.clang-tidy`** — `HeaderFilterRegex: 'src/.*'` is matched against the
  absolute path, so lint's reach into `tests/` headers depends on whether the checkout path
  contains `src/` (measured 2026-09-16, R-CLEAN-04's text). It reaches R-STYLE-02 and R-CLEAN-02
  the same way. `ExcludeHeaderFilterRegex` is supported by the installed LLVM 23.1.0 and would
  make the vectors' exclusion path-independent; nothing was changed because nothing asked for it.

- **Operator — `CLAUDE.md` §Conventions check 1** says a green rule line over a violated rule was
  "measured four times here". Round 28 measured a fifth (the prefix check). The file is the
  operator's and was not edited.

- **Taste from validation 2026-09-21, recorded and not fixed (step 5):** `spec.md` §clang-tidy's
  header filter says `make test` "goes from about 1:40 to about 2:30", while
  `verify.md:178-180` tells the operator that runs land "between 1:40 and 2:20". Two files in one
  diff give different prose ranges for the same measurement (the reviewer measured 2:06.86, inside
  both). The acceptance criterion is `real < 3m` and passes either way. Also: §Files this phase
  writes describes the `tests/test_repo_shape.sh` row as "the checks behind R-ERR-01, R-ERR-02 and
  R-CLEAN-04", while that file also carries R-PROTO-05's re-scoped finder and the three new accept
  cases — an incomplete summary rather than a wrong one, both accounted for elsewhere in the spec.

- **Taste from validation 2026-09-17, recorded and not fixed (step 5).** Six items; the first
  is the one that looks like debt rather than preference:
  1. `case_report_is_wide_enough` cannot fail — `kButtonBytes` is *derived* from `kButtonCount`
     and `kBitsPerByte`, so `kButtonBytes * kBitsPerByte >= kButtonCount` and
     `kReportLen == kButtonBytes + 1` restate the definitions. Its comment claims it fails if
     `kButtonCount` grows past what `kButtonBytes` covers, which the arithmetic forbids. Same
     shape as the green paint §Plan step 13 rejected for `rule_proto04`.
  2. `src/core/hid_report.h`'s "adding an eleventh button widens the report" holds only if
     `kButtonCount` is bumped with the enumerator; nothing ties the constant to `Button`, and the
     loop in `case_report_gives_every_button_its_own_bit` iterates to `kButtonCount`. What would
     catch an unbumped enumerator is `-Wswitch` in the test's `state_with`, not the report width.
  3. `case_analog_whammy_full`'s comment says reading the wrong index yields `kWhammyRest`; true
     only of the three other *axis* indices — payload indices 0 and 1 hold `0xFF`, which satisfies
     both halves of that case. `case_analog_idle_whammy_is_rest` is what rejects those two.
  4. Eight of the ten controls are never asserted *pressed* anywhere: a mapper that swapped
     `kMaskStart`/`kMaskSelect`, or read tilt out of the low byte, passes every line. Consistent
     with §Vectors as written, and the positions are `TODO(09-guitar-observe)` guesses.
  5. `tests/test_style.sh:22-24` still points at `.clang-tidy` "for the upgrade path" to a
     `compile_commands.json`, while `.clang-tidy`'s header now opens that section with "**NOT** a
     generated compile_commands.json".
  6. §Context pointers calls `tests/test_repo_shape.sh` "a reject/accept/wiring case per rule";
     R-ERR-03, R-ERR-04 and R-PROTO-05 have reject and wiring cases but no accept case.

- **Taste from validation 2026-09-16, recorded and not fixed (step 5):** `kButtonCount = 10` is a
  literal not tied to `enum class Button`, though `spec.md` says the layout constants are "derived
  from each other"; `tests/vectors/unknown_id.h` cites a section of the spec ("§Goal's
  precedence") from a test file; `rule_proto02`'s comment narrates its clauses as a history
  ("Three claims… fourth… fifth… sixth") rather than stating what it asserts now.

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
  **[Corrected 2026-09-15: the reviewer returned SEVEN, not six. This list omitted
  `tests/test_repo_shape.sh`'s accept floor, which is item (7) below and was described in
  §Deviations round 8 but never numbered here; the item labelled "a seventh" at the end is the
  eighth. The miscount was repeated to the operator. Corrected in place rather than quietly,
  the way round 2's reconciliation miss was.]**
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
  exit 0;
  (7) `tests/test_repo_shape.sh`'s false-positive floor, moved 13 → 20, which the spec never
  fixes — it pins the wiring count and nothing else.
  An eighth of the same class was found by this gate rather than the reviewer: §Plan
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

## Validation — 2026-09-15 (round 2 against the re-expanded spec)

Iteration 2 since the `escaped to /expand-phase` verdict of 2026-09-11. The escape does not
fire.

- **file set, corrected before any gate ran.** `notes.md` §Outcome said `- base: working tree`,
  true while the phase had no branch. Round 9 committed the work to `feat/01-ps2-codec`, so the
  working tree is empty and that literal would have produced a file set of **zero files** — the
  boundary sweep, the independent review and the closure test all reporting `pass` having looked
  at nothing, which is the failure mode this command's own file-set section warns about. The
  base is now `24d489f`; `git diff 24d489f` against the working tree gives 34 files, and no path
  in `.claude/workflow/installed` falls inside it.
- criteria: **20 passed / 0 failed.** `make test` **OK** in **2:07** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 9 vectors, 0 stray `tests/` headers,
  0 / 0 for the two `src/` greps, **3** `TODO(09-guitar-observe)` markers against the newly
  pinned `3`, both ADRs, driver exit 0, accounting 3 and 4 rule(s), `wiring cases: 10/10`,
  **`false-positive cases: 20 (floor 20)`** against the newly pinned floor, `neutered: 33/33`,
  `alternations: 64/64`, `test_phase_docs.sh` 0. The two backreference greps ran under
  `/usr/bin/grep`. M1-M3 are the driver's rejection cases (`3/3`); **M4 ran with the corrected
  instruction** — a `cp -a` copy, which carries both `.git` and the uncommitted work — and
  produced exit 1 with `FAIL: test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04,
  reported by nothing but its own cases — the real run is gone`.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (34 files, 9 under `src/core/`; 13 active `deny` rules). Passed one
  path per argument through `xargs` per §Notes to `/validate-phase` note 2, and proved live
  before being recorded: `#include "src/hal/bus_io.h"` in `src/core/link.h` is caught as
  `deny core -> hal`, and the file was restored identical.
- index: **STALE on entry** (built at `24d489f`, HEAD now `5b04885`), rebuilt; fresh at `5b04885`.
- independent review: **contradicts: 1** — §Files' `ps2_protocol.h` row declares its constant
  list exhaustive; the header declares **twenty** `constexpr`s and the row names **eleven**.
  Measured, not taken on the reviewer's word: missing are `kIdDigital`, `kIdAnalog`, `kIdConfig`,
  `kHeaderIndex`, `kReadyIndex`, `kPayloadIndex`, `kPrefixLen`, `kPayloadNibbleMask`,
  `kBytesPerNibble`. The sentence was added by round 9 to close the `kPadByte` pointer and was
  never checked against the header. **undecidable: 3** — (1) whether an over-long buffer is
  well-formed input: `decode` refuses only `bytes.size() < expected_len`, so trailing bytes are
  silently ignored, and nothing in the spec or the vectors decides it; (2) the precedence of
  §Goal's `NotReady` and `AckTimeout` bullets, both written unconditionally, where `decode`
  checks the ready byte first and so reports `NotReady` for a frame that is both — which
  decides what `FaultCause` trace mode prints; (3) the driver's hand-copied `CXXFLAGS`, which
  the spec never states, so nothing can tell whether the duplicate is faithful. Three taste
  items are in §Deviations, unfixed.
- closure test: **fail** — three `undecidable` findings are three missing pointers. The rest
  passes: four `notes.md` sections present and non-empty, every file in the set reachable, no
  untracked path, and every quantified claim in the spec now carries its enumeration (the nine
  vectors, ten controls, four concerns, three markers, four `LinkState`, five `FaultCause`, two
  clang-tidy flags) — which is what the round-9 work bought and what the `contradicts` above is
  the exception to.
- upstream: **none** — no path in `.claude/workflow/installed` is in the phase's file set. The
  `contradicts` routing defect recorded in round 9 stands: this round's `contradicts` is again a
  spec sentence rather than a code fault, and the command would again route it to
  `/implement-phase` as a code bug. `/belay-feedback` is owed for that and for the two
  workarounds in §Notes to `/validate-phase`.
- verdict: **returned to implementation**

## Validation — 2026-09-15 (round 3 against the re-expanded spec — escape threshold)

Iteration 3 since the 2026-09-11 `escaped to /expand-phase` verdict. A gate failed, so the
iteration-3+ escape fires.

- **file set, checked first per `spec.md` §Notes to `/validate-phase` note 3.** `- base:`
  names `24d489f`, confirmed a real commit by `git cat-file -t`. The file set against the
  working tree is **35 files**, no path from `.claude/workflow/installed` inside it. Not empty,
  so the gates below measured something.
- criteria: **19 passed / 1 failed.**
  **FAILED — `grep -rn 'tests/vectors' src/ | wc -l` → `1`, expect `0` (R-PROTO-05).** The hit
  is `src/core/ps2_frame.cpp:22`, a comment reading "See tests/vectors/truncated_not_ready.h"
  added in round 11. `make test` passes on the same tree: `find_proto05` goes through `hits`,
  which strips `//` comments before grepping, so the rule's bound check cannot see a reference
  in a comment while the criterion can. See §Deviations round 14 — the criterion and the check
  encode two different readings of the same rule, and the spec does not say which is meant.
  The other nineteen pass: `make test` **OK** in **2:09** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, **10** vectors, 0 stray `tests/` headers,
  0 `.value()`, **3** `TODO(09-guitar-observe)` markers, both ADRs, driver exit 0, accounting
  3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **not run** — step 5 and the sweep are gated on steps 1-4 being clean, and
  step 1 failed. The last sweep, 2026-09-15 round 2, was clean over 34 files and proved live.
- index: **STALE on entry** (built at `5b04885`, HEAD `7ef3ecf`), rebuilt; fresh at `7ef3ecf`.
- independent review: **not dispatched.** Step 5 says to review only once 1-4 are clean, and
  never to review code the cheap gates already reject.
- closure test: **fail** — a failing acceptance criterion means the phase's own executable
  definition of done is not met. The record itself is in order: four `notes.md` sections
  present and non-empty, 35 files all reachable from §Context pointers, §Files this phase
  writes or §Plan, no untracked path, and every quantified claim carrying its enumeration.
- upstream: **none** — no path in `.claude/workflow/installed` is in the phase's file set. The
  `contradicts`-routing defect recorded in rounds 9 and 10 is still owed to `/belay-feedback`,
  along with the three workarounds in §Notes to `/validate-phase`.
- verdict: **escaped to /expand-phase: spec re-expanded.** Status moved `in-progress` →
  `pending` by this command.

### Escape OVERRIDDEN by the operator, 2026-09-15 — deliberate, and recorded so it can be judged

The re-expansion was **not** run. The status was returned to `in-progress` and the one finding
was fixed in place. This is a decision, not a gate somebody skipped, and it is written here so
that if the next round fails on something structural, this paragraph is what tells that session
the override was the mistake.

**Why.** The escape counts rounds and reads nothing else, so it cannot tell three rounds of
structural failure from three rounds converging on a typo. Its premise is "if you're on
iteration 3+, the spec is wrong". The evidence says this spec is not:

| validation, since the 2026-09-15 escape | `undecidable` | `contradicts` |
|---|---|---|
| #1 | 5 | 0 |
| #2 | 3 | 0 |
| #3 | 0 | 1 — a section reference |

The round-3 reviewer returned `undecidable: none` explicitly, after checking every quantified
claim in the spec against the diff. The count was right, `verify.md` was right; only the
spec's pointer to which section held the count was wrong.

**And re-expanding has a cost measured in this phase, not a hypothetical one.** Every
re-expansion here introduced at least one sentence founded only on the amendment that wrote it,
which the following round returned as a finding — rounds 19, 20 and this one are all that class.
Regenerating a spec that has just passed a full audit, to fix one token, is the worst
cost-benefit trade this phase has been offered.

**Filed as a package gap:** the escape should be able to read the trend in front of it and not
only the round count.

> **Superseded 2026-09-15 by validation round 4 — this override was a mistake, on its own
> terms.** The test written above was "if the next round fails on something structural, this
> note says the override was the mistake". The next round failed on something structural:
> R-CLEAN-04's undeclared file scope, with the spec's own audit table asserting the opposite.
> The escape was right and the round count was not the reason it was right — the reason is that
> the spec answered "which rules declare their scope" by enumerating them, and the enumeration
> had been falsified twice already when this override was written. That was visible at the time
> and was not weighed.
> **The iteration count in the record above is also wrong.** The reset is tied to an escape
> *producing a new spec*, not to the verdict being written; this one produced none, so the
> rounds that followed were iterations 4, not 1. `/belay-feedback` owes it along with the `contradicts` routing, the
`- base:` staleness trap and the operator-edit gap in the file set.


## Validation — 2026-09-15 (round 1 against the second re-expanded spec)

Iteration 1: zero `## Validation` sections follow the 2026-09-15 `escaped to /expand-phase`
verdict, so the escape is not in play.

- **file set, checked first per §Notes to `/validate-phase` note 3.** `- base:` names `24d489f`,
  confirmed a real commit; the set against the working tree is **35 files**; no path from
  `.claude/workflow/installed` inside it.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **2:06** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, **10** vectors, 0 stray `tests/` headers,
  **`grep -rn 'tests/vectors' src/` → 0** (the criterion that failed validation #3), 0
  `.value()`, **3** `TODO(09-guitar-observe)` markers, both ADRs, driver exit 0, accounting 3
  and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  **`rejection cases: 64`**, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
  M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand** in a `cp -a` copy, exit 1
  with `FAIL: test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by nothing
  but its own cases — the real run is gone`.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (35 files, 9 under `src/core/`; 13 active `deny` rules). Passed one
  path per argument through `xargs`, and proved live before being recorded: a
  `#include "src/hal/bus_io.h"` in `src/core/hid_report.h` is caught as `deny core -> hal`, and
  the file was restored identical.
- index: **STALE on entry** (built at `7ef3ecf`, HEAD `9e0dba7`), rebuilt; fresh at `9e0dba7`.
- independent review: **contradicts: none.** **undecidable: 5** — (1) `find_err02` recognises
  three return-type spellings while R-ERR-02 says "result struct", which the spec never defines;
  `id_from_byte`'s `std::optional<ControllerId>` matches none of them, so the binding owes a
  second scope clause beside the file-scope one it already has; (2) the `planned: 03-pio-bus`
  → `4` criterion lost its enumeration in the re-expansion — the four are R-SAFETY-07,
  R-PROTO-01, R-PROTO-06, R-ERR-05; (3) `rejection cases: 64` and `false-positive cases: 20`
  are counts whose base totals the spec never states, and step 14 defines the 64
  self-referentially as "one more than before step 14"; (4) `unknown_id.h` is 4 bytes while its
  header announces a 20-byte frame, so it is cut short as well as undeclared and its comment
  asserts the opposite — the outcome is unaffected but the spec fixes the vector's header and
  outcome and never its length; (5) §Plan step 10's check still predicts "and no rule line",
  which step 15 made false — step 12's identical sentence was reconciled and step 10's was not.
  Three taste items recorded above, unfixed.
- closure test: **fail** — five `undecidable` findings are five missing pointers. Everything
  else passes: four `notes.md` sections present and non-empty, all 35 files reachable, no
  untracked path, and every quantified claim in the spec carrying its enumeration except the two
  named in findings 2 and 3, which is what those findings are.
- upstream: **none** — no path in `.claude/workflow/installed` is in the phase's file set.
  `/belay-feedback` is still owed for the `contradicts`-routing defect (rounds 9-10), the
  `- base:` staleness trap, and the three workarounds in §Notes to `/validate-phase`.
- verdict: **returned to implementation** — as spec amendments plus this Deviations entry, not
  as code changes. Status stays `in-progress`.

## Validation — 2026-09-15 (round 2 against the second re-expanded spec)

Iteration 2: one `## Validation` section follows the 2026-09-15 `escaped to /expand-phase`
verdict, so the escape is not in play.

- **file set, checked first per §Notes to `/validate-phase` note 3 and CLAUDE.md's check 2.**
  `- base:` names `24d489f`, confirmed a real commit by `git cat-file -t`; the set against the
  working tree is **36 files**, one more than last round because the lengthened vector counts
  as a further modification; no path from `.claude/workflow/installed` inside it.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **1:59** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` **4**, **10** vectors, 0 stray `tests/`
  headers, `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, **3** markers, both ADRs, driver
  exit 0, accounting **3** and **4** rule(s), `wiring cases: 10/10`,
  `false-positive cases: 20 (floor 20)`, `rejection cases: 64`, `neutered: 33/33`,
  `alternations: 64/64`, `test_phase_docs.sh` 0. **The two derivations round 19 added check
  out:** `grep -cE '^reject '` → 64 and `grep -cE '^accept '` → 20, equal to what the run
  prints. M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand** in a `cp -a` copy,
  exit 1 with `FAIL: test_ps2_codec.py declares R-PROTO-02, R-PROTO-03, R-PROTO-04, reported by
  nothing but its own cases — the real run is gone`.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (36 files, 9 under `src/core/`; 13 active `deny` rules). One path
  per argument through `xargs`, and proved live before being recorded: a
  `#include "src/hal/bus_io.h"` in `src/core/ps2_protocol.h` is caught as `deny core -> hal`,
  and the file was restored identical.
- index: **STALE on entry** (built at `9e0dba7`, HEAD `2fe3788`), rebuilt; fresh at `2fe3788`.
- independent review: **contradicts: none** — the second consecutive round with none; the
  reviewer walked all fifteen Plan steps, found a faithful hunk for each, and reconciled the
  derived counts against the diff. **undecidable: 3** — (1) `CLAUDE.md` carries 16 added lines
  in the phase's diff while the spec names it only as a file to read, never as one this phase
  writes, and it is not among the four exempt workflow-written paths; (2) R-ERR-02's second
  scope clause in `docs/constraints.md` is named by no Plan step, and two statements that call
  R-ERR-02 already-declared went stale when round 19 added it; (3) §Vectors fixes a byte shape
  for nine vectors and none for `unknown_id.h`, which round 19 lengthened to 20 bytes with the
  reasoning recorded in the file instead of the table. Three taste items in §Deviations.
- closure test: **fail** — three `undecidable` findings are three missing pointers. Everything
  else passes: four `notes.md` sections present and non-empty, no untracked path, and every
  quantified claim carrying its enumeration — including the four counts round 19 fixed, which
  this round's reviewer verified by arithmetic against the diff rather than by trusting them.
- upstream: **none** — no path in `.claude/workflow/installed` is in the phase's file set.
  `/belay-feedback` still owed for the `contradicts`-routing defect, the `- base:` staleness
  trap, and the three workarounds in §Notes to `/validate-phase`.
- verdict: **returned to implementation** — as spec amendments plus the Deviations entry above,
  not as code changes. Status stays `in-progress`.

## Validation — 2026-09-15 (round 3 against the second re-expanded spec — escape threshold)

Iteration 3 since the 2026-09-15 `escaped to /expand-phase` verdict. A gate failed, so the
iteration-3+ escape fires.

- **file set, checked first.** `- base:` names `24d489f`, confirmed a real commit; the set
  against the working tree is **36 files**; no path from `.claude/workflow/installed` inside it.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **2:03** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, both ADRs, driver exit 0,
  accounting 3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `rejection cases: 64`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
  Step 16's check passes: `test_rule_traceability.py` exit 0 and R-ERR-02 names both
  narrowings. M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand**, exit 1 with
  the expected message.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (36 files, 9 under `src/core/`; 13 active `deny` rules). One path
  per argument through `xargs`, proved live: `#include "src/hal/bus_io.h"` in
  `src/core/guitar_state.h` is caught as `deny core -> hal`, file restored identical.
- index: **fresh without rebuilding** — `3e2e6ed` touched no source files.
- independent review: **contradicts: 1** — §Goal says the marker count is "the number
  `verify.md` §1 tells the operator to expect"; `verify.md` §1 is `## 1. Everything passes`
  and is about `make test`/`make lint`, while the count sits at line 166 under the unnumbered
  `## What this phase does NOT prove`. The count itself is correct and so is `verify.md`; the
  spec's pointer is not. **undecidable: none** — stated plainly by the reviewer after checking
  every quantified claim in the spec against the diff. Four taste items in §Deviations.
- closure test: **fail** — a `contradicts` means the spec asserts something about the diff that
  the diff does not contain. The record is otherwise in order: four `notes.md` sections present
  and non-empty, all 36 files accounted for, no untracked path.
- upstream: **none** — no path in `.claude/workflow/installed` is in the phase's file set.
  `/belay-feedback` still owed for the `contradicts`-routing defect, the `- base:` staleness
  trap, the operator-edit gap in the file set, and the three workarounds in §Notes to
  `/validate-phase`.
- verdict: **escaped to /expand-phase: spec re-expanded.** Status moved `in-progress` →
  `pending` by this command.

## Validation — 2026-09-15 (round 4 against the second re-expanded spec)

**Iteration 4**, not 1. The counter resets at an `escaped to /expand-phase` verdict because
"the escape's own output is a new spec"; the 2026-09-15 escape was overridden and produced no
new spec, so the reset does not apply and the three rounds before this one still count.

- **file set, checked first.** `- base:` names `24d489f`, a real commit; 36 files against the
  working tree; no path from `.claude/workflow/installed` inside it.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **2:07** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, both ADRs, driver exit 0,
  accounting 3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `rejection cases: 64`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
  Round 23's fix re-verified by reading `verify.md` itself: the section carrying the marker
  count is "What this phase does NOT prove", and that is what §Goal now names. M1-M3 are the
  driver's rejection cases (`3/3`); **M4 run by hand**, exit 1 with the expected message.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (36 files, 9 under `src/core/`; 13 active `deny` rules). One path
  per argument through `xargs`, proved live: `#include "src/hal/bus_io.h"` in
  `src/core/link.cpp` is caught as `deny core -> hal`, file restored identical.
- index: **fresh without rebuilding** — `37614b4` touched no source files.
- independent review: **contradicts: 1** — the spec's audit table places **R-CLEAN-04** in
  "Narrower than its text, and the text says so", but R-CLEAN-04's text records only the
  `const`-initializer gap and no file scope, while its check runs through the same
  `tidy_sources()` (`src/core/*.cpp`, `src/core/*.h`, `tests/*.cpp`) that the spec's row 1 uses
  to fault R-STYLE-02 and R-CLEAN-02. By the spec's own criterion R-CLEAN-04 belongs in row 1;
  the table says row 2. R-CLEAN-04 is a binding this phase owns (§Plan step 9), so the
  convention obliges the clause. A second undeclared narrowing on the same rule:
  `.clang-tidy`'s `HeaderFilterRegex` moved `(src|tests)/.*` → `src/.*`, which the spec binds
  only through step 11's criterion and never in the rule text. **undecidable: none** — the
  reviewer verified every claim the spec makes about another file, including the one round 23
  fixed, `verify.md` §3's six predicted `FAIL:` lines in order, the case labels quoted by Plan
  steps 5 and 12, the 64/20/10 distributions summing exactly, and both `docs/index/` line
  counts. Three taste items in §Deviations.
- closure test: **fail** — a `contradicts`, and the spec's self-contradiction is the finding.
- upstream: **none** in the file set. `/belay-feedback` owes the `contradicts` routing, the
  `- base:` staleness trap, the operator-edit gap, the escape reading round count rather than
  trend, and the three workarounds in §Notes to `/validate-phase`.
- verdict: **escaped to /expand-phase: spec re-expanded.** Status moved `in-progress` →
  `pending` by this command. Fired rather than reported because the 2026-09-15 override
  recorded its own test — "if the next round fails on something structural, this note says the
  override was a mistake" — and this round failed on something structural.

## Validation — 2026-09-16 (round 1 against the spec re-expanded at def180c)

Iteration 1: zero `## Validation` sections follow the 2026-09-15 `escaped to /expand-phase`
verdict, and that escape **did** produce a new spec, so the reset applies.

- **file set, checked first.** `- base:` names `24d489f`, a real commit; 36 files against the
  working tree; no path from `.claude/workflow/installed` inside it.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **2:07** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, both ADRs, driver exit 0,
  accounting 3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `rejection cases: 64`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
  §Plan step 17's own check returns **1** and `test_rule_traceability.py` exits 0. M1-M3 are the
  driver's rejection cases (`3/3`); **M4 run by hand**, exit 1 with the expected message.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (36 files, 9 under `src/core/`; 13 active `deny` rules). One path
  per argument through `xargs`, proved live: `#include "src/hal/bus_io.h"` in
  `src/core/guitar_state.cpp` is caught as `deny core -> hal`, file restored identical.
- index: **fresh without rebuilding** — `2e9e987` touched no source files.
- independent review: **NOT RUN — the subagent terminated on a session rate limit before
  producing a verdict** (`rate_limit`, HTTP 429; the limit resets 00:50 America/Santiago).
  It returned no `contradicts`, no `undecidable` and no taste list. **Nothing is inferred from
  its partial output and nothing is recorded in its place.**
- closure test: **not decidable this round.** Its mechanical half passes — four `notes.md`
  sections present and non-empty, 36 files accounted for, no untracked path, no `Owed` marker
  left in the Plan, every quantified claim carrying its enumeration or derivation. Its other
  half is the review, which did not run: an `undecidable` finding is defined as a missing
  pointer *found from outside your own head*, and there was no outside this round.
- upstream: **none** in the file set. `/belay-feedback` owes the `contradicts` routing, the
  `- base:` staleness trap, the operator-edit gap, the escape reading round count rather than
  trend, the `Owed`-marker gap named in §For later phases, and the three workarounds in
  §Notes to `/validate-phase`.
- verdict: **none of the three.** Not `done`: the gate that has returned a real finding in each
  of the last several rounds — including the one that fired the escape — did not run, and
  self-declaring the closure test on the round that would close the phase is the worst round to
  do it. Not `returned to implementation` and not `escaped`: nothing failed. Status stays
  `in-progress` pending the review, which the command's step 5 offers two ways to obtain — the
  operator pastes a three-input review from a fresh session, or this one is re-dispatched after
  the limit resets.

### Addendum 2026-09-16 — the independent review, obtained after the rate limit

- independent review: **contradicts: 5** — (1) the assignment table read "no" for R-CLEAN-04's
  scope clause, which the catalogue carries; (2) §Plan said steps 10-11 were the only work owed
  with 12-17 landed; (3) "two package workarounds" titled three; (4) step 10's check claimed any
  source-state mutation fails `R-PROTO-02`, true only for `AckTimeout`; (5) `find_err01` misses
  `static`/`inline` returns and R-ERR-01's text does not say so. **undecidable: 3** — (1) four
  §The link behaviours neither asserted nor declared unasserted; (2) claims about `notes.md`
  files withheld from the reviewer; (3) `HeaderFilterRegex` unanchored, low confidence, not
  verified. Three factual comment errors and three taste items listed apart.
- **How it was obtained, stated because it limits the next one.** It ran with exactly the three
  inputs step 5 prescribes and nothing else — `notes.md` was not opened. It ran in the session
  that then implemented round 28, so that session holds this phase's context and **must not**
  produce the next review; the next one is a fresh agent or the operator's fresh session.
- closure test: **fail** — five `contradicts`, and `undecidable` 1 and 3 are missing pointers.
- verdict: **returned to implementation.** Iteration 1 against the def180c spec; the next
  validation is iteration 2, below the escape threshold.

## Validation — 2026-09-17 (iteration 2 against the def180c spec)

Iteration 2: one `## Validation` section follows the 2026-09-15 `escaped to /expand-phase`
verdict (2026-09-16, returned to implementation). Below the escape threshold.

- **file set, checked first.** `- base:` names `24d489f`, confirmed a real commit; 36 files
  against the working tree; no path from `.claude/workflow/installed` inside it; tree clean.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **1:41** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, both ADRs, driver exit 0,
  accounting 3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `rejection cases: 64`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
  Both derivations recompute (`reject` 64, `accept` 20). Steps 18-22's own checks pass.
  M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand** in a `cp -a` copy, exit 1
  with exactly the predicted message.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (36 files, 9 under `src/core/`; 13 active `deny` rules, `src/core/`
  is a declared layer). One path per argument through `xargs`, input counted at 36. Proved live:
  `#include "src/hal/bus_io.h"` in `src/core/link.cpp` is caught as `deny core -> hal`, exit 1,
  file restored byte-identical.
- index: **was stale** at `2fe3788`, rebuilt to `81d838a`; `--check` now fresh.
- independent review: **contradicts: 3** — (1) `src/core/ps2_protocol.h:68` still says "A frame
  shorter than this announced nothing at all", the very sentence §Plan step 18 says it removed
  from `ps2_frame.cpp` and which §Goal item 1 contradicts; (2) §Out of scope says "the `.sh`
  checks are mutation-tested and the `.py` checks are not", while
  `tests/test_checks_are_live.py:49` holds `NO_MUTATE = {"test_style.sh"}` — so the file this
  phase bound R-CLEAN-04 to is held to the accounting property only; (3) §Plan step 10 says the
  uniformity mutation fails `R-PROTO-02` "only when the outcome is `AckTimeout`" and eleven lines
  later records it failing 7, the four plus three good frames from `Absent`. **undecidable: 5** —
  (1) which Plan step owns the `HeaderFilterRegex` narrowing `(src|tests)/.*` → `src/.*`, and
  whether narrowing two other phases' bindings was intended; (2) `tidy_sources()`'s "and nothing
  else" is stated in a glob notation the spec never fixes — git pathspec wildcards cross `/`;
  (3) whether the zero fill `kZeroFill` counts as an "expected byte" under R-PROTO-05;
  (4) whether ADR-0007's status line owes a partial-supersession note; (5) two spec claims that
  quantify files the reviewer was denied by construction. Six taste items.
- closure test: **fail** — three `contradicts`, and five `undecidable` are five missing pointers.
  Its mechanical half passes: four `notes.md` sections non-empty, all 36 files reachable from the
  spec or exempt, no untracked path, no `Owed` marker in the Plan.
- upstream: **none** in the file set. `/belay-feedback` owes the `contradicts` routing, the
  `- base:` staleness trap, the operator-edit gap, the escape reading round count rather than
  trend, and the `Owed`-marker gap.
- verdict: **returned to implementation.** Status stays `in-progress`. Findings 1-3 are code and
  spec text; the five `undecidable` are spec pointers, and finding 2 is a fact about the harness
  that the spec states more favourably than the harness supports — that one is a measurement to
  redo, not a sentence to soften.

## Validation — 2026-09-17 (iteration 3 against the def180c spec — escape fired)

Iteration 3: two `## Validation` sections follow the 2026-09-15 `escaped to /expand-phase`
verdict (2026-09-16, and earlier today). A gate failed, so the iteration-3+ escape fires.

- **file set, checked first.** `- base:` names `24d489f`, a real commit; **37 files** against the
  working tree (36 plus `docs/adr/0007-error-model.md`, §Plan step 28); no path from
  `.claude/workflow/installed` inside it; tree clean.
- criteria: **21 passed / 0 failed.** `make test` **OK** in **2:21** (cap 3m; 1:41 before step 23
  put the header filter back, which is the cost recorded in round 29), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, both ADRs, driver exit 0,
  accounting 3 and 4 rule(s), `wiring cases: 10/10`, `false-positive cases: 20 (floor 20)`,
  `rejection cases: 64`, `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (37 files, 9 under `src/core/`; 13 active `deny` rules). One path per
  argument through `xargs`, input counted at 37. Proved live: `#include "src/hal/bus_io.h"` in
  `src/core/ps2_frame.h` is caught as `deny core -> hal`, exit 1, file restored byte-identical.
- index: **was stale** at `81d838a`, rebuilt to `a1d2a48`.
- independent review: **contradicts: 5** — (1) §Plan step 20's own `— check:` returns **0**: the
  phrase it greps was removed by step 23, while §Plan's header says every step is landed;
  (2) step 20 says steps 11 and 17 were "reconciled to say 'in a checkout whose path contains no
  `src/`'" and neither says it — the phrase occurs once in the spec, in step 20 itself;
  (3) §Out of scope says "four of the five `.sh` checks are mutation-tested"; there are **six**
  and **five** are mutated (boundaries, phase_docs, repo_shape, secrets, tool_versions);
  (4) §Context pointers still calls `.clang-tidy`'s `HeaderFilterRegex` the one "§Vectors depends
  on", which step 23 and the `.clang-tidy` comment both deny; (5) §Context pointers promises
  "a reject/accept/wiring case per rule" in `tests/test_repo_shape.sh` — 20 `accept` cases cover
  seven finders, and R-PROTO-05, R-ERR-03 and R-ERR-04 have none, which §Acceptance criteria's own
  distribution already showed. **undecidable: 2** — (1) the derivation in §What a `test:` binding
  does and does not promise generates a file-scope obligation only in the `tidy_sources()` row, yet
  the assignment table gives R-ERR-02 one while it runs through `hits()`; `find_clean03` and
  `find_clean05` scan `src/` only and their rules' texts are silent, and the spec gives no rule for
  when a `hits`/`raw_hits` finder owes file scope; (2) the sentences deferring to `notes.md`, which
  the reviewer is starved of by construction. Four taste items.
- closure test: **fail** — five `contradicts`, and one `undecidable` is a missing derivation rule.
- upstream: **none** in the file set.
- verdict: **escaped to /expand-phase: spec re-expanded.** Status moved `in-progress` → `pending`
  by this command.

**Why the escape is right rather than unlucky, stated for whoever re-expands.** Four of the five
`contradicts` are sentences written **in round 29, while fixing round 28's findings** — a step
asserting a check that its own sibling step had just invalidated, a step describing reconciliations
it did not make, a count invented in the same edit that removed a wrong one, and a pointer left
behind by a revert. The fifth has survived every round since the section was written. Each
individual fix has been correct and measured; what is failing is the document, which now carries
28 Plan steps, several marked superseded in place, and a §Context pointers section describing a
tree three rounds older than the one on disk. A re-expansion derives the spec from the PHASES.md
row's Goal and this file, which is exactly the operation that drops accumulated scar tissue instead
of patching it again.

**What the re-expansion must carry forward** (beyond §Debt and §For later phases, which it reads
anyway): the refusal precedence and its four items; the `(src|tests)/.*` header filter and why the
narrowing was reverted; that `tests/test_style.sh` is not mutation-tested, so R-CLEAN-04 stands on
the accounting property; the spelling axis for every `find_*`; the `kZeroFill` ruling; and §Notes
to `/validate-phase`'s three package workarounds, items 1 and 2 of which die with the package
update and item 3 with an upstream fix that has not landed.

## Validation — 2026-09-21 (iteration 1 against the spec re-expanded at 90d7564)

Iteration 1: zero `## Validation` sections follow the 2026-09-17 `escaped to /expand-phase`
verdict, and that escape produced a new spec, so the reset applies.

- **file set, checked first.** `- base:` names `24d489f`, a real commit; **37 files** against the
  working tree; no path from `.claude/workflow/installed` inside it; tree clean but for the two
  `docs/index/` files this command rebuilt.
- criteria: **23 passed / 0 failed.** `make test` **OK** in **2:14** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1, both ADRs,
  ADR-0007 naming ADR-0011, driver exit 0, accounting 3 and 4 rule(s), `wiring cases: 10/10`,
  `false-positive cases: 23 (floor 23)`, `rejection cases: 64`, `neutered: 33/33`,
  `alternations: 64/64`, `test_phase_docs.sh` 0. Both derivations recompute (64 and 23).
  M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand** in a `cp -a` copy, exit 1
  with exactly the predicted message.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (37 files, 9 under `src/core/`; 13 active `deny` rules, `src/core/` is
  a declared layer). One path per argument through `xargs`, input counted at 37. Proved live:
  `#include "src/hal/bus_io.h"` in `src/core/hid_report.h` is caught as `deny core -> hal`, exit 1,
  file restored byte-identical.
- index: **was stale** at `a1d2a48`, rebuilt to `bc6fa5c`.
- independent review: **contradicts: 1** — §Context pointers says `grep -c '^### Round ' notes.md`
  returns **28** and that the rounds are therefore 29; the file returns **29**, so the sentence is
  false and its arithmetic now yields 30. Verified on the file. **undecidable: none** — the
  reviewer listed, separately and explicitly as not-defects, the claims it is structurally barred
  from checking (the sentences deferring to this file, to `00-scaffold/notes.md`, to the package
  and to `~/.claude-belay/feedback/`), having confirmed the half of each that is visible in the
  tree. Two taste items, in §For later phases above. The first dispatch of this review died on a
  session rate limit before producing any verdict; nothing was inferred from it and it was
  re-dispatched to a fresh agent after the limit reset.
- closure test: **fail** — one `contradicts`.
- upstream: **none** in the file set.
- verdict: **returned to implementation.** Status stays `in-progress`. The fix is a **deletion**,
  not a rewording: the sentence is a prose count of another file, which `00-scaffold` §For later
  phases already names as the shape with no check behind it, and this round is the third time it
  has rotted — the count was measured at 28, the same round then appended the `### Round 30`
  heading, and the line count beside it was updated while the round count was not. The line count
  is equally brittle and goes with it; §Context pointers needs the file named and its sections
  described, not measured.

### Taste from validation 2026-09-22 (iteration 2), recorded and not fixed

- `docs/adr/0011-pure-link-step.md` says "whose Plan step 6 is the first code to need the
  signature". §Plan now has no steps at all, so the pointer dangles. An ADR body is immutable
  here, so this is a stale cross-reference rather than a defect — but any future reader
  following it lands nowhere.
- §The link closes with "The saturation and the over-long buffer are the two that are not",
  which reads as if both were declared in §The link; the over-long buffer is declared in §Goal.
  §Out of scope phrases the same pair correctly.

## Validation — 2026-09-22 (iteration 2 against the spec re-expanded at 90d7564)

Iteration 2: one `## Validation` section follows the 2026-09-17 `escaped to /expand-phase`
verdict, so the escape threshold is not reached.

- **file set, checked first.** `- base:` names `24d489f`, confirmed a real commit by
  `git cat-file -t`; **37 files** against the working tree after `git add -N`; the
  `.claude/workflow/installed` subtraction removes **none** of them; tree carries only this
  round's two modified docs.
- criteria: **23 passed / 0 failed.** `make test` **OK** in **2:03** (cap 3m), `make lint` 0,
  `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers,
  `grep -rn 'tests/vectors' src/` 0, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1, both ADRs,
  ADR-0007 naming ADR-0011, driver exit 0, accounting 3 and 4 rule(s), `wiring cases: 10/10`,
  `false-positive cases: 23 (floor 23)`, `rejection cases: 64`, `neutered: 33/33`,
  `alternations: 64/64`, `test_phase_docs.sh` 0. Both derivations recompute (64 `reject`,
  23 `accept`). M1-M3 are the driver's rejection cases (`3/3`); **M4 run by hand** in a `cp -a`
  copy, exit 1 with exactly the predicted message naming all three rules.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (37 files, 9 under `src/core/`; 13 active `deny` rules, `src/core/` is
  a declared layer). One path per argument through `xargs`, input counted at 37. Proved live:
  `#include "src/hal/bus_io.h"` in `src/core/hid_report.h` is caught as `deny core -> hal`, exit 1,
  file restored byte-identical.
- index: **fresh** — `bc6fa5c -> 445f854 touched no source files`.
- independent review: **contradicts: 1** — the `make test` runtime figure disagrees between two
  files this phase writes. `spec.md` §clang-tidy's header filter says the revert took the run
  "from about 1:40 to about 2:30"; `verify.md:178` tells the operator runs "have landed between
  1:40 and 2:20". Verified: the post-revert measurements on record are **2:29** (round 30),
  **2:14** (validation 2026-09-21) and **2:03/2:04** (round 31 and this run), so `verify.md`'s
  spread is wrong at **both** ends — 1:40 is the *pre*-revert figure, and 2:20 excludes a
  measured run. An operator timing 2:29 is inside the spec and outside the range `verify.md`
  calls normal. **undecidable: 2** — (1) §What a `test:` binding does and does not promise derives
  the clause obligation from the three parts of a *grep* check, then states that R-PROTO-02/03/04
  run no scanner the table covers; it never says whether a non-grep check generates a clause
  obligation at all, so whether those three rules owe a scope clause is not decidable from the
  spec. (2) §Out of scope releases the *writing* of the file-scope clause for R-CLEAN-02 and
  R-CLEAN-03 to `00-scaffold`'s owner but never says whether a released clause also releases
  *compliance*, and the diff contains two instances that turn on it: `reject(` in
  `tests/test_ps2_codec.py` takes 5 parameters against R-CLEAN-02's unscoped "at most 3", and
  `const bool sources_reached` in `tests/ps2_codec_cases.cpp:670` is outside R-CLEAN-03's
  unscoped `is_`/`has_`/`can_`/`should_`. Both are green because neither check's file list reaches
  the file. Two taste items, in §For later phases above. The reviewer verified every numeric
  claim in §Goal, §Acceptance criteria and the assignment table against the tree and found
  **none false** — including the two counts round 31 kept — and listed separately, as not-defects,
  the sentences it is structurally barred from checking, with the visible half of each confirmed.
- closure test: **fail** — two `undecidable` findings; each is a missing pointer.
- upstream: **none** in the file set.
- verdict: **returned to implementation.** Status stays `in-progress`.

**Routing, so the next session does not have to guess.** Two different fixes, and the second is
an amendment that owes reconciliation:

1. The `contradicts` is a **measurement**, not a wording choice: re-time the tree and write one
   number that both files agree on, rather than adjusting one to match the other. Four runs are on
   record (2:03, 2:04, 2:14, 2:29) and they are the post-revert spread; whatever replaces the two
   sentences must cover 2:29 and must not claim 1:40, which no post-revert run has produced.
   `docs/constraints.md` carries no runtime figure, so the reconciliation set is these two files.
2. The two `undecidable` findings are fixed in **`spec.md`** plus this entry, never by changing
   code to satisfy the reviewer. For (1), §What a `test:` binding does and does not promise must
   say what a non-grep check owes — the honest answer is probably that the three-part derivation
   is a property of grep checks and that a vector-driven check's scope clause is its vector list,
   but that is a decision to make and write, not one to assume. For (2), §Out of scope must say
   explicitly whether releasing a clause releases compliance with the rule's text; if it does not,
   the two instances above are defects owed to some phase and must be named as such.

### Taste from validation 2026-09-22 (iteration 3), recorded and not fixed

- **Latent UB in several cases: `*frame` is dereferenced before `has_value()` is tested.** The
  `has_value()` call sits inside the `is_ok` conjunction *after* the deref, so a regression that
  made `decode` refuse would give undefined behaviour instead of a `FAIL:` line. Affects
  `case_digital_idle_maps_to_nothing_pressed`, `case_digital_pressed_maps_one_fret_and_one_strum`,
  `case_analog_whammy_full`, `case_analog_idle_whammy_is_rest`, both `case_config_mode_*`,
  `case_report_carries_the_whammy_end_to_end` and `rule_proto04`. None of the three automated
  mutations reaches it, so it is latent. This is the most substantive item in this list.
- `case_digital_payload_is_zero_filled` writes `is_ok = frame->payload[i] == kZeroFill;`
  (assignment, not `&&`) inside the loop; correct only because the loop guard re-tests `is_ok`.
- R-PROTO-04's new clause calls `0x80` and `0xFF` "the two ends of the axis". `0x80` is the
  midpoint of the byte range; `0x00..0x7F` is entirely unexercised and the clause does not say so.
- `state_for()` in `src/core/link.cpp` carries an unreachable `return LinkState::Absent;` after a
  switch covering every enumerator; `-Wswitch` already covers the stated risk.
- §Files this phase writes marks the `ps2_protocol.h` row "illustrative, not exhaustive"; the
  `guitar_state.h` row carries no such caveat but also omits published names (`kFretCount`, the
  mask block).

## Validation — 2026-09-22 (iteration 3 against the spec re-expanded at 90d7564)

Iteration 3: two `## Validation` sections follow the 2026-09-17 `escaped to /expand-phase`
verdict, so **the iteration-3+ escape fires.**

- **file set, checked first.** `- base:` names `24d489f`, confirmed a real commit by
  `git cat-file -t`; **37 files** against the working tree after `git add -N`; the
  `.claude/workflow/installed` subtraction (39 paths) removes **none** of them.
- criteria: **23 passed / 0 failed.** `make test` **OK**, `make lint` exit 0, timed run **2:07**
  (cap 3m), `planned: 01-ps2-codec` 0, `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/`
  headers, 0 `tests/vectors` refs under `src/`, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1,
  both ADRs, ADR-0007 naming ADR-0011, driver exit 0, accounting 3 and 4 rule(s),
  `wiring cases: 10/10`, `false-positive cases: 23 (floor 23)`, `rejection cases: 64`,
  `neutered: 33/33`, `alternations: 64/64`, `test_phase_docs.sh` 0. Both derivations recompute.
  M4 not re-run this round: the gate failed at step 5 before it was owed.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (37 files one per argument through `xargs`, input counted at 37;
  13 active `deny` rules, 9 files under the declared layer `src/core/`). Proved live:
  `#include "src/hal/bus_io.h"` in `src/core/hid_report.h` is caught, file restored
  byte-identical.
- index: was **stale** (this round's two `tests/` edits); rebuilt, `index fresh (445f854)`.
- independent review: **contradicts: 4, undecidable: 1.** All four `contradicts` were introduced
  by round 32 — the round written to close the previous round's findings — and all four were
  verified against the tree before being accepted:
  1. **R-PROTO-02's new scope clause is false.** It says only a digital frame is cut and only one
     byte short. `rule_proto02`'s seventh clause (`ps2_codec_cases.cpp:696`) and
     `case_cut_before_the_ready_slot_reports_the_abort` (`:276`) both decode
     `span{ kUnknownId }.first( kReadyIndex )` — **1 byte of a 20-byte `0x79` frame**: a different
     length and a non-digital frame. It also contradicts §Vectors' own `unknown_id.h` row and
     §Goal precedence item 1.
  2. **The same clause misstates source coverage.** It says
     `cut_drops_the_link_from_every_source` asserts "all four `LinkState` members". The function
     constructs **three** links (`from_digital`, `from_analog`, `from_negotiating`). The false
     sentence was copied out of the function's own pre-existing comment rather than measured —
     which is precisely what the clause-writing sentence claimed had not happened.
  3. **"longest is `real_run` at 42 lines" is false.** `real_run` is **8** lines
     (`test_ps2_codec.py:102-109`). 42 was a `def`-to-`def` span produced by a careless `awk`,
     not a function length. The *verdict* (clean, under a cap of 60) survives; the measurement
     does not. The real longest is `reject` at 35.
  4. **Off-by-one in §Out of scope.** "Four violations … and **one more class** was checked and
     is clean" precedes a table carrying **two** clean rows.

  **undecidable: 1** — nothing in the spec fixes whether an `Analog` frame's buttons come from the
  same two payload indices as a digital one. `guitar_state.cpp` gates on
  `reports_controls( id ) → Digital || Analog` and then reads `kButtonsLowIndex`/`kButtonsHighIndex`
  for both; no spec sentence and no vector pins analog button positions. The fact exists only as a
  code comment in `guitar_state.h`.

  The reviewer verified every acceptance-criteria number, the 64/23 distributions, the 14-row
  assignment table's list/scanner pairs and the `test:`-bound-rule enumeration (23 = 14 + 3 + 6)
  against the tree and found **none false**. Five taste items are above.
- closure test: **fail** — one `undecidable` is a missing pointer, and four `contradicts` stand.
- upstream: **`.claude/commands/validate-phase.md`** — step 5 says to pass "the phase's diff
  restricted to step 3's file set" *and* "Nothing else — not `notes.md`", which are contradictory;
  the file set contains `notes.md`. Hit live this round: the first review was dispatched with
  `notes.md` in its diff, killed before it reported, and re-dispatched over 36 files with the
  exclusion named. This is exactly the defect §Notes to `/validate-phase` logs as workaround 1,
  fixed upstream in **792e9d9** while this repo is pinned at **f001884**. `/belay-feedback`
  recommended.
- verdict: **escaped to /expand-phase: spec re-expanded.** Status moved `in-progress` → `pending`
  by this command.

**Why the escape, beyond the counter.** The three most recent rounds all failed on one class:
*a prose claim about the tree that no check holds.* Round 31 deleted a false count of `notes.md`'s
rounds. Round 32 wrote "no post-revert run has produced 1:40" and the next acceptance run produced
1:41. Round 32's clauses — written to close round 31's findings — carry three more false
measurements. Each round's fix is the next round's defect, which is the loop the escape exists to
stop. The re-expansion should treat this as a structural question, not four typos: **the spec
accumulates factual assertions about the tree that only a human reader can falsify, and the review
step is the only thing that ever falsifies them.** Either those assertions get a check
(an acceptance criterion that recomputes them), or they do not belong in prose. The clauses this
round added to `docs/constraints.md` for R-PROTO-02/03/04 are the live example: three sentences
about what the vectors reach, none of which any command recomputes.

## Validation — 2026-09-22 (iteration 6 against the spec re-expanded at 90d7564) — CLOSED

- **file set, checked first.** `- base:` names `24d489f`, confirmed a real commit by
  `git cat-file -t`; **37 files** against the working tree after `git add -N`; the
  `.claude/workflow/installed` subtraction (39 paths) removes **none** of them.
- criteria: **23 passed / 0 failed** (re-run after round 36's fixes). `make test` **OK**,
  `make lint` exit 0, traceability 0, driver 0, `test_phase_docs.sh` 0, `rejection cases: 64`,
  `false-positive cases: 23 (floor 23)`, `wiring cases: 10/10`, `neutered: 33/33`,
  `alternations: 64/64`, accounting 3 and 4 rule(s), `planned: 01-ps2-codec` 0,
  `planned: 03-pio-bus` 4, 10 vectors, 0 stray `tests/` headers, 0 `tests/vectors` refs under
  `src/`, 0 `.value()`, 3 markers, `HeaderFilterRegex` 1, both ADRs, ADR-0007 naming ADR-0011.
  Both derivations recompute.
- project gates: test **pass**, lint **pass**, typecheck **gap** — `workflow gap: no 'typecheck'
  tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the
  command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never
  overwrites.`
- boundary sweep: **clean** (37 files one per argument through `xargs`, input counted at 37;
  13 active `deny` rules, 9 files under the declared layer `src/core/`). Proved live earlier this
  session: `#include "src/hal/bus_io.h"` in `src/core/hid_report.h` is caught as `deny core -> hal`,
  exit 1, file restored byte-identical.
- index: rebuilt, `index fresh (445f854)`.
- independent review: **3 `contradicts` + 1 `undecidable` + 1 taste, all fixed in round 36 above.**
  The reviewer additionally reproduced `verify.md` §3 end to end — copied the tree, applied the
  `std::nullopt -> ControllerId::Digital` mutation, ran the driver, and got exactly the six FAIL
  lines in the order and wording `verify.md` predicts, ending `rejection cases: 2/3`, exit 1 — and
  re-derived every acceptance number, both case distributions member by member, all 14
  assignment-table rows, the refusal order against `ps2_frame.cpp`, and both enum memberships.
- closure test: **pass.** All four `notes.md` sections present and non-blank; every file in the
  37-file set reachable from the spec (the four `.cpp` files by the `header / .cpp` shorthand in
  §Files this phase writes and §Context pointers); every quantified claim in §Goal and §Acceptance
  criteria names its members.
- upstream: **`.claude/commands/validate-phase.md`** — three defects, all logged in
  `~/.claude-belay/feedback/pico-sg2hid.md`. Two were hit live this session: step 5's
  diff/`notes.md` contradiction (logged `resolved (f001884)`, the installed version, yet present),
  and the iteration-3+ escape firing on a round count. A new entry was filed 2026-09-22 with six
  iterations of evidence. **No local patch was applied to the command** — belay is frozen here and
  every local workaround must be removed before it can be updated, so the correct route was
  executed by hand and recorded in round 33 instead.
- **verdict: done.**

**How this phase closed, stated plainly because the gate did not produce this verdict on its own.**
The sixth review's four findings were fixed in round 36 and a **seventh review was not run**; the
operator ended the loop. That is a deliberate stop, not a clean final pass, and the reason is the
measured one: across six iterations this spec produced **22 findings, every one about a document
this phase writes and none about the code**, while `src/` stayed frozen at `a1d2a48` (2026-09-17)
and the reviewer reproduced the operator procedure end to end. The loop was auditing its own
documentation at a steady rate, so continuing bought rounds and not correctness. What the code has
is six independent audits of every acceptance number, both distributions, the refusal order, both
enum memberships and a live reproduction of the mutation procedure. What it does not have is a
seventh review returning zero findings on the prose.

**Open debt carried past `done`,** all in §Debt above with upgrade paths: R-CLEAN-03's subject is
undefined (bool variables only, or also bool-returning function names); the fifth unmeasured
concern — digital and analog sharing button payload indices — carries no `TODO(09-guitar-observe)`
marker; `reject( )` and `accept( )` in `tests/test_repo_shape.sh` take four positional parameters
against R-CLEAN-02 and are `00-scaffold`'s to fix; and the latent `*frame` dereference before
`has_value()` in several cases, recorded in §For later phases and the most substantive of them.

**Before belay is updated:** §Notes to `/validate-phase` in `spec.md` carries three package
workarounds to delete once their fixes land, and `docs/phases/01-ps2-codec/` carries no local patch
to any package file — `grep -rn 'LOCAL PATCH' .claude/` returns nothing.
