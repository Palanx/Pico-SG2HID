# Phase 14-scaffold-nongrep-clauses — notes

## Outcome

- `docs/constraints.md`: R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02 and R-PROC-01 each gained a
  `**Scope, recorded 2026-09-24 because the check reads less than this text says:**` clause,
  inside the rule text and before ` — test: `, naming the check's function (`sweep( )`,
  `scan( )`, `check_version( )`, `arm_compiles( )`, `check( )`), its measured misses and its
  reach items. Bindings unchanged. No check, test, source or workflow file changed.
- `docs/phases/14-scaffold-nongrep-clauses/verify.md`: no-hardware operator check, with a by-hand
  re-run of the `#include <hal/bus.h>` measurement (exit 0) against its quoted control (exit 2).
- Measurement: every fixture in its own subdirectory of the session scratchpad (outside the
  repo), run with `PATH=/usr/bin:/bin` plus the tool directory it needed. `missed` = not reported.
  - **R-ARCH-02**, hook direct (`CLAUDE_PROJECT_DIR=… boundary-check.sh <file>`, 2 = reported),
    all in `src/core/x.cpp` unless noted — missed (exit 0): `#include <hal/bus.h>`,
    `# include "hal/bus.h"`, `#<TAB>include "hal/bus.h"`, `#include"hal/bus.h"`,
    `#include_next "hal/bus.h"`, macro include (`#define HAL_HEADER …` / `#include HAL_HEADER`),
    include split by `\` continuation, `import hal.bus;`, `src/common/y.h` (no layer) including
    `hal/bus.h`. Correctly not reported (not named): `// #include "hal/bus.h"`,
    `#include "halbus.h"`. Reported (exit 2): control `#include "hal/bus.h"`,
    `"../hal/bus.h"`, `"src/hal/bus.h"`, `#if 0` include (loud false positive, named),
    `src/core/a b.cpp` handed directly, `src/core/x.inl`.
  - **R-ARCH-02**, extracted `sweep( )` (1 = reported) — missed (0): `src/core/a b.cpp` (split by
    unquoted `for f in $( find … )`), transitive `src/core/x.cpp` → `common/y.h` → `hal/bus.h`,
    `src/core` as a symlink to a directory outside the tree. Reported (1): control, `x*.cpp`,
    `src/core/deep/er/x.h`, `src/core/.hidden.h`.
  - **R-SEC-01**, gitleaks 8.30.1, `gitleaks dir|git <d> --no-banner --redact`, tokens assembled at
    runtime as in `tests/test_secrets.sh` — missed (exit 0): low-entropy `password = "…"`,
    16-char password with `!#@` punctuation, token split across two literals assigned to a
    neutral name (`x = "…" "…"`), token committed then `git reset --hard` away (git), token only in
    a commit message (git), token only in an annotated tag message (git). Reported (exit 1):
    control GitHub token, 16-char alnum `password = "…"`, 22-char `db_password`, PEM private-key
    block, split literal assigned to `TOKEN` (the first half trips a generic rule — not named),
    base64 of the token (neutral name and `TOKEN=`), `.gitignore`d file (dir), stash-only (git),
    unmerged branch (git), `git notes` (git), staged-never-committed (dir reports; git does not —
    not named, the tree scan covers it). Reach item: the `OPTIONAL_TOOLS=1` skip branch at the top
    of `tests/test_secrets.sh`, quoted in the clause.
  - **R-TOOL-01**, stubs on `PATH` through extracted `ver_num( )`, `resolve( )`,
    `check_version( )` — passed although below floor: `clang-format 99 version 14.0.6` (floor 23),
    `x86_64-apple clang-format version 14.0.6` (read as 86), `Python 3.8.0rc1` vs floor `3.8.1`,
    passing banner with exit 1, `Ubuntu clang-format version 23.0.0-1~exp1` (at floor; pre-release
    suffix ignored). Reported: `Python 3.7.9` vs 3.8, `clang-format version 22.9.0`,
    `clang-format 2 version 14.0.6` (the spec's candidate — reported because 2 < 23, so the clause
    names the inverse), LLVM banner with URL reading 14, banner on stderr first (`2>&1 | head -1`
    reads it). Keg-only: this machine has `/opt/homebrew/opt/llvm/bin/clang-format`; under
    `PATH=/usr/bin:/bin`, `check_version clang-format 23 --version` → ok on 23.1.0 while
    `command -v clang-format` (R-STYLE-01's lookup, `tests/test_style.sh:95`) → exit 1. Reach:
    probes are clang-format, clang-tidy, arm-none-eabi-g++, python3; called and unprobed are
    `$(CXX)` (`Makefile:13`), `git`, `bash` (`boundary-check.sh` shebang) and `gitleaks`.
  - **R-TOOL-02**, stubs through extracted `arm_compiles( )` — all passed (rc 0): compiles on `-c`
    and fails any link; compiles `<cstdint>` and fails on `<expected>`; exits 0 writing an empty
    object regardless of flags.
  - **R-PROC-01**, real `check( )` via `make_repo( )`, `PYTHONDONTWRITEBYTECODE=1` — missed (empty
    problem list): `* **R-X-01**` bullet with missing test file, indented `  - **R-X-01**`, `*` bullet
    with no binding, `manual:` rule with marker under `docs/`, `.claude/commands/`, `build/`,
    `src/node_modules/`, `test:` file whose only marker is `R-X-011` (with R-X-011 declared manual
    it is a different line error, see below; undeclared → nothing reported at all), second binding
    appended after `manual:` (`… manual: x` then a `test:` binding to a missing file), `planned:`
    phase with status `Done`, `CLAUDE.md` `R-X-01..03`. Not named because consistent with the rule
    text: `planned:` phase `superseded by …`, marker text inside a string, marker in a second file,
    lowercase `r-y-02` in `CLAUDE.md`. Reported: control, `planned: x-foo`, `planned: 1-foo`,
    trailing-space done phase, 3-digit id, doubled `test:` binding, `CLAUDE.md` `R-Y-02`. Crashed:
    `test:` path that is a directory → `IsADirectoryError` (loud; named).
- Acceptance: every criterion in spec §Acceptance criteria returned its expected value — the five
  per-rule greps 1 each, clause count 5, bindings 1/1/2/1, traceability exit 0, protected-path
  diff 0, `verify.md` headings 2, `make test` exit 0 with no `FAIL:` or `skip:` line, `make lint`
  exit 0. Working tree after the run: only `docs/constraints.md`, `docs/phases/PHASES.md` and this
  phase's `notes.md` and `verify.md`.

## Deviations

- Spec step 3 lists the tools called as `CXX`, `gitleaks`, `git`, `clang-format`, `clang-tidy`,
  `python3`. `bash` is also called (the boundary hook's `#!/usr/bin/env bash`, needed for its
  `< <( … )`) and is unprobed, so the clause names it too.
- Spec step 3's candidate `clang-format 2 version 14.0.6` is reported (2 < 23). The clause
  names the measured inverse — a first number above the floor — instead.
- Fixtures beyond the minimum set (permitted by §Plan "How to measure"), listed above.
- None otherwise. Files read: the spec's Context pointers plus `docs/templates/notes.md` (named by
  `/implement-phase` itself).

## Debt

- None added. The clauses record gaps; closing them is out of scope (spec §Out of scope).

## For later phases

- Candidate fixes for the operator to decide on after validation (spec §Out of scope: each needs its
  own row; none is scheduled by this entry):
  - `tests/test_boundaries.sh` `sweep( )`: iterate `find … -print0` / `while read`, so filenames
    with spaces are not split.
  - `tests/test_rule_traceability.py`: bound `MARKER` and the substring test after the two digits
    (`R-X-011` currently satisfies `R-X-01`); report `ANY`-bullet variants (`* **R-`, indented);
    turn a directory `test:` path into a finding instead of a crash.
  - `tests/test_tool_versions.sh` `ver_num( )`: anchor on the word `version` where the banner has
    one, so `x86_64-…` is not read as 86.
  - The `.claude/hooks/boundary-check.sh` misses (`<hal/…>`, `# include`) belong to the workflow
    package — route through `/belay-feedback`, not a row.
- Independent-review taste notes (validation 2026-09-24, not blocking): the R-ARCH-02 and
  R-PROC-01 clauses are very long single lines; `verify.md`'s hand re-run uses `$S` directly rather
  than the spec's `$S/tN` per-fixture layout (equivalent for one fixture).

## Validation — 2026-09-24
- criteria: 15 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`). A first `scripts/check.sh` run reported `test` FAIL while a second `make test` ran concurrently in the same tree; re-run alone → exit 0, all gates passed.
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 14066 (first)
- upstream: none
- not-ours: none
- verdict: done
