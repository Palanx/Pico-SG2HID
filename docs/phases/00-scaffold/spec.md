# Phase 00-scaffold — Make every rule that needs no product code real, and prove the checks are running

<!-- Re-expanded 2026-09-02 (round 9), after the round-8 validation fired the iteration-3+
     escape for the second time. Derived from the PHASES.md row's Goal and from notes.md
     §Deviations / §For later phases — never from the tree. Where the code disagrees with
     what is written here, the code is what changes; §Plan step 0 lists those owed edits.
     notes.md is the account of rounds 1-8 and is never rewritten.

     What is different from the round-6 spec, and why this is not round 6 with more cases:
     rounds 4, 7 and 8 each returned the same defect in a new place — a check reporting
     success while the thing it names never ran — and each was closed by enumerating one
     more rejection case by hand. Round 8 then wrote a universal claim into the Goal that
     the code satisfied for one finder out of eight. This spec stops enumerating. The
     coverage question becomes a machine's question (§Plan steps 2-4), and the Goal is
     written to what that machine actually proves, with everything it does not prove
     declared as debt with an owning phase (§Goal, "What this phase does NOT prove"). -->

## Goal

After this phase, `make test` runs a suite that turns fourteen rules currently marked
`planned: 00-scaffold` into rules with deterministic tests; the rule catalogue can no longer
drift from its tests without the build failing; **and the suite proves, by mutating its own
source, that each of those tests is still connected to the repository it claims to check.**

The third clause is the phase's reason to exist and the one rounds 1-8 kept failing. There is
no product code under `src/` yet, so almost every check passes vacuously: a check that has
been silently disconnected is indistinguishable from a check that is working, and eight
rounds of reading the code did not distinguish them — only deliberately breaking something
and watching the output ever did.

What exists afterwards that does not exist now:

- **A liveness harness, `tests/test_checks_are_live.py`** — one new file, in `python3` because splitting a regex into its alternatives at every nesting depth is a parser and the shell is the wrong tool for one; `python3` is inside the floor R-PROC-04 guarantees, so this costs no dependency, and the whole
  answer to the third clause. It does not check the repository; it checks the other checks,
  by deriving mutations **from their source** rather than from a list somebody maintains.
  Three properties, none of them an enumeration (details and the exact mutation forms in
  §Plan steps 2-4):
  1. **Accounting.** Every rule id declared in a check file's header produces a result line
     when that file runs for real. A deleted call site becomes a missing rule, not a silent
     pass.
  2. **Neutering.** Making any single check function find nothing must make its file exit
     non-zero. A pattern that stops matching is caught whether or not anyone wrote a case
     for it.
  3. **Alternation.** Removing any one alternative from any check function's pattern must
     make its file exit non-zero. This is what rounds 7 and 8 tried to reach by hand and
     got to one finder out of eight; generated from the pattern text, it cannot go stale
     when an alternative is added.
  Properties 2 and 3 do **not** reach the reporting path on their own, and an earlier draft
  of this spec claimed they did. They mutate what a check *finds*; a case that calls the
  finder directly still passes when the code deciding "is this a violation" is gutted, and
  when the verdict never reaches the exit code. Two structural requirements in the check
  files close that, and they are why the harness is not the whole answer (§Plan step 6):
  every rejection and accept case runs **through** the shared `report` function rather than
  re-deciding for itself, and each rule gets a **wiring case** driving the real aggregate on
  a tree carrying exactly that rule's violation, requiring both the rule's name in the output
  and a failing exit.
- **Six checks under `tests/`** — five `.sh` and one `.py` — plus the harness above, all
  picked up by the Makefile's `tests/test_*.{cpp,sh,py}` glob with no Makefile edit.
- **`tests/test_rule_traceability.py`**, which parses `docs/constraints.md` §Invariants under
  ADR-0005's grammar and fails on drift in either direction: a `test:` path that does not
  exist, a `test:` path lacking its `RULE <id>` marker, a marker naming an undeclared id, a
  `manual:` rule carrying a marker anyway, a `planned:` phase that does not exist in
  `docs/phases/PHASES.md`, a `planned:` phase that is already `done`, a duplicate id, an
  unparsable rule line, and an id `CLAUDE.md` mentions that the catalogue does not declare.
  Nine failure modes, each with its own message naming the id and the file.
- **Five shell checks over the shape of the repository** — layer boundaries, `core` purity,
  no heap, no exceptions, boolean naming, TODO references, no inheritance in `core`, no
  `.value()`, test-vector provenance, per-phase `verify.md`, absence of secrets, and minimum
  tool versions.
- **A rejection case on every check, and an accept case wherever the pattern could plausibly
  misfire on legitimate code.** These remain the mechanism that *catches*: the harness only
  proves they are sufficient, and reports precisely which alternative has none. A check
  without its rejection case is not finished; a rejection case that would also pass when the
  check cannot run is not a rejection case.
- **Fourteen rules in `docs/constraints.md` moved from `planned: 00-scaffold` to
  `test: <path>`**, each test file carrying the matching `RULE <id>` marker comment.
- Where a rule has two clauses that no single binding can honestly cover — one checkable now,
  one a property of code this phase is forbidden to write — **splitting it is in scope**, and
  the half that cannot be checked becomes a new rule with a `planned: <phase-id>` binding.
  That happened once: R-ERR-03 kept the source clause (no `throw`/`try`/`catch` under `src/`)
  and its flags clause became **R-ERR-05** (firmware builds pass `-fno-exceptions -fno-rtti`),
  `planned: 03-pio-bus`. A split leaves the count at fourteen bindings moved — the new rule is
  debt this phase declares, not a fifteenth binding it delivers.
- **`docs/phases/00-scaffold/verify.md`**, the operator-facing procedure (R-PROC-02).

Observable behaviour: `make test` exits 0 on the clean repo; exits non-zero — naming the rule
id and the offending path — when any of the fourteen violations is introduced by hand; and
exits non-zero when any check function is neutered, any alternative of any check pattern is
removed, or any real-run call site is deleted. The §Acceptance criteria adversarial block is
the first of those, written out; the harness is the second and third, run by `make test`
itself.

### What this phase does NOT prove, and who owns it

Round 8 failed because its Goal claimed more than its Plan delivered, and rounds 4 and 7
failed the same way. The gap is closed here by naming it rather than by widening the claim.
Each item below is real, is left undone on purpose, and has an owning phase:

- **The `.py` check's internals are mutated only at its call sites, not through its logic.**
  The harness's properties 2 and 3 are defined over the shell check functions; the nine
  traceability failure modes are covered by their own nine rejection cases, which is a
  hand-enumeration and is known to be the weaker form. Owner: **`01-ps2-codec`**, which is the
  first phase to add a rule whose check is not a grep. Recorded as `belay-debt:` in the
  harness.
- **Block comments and string literals are stripped by nothing.** The eight repo-shape checks
  are greps, not parsed C++, so a forbidden token inside `/* */` or inside a string is a false
  positive. Upgrade path is `clang-query`, which needs the `compile_commands.json` that
  `.clang-tidy` also wants. Owner: **`03-pio-bus`** at the earliest. Already a `belay-debt:`
  comment.
- **R-SEC-01's binding is vacuous on a machine with no `gitleaks`.** `OPTIONAL_TOOLS=1` turns
  the whole check into a skip, so the rule is bound to a test that may not run. That is the
  deliberate price of the clean-clone promise in the `PHASES.md` row (C++23 + `python3` and
  nothing else). Owner: no phase; it is a property of the floor, and the harness's accounting
  property must therefore treat a skipped file as unproven rather than as passing.
- **The harness cannot prove itself.** A liveness harness that is silently broken reports no
  gaps, which is the very failure it exists to catch, one level up. It gets a bootstrap floor
  instead of a regress: a fixture check file shipped with a known-uncovered alternative, which
  the harness must report. That is §Plan step 5, and it is the last turtle — stated here so
  nobody rediscovers the regress as a finding.

### How counts are stated in this spec

Rounds 2-5 produced three separate validation findings from one cause: an exact case count
written in a Plan step and again in the Acceptance criteria, with one of the two left stale by
an amendment. **Every case is enumerated by name in its Plan step; every count is a floor
(`>= n`) equal to the number of names in that step.** Round 8 then produced a fourth finding
from the opposite cause — a *universal* claim ("every alternation of every finder") stated in
§Goal and step 5 while the step enumerated eighteen names. So, added: **a floor may never be
written next to a universal claim about the same thing.** Where coverage is universal it is
generated and asserted by the harness, and the Plan step says which mechanism generates it;
where coverage is enumerated it is a floor and the claim above it is scoped to those names.

### Files this phase writes that no Plan step names

Two files change inside this phase without appearing in any Plan step, and neither is this
phase escaping its scope. `CLAUDE.md` states the general rule, so a reviewer holding only that
file and this spec can decide it; what follows is the phase-specific detail:

- **`docs/phases/PHASES.md`** — the workflow commands write the `status` column, and the row's
  coarse `acceptance` *text* may be corrected when it has drifted from this spec's Goal
  (`/expand-phase` §Failure modes allows exactly that; it is how "the eleven rules" became
  "the fourteen rules"). Plan step 9 makes one such correction, for the same reason: the row
  promises "every check ships a rejection case proving it fails on a bad tree", which round 8
  proved is not sufficient to make a check live. What is append-only is the *cut*: a row's id,
  goal and dependencies are superseded by new rows, never edited or deleted.
- **`docs/index/`** — generated output, exempt *structurally*: `/validate-phase` step 6
  rebuilds the index and excuses the whole directory, whatever changed in it and whoever
  changed it. It is not scoped to this phase's own edits, and a diff of `docs/index/` is never
  evidence about a phase.

**A third category, for the reviewer this spec will be handed to.** `/validate-phase` step 5
supplies this spec *whole* as its own input and withholds `notes.md` deliberately, so the diff
that accompanies it is path-filtered and carries neither file. Their absence is the gate
working, not a skipped step — it has been returned as `undecidable` twice and is filed
upstream (`~/.claude-belay/feedback/pico-sg2hid.md`, `commands/validate-phase.md`, open).
Likewise, files that `.claude/workflow/installed` names are the belay package's; they enter
the diff through `chore(belay):` commits and are not this phase's work.

## Context pointers

- `CLAUDE.md` — the session contract; §"Rules are bound to tests" states the binding this phase implements.
- `docs/phases/00-scaffold/notes.md` — **read §Deviations "Round 8 validation" and §For later phases first.** They carry the eight verified mutations that survive today, the reproduction command, and the reason the "check function plus rejection case" pattern is insufficient. Everything §Plan step 0 owes comes from there.
- `docs/constraints.md` — the rule catalogue. §Invariants is what the meta-test parses, and the fourteen `planned: 00-scaffold` lines are the work list. The binding grammar is in the section preamble; §Observed conventions is where a verified finding goes.
- `docs/adr/0005-rule-test-traceability.md` — why the binding exists, the three-binding grammar, and what each failure mode must report.
- `docs/adr/0002-hardware-free-core.md` — the layering the boundary and purity checks enforce, and why `core` must not reach the SDK.
- `docs/adr/0007-error-model.md` — why `-fno-exceptions` is a rule (R-ERR-03 source clause, R-ERR-05 flags clause); its decoding half is superseded by ADR-0009.
- `docs/adr/0008-cpp23.md` — the C++23 decision, the version measurements behind R-TOOL-01's floors, and the two-toolchain `PATH` trap R-TOOL-02 exists to catch.
- `docs/adr/0009-std-expected.md` — why `.value()` is forbidden and must be a grep (R-ERR-04), not a comment.
- `.claude/workflow/boundaries.rules` — the machine-readable layer edges. `tests/test_boundaries.sh` drives the existing hook with this file; it does not reimplement it.
- `.claude/hooks/boundary-check.sh` — read its header before wrapping it. One file path in, **exactly two exit codes**: `2` a forbidden dependency, `0` everything else (clean *or* no rule reached the file). No third code is produced and none may be added, so sweeping the tree is the wrapper's job.
- `.claude/hooks/lib/common.sh` — line 22, `ROOT="${CLAUDE_PROJECT_DIR:-$PWD}"`. The hook reads `$ROOT/.claude/workflow/boundaries.rules`, so a fixture tree is only judged if it carries its own copy of the rules file and the hook is invoked with `CLAUDE_PROJECT_DIR` pointing at the fixture.
- `tests/test_repo_shape.sh` — read it before building the harness: it is the file steps 3 and 4 mutate most, and it shows where a check function's pattern sits syntactically. Each is a one-line shell function whose body passes its extended regex as the **first single-quoted string** (`find_arch01( ) { hits '<pattern>' '<exclusions>' $( core_files "$1" ); }`), which is what makes "extract the pattern from the source" a five-line job rather than a parser. `tests/test_secrets.sh` and `tests/test_boundaries.sh` show the other two shapes the harness must handle: a check function taking a subcommand plus a root (`scan`), and one returning a distinguished code (`sweep`).
- `tests/test_style.sh` — the pattern every check follows: `RULE` markers in the header, a skip for a missing external tool, one `fail` accumulator, a `FAIL:` line naming the rule.
- `Makefile` — `test` globs `tests/test_*.cpp|.sh|.py` and sets `OPTIONAL_TOOLS=1`, so a missing external tool becomes a skip. `CXXFLAGS` already carries `-UNDEBUG`. `lint` runs `tests/test_style.sh` **and nothing else**, so a check needing `gitleaks` or the ARM toolchain can never break `make lint`.
- `docs/phases/PHASES.md` — the phase table the traceability test reads to validate `planned:` targets, whose `status` column drives R-PROC-02, and whose `00-scaffold` row step 9 corrects.
- `docs/templates/notes.md` — the shape of the `notes.md` this phase leaves behind.

No dependency phases, so there is no dependency `notes.md` to absorb.

## Plan

Steps 1-8 are the checks themselves and are largely already built; step 0 says exactly where
the built code disagrees with this spec, and steps 2-5 are the new mechanism. Implement 0
first — it is small and it removes five known-false statements from the tree — then 2-5, which
is where the phase's remaining risk lives.

0. **Close what round 8 left owed** — touches `tests/test_repo_shape.sh`,
   `tests/test_secrets.sh`, `tests/test_tool_versions.sh`, `tests/test_rule_traceability.py`,
   `docs/phases/00-scaffold/verify.md`, `docs/constraints.md`. Six items, each a statement in
   the tree that is false. The code is what changes.
   - **(0a) Delete the universal claims the code does not satisfy.** The comment in
     `tests/test_repo_shape.sh` above the round-8 cases ("One case per alternation, not one
     per rule … The unit of self-test here is the alternation") describes step 3's harness,
     not the eighteen hand-written cases below it. Reword it to what those cases are — the
     rejection cases the harness drives — and let the harness make the universal claim.
   - **(0b) Delete the false comment in `tests/test_secrets.sh`.** "Delete the `scan git` call
     above and this is what stops it going unnoticed" is untrue: deleting that call leaves the
     file green today (`notes.md` §Deviations, F2). Step 2's accounting property is what makes
     it true; until that lands the comment must not claim it.
   - **(0c) `ok:` may never prefix a shortfall.** `tests/test_tool_versions.sh:175` and
     `tests/test_rule_traceability.py:270` print `ok:   … rejection cases: n/m`
     unconditionally, before the comparison that sets the failure. Both files must print
     `FAIL:` when the floor is not met, as `test_repo_shape.sh` and `test_secrets.sh` already
     do. The build was never fooled; the reader was, and this phase is read by an operator.
   - **(0d) `verify.md` must not promise what a shared function does not buy.** Its round-8
     paragraph tells the operator that the real run and the rejection case calling the same
     function is "what makes an `ok:` line evidence that something ran". It is evidence that
     the *function* works. Rewrite it around the harness, and keep it a durable operator
     reference: any sample output must match what the checks emit today.
   - **(0e) The §Style authorization must name a check this phase writes.** Step 9 of the
     round-8 spec authorized correcting a `docs/constraints.md` §Style paragraph "when a check
     this phase writes proves it wrong", but the clang-tidy measurement behind that paragraph
     comes from `tests/test_style.sh`, which this phase only renamed a variable in. Either
     attribute the correction to the check that actually produced it or drop the claim; do not
     leave an authorization that does not reach its hunk.
   - **(0f) One `RULE`-marker message per failure mode must name what §Goal says it names.**
     The duplicate-id, unparsable-line and `planned:` messages in the traceability check name
     no file, and the unparsable one names no id, against §Goal's "each with its own message
     naming the id and the file". Fix the messages, not the Goal.
   — check: `sh tests/test_repo_shape.sh`, `sh tests/test_secrets.sh`,
   `sh tests/test_tool_versions.sh`, `python3 tests/test_rule_traceability.py` → each exit 0;
   `grep -rn 'ok:   .*rejection cases' tests/ | wc -l` equals the number of files that print
   one **inside a floor-passing branch** (verify by reading, four files); `make test` → exit 0.

1. **The optional-tool flag is one flag** — touches `tests/test_style.sh`, `Makefile`.
   `STYLE_OPTIONAL` is `OPTIONAL_TOOLS`, so one flag governs every check needing a tool
   outside the C++23 + `python3` floor (R-PROC-04). `-UNDEBUG` is already in `CXXFLAGS` from
   the ADR-0008 change — verify it is still there rather than adding it again.
   — check: `grep -c STYLE_OPTIONAL Makefile tests/test_style.sh` → `0` in both;
   `grep -c UNDEBUG Makefile` → `1`; `make test` → exit 0.

2. **Harness property 1 — accounting: every declared rule produces a result line.** Touches
   the new `tests/test_checks_are_live.py` and the header of each check file. Each check file
   already declares its rules as `# RULE <id> — …` header comments. The harness runs each
   check file for real, captures its output, and requires that **every declared id appears in
   at least one `ok:` or `FAIL:` line**, and that every `ok:`/`FAIL:` line naming a rule id
   declares that id in the header. Both directions: an undeclared id in the output is as much
   a drift as a declared id with no line.
   A rule whose check makes more than one independent real-run call needs one label per call,
   or deleting one call is invisible — R-SEC-01 is exactly that case (working tree *and*
   history; `notes.md` F2). So a check file may declare `# LIVE <id><rest>` lines
   alongside its `RULE` lines, where everything after the id is the literal prefix of the
   result line that proves the call happened (`# LIVE R-SEC-01 (history)`,
   `# LIVE R-TOOL-01: clang-tidy`). Each declared label must prefix **exactly one** result
   line. Not a substring, and not merely at-least-one: `(history)` also occurs inside
   `R-SEC-01 false-positive case (history)`, so a substring test was satisfied by an
   unrelated line and the deleted scan went on passing. `RULE` without `LIVE` means one call
   is enough. R-SEC-01 (tree, history) and R-TOOL-01 (four tool probes) are the two rules
   that need them.
   **A skipped file is unproven, not passing.** `OPTIONAL_TOOLS=1` turns a missing external
   tool into a skip, and a skipped check produces no result lines; the harness must report
   `unproven: <file> (skipped)` and must not count it as satisfied.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing one `ok:` line per check
   file with its rule-and-label count; then, with the `report R-ERR-04 "$( find_err04 "$1" )"`
   line deleted from a copy of `tests/test_repo_shape.sh`, the harness exits non-zero naming
   `R-ERR-04` as declared but never reported; and with the real `scan git "$ROOT"` block
   deleted from a copy of `tests/test_secrets.sh`, it exits non-zero naming the missing
   `(history)` label.

3. **Harness property 2 — neutering: a check that finds nothing must fail its file.** Touches
   the harness. For every check function in every shell check file — located by a declared
   naming convention, not a hand-written list — produce a copy of the file with that
   function's body replaced by a body that finds nothing, run the copy, and require a non-zero
   exit. The convention is what makes this generated rather than enumerated: **a check
   function is a shell function whose name begins `find_`, `check_`, `sweep` or `scan`**, and
   the harness reports the set it discovered so a function outside the convention is visible
   rather than silently unmutated.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing the discovered function set
   and `neutered: n/n caught` with `n` equal to that set's size; and, with one hand-written
   rejection case deleted from a copy of `tests/test_repo_shape.sh`, the harness exits
   non-zero naming the function whose neutering then went uncaught.

4. **Harness property 3 — alternation: removing any alternative must fail its file.** Touches
   the harness. For each check function found in step 3, extract its extended-regex pattern
   from the source, split it into **top-level** alternatives (`|` outside any `(…)` group and
   any `[…]` class), and for each alternative produce a copy of the file with that alternative
   removed *cleanly* — dropping the alternative together with one adjacent `|`, never leaving
   an empty alternative, which would match everything and fail for the wrong reason. Run each
   copy; require a non-zero exit. Report the count and, on failure, name the file, the
   function and the exact alternative that no case covers.
   This is the property rounds 7 and 8 reached for by hand and got to one finder out of eight
   (`notes.md` §Deviations, F1: `find_arch01` has 31 alternatives and 2 cases; `find_err03`
   has 3 and 1; `find_clean09` has 3 covering 2). Expect this step to **fail first and require
   new rejection cases in `tests/test_repo_shape.sh`** — that is the step working. The cases
   it demands are found by running it, never by enumerating them here; a list in this spec
   would be the round-8 defect again.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing `alternations: n/n caught`;
   and, with `hardware/|` removed from `find_arch01` in a copy, the harness names that
   alternative as uncovered and exits non-zero. `make test` → exit 0.

5. **The harness's bootstrap floor** — touches the harness and
   `tests/fixtures/incomplete_check.sh`, which this step creates. A harness
   that is silently broken reports no gaps, which is the failure it exists to catch. It cannot
   test itself without a regress, so it gets one fixture instead: a **deliberately incomplete
   check file** carrying a pattern with two alternatives and a rejection case for only one.
   The harness must report exactly that alternative as uncovered. If the harness ever stops
   working, this fixture stops being reported and the harness fails on its own floor.
   Ship the fixture as data the harness reads, not as a `tests/test_*` file — the Makefile
   glob would otherwise run it as a check and it is designed to be incomplete. It carries no
   `RULE` marker either: it binds no rule, and a marker there would make the traceability
   meta-test report a rule the catalogue never declares.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing
   `bootstrap: fixture gap reported as expected`; and, with the fixture's missing rejection
   case added by hand, the harness exits non-zero because its floor no longer proves anything.

6. **The six checks themselves.** Touches `tests/test_rule_traceability.py` (R-PROC-01),
   `tests/test_boundaries.sh` (R-ARCH-02), `tests/test_repo_shape.sh` (R-ARCH-01, R-ARCH-03,
   R-ERR-03, R-ERR-04, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-PROTO-05),
   `tests/test_phase_docs.sh` (R-PROC-02), `tests/test_secrets.sh` (R-SEC-01) and
   `tests/test_tool_versions.sh` (R-TOOL-01, R-TOOL-02). These are built and passing; this
   step is where their required properties are stated, so that the harness in steps 2-5 has
   something to hold them to and a re-implementation has something to rebuild from.
   - Every check takes **the tree root as an argument**, so one function serves both the real
     tree and a temp bad tree. Scope: `*.cpp` and `*.h` under `<root>/src`, narrowed to
     `<root>/src/core` for the two rules whose text says `core`. `tests/`, `docs/`, `tools/`
     and the build system are outside the checked set — including for R-CLEAN-03 and
     R-CLEAN-05, whose catalogue text names no scope: a test file legitimately holds literal
     expected bytes and bare scratch names. Line comments are stripped before matching,
     **except** for R-CLEAN-05, which is a rule about comments and whose text stripping would
     delete.
   - A rule with two clauses needs both checked. R-ARCH-01 is hardware header *and*
     hosted-only standard header; R-CLEAN-09 is annotated *and* default-specifier inheritance;
     R-SEC-01 is the working tree *and* the commit history — a secret deleted in the next
     commit is still in the repository. R-ARCH-01's hosted-only list is built from a criterion,
     not from memory: a header is hosted-only here if using it implies dynamic allocation,
     exceptions, threads, locale or an OS filesystem. The list is a floor, not a closed set.
   - R-ERR-04's grep is deliberately wider than its catalogue text. The text says `.value()`
     "on a `std::expected`"; a grep cannot see the receiver's type, so it matches every
     `.value(` under `src/`. Accepted price with a reason: `std::optional::value()` fails
     identically — it throws, and under `-fno-exceptions` throwing is `abort`. A user type
     with a `value()` accessor is the trigger to narrow it, not before.
   - `tests/test_boundaries.sh` drives `.claude/hooks/boundary-check.sh` over **every file
     under `src/`** — every file, not a filtered set: the hook decides for itself what an
     import line is, so filtering by extension would be the wrapper inventing a scope the rule
     does not have. The hook and its rules file are tracked repository content, so a missing
     or non-executable hook is a hard `FAIL`, never an `OPTIONAL_TOOLS` skip. The wrapper
     reads three outcomes from the hook's two documented codes: `0` clean, `2` a breach,
     **anything else means the hook could not run** — refusing to read a crash as a breach,
     because `FAIL: forbidden dependency direction` sends the reader hunting for an import
     that does not exist. `HOOK` is overridable so the stub case is runnable as written.
   - `tests/test_secrets.sh` probes capability, not version: a `gitleaks` that does not know
     the `dir` or `git` subcommand exits non-zero and would be reported as "found secrets", a
     misdiagnosis. `gitleaks git` over a directory that is not a repository exits 1 — the same
     code as a finding — so a missing `.git` gets its own `FAIL:` line. gitleaks is
     deliberately given **no** R-TOOL-01 version floor: the floor would need a release number
     nobody here has measured, and these probes answer the question the floor was for.
   - `tests/test_tool_versions.sh` compares the whole version, not the leading integer.
     **"Absent" is a different question for the two tool families, and the asymmetry is
     intentional.** For `clang-format`/`clang-tidy` it means "not resolvable": `PATH` first,
     then the keg-only LLVM prefixes Homebrew leaves unlinked, without which R-TOOL-01 skips
     `clang-tidy` forever on this machine. For `arm-none-eabi-*` it means "not on `PATH`", full
     stop: R-TOOL-02 is a claim about the binary *first on `PATH`*, and resolving elsewhere
     would hide the shadowing trap the rule exists to catch. R-TOOL-02 compiles a two-line TU
     including `<cstdint>` for `-mcpu=cortex-m0plus -mthumb` and **passes those two flags and
     no others** — `-std=c++23` is the GCC 13+ spelling, so a compiler at R-TOOL-01's own floor
     of 12 would fail and be misreported as lacking a target C library.
   - **Two structural requirements the harness cannot supply, and the reason it is not the
     whole answer.** Every rejection and accept case runs *through* the shared `report`
     function and asserts its verdict, rather than re-deciding "is the finder's output
     non-empty" for itself — a case that bypasses `report` leaves the line deciding what
     counts as a violation load-bearing for nothing. And each rule gets a **wiring case**:
     the real aggregate (`run_all`) driven over a tree carrying exactly that rule's
     violation, requiring both the rule's name in the output *and* the run to end failed.
     Without it, dropping one `|| fail=1` prints the `FAIL:` line and still exits 0. Both
     were found by mutating for them after the harness reported all-green.
   - Any `RULE`-shaped fixture text is assembled at runtime; a literal marker in a check's
     source is a real marker and the traceability checker will report it. The same is true of
     the fake token in `tests/test_secrets.sh`, which is also **not** the AWS documentation key
     `AKIAIOSFODNN7EXAMPLE` — gitleaks allowlists that one, so using it would make a broken
     check look like a passing one.
   — check: each of the six files run directly → exit 0; `make test` → exit 0, `OK`.

7. **Flip the fourteen rules and add the markers** — touches `docs/constraints.md` and the six
   check files. Each `planned: 00-scaffold` becomes ``test: `tests/<file>` `` and each check
   file carries `# RULE <id> — docs/constraints.md §Invariants — <one line>`, naming
   §Invariants (the subsection headings under it are not section names for this purpose).
   Two other kinds of hunk in `docs/constraints.md` are authorized by this step: **a verified
   finding goes to §Observed conventions**, per `CLAUDE.md` §Conventions — that is the
   catalogue's job and this spec does not get to route it elsewhere; and **a §Style paragraph
   may be corrected when a check *in this repository* proves it wrong**, with the check named
   in the hunk (step 0e). Neither changes a binding.
   — check: `grep -c 'planned: 00-scaffold' docs/constraints.md` → `0`;
   `python3 tests/test_rule_traceability.py` → exit 0.

8. **Write `docs/phases/00-scaffold/verify.md`** — touches that file. For a non-specialist:
   what was built, why a test that passes on an empty repo is worth anything, why a check can
   pass while enforcing nothing, and a hands-on procedure — break one rule on purpose, watch
   `make test` name it, put it back. It is a **durable operator reference, not a session
   report**: any sample output must match what the checks emit today, each break/run/restore
   cycle must be runnable literally and in order, and anything true only while this phase was
   being written belongs in `notes.md`. Include the clang-tidy limitation where an operator
   will actually meet it: a scratch file with an `#include` also fails R-STYLE-02 with
   `file not found`, which is a tooling result and not the rule under test.
   — check: `sh tests/test_phase_docs.sh` → exit 0;
   `test -s docs/phases/00-scaffold/verify.md`; every command `verify.md` prints can be pasted
   into a shell in the order given, and each produces the `FAIL:` line it promises.

9. **Correct the `PHASES.md` row's acceptance text** — touches `docs/phases/PHASES.md`. The
   row promises "every check ships a rejection case proving it fails on a bad tree". Round 8
   proved a rejection case is necessary and not sufficient: eight mutations survive it
   (`notes.md` §Deviations). Reword the coarse acceptance to match this spec's Goal — every
   check is proven live by mutations generated from its own source — leaving the row's id,
   goal and dependencies untouched.
   — check: `grep -n '^| 00-scaffold' docs/phases/PHASES.md` shows the corrected text;
   `python3 tests/test_rule_traceability.py` → exit 0 (it reads this table).

10. **Record the round** — touches `docs/phases/00-scaffold/notes.md`. Append this round's
    outcome and deviations; never rewrite the earlier rounds.
    — check: `test -s docs/phases/00-scaffold/notes.md` → exit 0.

## Acceptance criteria

```
make test                                          # expect: exit 0, "OK"
make lint                                          # expect: exit 0
python3 tests/test_checks_are_live.py              # expect: exit 0; accounting per file, neutered n/n, alternations n/n, bootstrap gap reported
python3 tests/test_rule_traceability.py            # expect: exit 0, rejection cases >= 9
sh tests/test_boundaries.sh                        # expect: exit 0, prints "rejection cases: n/n", n >= 2
sh tests/test_repo_shape.sh                        # expect: exit 0, one ok: line per rule, rejection/accept/wiring counts, wiring 8/8
sh tests/test_phase_docs.sh                        # expect: exit 0, rejection case confirmed
sh tests/test_secrets.sh                           # expect: exit 0, tree and history both scanned
sh tests/test_tool_versions.sh                     # expect: exit 0, names each tool and its version
grep -c 'planned: 00-scaffold' docs/constraints.md # expect: 0
grep -c 'STYLE_OPTIONAL' Makefile tests/test_style.sh   # expect: 0 in both files
test -s docs/phases/00-scaffold/verify.md          # expect: exit 0
test -s docs/phases/00-scaffold/notes.md           # expect: exit 0
time make test                                     # expect: real under 2m0s (the harness runs each check once per generated mutant)
```

No rejection-case floors appear above for the files the harness covers: the harness is what
asserts their sufficiency, and a floor beside it would be the round-8 defect — a count and a
universal claim about the same thing, drifting apart. The two floors that remain
(`test_rule_traceability.py`, `test_boundaries.sh`) are enumerations the harness does not
generate, and §Goal says so.

**Liveness block — the point of the phase, and what distinguishes this round from rounds 1-8.**
Each mutation must make `make test` exit non-zero. Run one at a time; restore between.

```
cp tests/test_repo_shape.sh /tmp/rs.bak
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_repo_shape.sh'); s = p.read_text()
p.write_text(s.replace('hardware/|', '', 1))          # an alternative with no case
EOF
make test                                          # expect: non-zero — the R-ARCH-01 rejection case for that alternative stops firing
cp /tmp/rs.bak tests/test_repo_shape.sh ; make test # expect: exit 0, "OK"

python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_repo_shape.sh'); s = p.read_text()
p.write_text(s.replace('    report R-ERR-04   "$( find_err04   "$1" )"\n', '', 1))
EOF
make test                                          # expect: non-zero, R-ERR-04 declared but never reported
cp /tmp/rs.bak tests/test_repo_shape.sh ; make test # expect: exit 0, "OK"

python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_repo_shape.sh'); s = p.read_text()
p.write_text(s.replace('if [ -n "$2" ]; then', 'if false; then', 1))   # gut report()
EOF
make test                                          # expect: non-zero — every rejection case reports "did not fire"
cp /tmp/rs.bak tests/test_repo_shape.sh ; make test # expect: exit 0, "OK"

python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_repo_shape.sh'); s = p.read_text()
p.write_text(s.replace('"$( find_err04   "$1" )" || fail=1', '"$( find_err04   "$1" )"', 1))
EOF
make test                                          # expect: non-zero — R-ERR-04 printed but never reaching the exit code
cp /tmp/rs.bak tests/test_repo_shape.sh ; make test # expect: exit 0, "OK"

cp tests/test_secrets.sh /tmp/se.bak
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_secrets.sh'); s = p.read_text()
i = s.index('if scan git "$ROOT" "$hist_out"; then'); j = s.index('\nfi\n', i) + 4
p.write_text(s[:i] + s[j:])                        # delete the real history scan
EOF
make test                                          # expect: non-zero, missing LIVE label R-SEC-01 (history)
cp /tmp/se.bak tests/test_secrets.sh ; make test    # expect: exit 0, "OK"
rm -f /tmp/rs.bak /tmp/se.bak
```

**Adversarial block — the rules still have to catch violations.** Each case must make
`make test` exit non-zero **and name its rule id**, and the tree must be back to green after
each. Run them one at a time, in order; each line removes its own file before the next writes
one, because two cases sharing a filename hide each other:

```
mkdir -p src/core   # git tracks no empty directory: absent in a fresh clone

printf '#include "pico/stdlib.h"\n'  > src/core/x.h  ; make test ; rm -f src/core/x.h    # R-ARCH-01
printf '#include <vector>\n'         > src/core/x.h  ; make test ; rm -f src/core/x.h    # R-ARCH-01
printf '#include "hal/bus.h"\n'      > src/core/x.h  ; make test ; rm -f src/core/x.h    # R-ARCH-02
printf '#include "hal/bus.h"\n'      > src/core/x.hpp; make test ; rm -f src/core/x.hpp  # R-ARCH-02 (non-.h/.cpp)
printf 'auto* p = new int;\n'        > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-ARCH-03
printf 'std::string s;\n'            > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-ARCH-03
printf 'try { } catch ( ... ) { }\n' > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-ERR-03
printf 'auto v = e.value( );\n'      > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-ERR-04
printf 'bool flag = true;\n'         > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-CLEAN-03
printf '// TODO: fix\n'              > src/core/x.cpp; make test ; rm -f src/core/x.cpp  # R-CLEAN-05
printf 'struct A : public B { };\n'  > src/core/x.h  ; make test ; rm -f src/core/x.h    # R-CLEAN-09
printf 'struct A : B { };\n'         > src/core/x.h  ; make test ; rm -f src/core/x.h    # R-CLEAN-09

rmdir src/core src 2>/dev/null ; make test          # expect: exit 0, "OK"
```

And the negative half — the checks must stay quiet on legitimate code:

```
mkdir -p src/core && printf 'std::string_view sv;\n' > src/core/x.h
sh tests/test_repo_shape.sh                         # expect: exit 0 — R-ARCH-03 does not fire
rm -f src/core/x.h ; rmdir src/core src 2>/dev/null
```

This one runs the repo-shape check rather than `make test`, and the reason is a limitation
worth knowing before `01-ps2-codec` writes its first header — recorded as a finding in
`docs/constraints.md` §Observed conventions, which is where `CLAUDE.md` §Conventions sends a
verified fact about tooling behaviour: on this machine clang-tidy cannot resolve *any* standard
header without a `compile_commands.json`, so a scratch file containing `std::` or an
`#include <…>` fails R-STYLE-02 with `error: 'string_view' file not found
[clang-diagnostic-error]` — a tooling result, not an R-ARCH-03 one. The working invocation that
measured it is in `notes.md` §For later phases.

## Out of scope

- **Any PS2 protocol logic, type, or constant.** `Ps2Frame`, `DecodeStatus`, `LinkState`, the
  `std::expected` signatures of ADR-0009 and the vectors under `tests/vectors/` belong to
  `01-ps2-codec`. This phase creates no `src/core/*.cpp` — the checks are written against a
  tree with no product code, and that is deliberate.
- **`src/core/pins.h` and any GPIO number.** Belongs to `02-wiring`; R-SAFETY-01..03 stay
  `planned:`.
- **`CMakeLists.txt`, `PICO_SDK_PATH`, any firmware build.** Belongs to `03-pio-bus`, which
  also owns R-ERR-05. `make firmware` keeps failing with its current message. The ARM
  toolchain itself is not out of scope — step 6 probes it — but this phase never builds with
  it.
- **A C++ test harness, assertion library or test framework.** `<cassert>` plus the Makefile's
  per-file compile-and-run is the harness; `-UNDEBUG` is what makes it trustworthy.
- **A mutation-testing framework or coverage tool.** Steps 2-5 are three properties over the
  project's own check files, implemented in the shell and `python3` the floor already
  guarantees. Reaching for `mutmut`, `cosmic-ray` or `gcov` adds a dependency outside the
  clean-clone promise to do less than the twelve-line generators above.
- **Mutating the `.py` check's internals**, and **mutating `tests/test_style.sh`** — declared
  debt in §Goal with `01-ps2-codec` as owner. The harness covers the `.py` file's accounting
  property only.
- **Enforcing R-CLEAN-04 (magic numbers).** Belongs to `01-ps2-codec`:
  `readability-magic-numbers` needs real code to tune exclusions against.
- **Upgrading the greps to `clang-query`.** Declared debt in §Goal, owner `03-pio-bus`.
- **Installing a git `pre-commit` hook.** The security gate runs as a Claude `PreToolUse` hook
  only. Wiring it into `.git/hooks/` is an operator decision that has been raised and not
  answered.
- **Making `make test` fast.** The harness runs each check file once per generated mutant, so
  the suite's runtime grows with the number of alternatives. Measure it, state it in
  `verify.md`, and leave it: an acceptance criterion caps the suite at two minutes, and
  parallelising is an optimisation nobody has yet needed.
- **Splitting this phase.** Considered and rejected three times. The checks share one harness
  and one set of conventions, and the liveness harness is meaningful only against all of them
  at once — two rows would duplicate the machinery for no gain.
- **The accumulated taste notes** — `.sh` file modes, `\b` versus `([^_[:alnum:]]|$)`,
  `find`-loop word splitting, `head -8` truncation, the `PATH=` prefix on a function call,
  `SKIP_DIRS` listing `node_modules`, `find_clean09`'s line-based `enum class` exclusion. All
  in `notes.md`; none changes what a rule catches. Fix them when touching the line for another
  reason.
