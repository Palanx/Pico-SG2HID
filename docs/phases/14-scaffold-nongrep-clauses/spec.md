# Phase 14-scaffold-nongrep-clauses — scope clauses for five rules bound to non-grep checks

## Goal

Five rules in `docs/constraints.md` §Invariants are bound `test:` to a check that is not a grep.
None of their texts states a scope, and a rule with no scope text claims that its check covers all
of it. After this phase, each of the five rule lines says in its own text what its check was
**measured** not to see. Nothing else changes: no check, no test case, no source file, no binding.

| rule | check | what bounds its reach |
|---|---|---|
| R-ARCH-02 | `tests/test_boundaries.sh` — `sweep( )` runs `.claude/hooks/boundary-check.sh` on each file `find "$root/src" -type f` returns | the hook's `IMPORT_RE` and its layer-name match; the set of files `sweep( )` hands it |
| R-SEC-01 | `tests/test_secrets.sh` — `scan dir` and `scan git` (gitleaks, default rules, `--redact`) | gitleaks' default ruleset and the commits `gitleaks git` walks; skipped under `OPTIONAL_TOOLS=1` when gitleaks is absent |
| R-TOOL-01 | `tests/test_tool_versions.sh` — `probe` × 4 through `check_version( )`, `resolve( )`, `ver_num( )` | which tools are probed, where `resolve( )` looks, and how much of a version `ver_num( )` compares |
| R-TOOL-02 | `tests/test_tool_versions.sh` — `arm_compiles( )` | the probe translation unit and its flags (`-c`, no link) |
| R-PROC-01 | `tests/test_rule_traceability.py` — `check( )` | `RULE_LINE`, `ANY_RULE_LINE`, `RULE_ID`, `SKIP_PREFIXES` / `SKIP_DIRS`, `PHASE_ROW` |

R-PROC-02, the sixth non-grep rule named in `01-ps2-codec/spec.md`, is not in this phase: it
already carries a 2026-09-23 clause.

**What counts as a measured form.** A form is one fixture run through the real check's code, in a
scratch directory **outside the repo**, where the check reported nothing. Examples: an import line,
a planted secret, a stub tool, or a catalogue line. A **reach item** is a named part of the rule's
text that the check never examines, such as a tool the rule covers that no probe runs. A reach item
is shown by quoting the check's code. A clause names only measured forms and reach items that were
shown this way. A form that was not run is not named. A form that gets reported may be named, but
the clause does not have to name it.

Each clause uses the house heading `**Scope, recorded <date> because the check reads less than this
text says:**`, where `<date>` is the day of the measurement. It names the check's function(s), for
example `sweep( )`, `scan( )`, `check_version( )`, `arm_compiles( )` or `check( )`. It sits inside
the rule text, before ` — test: `.

## Context pointers

- `CLAUDE.md` — §Rules are bound to tests. This phase changes rule text only, so no test moves.
- `docs/constraints.md` — the five rule lines are where every edit lands
  (`/usr/bin/grep -nE '^- \*\*R-(ARCH-02|SEC-01|TOOL-01|TOOL-02|PROC-01)\*\*' docs/constraints.md`).
  For register, R-PROC-02's clause is the house example of a non-grep clause ("the check proves …,
  not …"). R-ERR-02's "Measured one declaration per form: …" is the example of listing measured
  forms.
- `docs/phases/01-ps2-codec/spec.md` §"A check that is not a grep owes a clause too, and this is
  what it is derived from" — the ruling this phase carries out, and the table of what bounds a
  non-grep check's reach.
- `tests/test_boundaries.sh`, `.claude/hooks/boundary-check.sh`, `.claude/workflow/boundaries.rules`
  — R-ARCH-02's check. The hook's header comment already states its own misses (`belay-debt:`).
  Read those misses as leads to measure, not as findings.
- `tests/test_secrets.sh` — R-SEC-01's check.
- `tests/test_tool_versions.sh`, `Makefile`, `tests/test_style.sh` (its tool lookups only) —
  R-TOOL-01/02's check and the tools the other checks actually call.
- `tests/test_rule_traceability.py` — R-PROC-01's check. Its `RULE_LINE` also constrains every
  edit: a rule line stays `- **R-…** — <text> — test: <path>` on a single line, and the clause must
  not contain ` — test: `, ` — manual: ` or ` — planned: `.
- `docs/phases/16-scaffold-spelling-clauses/spec.md` and `notes.md` — the precedent phase. Copy its
  scratch-directory method, its per-rule checks, and its no-hardware `verify.md`.
- `.claude/rules/tech-debt.md` — empty today. Nothing to consult.

## Plan

**How to measure.** Steps 1–5 each run fixtures under `$S`, a scratch directory outside the repo
(the session scratchpad). Each fixture gets a fresh subdirectory. Never write a fixture inside the
repo. Run the commands with `PATH=/usr/bin:/bin:<tool dirs>` so that `grep` is BSD `/usr/bin/grep`.
Record every fixture and its result (reported / not reported) in `notes.md` §Outcome, the same way
`16-…/notes.md` does. The candidate forms listed in each step are the minimum set to run. Add any
others the code suggests. A candidate that turns out to be reported is not named in the clause.

1. **R-ARCH-02.** Per fixture: copy `.claude/workflow/boundaries.rules` into
   `$S/tN/.claude/workflow/` and write one file under `$S/tN/src/`. Then run
   `CLAUDE_PROJECT_DIR="$S/tN" .claude/hooks/boundary-check.sh "$S/tN/src/<file>"; echo $?`.
   `2` means reported and `0` means not reported. For forms that depend on the file set, extract
   `sweep( )` with `sed -n '/^sweep( )/,/^}/p' tests/test_boundaries.sh`, set `HOOK` to the real
   hook, and call `sweep "$S/tN"`. Candidates, all written in `src/core/x.cpp` unless noted:
   `#include "hal/bus.h"` (control, expect reported), `#include <hal/bus.h>`, `# include "hal/bus.h"`,
   `#include"hal/bus.h"`, `#include HAL_HEADER` via a macro, a transitive include through a file
   under `src/` in no declared layer (for example `src/common/y.h` including `hal/bus.h`), a file in
   no layer that includes a layer it names, a file whose name contains a space, and an include
   inside `#if 0` (expect a loud false positive). Write the clause into R-ARCH-02's line. Check:
   `/usr/bin/grep -E '^- \*\*R-ARCH-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'sweep( )' | wc -l`
   → 1, and `python3 tests/test_rule_traceability.py` → exit 0.
2. **R-SEC-01.** Per fixture: run `gitleaks dir "$S/tN" --no-banner --redact` for the tree, or
   `gitleaks git "$S/tN" --no-banner --redact` for history. Exit 0 means not reported. Build every
   token at runtime from pieces, the way `tests/test_secrets.sh` builds `token`, so that no
   detectable literal ever sits in the repo, in `notes.md` or in the rule text. Name forms by their
   shape, never by their value. Candidates: the file's own GitHub token (control, expect reported),
   a low-entropy `password = "…"` assignment, the token split across two string literals, the token
   base64-encoded, the token in a `.gitignore`d file (tree scan), the token only in a commit no
   branch reaches (reset away), the token only in `git stash`, and the token only in a commit
   message. There is also one reach item, which needs no fixture: `make test` sets
   `OPTIONAL_TOOLS=1`, so without gitleaks the rule is not checked at all. Quote the skip branch as
   evidence. Write the clause. Check: the step-1 grep with `R-SEC-01` / `scan( )` → 1, and
   traceability → exit 0.
3. **R-TOOL-01.** Reach items come from reading the checks. List the tools the checks and
   `make test` actually call (`CXX` in `Makefile`, `gitleaks`, `git`, `clang-format`, `clang-tidy`,
   `python3`), compare that list with the four `probe` lines, and name the tools that are called but
   never probed. `tests/test_secrets.sh` explains why gitleaks has no floor; the clause may cite that
   reason. Measured forms use stubs on `PATH`, with the functions extracted by
   `sed -n '/^ver_num( )/,/^}/p;/^resolve( )/,/^}/p;/^check_version( )/,/^}/p' tests/test_tool_versions.sh`.
   Candidates: a banner below the floor only in the patch number, a banner whose first number is not
   the version (for example `clang-format 2 version 14.0.6`), and a `clang-format` absent from `PATH`
   but present in a keg-only LLVM prefix. `resolve( )` probes the keg-only binary, while
   `tests/test_style.sh`'s R-STYLE-01 branch reads `PATH` only. Measure this one only if the machine
   has the keg, and do not name it otherwise. Write the clause. Check: the step-1 grep with
   `R-TOOL-01` / `check_version( )` → 1, and traceability → exit 0.
4. **R-TOOL-02.** Use stubs through the extracted `arm_compiles( )`
   (`sed -n '/^arm_compiles( )/,/^}/p'`). Candidates: a compiler that succeeds on `-c` and cannot
   link, and a compiler that compiles `<cstdint>` but has no `<expected>`. The probe passes no
   `-std=`; the file's comment says why. Write the clause. Check: the step-1 grep with `R-TOOL-02` /
   `arm_compiles( )` → 1, and traceability → exit 0.
5. **R-PROC-01.** Run each fixture through the real `check( )`:
   `PYTHONDONTWRITEBYTECODE=1 python3 -c "import sys; sys.path.insert(0,'tests'); import test_rule_traceability as t; …"`.
   Without that variable the import writes `tests/__pycache__/`, and step 6 then fails.
   Build each repo with `t.make_repo(<tmp under $S>, rules_body, claude_body, files)`. An empty
   problem list means not reported. Candidates: a rule declared with `* **R-X-01**` or an indented
   `  - **R-X-01**` (neither is seen at all), a `manual:` rule whose marker sits under `docs/` or
   `.claude/commands/`, a `test:` path that is a directory, a marker that only appears inside a
   longer token (`RULE R-X-01` as a prefix of `RULE R-X-011`), a planned phase whose id is not
   `NN-…`, and a `CLAUDE.md` mention in range notation (`R-X-01..03`, where only `R-X-01` is
   extracted). Write the clause. Check: the step-1 grep with `R-PROC-01` / `check( )` → 1, and
   traceability → exit 0.
6. **No scratch in the repo, checks untouched.** Check:
   `git status --porcelain --untracked-files=all` lists only `docs/constraints.md`, files under
   `docs/phases/14-scaffold-nongrep-clauses/`, `docs/phases/PHASES.md` and `docs/index/`.
   `git diff --name-only main -- src tests .claude/hooks .claude/workflow Makefile .clang-tidy .clang-format`
   → empty.
7. **`verify.md` for the operator** — touches `docs/phases/14-scaffold-nongrep-clauses/verify.md`.
   It covers three things. First, what changed: documentation only, no behaviour. Second, why a rule
   that claims more than its check measures matters. Third, how to re-run one measurement by hand:
   the R-ARCH-02 hook command from step 1 with one form the clause names, and the expected exit
   code. It needs no physical steps and says so. It carries a `## What was built` heading and a
   `## Check it yourself` heading (R-PROC-02). Check:
   `/usr/bin/grep -cE '^#+ (What was built|.*[Cc]heck it)' docs/phases/14-scaffold-nongrep-clauses/verify.md`
   → 2.
8. **Full gates.** Check: `make test` → exit 0; `make lint` → exit 0.

## Acceptance criteria

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.

```
/usr/bin/grep -E '^- \*\*R-ARCH-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'sweep( )' | wc -l            # expect: 1
/usr/bin/grep -E '^- \*\*R-SEC-01\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'scan( )' | wc -l              # expect: 1
/usr/bin/grep -E '^- \*\*R-TOOL-01\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'check_version( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-TOOL-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'arm_compiles( )' | wc -l     # expect: 1
/usr/bin/grep -E '^- \*\*R-PROC-01\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'check( )' | wc -l            # expect: 1
/usr/bin/grep -E '^- \*\*R-(ARCH-02|SEC-01|TOOL-01|TOOL-02|PROC-01)\*\*' docs/constraints.md | /usr/bin/grep -o '\*\*Scope, recorded' | wc -l      # expect: 5 (one clause each)
/usr/bin/grep -cE '^- \*\*R-ARCH-02\*\* — .* — test: `tests/test_boundaries\.sh`$' docs/constraints.md             # expect: 1 (binding unchanged)
/usr/bin/grep -cE '^- \*\*R-SEC-01\*\* — .* — test: `tests/test_secrets\.sh`$' docs/constraints.md                 # expect: 1
/usr/bin/grep -cE '^- \*\*R-TOOL-0[12]\*\* — .* — test: `tests/test_tool_versions\.sh`$' docs/constraints.md       # expect: 2
/usr/bin/grep -cE '^- \*\*R-PROC-01\*\* — .* — test: `tests/test_rule_traceability\.py`$' docs/constraints.md      # expect: 1
python3 tests/test_rule_traceability.py       # expect: exit 0
git diff --name-only main -- src tests .claude/hooks .claude/workflow Makefile .clang-tidy .clang-format | wc -l    # expect: 0
/usr/bin/grep -cE '^#+ (What was built|.*[Cc]heck it)' docs/phases/14-scaffold-nongrep-clauses/verify.md          # expect: 2
make test                                     # expect: exit 0
make lint                                     # expect: exit 0
```

## Out of scope

- **Fixing what the clauses record.** This covers widening `IMPORT_RE`, quoting `sweep( )`'s loop,
  adding a probe or a floor, linking in `arm_compiles( )`, and tightening `RULE_LINE`. A clause
  records a gap; closing one is check-harness work, and each fix gets its own row. Two things are
  also excluded here: editing `.claude/hooks/boundary-check.sh`, which is the workflow package's
  file and goes through `/belay-feedback`, and editing `.claude/workflow/boundaries.rules`, which
  needs a superseding ADR.
- **R-PROC-02.** It already has its clause (13-scaffold-check-gaps).
- **The grep-bound rules.** 16-scaffold-spelling-clauses closed them.
- **A clause on `.claude/workflow/toolchain.json` or on `make test`'s own reach.** The phase covers
  the five rules and nothing wider.
- **Closed phases are not edited.** That includes `docs/phases/01-ps2-codec/`, whose §Out of scope
  release of these clauses stays as written.
- **Adding a tech-debt entry for any recorded gap.** If one looks worth fixing, propose it after
  validation; the operator decides.
