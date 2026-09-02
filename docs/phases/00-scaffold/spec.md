# Phase 00-scaffold — Make every rule that needs no product code real

<!-- Re-expanded 2026-09-02 (round 6). Rounds 1-5 patched the previous spec in place
     eleven times; round 4 diagnosed the amendment loop as the defect source and round 5
     escalated the phase to `pending` for one coherent rewrite. This file is that rewrite:
     derived from the PHASES.md row's Goal and from notes.md, not from the tree. Where the
     code that exists disagrees with what is written here, the code is what changes —
     §Plan step 0 lists exactly those five owed edits. notes.md is the account of the five
     earlier rounds and is never rewritten. -->

## Goal

After this phase, `make test` runs a suite that turns fourteen rules currently marked
`planned: 00-scaffold` into rules with deterministic tests, and the rule catalogue can no
longer drift from its tests without the build failing.

What exists afterwards that does not exist now:

- **Six new test files under `tests/`** — five `.sh` and one `.py`. The Makefile globs
  `tests/test_*.{cpp,sh,py}`, so all six are picked up with no Makefile edit.
- **`tests/test_rule_traceability.py`**, which parses `docs/constraints.md` §Invariants
  under ADR-0005's grammar and fails on drift in either direction: a `test:` path that does
  not exist, a `test:` path lacking its `RULE <id>` marker, a marker naming an undeclared
  id, a `manual:` rule carrying a marker anyway, a `planned:` phase that does not exist in
  `docs/phases/PHASES.md`, a `planned:` phase that is already `done`, a duplicate id, an
  unparsable rule line, and an id `CLAUDE.md` mentions that the catalogue does not declare.
  Nine failure modes, each with its own message naming the id and the file.
- **Five shell checks over the shape of the repository** — layer boundaries, `core` purity,
  no heap, no exceptions, boolean naming, TODO references, no inheritance in `core`, no
  `.value()`, test-vector provenance, per-phase `verify.md`, absence of secrets, and
  minimum tool versions.
- **A negative self-test on every check.** This is the load-bearing part of the phase and
  the reason it is not just fourteen greps. Almost every check here passes vacuously today
  — there is no code under `src/` yet — so a check that is silently broken is
  indistinguishable from a check that is passing. Each check therefore builds a
  deliberately-bad tree in a temp directory and asserts that the check *rejects* it, and,
  wherever the pattern could plausibly misfire on legitimate code, an **accept** case that
  asserts it does not. A check without its rejection case is not finished; a rejection case
  that would also pass when the check cannot run is not a rejection case (round 5's
  confirmed defect — see Plan step 0).
- **Fourteen rules in `docs/constraints.md` moved from `planned: 00-scaffold` to
  `test: <path>`**, each test file carrying the matching `RULE <id>` marker comment.
- Where a rule turns out to have two clauses that no single binding can honestly cover —
  one checkable now, one a property of code this phase is forbidden to write — **splitting
  it is in scope**, and the half that cannot be checked becomes a new rule with a
  `planned: <phase-id>` binding. That happened once: R-ERR-03 kept the source clause (no
  `throw`/`try`/`catch` under `src/`) and its flags clause became **R-ERR-05** (firmware
  builds pass `-fno-exceptions -fno-rtti`), `planned: 03-pio-bus`. A split leaves the count
  at fourteen bindings moved — the new rule is debt this phase declares, not a fifteenth
  binding it delivers.
- **`docs/phases/00-scaffold/verify.md`**, the operator-facing procedure (R-PROC-02).

Observable behaviour: `make test` exits 0 on the clean repo, and exits non-zero — naming
the rule id and the offending path — when any of the fourteen violations is introduced by
hand. The §Acceptance criteria adversarial block is that behaviour, written out.

### How counts are stated in this spec

Rounds 2-5 produced three separate validation findings from one cause: an exact case count
written in a Plan step and again in the Acceptance criteria, with one of the two left stale
by an amendment. This spec fixes the shape rather than the numbers. **Every case is
enumerated by name in its Plan step; every count is a floor (`>= n`) equal to the number of
names in that step.** A floor cannot contradict its own enumeration, still fails when a
case disappears, and does not have to be edited when a later round adds one. Where a
number appears twice anyway, the Plan step is authoritative.

## Context pointers

- `CLAUDE.md` — the session contract; §"Rules are bound to tests" states the binding this phase implements.
- `docs/constraints.md` — the rule catalogue. §Invariants is what the meta-test parses, and the fourteen `planned: 00-scaffold` lines are the work list. The binding grammar is in the section preamble.
- `docs/adr/0005-rule-test-traceability.md` — why the binding exists, the three-binding grammar, and what each failure mode must report.
- `docs/adr/0002-hardware-free-core.md` — the layering the boundary and purity checks enforce, and why `core` must not reach the SDK.
- `docs/adr/0007-error-model.md` — why `-fno-exceptions` is a rule (R-ERR-03 source clause, R-ERR-05 flags clause); its decoding half is superseded by ADR-0009.
- `docs/adr/0008-cpp23.md` — the C++23 decision, the version measurements behind R-TOOL-01's floors, and the two-toolchain `PATH` trap R-TOOL-02 exists to catch.
- `docs/adr/0009-std-expected.md` — why `.value()` is forbidden and must be a grep (R-ERR-04), not a comment.
- `.claude/workflow/boundaries.rules` — the machine-readable layer edges. `tests/test_boundaries.sh` drives the existing hook with this file; it does not reimplement it.
- `.claude/hooks/boundary-check.sh` — read its header before wrapping it. It takes one file path and produces **exactly two exit codes**: `2` a forbidden dependency, `0` everything else (clean *or* no rule reached the file). Its header states that no third code is produced and none may be added. It is a per-file gate, so sweeping the tree is the wrapper's job.
- `.claude/hooks/lib/common.sh` — line 22, `ROOT="${CLAUDE_PROJECT_DIR:-$PWD}"`. The hook reads `$ROOT/.claude/workflow/boundaries.rules`, so a fixture tree can only be judged if it carries its own copy of the rules file and the hook is invoked with `CLAUDE_PROJECT_DIR` pointing at the fixture. That is what lets the rejection cases run against the real hook at its real path.
- `tests/test_style.sh` — the pattern every new check follows: `RULE` markers in the header, a skip for a missing external tool, one `fail` accumulator, a `FAIL:` line naming the rule. Copy its shape.
- `Makefile` — `test` globs `tests/test_*.cpp|.sh|.py` and sets `OPTIONAL_TOOLS=1`, so a missing external tool becomes a skip. `CXXFLAGS` already carries `-UNDEBUG`. `lint` runs `tests/test_style.sh` **and nothing else**, so a check needing `gitleaks` or the ARM toolchain can never break `make lint`.
- `docs/phases/PHASES.md` — the phase table the traceability test reads to validate `planned:` targets, and whose `status` column drives R-PROC-02.
- `docs/phases/00-scaffold/notes.md` — the record of rounds 1-5: every deviation, the accepted debt, and the four `contradicts` classes that have actually fired here. Read §Debt and §For later phases before changing any check.
- `docs/templates/notes.md` — the shape of the `notes.md` this phase leaves behind.

No dependency phases, so there is no dependency `notes.md` to absorb.

## Plan

0. **Close what round 5 left owed** — touches `tests/test_boundaries.sh`,
   `tests/test_repo_shape.sh`, `tests/test_secrets.sh`,
   `docs/phases/00-scaffold/verify.md`. These five items are the difference between the
   code as it stands and what this spec requires; the code is what changes.
   - **(0a) A rejection case must not pass when the hook cannot run.** Round 5's confirmed
     `contradicts`: `test_boundaries.sh`'s positive rejection case is `if sweep …; then FAIL
     else ok`, so `sweep`'s "the hook did not run" return lands in the `ok` branch. With a
     broken hook the whole file prints three `ok:` lines and exits 0 — a check that enforces
     nothing while looking green, which is the exact failure this phase exists to make
     impossible. Assert the specific code: `sweep "$tmp" >/dev/null 2>&1; [ $? -eq 1 ]`.
   - **(0b) R-ARCH-03 must not fire on `std::string_view`.** A bare `std::string`
     alternation matches `std::string_view sv;`, which allocates nothing and is legal in
     `core` — the phase's own R-ARCH-01 accept case already declares `#include <string_view>`
     legal, so the two halves currently contradict each other. Anchor the match so the
     identifier must end (`std::string([^_[:alnum:]]|$)` is portable; `\b` also works on
     this machine's grep) and add the accept case named in step 5.
   - **(0c) The `= delete` accept case is vacuous.** Its input is
     `Bus( const Bus& ) = delete;`, and the base pattern requires `delete` followed by
     whitespace and an identifier — so the input never reaches the `=[[:space:]]*delete`
     exclusion it is named for, and deleting the exclusion leaves the case passing. No legal
     C++ line both matches the base alternation and needs that exclusion — every `= delete`
     form ends `delete;` — so the exclusion is unreachable: remove it and keep the accept
     case, which then pins the base pattern's "whitespace plus an identifier" requirement
     (tighten the pattern to a bare `delete` word match and the case must fail). A case that
     passes with the logic removed is not a check.
   - **(0d) A too-old `gitleaks` must not be reported as a leak.** `test_secrets.sh` hard-codes
     the `dir` and `git` subcommands; a gitleaks that does not know them exits non-zero and
     the check prints `FAIL: R-SEC-01: gitleaks found secrets`, a misdiagnosis of the class
     step 4 goes to lengths to prevent for the boundary hook. Probe capability, not version:
     `gitleaks dir --help` exits 0 where the subcommand exists and 1 where it does not
     (verified on gitleaks 8.30.1 here), so a failed probe reports `gitleaks too old` under
     its own `FAIL:` line. **gitleaks is deliberately given no R-TOOL-01 version floor** —
     the floor list would then need a number nobody here has measured, and the failure that
     matters is the misdiagnosis, not the version.
   - **(0e) `verify.md` §3 must be runnable literally.** It writes the same two scratch
     filenames four times over with a single cleanup after the block, so an operator
     following it line by line sees two of the four rules fire. Use §2's shape — break, run,
     restore — once per rule, matching the adversarial block below.
   — check: `sh tests/test_boundaries.sh` → exit 0; then, with `HOOK` pointed at a stub that
   always exits 3, the same file exits non-zero. `sh tests/test_repo_shape.sh` → exit 0.
   `sh tests/test_secrets.sh` → exit 0. `make test` → exit 0, `OK`.

1. **Generalise the optional-tool flag** — touches `tests/test_style.sh`, `Makefile`.
   `STYLE_OPTIONAL` becomes `OPTIONAL_TOOLS`, so one flag governs every check needing a tool
   outside the C++23 + `python3` floor (R-PROC-04). `-UNDEBUG` is already in `CXXFLAGS` from
   the ADR-0008 change — verify it is still there rather than adding it again.
   — check: `grep -c STYLE_OPTIONAL Makefile tests/test_style.sh` → `0` in both;
   `grep -c UNDEBUG Makefile` → `1`; `make test` → exit 0.

2. **Write `tests/test_rule_traceability.py`** (R-PROC-01) — touches that file. Parse
   `docs/constraints.md` with the ADR-0005 grammar, cross-check against
   `docs/phases/PHASES.md` and `CLAUDE.md`, and walk the repo for `RULE <id>` markers,
   skipping `.git/`, `build/`, `docs/` and `.claude/commands/`. Each of the nine failure
   modes in the Goal reports its own message naming the id and the file. Pure `python3`, no
   third-party imports.
   — check: `python3 tests/test_rule_traceability.py` → exit 0 on the current tree.

3. **Give step 2 its rejection cases** — touches the same file. One per failure mode, nine
   named: missing `test:` path; `test:` path without its marker; marker naming an undeclared
   id; `manual:` rule carrying a marker; `planned:` phase absent from `PHASES.md`;
   `planned:` phase already `done`; duplicate id; unparsable rule line; id mentioned in
   `CLAUDE.md` but undeclared. Each builds a temp copy of the catalogue with exactly that
   defect, so the parser must take the paths it reads as parameters rather than hardcoding
   them. Assemble any `RULE`-shaped fixture text at runtime: a literal marker in this file's
   source is a real marker, and the checker will report itself.
   — check: `python3 tests/test_rule_traceability.py` → exit 0, printing a rejection-case
   count `>= 9`.

4. **Write `tests/test_boundaries.sh`** (R-ARCH-02) — touches that file. Run
   `.claude/hooks/boundary-check.sh` over **every file under `src/`** — every file, not a
   filtered set: the hook decides for itself what an import line is, so filtering by
   extension would be the wrapper inventing a scope the rule does not have (round 3 proved
   it: `src/core/x.hpp` was invisible until the filter came off).
   The hook and its rules file are **tracked repository content** (`git ls-files
   .claude/hooks .claude/workflow` lists both), so a clean clone has them and a missing or
   non-executable hook is a hard `FAIL`, never an `OPTIONAL_TOOLS` skip — the clean-clone
   promise in the `PHASES.md` row is about *external* tools, not files the repo ships.
   The wrapper reads three outcomes from the hook's two documented codes: `0` clean, `2`
   a layering breach, **anything else means the hook could not run** (bad interpreter,
   missing dependency, not executable). The third is not a third code the hook produces —
   its header forbids that — it is the wrapper refusing to read a crash as a breach, because
   `FAIL: forbidden dependency direction` sends the reader hunting for an import that does
   not exist. Each outcome gets its own `FAIL:` line, and `sweep` returns `1` for a breach
   and `2` for "did not run" so callers can tell them apart.
   Two rejection cases, both asserting the *specific* return: a temp tree whose
   `src/core/x.cpp` includes `"hal/bus.h"` (with its own `boundaries.rules` copy and
   `CLAUDE_PROJECT_DIR` pointed at it) must give `sweep` → `1`; a stub hook exiting `3` over
   that same breaching tree must give `sweep` → `2`, reported as a broken hook.
   — check: `sh tests/test_boundaries.sh` → exit 0 and prints `rejection cases: n/n` with
   `n >= 2`, like the other three files.

5. **Write `tests/test_repo_shape.sh`** (R-ARCH-01, R-ARCH-03, R-ERR-03, R-ERR-04,
   R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-PROTO-05) — touches that file. Eight greps over a
   **root passed as an argument**, so one function serves both the real tree and a temp bad
   tree. Scope: `*.cpp` and `*.h` under `<root>/src`, narrowed to `<root>/src/core` for the
   two rules whose text says `core`. `tests/`, `docs/`, `tools/` and the build system are
   outside the checked set — including for R-CLEAN-03 and R-CLEAN-05, whose catalogue text
   names no scope: a test file legitimately holds literal expected bytes and bare scratch
   names. Line comments are stripped before matching, **except** for R-CLEAN-05, which is a
   rule about comments and whose text stripping would delete.
   A rule with two clauses needs both checked. R-ARCH-01 is hardware header *and*
   hosted-only standard header; R-CLEAN-09 is annotated *and* default-specifier inheritance.
   R-ARCH-01's second clause needs a list the catalogue does not supply — build it from the
   criterion, not from memory: a header is hosted-only here if using it implies dynamic
   allocation, exceptions, threads, locale or an OS filesystem (`<iostream>`, `<vector>`,
   `<string>`, `<memory>`, `<new>`, `<stdexcept>`, `<thread>`, `<filesystem>`, `<regex>`,
   `<random>`, …). The list is a floor, not a closed set: a header nobody has written yet is
   not a gap in this phase. Anchor the include match on the closing `>` so `<string_view>`
   is not read as `<string>`.
   R-ERR-04's grep is deliberately wider than its catalogue text. The text says `.value()`
   "on a `std::expected`"; a grep cannot see the receiver's type, so it matches every
   `.value(` under `src/`. Accepted price with a reason: `std::optional::value()` fails
   identically — it throws, and under `-fno-exceptions` throwing is `abort` — so the wider
   match forbids nothing this project wants. A user type with a `value()` accessor is the
   trigger to narrow it, not before.
   Rejection cases, ten named: R-ARCH-01 hardware header; R-ARCH-01 hosted-only header;
   R-ARCH-03 heap; R-ERR-03 `try`/`catch`; R-ERR-04 `.value(`; R-CLEAN-03 bare `flag`;
   R-CLEAN-05 bare `TODO`; R-CLEAN-09 annotated base; R-CLEAN-09 default-specifier base;
   R-PROTO-05 a `src/` file referencing `tests/vectors/`.
   Accept cases, eleven named — five freestanding includes `<cstdint>`, `<string_view>`,
   `<array>`, `<span>`, `<expected>` (R-ARCH-01); `std::string_view sv;` (R-ARCH-03, step
   0b); a genuinely deleted operator (R-ARCH-03, step 0c); `bool is_ready` and
   `bool m_has_ack` (R-CLEAN-03); `// TODO(09-guitar-observe): …` (R-CLEAN-05); and
   `enum class E : uint8_t`, a fixed underlying type and not a base (R-CLEAN-09).
   One `RULE` marker per rule in the header, one `FAIL:` line per rule naming the offending
   path.
   — check: `sh tests/test_repo_shape.sh` → exit 0, eight `ok:` lines, rejection cases
   `>= 10`, accept cases `>= 11`.

6. **Write `tests/test_phase_docs.sh`** (R-PROC-02) — touches that file. For every row in
   `docs/phases/PHASES.md` whose status is `done`, assert `docs/phases/<id>/verify.md`
   exists and is non-empty. One rejection case — a temp phase table with a `done` row and no
   `verify.md` — and one accept case, a `done` row whose `verify.md` is present.
   — check: `sh tests/test_phase_docs.sh` → exit 0 (no `done` phases yet, and the rejection
   case proves that is not why it passed).

7. **Write `tests/test_secrets.sh`** (R-SEC-01) — touches that file. The rule says "source,
   config **or history**", so this is two scans: `gitleaks dir` over the working tree *and*
   `gitleaks git` over the commit history — a secret deleted in the next commit is still in
   the repository, and the tree scan alone leaves the third clause unbound. Each scan gets
   its own temp output path (`mktemp`), so neither overwrites the other's findings before
   they are read. A missing `gitleaks` follows the `OPTIONAL_TOOLS` convention from step 1;
   a `gitleaks` too old for the subcommands is step 0d, and is not the same message.
   One rejection case: a temp file holding a non-allowlisted fake token — **not** the AWS
   documentation key `AKIAIOSFODNN7EXAMPLE`, which gitleaks allowlists and which would make
   a broken check look like a passing one. Assemble the fake token at runtime; a literal one
   in this file's source is itself a detectable secret, and the check will fail on itself.
   — check: `sh tests/test_secrets.sh` → exit 0, naming both scans and confirming the
   rejection case ran.

8. **Write `tests/test_tool_versions.sh`** (R-TOOL-01, R-TOOL-02) — touches that file. For
   each of `clang-format`, `clang-tidy`, `arm-none-eabi-g++` and `python3`: if absent, skip
   under `OPTIONAL_TOOLS`; if present, parse the version and fail below R-TOOL-01's floor.
   Compare the whole version, not the leading integer — `python3 >= 3.8` is unenforceable by
   a major-only comparison and every Python this project meets is major 3.
   **"Absent" is a different question for the two tool families, and the asymmetry is
   intentional.** For `clang-format`/`clang-tidy` it means "not resolvable": `PATH` first,
   then the keg-only LLVM prefixes Homebrew leaves unlinked (`/opt/homebrew/opt/llvm/bin`,
   `/usr/local/opt/llvm/bin`) — the same probe `tests/test_style.sh` already does, without
   which R-TOOL-01 skips `clang-tidy` forever on this machine. For `arm-none-eabi-*` it
   means "not on `PATH`", full stop: R-TOOL-02 is a claim about the binary *first on `PATH`*,
   and resolving elsewhere would hide the shadowing trap the rule exists to catch. The
   resolver therefore returns early for those names, which is also what guarantees
   R-TOOL-01 and R-TOOL-02 judge the same binary.
   Then R-TOOL-02: compile a two-line TU including `<cstdint>` for `-mcpu=cortex-m0plus
   -mthumb` with whichever `arm-none-eabi-g++` is first on `PATH`, and fail if it cannot.
   Pass those two flags and no others — a `-std=` flag is not needed to prove `<cstdint>`
   resolves, and `-std=c++23` is the GCC 13+ spelling, so a compiler at R-TOOL-01's own
   floor of 12 would fail R-TOOL-02 and be misreported as a cross-compiler with no target C
   library.
   Rejection cases, four named: a stub whose `--version` reports a release below the floor;
   a stub whose banner carries no version at all; a stub that clears the major but not the
   minor; a stub compiler that cannot build the `<cstdint>` probe.
   — check: `sh tests/test_tool_versions.sh` → exit 0, naming each tool and the version
   found, rejection cases `>= 4`.

9. **Flip the fourteen rules and add the markers** — touches `docs/constraints.md` and the
   six test files. Each `planned: 00-scaffold` becomes ``test: `tests/<file>` `` and each
   test file carries `# RULE <id> — docs/constraints.md §Invariants — <one line>`, naming
   the section where the rule is actually declared (§Invariants — the subsection headings
   under it are not section names for this purpose).
   — check: `grep -c 'planned: 00-scaffold' docs/constraints.md` → `0`;
   `python3 tests/test_rule_traceability.py` → exit 0.

10. **Write `docs/phases/00-scaffold/verify.md`** — touches that file. For a
    non-specialist: what was built, why a test that passes on an empty repo is worth
    anything, and a hands-on procedure — break one rule on purpose, watch `make test` name
    it, put it back. It is a **durable operator reference, not a session report**: any
    sample output it prints must match what the checks emit today, each break/run/restore
    cycle must be runnable literally and in order (step 0e), and anything true only while
    this phase was being written belongs in `notes.md`.
    — check: `sh tests/test_phase_docs.sh` → exit 0; `test -s docs/phases/00-scaffold/verify.md`;
    every command `verify.md` prints can be pasted into a shell in the order given, and each
    produces the `FAIL:` line it promises.

11. **Record the round** — touches `docs/phases/00-scaffold/notes.md`. Append this round's
    outcome and deviations; never rewrite the earlier rounds.
    — check: `test -s docs/phases/00-scaffold/notes.md` → exit 0.

## Acceptance criteria

```
make test                                          # expect: exit 0, "OK"
make lint                                          # expect: exit 0
python3 tests/test_rule_traceability.py            # expect: exit 0, rejection cases >= 9
sh tests/test_boundaries.sh                        # expect: exit 0, prints "rejection cases: n/n", n >= 2
sh tests/test_repo_shape.sh                        # expect: exit 0, 8 rules ok, rejection >= 10, accept >= 11
sh tests/test_phase_docs.sh                        # expect: exit 0, rejection case confirmed
sh tests/test_secrets.sh                           # expect: exit 0, tree and history both scanned, rejection case confirmed
sh tests/test_tool_versions.sh                     # expect: exit 0, names each tool and its version, rejection cases >= 4
grep -c 'planned: 00-scaffold' docs/constraints.md # expect: 0
grep -c 'STYLE_OPTIONAL' Makefile tests/test_style.sh   # expect: 0 in both files
test -s docs/phases/00-scaffold/verify.md          # expect: exit 0
test -s docs/phases/00-scaffold/notes.md           # expect: exit 0
```

**Adversarial block — run by hand, and the point of the phase.** Each case must make
`make test` exit non-zero **and name its rule id**, and the tree must be back to green
after each. Run them one at a time, in order; each line removes its own file before the
next writes one, because two cases sharing a filename hide each other:

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

And the negative half of the same idea — the checks must stay quiet on legitimate code:

```
mkdir -p src/core && printf 'std::string_view sv;\n' > src/core/x.h
sh tests/test_repo_shape.sh                         # expect: exit 0 — R-ARCH-03 does not fire
rm -f src/core/x.h ; rmdir src/core src 2>/dev/null
```

This one runs the repo-shape check rather than `make test`, and the reason is a limitation
worth knowing before phase `01-ps2-codec` writes its first header: on this machine
clang-tidy cannot resolve *any* standard header without a `compile_commands.json`, so a
scratch file containing `std::` or an `#include <…>` fails R-STYLE-02 with
`error: 'string_view' file not found [clang-diagnostic-error]` — a tooling result, not an
R-ARCH-03 one. Measured, with the working invocation, in `notes.md` §For later phases.

## Out of scope

- **Any PS2 protocol logic, type, or constant.** `Ps2Frame`, `DecodeStatus`, `LinkState`,
  the `std::expected` signatures of ADR-0009 and the vectors under `tests/vectors/` belong
  to `01-ps2-codec`. This phase creates no `src/core/*.cpp` — the checks are written
  against a tree with no product code, and that is deliberate.
- **`src/core/pins.h` and any GPIO number.** Belongs to `02-wiring`; R-SAFETY-01..03 stay
  `planned:`.
- **`CMakeLists.txt`, `PICO_SDK_PATH`, any firmware build.** Belongs to `03-pio-bus`, which
  also owns R-ERR-05. `make firmware` keeps failing with its current message. The ARM
  toolchain itself is not out of scope — step 8 probes it — but this phase never builds
  with it.
- **A C++ test harness, assertion library or test framework.** `<cassert>` plus the
  Makefile's per-file compile-and-run is the harness; `-UNDEBUG` is what makes it
  trustworthy. Adding a framework is the over-build this phase is shaped to avoid.
- **Enforcing R-CLEAN-04 (magic numbers).** Belongs to `01-ps2-codec`:
  `readability-magic-numbers` needs real code to tune exclusions against, and turning it on
  against an empty tree proves nothing.
- **Upgrading the greps to `clang-query`.** Recorded as a `belay-debt:` comment (the
  marker this workflow uses for an accepted shortcut with a named ceiling) in
  `tests/test_repo_shape.sh` and in `notes.md` §Debt: block comments and string literals are
  stripped by nothing, so a forbidden token inside `/* */` is a false positive. The upgrade
  needs the `compile_commands.json` `.clang-tidy` also wants, so both land together in
  `03-pio-bus` at the earliest.
- **Installing a git `pre-commit` hook.** The security gate runs as a Claude `PreToolUse`
  hook only. Wiring it into `.git/hooks/` is an operator decision that has been raised and
  not answered.
- **The round 4-5 taste notes** — `/tmp/gl.$$` beyond the two-output fix in step 7, the
  `.sh` file modes, the `( … ; [ $? -eq 2 ] )` idiom, R-ERR-05's position in the catalogue
  and its inline changelog, `find_clean03`'s per-line exclusion (recorded under §Debt),
  R-ERR-03's line-anchored `try` grep, and the traceability marker living in a docstring
  without a `#`. All listed in `notes.md`; none changes what a rule catches.
- **Splitting this phase.** Considered and rejected: eleven of the fourteen rules are the
  same shape (a check over a path root plus its rejection case) and share one helper. The
  substantial pieces are the traceability parser and the two tool probes. Two rows would
  duplicate the harness for no gain.
