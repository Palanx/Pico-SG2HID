# Phase 20-scaffold-check-harness-fixes — close five check gaps measured in 14-scaffold-nongrep-clauses

## Goal

Phase 14 recorded, in three rules' scope clauses, five forms their checks let through. This
phase changes the three checks so each form is reported, adds one rejection case per form,
and in the same edit removes each form from its rule's scope clause. After it:

| # | form | check after this phase | rule clause after this phase |
|---|---|---|---|
| 1 | a file under `src/` whose name contains a space | `sweep( )` in `tests/test_boundaries.sh` hands the hook every path `find` prints with no word splitting, so `src/core/a b.cpp` including `hal/bus.h` makes `sweep( )` return 1 | R-ARCH-02 no longer says a filename with a space is split or unreported |
| 2 | a `test:` file whose only marker is `RULE R-X-011`, for rule `R-X-01` | `check( )` in `tests/test_rule_traceability.py` reports `carries no 'RULE R-X-01' marker`; a marker's id is read in full (all its digits), so `R-X-011` is its own id | R-PROC-01 no longer says `R-X-011` satisfies `R-X-01` |
| 3 | a rule line written `* **R-X-01** …` | `check( )` reports it as an `unparsable rule line` | R-PROC-01 no longer says such a rule is not read |
| 4 | a rule line indented under another bullet (`  - **R-X-01** …`) | `check( )` reports it as an `unparsable rule line` | same sentence as 3 |
| 5 | a `test:` binding whose path is a directory | `check( )` reports a problem containing `is not a file`; it no longer raises `IsADirectoryError` | R-PROC-01 no longer says it crashes |
| 6 | a banner whose first number is not the version (`x86_64-apple clang-format version 14.0.6`, `clang-format 99 version 14.0.6`) | `ver_num( )` in `tests/test_tool_versions.sh` reads the first `N` or `N.N` after the first occurrence of the word `version` when the text contains it, and the first `N` or `N.N` in the text otherwise; both banners are below a floor of 23 | R-TOOL-01 no longer names either banner; its description of `ver_num( )` states the new reading |

Rows 3 and 4 are one gap (the row's item 3) measured in two spellings; the row counts five.
Every other form the three clauses name stays named and unchanged. Banners without the word
`version` (`Python 3.14.6`, bare `15.3.1` from `-dumpversion`) and floors (`23`, `3.8`) parse
as before. On the real tree `make test` stays green: `docs/constraints.md` has no `*`-bullet or
indented rule line and no file carries a three-digit marker.

## Context pointers

- `CLAUDE.md` §Rules are bound to tests — a check edit and its rule text move in the same edit.
- `docs/phases/14-scaffold-nongrep-clauses/notes.md` §Outcome — how each of the five forms was
  measured (fixtures, exit codes); §For later phases — the candidate fix per form.
- `docs/constraints.md` — the three rule lines this phase edits:
  `/usr/bin/grep -nE '^- \*\*R-(ARCH-02|PROC-01|TOOL-01)\*\*' docs/constraints.md`. Each is one
  line of the form `- **R-…** — <text> — test: <path>`; each clause starts at
  `**Scope, recorded 2026-09-24 because the check reads less than this text says:**`.
- `docs/adr/0005-rule-test-traceability.md` — the rule-line grammar `check( )` enforces: a rule
  line starts at column 0 with `- **`. A `*` or indented rule line is outside it, which is why
  forms 3–4 are reported as unparsable rather than accepted.
- `tests/test_boundaries.sh` — `sweep( )` (form 1), its rejection cases and the
  `rejection cases: N/2` floor line at the bottom.
- `tests/test_rule_traceability.py` — `MARKER`, `ANY_RULE_LINE`, `check( )`'s `test` branch
  (forms 2–5), `CASES`, `make_repo( )`, the `len(CASES) >= 9` floor and `(floor 9)` in
  `main( )`, and the word "nine" in `run_all( )`'s docstring and `main( )`'s comment.
- `tests/test_tool_versions.sh` — `ver_num( )` and its header comment (form 6), the rejection
  cases and the `rejection cases: N/4` floor line.
- `tests/test_checks_are_live.py` — neuters every function in the `.sh` checks and mutates the
  single-quoted strings of one-line function bodies; a surviving mutant fails `make test`. Its
  lines 17 and 77 (`CASE_LINE`) define a case line: a result line containing
  `rejection case`, `accept case` or `wiring case` is excluded from the real-run accounting, so
  every new case's result line must contain one of those words. Its module docstring (line 30)
  says "nine hand-written rejection cases".
- `docs/phases/14-scaffold-nongrep-clauses/verify.md` — house shape for this phase's
  `verify.md` (two headings R-PROC-02 requires: `^#+ What was built`, `^#+ .*[Cc]heck it`).

## Plan

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.
Probes named "probe N" are the numbered blocks in §Acceptance criteria.

1. **Form 1 — `sweep( )` without word splitting.** Replace `for f in $( find … )` with a loop
   that reads one path per line with no splitting and no globbing — e.g.
   `find "$sweep_root/src" -type f | { while IFS= read -r f; do …; done; exit "$sweep_bad"; }`
   and take the pipeline's status as the return value. Keep the 0/1/2 exit-code contract and
   the `2 ) [ "$sweep_bad" -eq 2 ] || sweep_bad=1` precedence unchanged. Add a rejection case:
   a tree whose only source is `src/core/a b.cpp` containing `#include "hal/bus.h"`, asserting
   `sweep` returns exactly 1, result line containing `rejection case`. Raise the floor to
   `-ge 3` and print `/3`. Measure a filename containing a newline once; if it is still
   missed, step 5 names it — touches `tests/test_boundaries.sh` — check:
   `sh tests/test_boundaries.sh` → exit 0 and prints `  ok:   rejection cases: 3/3`; probe 1 → `1`.
2. **Forms 2–5 — `check( )`.**
   - `MARKER` captures the full digit run (`RULE (R-[A-Z]+-\d+)`), so `RULE R-X-011` is
     collected as `R-X-011`, not `R-X-01`.
   - The `test` branch's marker test is bounded after the id (a regex with `(?!\d)`, not the
     substring `f"RULE {rule_id}" in …`).
   - `ANY_RULE_LINE` matches optional leading spaces/tabs and a `-` or `*` bullet before
     `**R-` (`[ \t]*`, never `\s*`, which crosses lines). `RULE_LINE` is unchanged, so such a
     line is reported as unparsable.
   - The `test` branch reports `bound to '<value>', which is not a file` when the path exists
     and is not a regular file, before reading it.
   - Append four `CASES`: longer-id marker (expect `carries no '… R-X-01' marker`, marker text
     built with `_M`), `*` bullet (expect `unparsable rule line`), indented bullet under a
     `- intro` line (expect `unparsable rule line`), directory path `tests/sub` with a file
     `tests/sub/x.sh` (expect `is not a file`). Floor `>= 9` and `(floor 9)` become 13; "nine"
     in `run_all( )`'s docstring and `main( )`'s comment becomes "thirteen" (or drops the count).
   — touches `tests/test_rule_traceability.py` — check:
   `python3 tests/test_rule_traceability.py` → exit 0 and prints
   `  ok:   R-PROC-01 rejection cases: 13/13`; probe 2 → four lines, each ending `reported`.
3. **Form 6 — `ver_num( )`.** Read the first `N` or `N.N` after the first `version` when the
   text contains that word, else the first in the text; keep `major*100+minor`. Update its
   header comment to state that. Add two rejection cases through `check_version <stub> 23` (a stub
   name other than `clang-format`, whose stub the R-TOOL-01 wiring case needs at 14)
   with stub banners `x86_64-apple clang-format version 14.0.6` and
   `clang-format 99 version 14.0.6`, and one accept case (`x86_64-apple clang-format version 23.1.0`
   passes floor 23; its result line contains `accept case`, it adds to no rejection count). Raise
   the rejection floor to `-ge 6` and print `/6` — touches `tests/test_tool_versions.sh` — check:
   `OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh` → exit 0 and prints
   `  ok:   rejection cases: 6/6`; probe 3 → `rejected rejected ok ok`.
4. **Liveness still holds.** Update "nine" at `tests/test_checks_are_live.py` line 30 to match
   step 2 (docstring only; no code change) — touches `tests/test_checks_are_live.py` — check:
   `python3 tests/test_checks_are_live.py` → exit 0.
5. **Clauses.** In `docs/constraints.md`, on the three rule lines only:
   - R-ARCH-02: delete the sentence beginning `` `sweep( )` iterates the unquoted output of
     `find` `` up to and including `although the hook reports it when handed the path directly;`;
     keep the symlink clause as its own sentence. If step 1 measured a newline filename missed,
     name that instead.
   - R-PROC-01: rewrite the `ANY_RULE_LINE` sentence to say a rule written as `* **R-…**` or
     indented is reported as an unparsable rule line; delete the `R-X-011` item from the
     measured list; delete the sentence `It also errs loudly: … IsADirectoryError …`.
   - R-TOOL-01: rewrite the `ver_num( )` sentence to the step 3 reading; delete the item
     `a banner whose first number is not the version and is above the floor (…)` from the
     measured list, keeping `Python 3.8.0rc1` and the exit-1 item.
   Re-date each edited clause's `recorded` date to the day of the edit. Bindings unchanged —
   touches `docs/constraints.md` — check: the clause greps in §Acceptance criteria;
   `python3 tests/test_rule_traceability.py` → exit 0.
6. **`verify.md` for the operator** — touches `docs/phases/20-scaffold-check-harness-fixes/verify.md` —
   both R-PROC-02 headings; explains each form in plain terms (what the check was fooled by,
   why it mattered) and gives probes 1–3 as hand checks with their expected output. No
   hardware; the file says so. Check: both heading greps in §Acceptance criteria print `1` or more.
7. **Full gates** — check: `make test` → exit 0; `make lint` → exit 0; `git status --porcelain`
   shows no `zz` probe file and no `__pycache__` under `tests/`.

## Acceptance criteria

Run from the repo root under `sh` (not zsh). Each probe cleans up after itself.

```
# probe 1 — form 1: a layer breach in a file whose name has a space fails R-ARCH-02
printf '#include "hal/bus.h"\n' > 'src/core/zz probe.cpp'; sh tests/test_boundaries.sh | /usr/bin/grep -c '^  FAIL: R-ARCH-02: forbidden dependency direction$'; rm -f 'src/core/zz probe.cpp'   # expect: 1 (today: 0)
sh tests/test_boundaries.sh | /usr/bin/grep -c '^  ok:   rejection cases: 3/3$'   # expect: 1
```

```
# probe 2 — forms 2-5 through the real check( ) and make_repo( )
PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
import sys, tempfile, shutil; sys.path.insert(0, "tests"); import test_rule_traceability as t
M = "RU" "LE"
forms = [
    ("longer-id marker", "- **R-X-01** — text — test: `tests/t.sh`\n", {"tests/t.sh": f"# {M} R-X-011\n"}),
    ("star bullet",      "* **R-X-01** — text — test: `tests/missing.sh`\n", {}),
    ("indented bullet",  "- intro\n  - **R-X-01** — text — test: `tests/missing.sh`\n", {}),
    ("directory path",   "- **R-X-01** — text — test: `tests/sub`\n", {"tests/sub/x.sh": f"# {M} R-X-01\n"}),
]
for label, body, files in forms:
    d = tempfile.mkdtemp()
    try:
        print(label + ":", "reported" if t.check(d, *t.make_repo(d, body, files=files)) else "missed")
    except Exception as e:
        print(label + ":", "crashed", type(e).__name__)
    finally:
        shutil.rmtree(d)
PY
# expect: four lines, each ending "reported" (today: missed, missed, missed, crashed IsADirectoryError)
python3 tests/test_rule_traceability.py | /usr/bin/grep -c '^  ok:   R-PROC-01 rejection cases: 13/13$'   # expect: 1
```

```
# probe 3 — form 6: banners through the real ver_num( ), resolve( ), check_version( ), floor 23
sh -c '
S=$( mktemp -d ) || exit 1
sed -n "/^ver_num( )/,/^}/p;/^resolve( )/,/^}/p;/^check_version( )/,/^}/p" tests/test_tool_versions.sh > "$S/fns.sh"
. "$S/fns.sh"
for b in "x86_64-apple clang-format version 14.0.6" "clang-format 99 version 14.0.6" "x86_64-apple clang-format version 23.1.0" "Homebrew clang-format version 23.1.0"; do
    printf "#!/bin/sh\necho \"%s\"\n" "$b" > "$S/clang-format"; chmod +x "$S/clang-format"
    if PATH="$S:$PATH" check_version clang-format 23 --version >/dev/null 2>&1; then printf "ok "; else printf "rejected "; fi
done; echo; rm -rf "$S"'
# expect: rejected rejected ok ok (today: ok ok ok ok)
OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh | /usr/bin/grep -c '^  ok:   rejection cases: 6/6$'   # expect: 1
```

```
# Clauses no longer name the five forms
/usr/bin/grep -E '^- \*\*R-ARCH-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'contains a space'   # expect: 0
/usr/bin/grep -E '^- \*\*R-PROC-01\*\*' docs/constraints.md | /usr/bin/grep -cF 'R-X-011'           # expect: 0
/usr/bin/grep -E '^- \*\*R-PROC-01\*\*' docs/constraints.md | /usr/bin/grep -cF 'is not read at all' # expect: 0
/usr/bin/grep -E '^- \*\*R-PROC-01\*\*' docs/constraints.md | /usr/bin/grep -cF 'IsADirectoryError' # expect: 0
/usr/bin/grep -E '^- \*\*R-TOOL-01\*\*' docs/constraints.md | /usr/bin/grep -cE 'x86_64-apple|clang-format 99' # expect: 0
# Forms the row does not name are still recorded
/usr/bin/grep -E '^- \*\*R-ARCH-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'symlink'          # expect: 1
/usr/bin/grep -E '^- \*\*R-PROC-01\*\*' docs/constraints.md | /usr/bin/grep -cF 'R-X-01..03'       # expect: 1
/usr/bin/grep -E '^- \*\*R-TOOL-01\*\*' docs/constraints.md | /usr/bin/grep -cF '3.8.0rc1'         # expect: 1
/usr/bin/grep -E '^- \*\*R-(ARCH-02|PROC-01|TOOL-01)\*\*' docs/constraints.md | /usr/bin/grep -cF '**Scope, recorded' # expect: 3
/usr/bin/grep -cE '^- \*\*R-(ARCH-02|PROC-01|TOOL-01)\*\* — .* — test: `tests/test_(boundaries\.sh|rule_traceability\.py|tool_versions\.sh)`$' docs/constraints.md # expect: 3 (bindings unchanged)
# This phase's verify.md
/usr/bin/grep -cE '^#+ What was built' docs/phases/20-scaffold-check-harness-fixes/verify.md   # expect: 1 or more
/usr/bin/grep -cE '^#+ .*[Cc]heck it' docs/phases/20-scaffold-check-harness-fixes/verify.md    # expect: 1 or more
# Nothing left behind
git status --porcelain | /usr/bin/grep -cE 'zz|__pycache__'   # expect: 0
# Project gates (.claude/workflow/toolchain.json: test, lint; no typecheck tool configured)
python3 tests/test_rule_traceability.py   # expect: exit 0
python3 tests/test_checks_are_live.py     # expect: exit 0
make test                                 # expect: exit 0
make lint                                 # expect: exit 0
```

## Out of scope

- **`.claude/hooks/boundary-check.sh` misses** (`#include <hal/…>`, `# include`, macro includes,
  `import`, transitive includes, `#if 0`) — package-owned; `/belay-feedback`, not this phase.
- **The symlinked layer directory in `sweep( )`** — still named in R-ARCH-02; the row does not name it.
- **The other forms R-PROC-01 names** (`SKIP_PREFIXES`/`SKIP_DIRS` markers, a second binding
  swallowed by `RULE_LINE`, `Done` spelled with a capital, `CLAUDE.md` range notation, `RULE_ID`
  reading `R-X-011` in `CLAUDE.md`) — not in the row; no phase owns them.
- **The other forms R-TOOL-01 names** (pre-release suffixes, patch-level floors, exit status
  ignored, keg-only lookup mismatch with R-STYLE-01, unprobed tools) — not in the row.
- **Changing `RULE_LINE` or ADR-0005's grammar to accept `*` or indented rules** — the fix is to
  report them, not to widen the grammar; widening needs a superseding ADR.
- **R-TOOL-02, R-SEC-01 and their clauses** — untouched.
- **Editing closed phases' directories** (`14-scaffold-nongrep-clauses/` and earlier).
