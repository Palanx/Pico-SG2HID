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
drift from its tests without the build failing; and the suite proves, by mutating its own
source, that each of those tests is still connected to the repository it claims to check.

The third clause is the phase's reason to exist and the one rounds 1-11 kept failing. There is
no product code under `src/` yet, so almost every check passes vacuously: a check that has
been silently disconnected is indistinguishable from a check that is working, and reading the
code never once distinguished them — only deliberately breaking something and watching the
output did.

### The rule this spec is written under: no universal in prose

Rounds 9-11 amended this spec eleven times, and each amendment produced the next round's
finding somewhere the amendment did not reach. The cause is one shape, repeated: §Goal
asserted a universal in prose — "each rule gets a wiring case", "every rejection and accept
case runs through the shared `report` function" — while the Plan illustrated it with a
mechanism living in one file out of five. Each round closed one more file and the next round
returned the same finding under a new verdict name: `undecidable` in round 9, scoped away by
the operator in round 10, `contradicts` in round 11.

So this spec states **no universal in prose.** Every "each rule", "every check", "all files"
below is a table with one row per member and a measured current state. A requirement that
does not apply to a member gets a row saying so and why — a declared exception is decidable
by the starved reviewer in `/validate-phase` step 5, an omission is not. A universal that
cannot be enumerated is not a Goal and does not appear here.

The measurements in the tables' *today* columns were taken against the tree at `5c1422c`, by
mutation where a mutation can settle it. They are there so the Plan can state a gap; the
*must* columns come from the `PHASES.md` row's Goal, never from what the code happens to do.

### Table 1 — the fourteen rules and where each is checked

The membership every other table indexes. `tests/test_style.sh` is deliberately absent: it
binds R-STYLE-01, R-STYLE-02 and R-CLEAN-02, which predate this phase and are not among the
fourteen. Its place in the harness's file set is Table 5.

| # | rule | check file |
|---|---|---|
| 1 | R-ARCH-01 | `tests/test_repo_shape.sh` |
| 2 | R-ARCH-03 | `tests/test_repo_shape.sh` |
| 3 | R-ERR-03 | `tests/test_repo_shape.sh` |
| 4 | R-ERR-04 | `tests/test_repo_shape.sh` |
| 5 | R-CLEAN-03 | `tests/test_repo_shape.sh` |
| 6 | R-CLEAN-05 | `tests/test_repo_shape.sh` |
| 7 | R-CLEAN-09 | `tests/test_repo_shape.sh` |
| 8 | R-PROTO-05 | `tests/test_repo_shape.sh` |
| 9 | R-ARCH-02 | `tests/test_boundaries.sh` |
| 10 | R-PROC-02 | `tests/test_phase_docs.sh` |
| 11 | R-SEC-01 | `tests/test_secrets.sh` |
| 12 | R-TOOL-01 | `tests/test_tool_versions.sh` |
| 13 | R-TOOL-02 | `tests/test_tool_versions.sh` |
| 14 | R-PROC-01 | `tests/test_rule_traceability.py` |

### Table 2 — the two structural requirements, per rule

Both exist because the harness cannot reach them: its properties mutate what a check *finds*,
and a check can find correctly while the code deciding "is this a violation" is gutted, or
while the verdict never reaches the exit code.

- **SV — single verdict.** The decision "is this output a violation" is written **once** per
  rule, and the real run and every case consume that one decision. Measured by gutting that
  decision and requiring the file to fail; a file where the real run and the cases each
  re-decide passes that mutation and is marked ✗.
- **WC — wiring case.** The real aggregate is driven over a tree carrying exactly that rule's
  violation, and the case requires **both** the rule's name in the output **and** a failing
  exit. Measured by removing the `fail=1` on that rule's failure path and requiring the file
  to fail anyway.

| # | rule | SV today | how the verdict is shared | WC today | must |
|---|---|---|---|---|---|
| 1-8 | the eight in `test_repo_shape.sh` | ✓ | `report( )` | ✓ (`wiring cases: 8/8`) | hold |
| 9 | R-ARCH-02 | ✓ | `sweep( )`'s `case` on the hook's 0/1/2 | **✗** | add |
| 10 | R-PROC-02 | ✓ | `report( )` | ✓ | hold |
| 11 | R-SEC-01 | ✓ | `scan( )`'s return, consumed at all six sites | **✗** | add |
| 12 | R-TOOL-01 | ✓ | `check_version( )`'s floor comparison | **✗** | add |
| 13 | R-TOOL-02 | ✓ | `arm_compiles( )`'s return | **✗** | add |
| 14 | R-PROC-01 | ✓ | `check( )`'s problem list | **✗** | add |

**SV is 14/14 and stays that way; WC is 9/14 and must reach 14/14.** Round 11's reviewer
reported SV as 2/5 files, and the measurement does not support it: gutting
`check_version`'s floor comparison fails two rejection cases, and gutting `sweep`'s breach
arm fails the core→hal case. What made SV look broken is that the old §Goal named a *function*
(`report`) instead of stating the property, so a file satisfying the property by another
mechanism read as a violation. The requirement is the property; the mechanism column above is
per file and is not required to be the same function.

WC is the real gap and all five absences are live. The *today* column is the dated
measurement the Plan is written against; the state after §Plan step W lands is recorded in
`notes.md`, not here, so this table keeps meaning "the gap step W closes".
Measured, each against a full `make test`:

```
tests/test_secrets.sh:69        fail=1 -> :        make test rc=0   R-SEC-01 unbound
tests/test_boundaries.sh:66     fail=1 -> :        make test rc=0   R-ARCH-02 unbound
tests/test_tool_versions.sh:104 drop || fail=1     make test rc=0   R-TOOL-01 unbound
```

R-TOOL-02 and R-PROC-01 are the same shape and are listed as owed on that basis, not measured
separately; §Plan step W says the implementer measures them before and after.

### Table 3 — the liveness harness's three properties and their scope

| property | applies to | scope is decided by | today |
|---|---|---|---|
| accounting | every file in Table 5 | Table 5, explicitly | 7 of 7 |
| neutering | every function defined in a Table 5 file marked *mutated* | discovery, no name list | 26/26 |
| alternation | both quoted strings of every **one-line** function body in those files | the house shape, no name list | 59/59 |

The two generated properties carry no floor, by §How counts are stated: their claim is
universal and machine-derived, so a number beside them would be the round-8 defect. The
counts above are the current output, recorded as a measurement, not as a threshold.

### Table 4 — every count line, its floor, and the enumeration the floor equals

§How counts are stated requires a floor to equal a by-name list. Three of round 11's four
`undecidable` verdicts were floors with no list. Every count line the suite prints:

| file | count line | floor | the enumeration it equals |
|---|---|---|---|
| `test_repo_shape.sh` | `rejection cases: n` | none — guard `n > 0` only | universal, generated by the harness; the guard only catches "no case ran at all" |
| `test_repo_shape.sh` | `false-positive cases: n (floor 13)` | `>= 13` | the thirteen names in §Plan step 6 |
| `test_repo_shape.sh` | `wiring cases: n/8` | `= 8` | rules 1-8 of Table 1 |
| `test_boundaries.sh` | `rejection cases: n/2` | **`= 2` today, must be `>= 2`** | core→hal is refused; a hook that cannot run is not a violation |
| `test_secrets.sh` | `rejection cases: n (floor 2)` | `>= 2` | working tree: a planted token is reported; history: a deleted token is still found |
| `test_secrets.sh` | `false-positive cases: n (floor 2)` | `>= 2` | working tree: a plain file is not a secret; history: a clean history is not a secret |
| `test_tool_versions.sh` | `rejection cases: n/4` | **`= 4` today, must be `>= 4`** | clang-format below its floor; a cross-compiler with no target libc; a banner with no parsable version; python3 below a floor with a minor number |
| `test_rule_traceability.py` | `R-PROC-01 rejection cases: n/9` | **`= 9` today, must be `>= 9`** | the nine failure modes listed in the deliverables below |

The three exact equalities break when a case is added, which is the opposite of what a floor
is for. §Acceptance criteria says `n >= 2` for `test_boundaries.sh` while the file asserts
`-eq 2`; that disagreement is round 11's `undecidable` (c) and is resolved here in favour of
the floor.

### Table 5 — the accounting property's file set

`check_files()` takes every `tests/test_*.{sh,py}` the Makefile globs, minus the harness
itself. That is seven files, not the six that bind the fourteen rules, and round 11's
`undecidable` (d) was the gap between those two numbers.

| file | accounting | mutated | rules it declares | among the fourteen |
|---|---|---|---|---|
| `test_repo_shape.sh` | ✓ | ✓ | 8 | yes |
| `test_boundaries.sh` | ✓ | ✓ | R-ARCH-02 | yes |
| `test_phase_docs.sh` | ✓ | ✓ | R-PROC-02 | yes |
| `test_secrets.sh` | ✓ | ✓ | R-SEC-01 | yes |
| `test_tool_versions.sh` | ✓ | ✓ | R-TOOL-01, R-TOOL-02 | yes |
| `test_rule_traceability.py` | ✓ | ✗ — not a shell file; its logic is covered by nine rejection cases, the weaker form, owner `01-ps2-codec` | R-PROC-01 | yes |
| `test_style.sh` | ✓ | ✗ — §Out of scope; this phase only renamed a variable in it | R-STYLE-01, R-STYLE-02, R-CLEAN-02 | **no** |

`test_style.sh` is in the accounting set and out of everything else. It follows that its
three ids must each produce a result line the real run alone emits, exactly like the
fourteen — accounting is a property of the file set, not of the fourteen rules. Nothing else
about it is this phase's business. **Its ✓ above is only reproducible against the nine
emission sites §Plan step 2 now tabulates**, **and it is conditional**: it was measured on a
machine where both clang tools resolve. Where they do not, the `Makefile`'s `OPTIONAL_TOOLS=1`
turns both sites into skips, the file produces no `ok:`/`FAIL:` line, and §Plan step 2's own
rule applies — it is reported `unproven`, which is the correct outcome and not a ✓. On the
clean-clone floor R-PROC-04 promises, that is the state this file is in. With the tools
present: three ids, exactly two result lines per run, one of which names two ids at once, `,` on the `ok:` lines and ` / ` on the `FAIL:` and `skip:`
ones. Those rows record ids per line, site counts, prefixes and separators — and no ordering,
because no run exhibits every branch and an order measured from one run would be a fact about
that run. Round 12 returned two `undecidable` verdicts here for want of the rows.

### Table 6 — round 11's §For later phases items: in scope or not

Left open, this is round 11's `undecidable` (d)'s sibling and would return.

| item | in scope | why |
|---|---|---|
| mutants judged by exit code alone, so "caught for the wrong reason" counts as caught | **no** | both concrete instances were removed in round 11 by scoping patterns to one-line bodies; the general fix needs a check that is not a grep. Owner `01-ps2-codec` |
| an interrupted `make test` leaves `tests/mut_*.sh` behind | **yes** | observed three rounds running, and the orphans land where the traceability walk and the gitleaks tree scan read. §Plan step T |
| `test_boundaries.sh` prints its count with no `ok:`/`FAIL:` prefix, so the harness cannot see it | **yes** | the same file is being opened for its wiring case and its floor. §Plan step W |

### What exists afterwards that does not exist now

- **A liveness harness, `tests/test_checks_are_live.py`** — one file, in `python3` because
  splitting a regex into its alternatives at every nesting depth is a parser and the shell is
  the wrong tool for one; `python3` is inside the floor R-PROC-04 guarantees, so it costs no
  dependency. It does not check the repository; it checks the other checks, deriving
  mutations **from their source** rather than from a list. Its three properties and their
  scope are Table 3; the two things it cannot reach are Table 2.
- **Six checks binding the fourteen rules** (Table 1), plus `test_style.sh` in the accounting
  set (Table 5), all picked up by the Makefile's `tests/test_*.{cpp,sh,py}` glob with no
  Makefile edit.
- **`tests/test_rule_traceability.py`**, which parses `docs/constraints.md` §Invariants under
  ADR-0005's grammar and fails on drift in either direction. Nine failure modes, each with its
  own message naming the id and the file — and they are the enumeration Table 4's last row
  points at: a `test:` path that does not exist; a `test:` path present but carrying no
  marker; a marker naming an undeclared rule; a `manual:` rule carrying a marker anyway; a
  `planned:` phase absent from `docs/phases/PHASES.md`; a `planned:` phase already `done`; a
  duplicate rule id; an unparsable rule line; an id `CLAUDE.md` cites that the catalogue does
  not declare.
- **A rejection case on every check, and an accept case wherever the pattern could plausibly
  misfire on legitimate code.** These remain the mechanism that *catches*; the harness only
  proves they are sufficient and names the alternative that has none. The counts and their
  enumerations are Table 4.
- **Fourteen rules moved from `planned: 00-scaffold` to `test: <path>`**, each test file
  carrying the matching `RULE <id>` marker comment.
- Where a rule has two clauses no single binding can honestly cover, **splitting it is in
  scope** and the unbindable half becomes a new rule with a `planned: <phase-id>` binding.
  That happened once: R-ERR-03 kept the source clause and its flags clause became
  **R-ERR-05** (`-fno-exceptions -fno-rtti`), `planned: 03-pio-bus`. The count stays at
  fourteen — the new rule is debt this phase declares, not a fifteenth binding it delivers.
- **`docs/phases/00-scaffold/verify.md`**, the operator-facing procedure (R-PROC-02).

Observable behaviour: `make test` exits 0 on the clean repo and on a clean clone carrying
only C++23 and `python3`; exits non-zero — naming the rule id and the offending path — when
any of the fourteen violations is introduced by hand; exits non-zero when any function in a
Table 5 *mutated* file is neutered, when any alternative of any check pattern is removed,
when any real-run call site is deleted, and — also naming the rule — when the `fail=1` on any
of the fourteen rules' failure paths is removed; that last branch names the rule
because the wiring case whose assertion fails is the thing that reports it. The last of those is Table 2's WC column and is the clause rounds
9-11 could not make true for more than nine rules at a time.
One more fact belongs to this clause rather than to a Plan step, because `verify.md` is
required to teach it: on a machine where `clang-tidy` resolves, a scratch file carrying an
`#include` **also** fails the check binding R-STYLE-02 and R-CLEAN-02, over a header it
cannot find — a tooling
result, not the rule under test, because no `compile_commands.json` exists before
`03-pio-bus`. Where the tool does not resolve, that check is skipped and the failure does not
appear at all.

### What `verify.md` may assert about the suite, and on whose authority

`docs/phases/00-scaffold/verify.md` is the operator's only entry point (R-PROC-02) and it
necessarily makes claims about what the suite does. Round 13 found that this spec had never
said which claims are legitimate, and §Plan step 8 was asked to invent that authority three
times in one round; each wording split on a different sentence, because the authority does
not belong to a Plan step. It is stated here, once:

**The *Observable behaviour* paragraph above is the complete set of facts `verify.md` may
state about the *output* of a run.** That scope is the whole of the clause and is stated
first, because getting it wrong in either direction is what three earlier wordings did: facts
about what a check *is*, what it *decides*, and how it is *built* are not run output and are
not governed here at all — they are founded the way every other claim in this spec is founded,
on a Plan step or a table, and `verify.md` needs them to satisfy `CLAUDE.md` §Teach, don't
just deliver. "This check compares each tool against a floor" is a fact about what the check
decides (§Plan step 6). "Its line puts the floor before the version found" is a fact about the
output, and is governed.
Within that scope the clause is closed. `verify.md` may say that a run exits zero or non-zero
and under which condition;
it may name the rule id and the offending path; it may say which of the mutations listed there
makes the suite fail; it may state the clang-tidy fact and its condition. Anything a reader
could only learn by looking at a check's `echo` — how many lines it emits, in what order, what
a line holds besides the id and the path, colour, indentation, the separator between two ids —
is outside the clause, and is outside it **whether quoted or described**, because a described
format goes stale exactly as fast as a quoted one.

This settles, so that no Plan step has to, each of the following — no count here, because the
list is the enumeration and a number beside it would be §How counts are stated's own defect:

- **The id and the path are one fact, not two.** The clause guarantees them together, so
  stating them together is the clause being quoted, not a layout being described — and that
  includes saying they arrive together in one report, which is the form the guarantee takes.
  What the clause does not carry is their order, their spacing, or what else sits with them.
  **The same holds for a check that binds two rules**: the clause names both ids in one
  guarantee, so saying both arrive in one naming is the clause. Their separator is not, and
  §Plan step 2's rows record it for `test_style.sh` as source shape, never as something
  `verify.md` may repeat.
- **Widening the clause is how `verify.md` earns the right to state something new.** A round
  that wants the operator told a new fact adds it to *Observable behaviour* first, where the
  Plan and the Acceptance criteria can both see it. That is the amendment; there is no second
  route and there are no exceptions, which is why the clang-tidy fact was folded into the
  clause above rather than declared an exception to it.
- **`make test`'s own result is inside the clause** — it is the exit code, and its `OK`
  banner is that exit code's rendering, expected by name on §Acceptance criteria's first line.
  A check's result lines are not, beyond the id and the path the clause names.
- **A single check file's own exit status is inside the clause too**, on the same footing:
  §Plan step 8 requires `verify.md`'s procedures to be runnable literally and in order, and
  several of them run one check directly. What is *not* inside is the prefix a result line
  carries — `ok:`, `skip:`, `FAIL:` are text a line holds, so `verify.md` says a run failed
  and named a rule, never that a line began with a particular word. The §Acceptance grep
  whitelists a bare prefix because a grep cannot tell prose from a transcript; the whitelist
  is the grep's tolerance, never a licence.
- **The skip is inside the clause, for a tool `OPTIONAL_TOOLS` governs**: such a tool, when
  absent, is reported and skipped rather than failing, which is the clean-clone promise
  §Acceptance criteria and R-PROC-04 both carry. It is not a universal over "tools": §Plan
  step 6 makes `.claude/hooks/boundary-check.sh` a hard `FAIL` when it cannot run, precisely
  because it is tracked repository content and not an external tool. `verify.md` may state the
  skip; it may not state it of anything step 6 exempts.
- **`verify.md`'s references to its own structure are not claims about the suite.** Its
  numbered sections and its own lists are the document's, and counting them is not counting
  a check's output.
- **The clause governs runs of the suite as delivered, and nothing else.** What a defective
  past version printed, or what an inert check would print, is a fact about a defect and not
  about this suite's output — the clause exists to stop `verify.md` going stale against a
  suite that keeps changing, and a defect that has been closed cannot go stale. Those
  statements are founded where the defect is recorded (Table 2's measured `fail=1 -> :` block,
  §Plan step 2's `LIVE` discussion, `notes.md`), which is why `verify.md` may teach the four
  holes by saying what each one did.

The mechanical floor under all of this is the grep in §Acceptance criteria, and it is a floor
and not the rule: it sees quoted output and cannot see a described format, so the clause above
is what a reviewer applies and the grep is what a machine catches.

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

**Round 12 adds the clause the eleven amendments of rounds 9-11 were missing.** The rule
above was satisfied by every one of those amendments and the finding still came back, because
it governs *counts* and the defect was in *universals*: a claim over a set, written as prose,
whose members were never listed. So: **a universal claim is a table with one row per member,
or it is not made.** §Goal's tables are that; a Plan step may say "every X" only when it
points at the table whose rows are the Xs. The three exact equalities in Table 4 become
floors for the same reason — an equality is a count that breaks when the enumeration grows,
which is a floor written backwards.

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
the diff through `chore(belay):` commits and are not this phase's work. **A pointer to a
manifest the reviewer cannot read decides nothing** — it came back `undecidable` in round 10,
with `scripts/build-index.sh` singled out — so the package-owned paths this phase's diff
actually carries are written out here:

| path | why it is in the diff, and how to check without the manifest |
|---|---|
| `.claude/commands/expand-phase.md` | manifest; `git log --oneline -- <path>` shows `chore(belay):` commits only |
| `.claude/commands/validate-phase.md` | same |
| `.claude/hooks/boundary-check.sh` | same |
| `docs/templates/CLAUDE.adopted.md` | same |
| `docs/templates/CLAUDE.bootstrap.md` | same |
| `scripts/build-index.sh` | same — including the rewrite of its dependency-edge loop, which is package work, not this phase's |
| `.claude/workflow/belay-version` | `install.sh` writes it but the manifest does **not** name it; it rides in on the same `chore(belay):` commits. A manifest defect, filed upstream, not an exception to this rule |
| `CLAUDE.md` — the line "A phase's diff always carries files the workflow wrote…" | written by `694c902`, a `chore(belay):` commit (`git log -S` on the line confirms it). Round 9's reviewer returned this hunk as a `contradicts` for want of that fact |

The test is provenance, not path: a file whose every commit is `chore(belay):` is the
package's. Anything in the diff not in this table is judged by the closure test's ordinary
rule — reachable from the Context pointers or the Plan, or the phase escaped its scope.

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
- `tests/test_checks_are_live.py` — the liveness harness. Read `property_accounting`,
  `find_functions`, `patterns` and `bootstrap` before touching Table 3's properties; its
  module docstring carries the three properties and the `belay-debt:` note. §Plan step T may
  touch it instead of the `Makefile`.
- `tests/test_boundaries.sh` — R-ARCH-02 (Table 1 row 9). §Plan step W adds its wiring case,
  turns `rejection cases: n/2` into a prefixed result line and its `-eq 2` into `>= 2`. Its
  verdict is `sweep( )`'s `case` on the hook's 0/1/2, which is what SV means here — do not
  add a `report( )` to make it look like `tests/test_repo_shape.sh`.
- `tests/test_secrets.sh` — R-SEC-01 (row 11). §Plan step W adds its wiring case; the fixture
  it needs is already built by the rejection case below the real scans. Its two `LIVE` labels
  are what bind the tree and history scans separately (§Plan step 2).
- `tests/test_tool_versions.sh` — R-TOOL-01 and R-TOOL-02 (rows 12-13), four `LIVE` labels and
  four rejection stubs. §Plan step W adds two wiring cases and makes `n/4` a floor. `resolve`,
  `ver_num`, `check_version` and `arm_compiles` are all mutated by Table 3's neutering
  property — `arm_compiles` only since round 11, when the harness's brace walker learned to
  skip comments.
- `tests/test_rule_traceability.py` — R-PROC-01 (row 14) and the nine failure modes that are
  Table 4's last enumeration. `check()` is its shared verdict and `make_repo` builds the
  fixture §Plan step W's wiring case needs. Not mutated by Table 3 (Table 5 says why).
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
     `FAIL:` when the floor is not met, as `tests/test_secrets.sh` already does. The build was
     never fooled; the reader was, and this phase is read by an operator.
     **A third file, added round 10, and the earlier draft of this item was wrong about it.**
     That draft named `test_repo_shape.sh` among the files already doing the right thing. It
     is not one: when round 9 removed that file's rejection-case floor — correctly, under
     §How counts are stated — it left `echo "  ok:   rejection cases: $rejected"` printing
     unconditionally with `[ "$rejected" -gt 0 ] || fail=1` on the line below, which is this
     item's own banned shape reintroduced by the round that was closing it. The rule is
     general and has no exception for a floor of one: no `ok:` line anywhere reports a count
     that the next comparison may reject.
   - **(0d) `verify.md` must not promise what a shared function does not buy.** Its round-8
     paragraph tells the operator that the real run and the rejection case calling the same
     function is "what makes an `ok:` line evidence that something ran". It is evidence that
     the *function* works. Rewrite it around the harness, and keep it a durable operator
     reference — which means it states only what §Goal's *Observable behaviour* clause
     guarantees, per §Goal *What `verify.md` may assert about the suite*.
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
   `grep -rn 'ok:   .*rejection cases' tests/ | wc -l` → **`4`**, and each of those four lines
   must sit inside the branch taken only when its floor is met. The grep cannot see a branch,
   so that half is read, not run — and the earlier phrasing ("equals the number of files that
   print one inside a floor-passing branch") was circular: it compared a number to itself and
   could not fail. Today three of the four are inside such a branch and
   `tests/test_repo_shape.sh:220` is not, which is what 0c above owes. `make test` → exit 0.

1. **The optional-tool flag is one flag** — touches `tests/test_style.sh`, `Makefile`.
   `STYLE_OPTIONAL` is `OPTIONAL_TOOLS`, so one flag governs every check needing a tool
   outside the C++23 + `python3` floor (R-PROC-04). `-UNDEBUG` is already in `CXXFLAGS` from
   the ADR-0008 change — verify it is still there rather than adding it again.
   — check: `grep -c STYLE_OPTIONAL Makefile tests/test_style.sh` → `0` in both;
   `grep -c UNDEBUG Makefile` → `1`; `make test` → exit 0.

2. **Harness property 1 — accounting: every rule declared by a file in Table 5 produces a
   result line.** Touches the new `tests/test_checks_are_live.py` and the header of each file
   in **Table 5** — seven files, including `tests/test_style.sh`, whose three ids are not
   among the fourteen and are held to this property anyway because accounting is a property
   of the file set, not of the fourteen. Each check file
   already declares its rules as `# RULE <id> — …` header comments. The harness runs each
   check file for real, captures its output, and requires that **every id declared by a
   Table 5 file appears in at least one `ok:`/`FAIL:` line that the real run alone
   produces**, and that every
   `ok:`/`FAIL:` line naming a rule id declares that id in the header. Both directions: an
   undeclared id in the output is as much a drift as a declared id with no line.
   **`tests/test_style.sh`'s result-line shape**, the one file in Table 5 whose output no
   round wrote down — round 12's two `undecidable` verdicts are both this gap, and its
   Table 5 ✓ is unreproducible without the rows below. It declares **three** ids and has
   **nine** emission sites across two independent `if`/`elif`/`else` chains — five in one and
   four in the other, no site shared between them. Exactly one site per chain fires on any run,
   so a run **on a machine where both clang tools resolve** produces exactly **two** result
   lines, one per row below; where one does not, that chain's site emits `skip:`, which is not
   a result line, and the file is unproven per the rule above. Measured by reading
   the file, not by running it, since most of these branches need a tool absent or a source
   present — so this is a **dated measurement of one file's source**, not a count a check
   prints. §How counts are stated governs the second kind: a printed count needs a floor
   because the enumeration behind it grows. These rows are the first kind, the same shape as
   every *today* column in §Goal's tables, and like those they carry no floor and are true of
   the tree they were taken against. Nothing detects the day `test_style.sh` gains a branch;
   that gap is real, is not this phase's to close, and is recorded in `notes.md`
   §For later phases:

   | ids on the line | emission sites | prefixes | separator between the ids |
   |---|---|---|---|
   | `R-STYLE-01` alone | 5 — tool absent, config invalid, no sources, formatting differs, clean | `ok:` ×2, `FAIL:` ×2, and one skip-or-fail site of its own: `skip:` under `OPTIONAL_TOOLS=1`, `FAIL:` without it | n/a |
   | `R-STYLE-02` **and** `R-CLEAN-02` together | 4 — tool absent, no checkable sources, a diagnostic, clean | `ok:` ×2, `FAIL:` ×1, and a **second, distinct** skip-or-fail site of the same shape | **`,` on the `ok:` lines, ` / ` on the `FAIL:` and `skip:` ones** |

   Two things follow, and neither is stated anywhere else. **Accounting accepts several ids
   on one line**: the harness applies `RULE_IN_LINE` to each result line and counts every id
   it finds, so one line discharges two declarations and this file's three ids are satisfied
   by two lines. A criterion reading "one line per declared rule" would fail it, which is why
   §Goal's phrasing is *produces a result line*, not *produces its own*. And **`,` versus
   ` / ` is a branch, not a typo.** Anything quoting one of these lines is quoting one branch
   out of nine and is wrong for the rest — round 12 checked §3 of `verify.md` (which quotes
   the `FAIL:` branch, and is verbatim correct) against the clean tree's `ok:` line, and
   reported a defect that is not there. That is the concrete reason §Plan step 8 forbids
   `verify.md` quoting a result line at all.
   **"The id appears in some line" is not that criterion, and rounds 1-9 passed under it.**
   Every check ships rejection and accept cases and their lines name the rule too, so the
   question to ask is the inverse of round 9's: *does a line that is not the real run carry
   this id?* — and the answer is yes for every check in this phase, which is why deleting the
   real `sweep` from `tests/test_boundaries.sh` or the real run from
   `tests/test_phase_docs.sh` left both files, and the harness, green. The harness separates
   the two by the house convention that a case line says which case it is (`rejection case`,
   `false-positive case`, `accept case`, `wiring case`), and a declared id reported by nothing
   but case lines is a deleted real run, not a pass. The convention lives in the harness as
   one regex over the output, so it reaches a check written tomorrow and costs no declaration
   in the six check files.
   One gap the convention cannot close: a rule whose check makes **more than one independent
   real-run call**, where deleting one call still leaves a real-run line carrying the id —
   R-SEC-01 (working tree *and* history; `notes.md` F2) and R-TOOL-01 (four probes). Those
   need one label per call. So a check file may declare `# LIVE <id><rest>` lines
   alongside its `RULE` lines, where everything after the id is the literal prefix of the
   result line that proves the call happened (`# LIVE R-SEC-01 (history)`,
   `# LIVE R-TOOL-01: clang-tidy`). Each declared label must prefix **exactly one** result
   line, **and that line must be one of the real run's**. Not a substring, and not merely
   at-least-one: `(history)` also occurs inside `R-SEC-01 false-positive case (history)`, so
   a substring test was satisfied by a case line while the scan it named had been deleted.
   `RULE` without `LIVE` means one real-run call, which the case-line convention already
   pins. R-SEC-01 (tree, history) and R-TOOL-01 (four tool probes) are the two rules that
   need labels.
   **A *result line* is an `ok:` or a `FAIL:` line, and a `skip:` line is not one** — the
   accounting criterion is stated over `ok:`/`FAIL:` lines throughout and this names it once,
   because two different rows of this spec turned on the answer.
   **Any skip makes the file unproven, not passing** — *any* skip line, not only a wholly
   skipped file. `OPTIONAL_TOOLS=1` turns a missing external tool into a skip; a check that
   probes several tools emits `skip:` for the absent one and `ok:` for the rest, and that is a
   partial proof, which is not a proof. The harness must report `unproven: <file>` naming the
   skips and must not count it as satisfied. This is what keeps a machine where only one of the
   two clang tools resolves from reading as accounting drift: the file is unproven, not short a
   declared id.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing one `ok:` line per check
   file with its rule-and-label count; and, on a copy of the repository, each of these four
   deletions makes it exit non-zero. The first two are the ones that survived round 9, and a
   criterion that catches only some of the four has moved the defect rather than closed it:
   the real `sweep "$ROOT"` block in `tests/test_boundaries.sh` (→ `R-ARCH-02` reported by
   nothing but its own cases), the real `missing_verify "$ROOT/…"` block in
   `tests/test_phase_docs.sh` (→ the same, for `R-PROC-02`), the
   `report R-ERR-04 "$( find_err04 "$1" )"` line in `tests/test_repo_shape.sh`, and the real
   `scan git "$ROOT"` block in `tests/test_secrets.sh` (→ the missing `(history)` label).

3. **Harness property 2 — neutering: a check that finds nothing must fail its file.** Touches
   the harness. For every check function in every shell check file — located by a declared
   naming convention, not a hand-written list — produce a copy of the file with that
   function's body replaced by a body that finds nothing, run the copy, and require a non-zero
   exit. **Every function the file defines, with no naming convention** — that is what makes
   this generated rather than enumerated.
   **Amended round 11.** This step used to declare a convention ("a shell function whose name
   begins `find_`, `check_`, `sweep` or `scan`") and it failed twice over: the harness had
   grown a fifth name this step never declared, while `arm_compiles` — the whole of
   R-TOOL-02 — plus `ver_num` and `resolve` fell outside it, unmutated and reported nowhere,
   because the harness printed only the set it *discovered*. A convention needs a list and
   the list rots. Measured before the convention was removed: neutering **each** of the 25
   parsable functions in the five shell checks makes its file exit non-zero — finders, shared
   helpers and case drivers alike — so there is nothing for a convention to exclude. The
   count is now 26, `arm_compiles` included; it was unparsable until the harness's brace
   walker learned to skip comments, an apostrophe in "R-TOOL-01's own floor" having opened a
   quote state that ran to end of file and dropped the function silently.
   — check: `python3 tests/test_checks_are_live.py` → exit 0, printing the discovered function set
   and `neutered: n/n caught` with `n` equal to that set's size; and, with one hand-written
   rejection case deleted from a copy of `tests/test_repo_shape.sh`, the harness exits
   non-zero naming the function whose neutering then went uncaught.

4. **Harness property 3 — alternation: removing any alternative must fail its file.** Touches
   the harness. For each check function found in step 3, extract its extended-regex pattern
   from the source, split it into alternatives at **every nesting depth** — every `|` outside
   a `[…]` class, at depth 0 and inside every `(…)` group — and for each alternative produce
   a copy of the file with that alternative
   removed *cleanly* — dropping the alternative together with one adjacent `|`, never leaving
   an empty alternative, which would match everything and fail for the wrong reason. Run each
   copy; require a non-zero exit.
   **Both quoted strings of a finder, not just the first (amended round 11).** The house
   shape is `find_xxx( ) { hits '<pattern>' '<exclusions>' $( core_files "$1" ); }` and the
   exclusion list is as much the check's pattern as the inclusion one: dropping `can|` from
   `find_clean03`'s `(is|has|can|should)_` left the file and the harness green while
   `bool can_fire = true;` silently became a false positive. What covers an exclusion
   alternative is an **accept** case, never a rejection case — the property needed no other
   change to reach it, and it immediately demanded the two accept cases §Plan step 6 now
   enumerates as the twelfth and thirteenth. Scope is stated by the shape rather than by a
   list: a finder's body is **one line**, so its quoted strings are patterns; a multi-line
   body is a helper and its quoted strings are `sed`/`printf` expressions
   (`hits`'s own `s|//.*||` splits on `|` like a regex and yields mutants that are "caught"
   because sed breaks, inflating the count while proving nothing). The harness prints the
   functions it found no pattern in, so a finder that stops being a one-liner is visible. Report the count and, on failure, name the file, the
   function and the exact alternative that no case covers.
   **Depth is the property, not a detail.** `find_arch01` is one top-level branch wrapping
   two groups of 7 and 24 prefixes, so splitting at depth 0 alone yields *one* mutant for a
   pattern with thirty-odd forbidden forms — precisely the gap round 8 shipped. The round-9
   draft of this step said "top-level" while the paragraph below it argued for depth and the
   harness implemented depth; the word was stale and is corrected here rather than in the
   code.
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
   - **The two structural requirements the harness cannot supply are SV and WC, and their
     per-rule state is Table 2 — not this paragraph.** This is where round 11's `contradicts`
     came from: the requirement used to be stated here in prose, naming `report( )` and
     `run_all`, two mechanisms that exist in one file each, so a file satisfying the property
     by another means read as a violation and four files silently satisfying neither read as
     nothing at all. The definitions live in §Goal, the members live in Table 2's rows, and
     **the mechanism column is per file and is not required to be the same function**:
     `report( )`, `sweep( )`'s exit-code `case`, `scan( )`'s return, `check_version( )`'s
     floor comparison and `check( )`'s problem list each satisfy SV in their own file. What
     a Plan step may say here is which rows are owed, and §Plan step W says it.
   - **The thirteen accept cases, by name, because a floor must equal an enumeration.**
     `tests/test_repo_shape.sh` fails below `false-positive cases: 13`, and §How counts are
     stated forbids a count with no list beside it — round 10 returned that floor as
     `undecidable` for exactly this reason. Accept coverage is *not* generated by the harness
     (it mutates what a check finds, never what it must ignore), so it is an enumeration and
     the floor is `>= 11`: `= delete;` is not an allocation; `std::string_view` allocates
     nothing; an `is_`-prefixed bool; an `m_has_`-prefixed member bool; a `TODO` carrying a
     phase reference; the freestanding headers `<cstdint>`, `<array>`, `<span>` and
     `<expected>`; `<string_view>` is not `<string>`; and an `enum class` with a fixed
     underlying type. Round 11's alternation change added the other two prefixes R-CLEAN-03
     exempts — a `can_`-prefixed bool and a `should_`-prefixed bool — which is where the
     twelfth and thirteenth come from, and the floor is `>= 13`. Thirteen names, thirteen the
     floor: a fourteenth accept case raises both or neither.
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
   report**: each break/run/restore cycle must be runnable literally and in order, and
   anything true only while this phase was being written belongs in `notes.md`. Include the
   clang-tidy limitation where an operator will actually meet it — the fact, its two ids and its
   condition are §Goal's *Observable behaviour*, and this step neither narrows them to one id
   nor adds the diagnostic's wording.
   **What `verify.md` may assert is not this step's to define, and round 13's whole finding
   was that it had been treated as if it were.** The authority is §Goal, *What `verify.md` may
   assert about the suite*, and this step does not paraphrase it — a paraphrase is how the
   scope word went missing three times over. Read it there. Three wordings were tried here
   first (properties; a sourced vocabulary table; a decides/formats axis) and each split on a
   different sentence, because a Plan step cannot ground a claim the Goal never made. This step
   implements that clause and may neither narrow nor widen it.
   Commands are the one thing this step adds to it, and they stay literal: they are runnable,
   and this step's check runs them.
   What this costs: `verify.md` cannot teach the operator to read a specific line's layout.
   Everything §Goal guarantees survives, and a round that needs more amends the clause.
   **Why the property and not "keep the sample current".** This document has gone stale on
   quoted output in **four** rounds out of twelve, always because the round that broke it was
   a round that changed a check — which is every round (`notes.md`): round 2, a sample
   failure the code could not produce; round 3, an R-TOOL-01 sample whose floor and found
   version round 2 had swapped; round 9, a §3 claim not checkable from its inputs; round 12,
   a two-line promise against a three-line check plus a wiring paragraph that was never
   written. Each time the fix applied was the instance — update the sample — and each time it
   returned. That is F2 at the documentation level, and the class-level fix is to stop
   quoting. It also removes a defect no round named: round 2 fixed §2 by pinning the absolute
   path the check prints, and what shipped is `/path/to/pico-sg2hid/…` — a **placeholder**
   inside a block the surrounding sentence sells as what the check really prints. It is
   neither a transcript nor a property, and it is the shape a literal-output rule can never
   settle, because the moment the sample has to be machine-independent it stops being the
   sample. Two things settle it against "be careful": the
   §Acceptance criteria grep below, which is decidable by a reviewer who reads nothing else,
   and round 12's own experience of the alternative — a reviewer holding the literal output
   against §3 still got it wrong, because *which* literal line is correct depends on a
   branch the document does not fix (§Plan step 2, `test_style.sh`'s nine emission sites).
   The rejected alternative was **generating `verify.md` from the tree**, so that the round
   desynchronising it fails. It buys the same property for the cost of a generator plus a
   template, and a generated operator document stops being written for the operator, which
   is the only thing this file is for.
   — check: `sh tests/test_phase_docs.sh` → exit 0;
   `test -s docs/phases/00-scaffold/verify.md`; every command `verify.md` prints can be pasted
   into a shell in the order given, and each command the document says should fail produces a
   failing run naming the rule it promises — *naming the rule*, not printing a particular
   prefix, because §Goal's clause guarantees the id and not the line that carries it; and the
   no-transcript rule is a grep, not a reading: extract every `ok:`/`skip:`/`FAIL:` run in
   the file up to the next backtick and require each to be either a bare prefix (prose
   naming the shape) or `FAIL: <id>` with nothing after the id — §Acceptance criteria has
   the exact command and its expected **`0`**.
   **What it catches and what it is blind to, measured 2026-09-10 rather than reasoned about,
   because an earlier draft of this paragraph claimed a reach it does not have.** It catches a
   result line whose text continues *on that line* after the id — the deleted
   `ok:   R-TOOL-01: … 12+ (15.3.1)` sample scores 1 — and an inline backtick quote carrying
   the text after the id. It is blind to two shapes. One is a line *count* in prose ("expect
   two lines") which quotes nothing. The other is a **fenced transcript whose payload sits on a
   continuation line**: the block §2 shipped for four rounds —
   `FAIL: R-ARCH-03` with an indented path underneath — scores **0**, because the first line
   matches the bare-id whitelist and the second carries no prefix and is never extracted at
   all. That is the shape the rule was written for, and the grep cannot see it.
   So the grep is a floor and a narrow one: the clause in §Goal is what a reviewer applies, and
   two of the shapes it bans reach the tree only through reading. Widening the grep to read a
   fenced block's continuation lines was considered and not done — it would have to treat any
   indented line after a prefix line as output, which this spec's own prose would trip.

9. **Correct the `PHASES.md` row's acceptance text** — touches `docs/phases/PHASES.md`. The
   row promises "every check ships a rejection case proving it fails on a bad tree". Round 8
   proved a rejection case is necessary and not sufficient: eight mutations survive it
   (`notes.md` §Deviations). Reword the coarse acceptance to match this spec's Goal — every
   check is proven live by mutations generated from its own source — leaving the row's id,
   goal and dependencies untouched.
   — check: `grep -n '^| 00-scaffold' docs/phases/PHASES.md` shows the corrected text;
   `python3 tests/test_rule_traceability.py` → exit 0 (it reads this table).

W. **Close the wiring-case gap: Table 2's WC column reaches 14/14** — touches
   `tests/test_boundaries.sh` (R-ARCH-02), `tests/test_secrets.sh` (R-SEC-01),
   `tests/test_tool_versions.sh` (R-TOOL-01, R-TOOL-02) and
   `tests/test_rule_traceability.py` (R-PROC-01). Five rules, one step, because they are one
   defect: the rule's verdict is printed and never reaches the exit code, so deleting the
   `fail=1` on its failure path leaves the suite green.
   A wiring case is the shape §Goal defines and `tests/test_repo_shape.sh` already implements
   eight times: drive **the real aggregate** — not the finder — over a tree carrying exactly
   that rule's violation, then require **both** that the captured output names the rule
   **and** that the aggregate returned non-zero. Two assertions; either alone is satisfied by
   the defect.
   `test_boundaries.sh` and `test_secrets.sh` already build such a tree for their rejection
   cases, so the case is the assertion, not the fixture. `test_tool_versions.sh` has its four
   stubs. `test_rule_traceability.py` has `make_repo`. Nothing here needs a new fixture.
   Same step, same files, two items Table 6 puts in scope:
   `tests/test_boundaries.sh`'s `rejection cases: n/2` gets an `ok:`/`FAIL:` prefix so the
   harness's accounting can see it, and its `-eq 2` becomes `>= 2`; `test_tool_versions.sh`'s
   `n/4` and `test_rule_traceability.py`'s `n/9` become floors too (Table 4).
   — check: **before**, one at a time, each must leave `make test` at exit 0 — that is the
   defect, and an implementer who cannot reproduce it has not found the right line:
   `fail=1` → `:` at `tests/test_secrets.sh:69`; the same at `tests/test_boundaries.sh:66`;
   `|| fail=1` dropped from the `clang-tidy` call in `tests/test_tool_versions.sh`; the
   equivalent for R-TOOL-02 and for R-PROC-01. **After**, every one of those five must make
   `make test` exit non-zero, naming its rule. Then `sh tests/test_boundaries.sh` prints
   `ok:   rejection cases: n/2` and `grep -rn 'ok:   .*rejection cases' tests/ | wc -l` → `5`,
   all five inside a floor-passing branch.

T. **The orphaned mutants** — touches `Makefile` (or `tests/test_checks_are_live.py`). A
   `make test` killed mid-run leaves `tests/mut_*.sh` behind: the harness unlinks each mutant
   in a `finally`, which `SIGTERM` skips. Observed in rounds 9, 10 and 11, and the orphans sit
   where `tests/test_rule_traceability.py` walks for `RULE` markers and where the gitleaks
   tree scan reads. A `trap` that removes `tests/mut_*.sh`, or the same removal at harness
   start-up; not a `.gitignore` line, which hides them instead of removing them.
   — check: `python3 tests/test_checks_are_live.py` → exit 0 and `ls tests/mut_*.sh` finds
   nothing afterwards; then kill a run mid-flight (`timeout 5 make test` or Ctrl-C) and
   `ls tests/mut_*.sh` still finds nothing.

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
# verify.md quotes no result line: the only one allowed anywhere in it is a bare `FAIL: <id>`,
# and a backtick-quoted bare prefix ("one `ok:` line per rule") is prose, not a transcript.
grep -oE '(ok:|skip:|FAIL:)[^`]*' docs/phases/00-scaffold/verify.md \
  | grep -vcE '^(ok:|skip:|FAIL:|FAIL: R-[A-Z]+-[0-9]{2}) *$'                 # expect: 0
test -s docs/phases/00-scaffold/notes.md           # expect: exit 0
time make test                                     # expect: real under 2m0s (the harness runs each check once per generated mutant)
```

No **rejection**-case floors appear above for the files the harness covers: the harness is
what asserts their sufficiency, and a floor beside it would be the round-8 defect — a count
and a universal claim about the same thing, drifting apart. What remains is every count the
harness does *not* generate, each an enumeration named in its own Plan step: the nine
traceability failure modes (`test_rule_traceability.py`), the two boundary cases
(`test_boundaries.sh`), the two secret-scan cases and the four tool probes
(`test_secrets.sh`, `test_tool_versions.sh`), and the thirteen **accept** cases of §Plan step 6
— accept coverage is enumerated for the same reason, since mutating what a check finds says
nothing about what it must ignore. An earlier draft of this paragraph said "the two floors
that remain" and named two of those five; it was wrong, and round 10's reviewer found the
accept floor through the gap it left.

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
p.write_text(s.replace('    report R-ERR-04   "$( find_err04   "$1" )" || fail=1\n', '', 1))
EOF
make test                                          # expect: non-zero — R-ERR-04's wiring case fires first
                                                   # (run_all leaves fail=0), then the harness fails the file
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
i = s.index('    if scan git "$1" "$ra_hist"; then'); j = s.index('\n    fi\n', i) + 8
p.write_text(s[:i] + s[j:])        # delete the history scan from inside run_all (step W moved it)
EOF
make test                                          # expect: non-zero — the R-SEC-01 (history) wiring case fires,
                                                   # and the LIVE label prefixes no line
cp /tmp/se.bak tests/test_secrets.sh ; make test    # expect: exit 0, "OK"

# The two that survived round 9 and are the reason the accounting criterion was rewritten.
# A check whose rejection cases name its rule reports that rule whether or not the real run
# still happens, so these two must stay in this block permanently.
cp tests/test_boundaries.sh /tmp/bd.bak
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_boundaries.sh'); s = p.read_text()
p.write_text(s.replace('\nrun_all "$ROOT"\n', '\n', 1))   # delete the real sweep's call site
                                                   # (step W wrapped it in run_all)
EOF
make test                                          # expect: non-zero — R-ARCH-02 reported by nothing but its own cases
cp /tmp/bd.bak tests/test_boundaries.sh ; make test # expect: exit 0, "OK"

cp tests/test_phase_docs.sh /tmp/pd.bak
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_phase_docs.sh'); s = p.read_text()
p.write_text(s.replace('run_all "$ROOT/docs/phases/PHASES.md" "$ROOT/docs/phases" || fail=1\n', '', 1))
EOF
make test                                          # expect: non-zero — R-PROC-02, same message
cp /tmp/pd.bak tests/test_phase_docs.sh ; make test # expect: exit 0, "OK"

# And the shared verdict: gutting it must break every case, not only the real run's copy.
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_phase_docs.sh'); s = p.read_text()
p.write_text(s.replace('if [ -n "$2" ]; then', 'if false; then', 1))
EOF
sh tests/test_phase_docs.sh                        # expect: non-zero — rejection AND wiring case fail
cp /tmp/pd.bak tests/test_phase_docs.sh
rm -f /tmp/rs.bak /tmp/se.bak /tmp/bd.bak /tmp/pd.bak
```

**Wiring block — Table 2's WC column, one mutation per failure path.** Each must make
`make test` exit non-zero and name its rule. The eight rows held by `tests/test_repo_shape.sh`
are covered by its `wiring cases: 8/8` line and by the `|| fail=1` mutation in the liveness
block above; these are the rest. **Six mutations for five rules**: R-SEC-01 has two
independent failure paths — the tree scan and the history scan, the same two its `# LIVE`
labels pin — and a rule with N independent failure paths needs N wiring cases, because one
fixture that trips both is satisfied by either flag alone. That was built the wrong way first
and passed while each branch's flag was deleted in turn.

Anchored on content, not on line numbers: an earlier draft of this block named line 69 and
line 66, step W moved both, and a criterion that mutates nothing reports the exit 0 it was
given. Run one at a time; each restores the file before the next.

```
for f in tests/test_secrets.sh tests/test_boundaries.sh \
         tests/test_tool_versions.sh tests/test_rule_traceability.py; do
    cp "$f" "/tmp/$( basename "$f" ).bak"
done
drop() { python3 -c "$1"; make test; cp "/tmp/$( basename "$2" ).bak" "$2"; }
# each `make test` below: expect non-zero, naming the rule in the comment

drop "import pathlib;p=pathlib.Path('tests/test_secrets.sh');s=p.read_text();i=s.index('gitleaks found secrets in the tree');j=s.index('        fail=1',i);p.write_text(s[:j]+'        :     '+s[j+8:])" tests/test_secrets.sh              # R-SEC-01 (working tree)
drop "import pathlib;p=pathlib.Path('tests/test_secrets.sh');s=p.read_text();i=s.index('secrets in the commit history');j=s.index('        fail=1',i);p.write_text(s[:j]+'        :     '+s[j+8:])" tests/test_secrets.sh                   # R-SEC-01 (history)
drop "import pathlib;p=pathlib.Path('tests/test_boundaries.sh');s=p.read_text();i=s.index('forbidden dependency direction');j=s.index('        fail=1',i);p.write_text(s[:j]+'        :     '+s[j+8:])" tests/test_boundaries.sh              # R-ARCH-02
drop "import pathlib;p=pathlib.Path('tests/test_tool_versions.sh');s=p.read_text();p.write_text(s.replace('probe( ) { check_version \"\$@\" || fail=1; }','probe( ) { check_version \"\$@\"; }',1))" tests/test_tool_versions.sh        # R-TOOL-01
drop "import pathlib;p=pathlib.Path('tests/test_tool_versions.sh');s=p.read_text();i=s.index('A cross-compiler with no target C library');j=s.index('            fail=1',i);p.write_text(s[:j]+'            :     '+s[j+12:])" tests/test_tool_versions.sh  # R-TOOL-02
drop "import pathlib;p=pathlib.Path('tests/test_rule_traceability.py');s=p.read_text();i=s.index('rule/test traceability');j=s.index('        return True',i);p.write_text(s[:j]+'        return False'+s[j+18:])" tests/test_rule_traceability.py  # R-PROC-01

rm -f /tmp/*.bak ; make test                       # expect: exit 0, "OK"
```

**Floors — Table 4's four floor rows, and none of them an equality.**

```
sh tests/test_boundaries.sh                        # expect: "ok:   rejection cases: n/2", n >= 2
sh tests/test_tool_versions.sh                     # expect: "ok:   rejection cases: n/4", n >= 4
python3 tests/test_rule_traceability.py            # expect: "ok:   R-PROC-01 rejection cases: n/9", n >= 9
grep -rn 'ok:   .*rejection cases' tests/ | wc -l  # expect: 5, each inside a floor-passing branch
```

**Orphaned mutants — §Plan step T.**

```
python3 tests/test_checks_are_live.py ; ls tests/mut_*.sh   # expect: harness exit 0, ls finds nothing
( make test & sleep 20 ; kill %1 ) ; ls tests/mut_*.sh      # expect: ls finds nothing after a killed run
```

**Clean-clone block — the floor the `PHASES.md` row promises.** `make test` must pass on a
machine with C++23 and `python3` and nothing else, reporting every check it could not prove
rather than failing. Simulate it by putting only `python3` and the system tools on `PATH`:

```
mkdir -p /tmp/shim && ln -sf "$(command -v python3)" /tmp/shim/python3
PATH=/tmp/shim:/usr/bin:/bin:/usr/sbin:/sbin make test
# expect: exit 0, "OK", with one `skip:` line per absent tool and one `unproven:` line per
# check the harness therefore could not prove — never `does not pass on the real tree`
rm -rf /tmp/shim
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
rm -f src/core/x.h

printf 'bool can_fire = true;\nbool should_retry = false;\n' > src/core/x.cpp
sh tests/test_repo_shape.sh                         # expect: exit 0 — R-CLEAN-03's exclusion
                                                    # list covers all four prefixes, not two
rm -f src/core/x.cpp ; rmdir src/core src 2>/dev/null
```

This one runs the repo-shape check rather than `make test`, and the reason is a limitation
worth knowing before `01-ps2-codec` writes its first header — recorded as a finding in
`docs/constraints.md` §Observed conventions, which is where `CLAUDE.md` §Conventions sends a
verified fact about tooling behaviour: on this machine clang-tidy cannot resolve *any* standard
header without a `compile_commands.json`, so a scratch file under `src/core/` fails R-STYLE-02
as a `clang-diagnostic-error` rather than a naming complaint — a tooling result, not an
R-ARCH-03 one. **Two different diagnostics, and they are stated apart here because an earlier
draft of this sentence merged them and was wrong** (re-measured 2026-09-10): a file carrying an
`#include`, quoted or angled, fails with *that header not found*; a file carrying a bare `std::`
and no include fails with *`std` undeclared*, never with a not-found. §Goal's *Observable
behaviour* states the first and not the second — no universal about `verify.md`'s procedures is
asserted here, because §How counts are stated would need a table of them and the document's own
§3 already says which of its scratch files carry an `#include`. The second diagnostic is why
this block runs `sh tests/test_repo_shape.sh` rather than `make test`. The
working invocation that measured the underlying limitation is in `notes.md` §For later phases,
and the limitation itself is the finding in `docs/constraints.md` §Observed conventions, which
says `clang-diagnostic-error` and is correct as written.

## Out of scope

**Table 6 is part of this section.** It decides the three items round 11 left in
§For later phases; the one it puts out of scope — judging a mutant by more than its exit code
— is out for a stated reason with an owning phase, not by omission.


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
- **Making `make test` fast — except for the one thing the two-minute cap already forces.**
  The harness runs each check file once per generated mutant, so the suite's runtime grows
  with the number of alternatives. Measure it, state it in `verify.md`, and leave it.
  **Amended round 11, with the measurement that forced it:** this item used to say
  "parallelising is an optimisation nobody has yet needed", which contradicted the
  acceptance criterion two sections up. Serially — `workers = 1` — the harness alone takes
  **3m11s** for its 85 mutants on this machine, against a cap of **2m0s** for the whole
  suite; with the thread pool it is **56s**. A mutant is a subprocess that spends its life
  waiting on other subprocesses, so the pool is I/O concurrency, not tuning, and the cap
  cannot be met without it. Also in scope for the same reason: one shared `case_tmp` in
  `tests/test_repo_shape.sh` instead of a `mktemp -d` per case. What stays out of scope is
  everything past the cap — no profiling, no caching of check runs, no incremental mutant
  selection. When the cap is next threatened, the answer is to raise the cap in a
  re-expansion or cut mutants, not to optimise here.
- **Splitting this phase.** Considered and rejected three times. The checks share one harness
  and one set of conventions, and the liveness harness is meaningful only against all of them
  at once — two rows would duplicate the machinery for no gain.
- **The accumulated taste notes** — `.sh` file modes, `\b` versus `([^_[:alnum:]]|$)`,
  `find`-loop word splitting, `head -8` truncation, the `PATH=` prefix on a function call,
  `SKIP_DIRS` listing `node_modules`, `find_clean09`'s line-based `enum class` exclusion. All
  in `notes.md`; none changes what a rule catches. Fix them when touching the line for another
  reason.
