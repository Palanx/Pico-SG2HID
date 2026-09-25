# Phase 21-scaffold-filename-splitting — pass file lists to the checks without word splitting

## Goal

Two check files hand their file lists to a tool or a scanner through unquoted expansion, so
a file whose name contains a space is split into two paths that do not exist. This phase
makes every one of those lists reach its consumer one whole path per file, and adds one
rejection case per list. After it:

| # | list | where it is split today | rules that read through it | after this phase |
|---|---|---|---|---|
| 1 | `src_files( )` | `tests/test_repo_shape.sh`: every finder passes `$( src_files "$1" )` unquoted; `hits( )` / `raw_hits( )` skip both halves with `[ -f ]` | R-ARCH-03, R-ERR-03, R-ERR-04, R-CLEAN-03, R-CLEAN-05, R-PROTO-05 | a `throw` in `src/core/a b.cpp` fails R-ERR-03 |
| 2 | `core_files( )` | same, `$( core_files "$1" )` | R-ARCH-01, R-CLEAN-09, R-ERR-01 | a `virtual` in `src/core/a b.hpp` fails R-CLEAN-09 |
| 3 | `core_headers( )` | same, `$( core_headers "$1" )` | R-ERR-02 | `LinkState step( Link& link );` in `src/core/a b.h` fails R-ERR-02 |
| 4 | `sources()` → clang-format | `tests/test_style.sh`: `echo "$files" \| xargs clang-format …` | R-STYLE-01 | a misformatted `src/core/a b.cpp` makes R-STYLE-01 print that file's `clang-format-violations` diagnostic, and no `No such file` line |
| 5 | `tidy_sources()` → clang-tidy | `tests/test_style.sh`: `for f in $files` | R-STYLE-02, R-CLEAN-02, R-CLEAN-04 | a naming violation in `src/core/a b.cpp` is reported as that file's `readability-identifier-naming` diagnostic, and no `no such file or directory` line |

Measured before this phase (2026-09-24, fixtures named `src/core/zz probe.*`): rows 1–3 each
print `ok:` for the rule; rows 4–5 each print `FAIL:`, but the only diagnostics are
`src/core/zz: No such file or directory` / `probe.cpp: No such file or directory` (row 4) and
`error: no such file or directory: '…/src/core/zz'` (row 5).

The finders' patterns, the extension filters in `src_files( )` / `core_files( )` /
`core_headers( )`, `sources()` / `tidy_sources()`, the tool flags, and every result line's
prefix and wording are unchanged. None of the rules in the table has a scope clause that
mentions file names containing a space, so no clause changes; `make test` and `make lint`
stay green on the real tree.

## Context pointers

- `CLAUDE.md` §Rules are bound to tests — a check edit and its rule text move in the same edit
  (here: no rule text changes; bindings are unchanged).
- `docs/phases/20-scaffold-check-harness-fixes/notes.md` §For later phases, first item — the
  measurement that opened this row; §Outcome, first bullet — how `sweep( )` in
  `tests/test_boundaries.sh` was fixed for the same defect (`| { while IFS= read -r f; …; }`).
- `tests/test_repo_shape.sh` — `src_files( )`, `core_files( )`, `core_headers( )` (lines ~41–54);
  `hits( )`, `raw_hits( )` (~80–104); the ten `find_*( )` one-liners (~129–165); `reject( )`
  and its cases (~194–334). `reject( )` already quotes its path argument, so a case path may
  contain a space.
- `tests/test_style.sh` — the whole file (144 lines): the R-STYLE-01 block (`xargs
  clang-format`) and the R-STYLE-02 block (`for f in $files`), the header comment explaining the
  `-xc++` / `-isysroot` flags, and `skip_or_fail( )` for `OPTIONAL_TOOLS=1`.
- `tests/test_checks_are_live.py` — constraints the edit must respect:
  - `patterns( )` (docstring at ~191): a finder is mutated only if its body is **one line**,
    and its patterns are the **first two single-quoted strings** in that body. A finder that
    grows a second line, or a single-quoted string before its pattern, silently stops being
    mutated (reported only as `no check pattern in …`).
  - `CASE_LINE` (~77): a result line containing `rejection case(s)`, `false-positive case(s)`,
    `accept case(s)` or `wiring case(s)` is a case line, excluded from the real-run accounting.
    Every new case's `ok:`/`FAIL:` line must contain one of those phrases.
  - `NO_MUTATE = {"test_style.sh"}` (~50): `test_style.sh` is held to the accounting property
    only; its cases are not mutation-tested.
- `.clang-format`, `.clang-tidy` — both tools find their config by walking up from the
  **file's** directory; a scratch file outside the repo finds neither unless the case copies
  them next to it. `.clang-tidy` sets `FunctionCase: lower_case`, so a function named
  `BadName` is a naming violation.
- `Makefile` — `make test` runs every `tests/test_*.sh` with `OPTIONAL_TOOLS=1`; `make lint`
  runs `tests/test_style.sh` without it.
- `docs/phases/20-scaffold-check-harness-fixes/verify.md` — house shape for this phase's
  `verify.md` (R-PROC-02 needs headings matching `^#+ What was built` and `^#+ .*[Cc]heck it`).

## Plan

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.
"Probe N" names a numbered block in §Acceptance criteria.

1. **Rows 1–3 — `tests/test_repo_shape.sh`.** Make `hits( )` and `raw_hits( )` receive the file
   list unsplit and iterate it one line per path — e.g. every finder passes the list as one
   quoted argument (`"$( core_files "$1" )"`) and the helper reads it with
   `printf '%s\n' "$list" | while IFS= read -r f; do …; done`. Whatever the shape: each finder
   stays a one-line body whose first two single-quoted strings are its pattern and exclusion
   (`raw_hits` finders: one); `hits( )` / `raw_hits( )` still return 0 and print
   `path:line: text`; an empty list prints nothing. Add three rejection cases through the
   existing `reject( )`, one per list, each with a path containing a space:
   `find_err03 'src/core/a b.cpp' 'throw Status::kBad;'`,
   `find_clean09 'src/core/a b.hpp' 'virtual void poll( );'`,
   `find_err02 'src/core/a b.h' 'LinkState step( Link& link );'`. Update any comment in the file
   that describes the finders' call shape — check: `sh tests/test_repo_shape.sh` → exit 0, no
   `FAIL:` line, and its `ok:   rejection cases: N` count is 3 higher than before the step;
   probe 1 → `1 1 1`.
2. **Rows 4–5 — `tests/test_style.sh`.** Feed `$files` to clang-format and to the clang-tidy
   loop one whole line per path (e.g. `printf '%s\n' "$files" | while IFS= read -r f; do …; done`,
   one tool invocation per file for both; the clang-tidy loop keeps one invocation per file and
   its flags unchanged). `$( tidy_lang "$f" )` and `$SYSROOT` stay unquoted on purpose (empty →
   no argument; `-isysroot <path>` → two). Put each block's per-file loop in a function that
   takes the file list, so the real run and the cases call the same code. Add one rejection
   case per block, run only when that block's tool was found:
   - a scratch dir from `mktemp -d` (removed by `trap … EXIT`) holding copies of `.clang-format`
     and `.clang-tidy`;
   - R-STYLE-01 case: `"$tmp/a b.cpp"` containing `int  x=1;`; the function's output must contain
     `a b.cpp:` and `clang-format-violations` and must not contain `No such file`;
   - R-STYLE-02 case: `"$tmp/a b.cpp"` containing `void BadName( ) {}`; the output must contain
     `a b.cpp:` and `readability-identifier-naming` and must not contain
     `no such file or directory`;
   - each prints `  ok:   <rule> rejection case: file name with a space` or
     `  FAIL: <rule> rejection case: …` and sets `fail=1` on failure.
   Update the header comment to name the cases — check: `sh tests/test_style.sh` → exit 0 and
   prints two `ok:` lines containing `rejection case`; `OPTIONAL_TOOLS=1 sh tests/test_style.sh`
   → exit 0; probe 2 → `1 0`; probe 3 → `1 0`.
3. **Liveness still holds** — no edit expected — check:
   `python3 tests/test_checks_are_live.py` → exit 0, and its output lists no function of
   `test_repo_shape.sh` whose name starts with `find_` under `no check pattern in`.
4. **Newline, measured once, not fixed.** Plant `src/core/a<newline>b.cpp` with a `throw`, run
   `sh tests/test_repo_shape.sh`, record in `notes.md` whether R-ERR-03 fails, and delete the
   file. No clause edit either way (§Out of scope) — check: `git status --porcelain` shows no
   file under `src/core/` that the phase did not have before.
5. **`verify.md` for the operator** — touches
   `docs/phases/21-scaffold-filename-splitting/verify.md` — both R-PROC-02 headings; explains in
   plain terms what word splitting is, why a space in a file name hid the file from the checks,
   and gives probes 1–3 as hand checks with expected output. No hardware; the file says so —
   check: both heading greps in §Acceptance criteria print `1` or more.
6. **Full gates** — check: `make test` → exit 0; `make lint` → exit 0; `git status --porcelain`
   shows no `zz` probe file and no `__pycache__` under `tests/`.

## Acceptance criteria

Run from the repo root under `sh` (not zsh). Each probe cleans up after itself. Probes 2–3
need clang-format and clang-tidy (Homebrew LLVM), as `make lint` does.

```
# probe 1 — rows 1-3: one violation per list, in a file whose name has a space
sh -c '
printf "void f( ) { throw 1; }\n"        > "src/core/zz probe.cpp"; a=$( sh tests/test_repo_shape.sh | /usr/bin/grep -c "^  FAIL: R-ERR-03$" );   rm -f "src/core/zz probe.cpp"
printf "virtual void poll( );\n"         > "src/core/zz probe.hpp"; b=$( sh tests/test_repo_shape.sh | /usr/bin/grep -c "^  FAIL: R-CLEAN-09$" ); rm -f "src/core/zz probe.hpp"
printf "LinkState step( Link& link );\n" > "src/core/zz probe.h";   c=$( sh tests/test_repo_shape.sh | /usr/bin/grep -c "^  FAIL: R-ERR-02$" );   rm -f "src/core/zz probe.h"
echo "$a $b $c"'
# expect: 1 1 1 (today: 0 0 0)
```

```
# probe 2 — row 4: R-STYLE-01 diagnoses the formatting, not a missing file
sh -c '
printf "int  x = 1;\n" > "src/core/zz probe.cpp"; out=$( sh tests/test_style.sh 2>&1 ); rm -f "src/core/zz probe.cpp"
printf "%s\n" "$out" | /usr/bin/grep -c "zz probe.cpp:.*clang-format-violations" | tr "\n" " "
printf "%s\n" "$out" | /usr/bin/grep -c "No such file"'
# expect: 1 0 (today: 0 2)
```

```
# probe 3 — row 5: R-STYLE-02 reports the naming violation, not a missing file
sh -c '
printf "void BadName( ) {}\n" > "src/core/zz probe.cpp"; out=$( sh tests/test_style.sh 2>&1 ); rm -f "src/core/zz probe.cpp"
printf "%s\n" "$out" | /usr/bin/grep -c "zz probe.cpp:.*readability-identifier-naming" | tr "\n" " "
printf "%s\n" "$out" | /usr/bin/grep -c "no such file or directory"'
# expect: 1 0 (today: 0 2)
```

```
# The new cases exist and run
sh tests/test_style.sh | /usr/bin/grep -cE '^  ok:   R-STYLE-0[12] rejection case'        # expect: 2
/usr/bin/grep -cE "^reject find_(err03|clean09|err02) +'src/core/a b\.(cpp|hpp|h)'" tests/test_repo_shape.sh   # expect: 3
# Finders are still one-liners the liveness harness mutates
/usr/bin/grep -cE '^find_[a-z0-9]+\( ?\) *\{.*\}$' tests/test_repo_shape.sh                 # expect: 10
python3 tests/test_checks_are_live.py | /usr/bin/grep -E 'no check pattern in +test_repo_shape' | /usr/bin/grep -c 'find_'   # expect: 0
# No affected clause claims a space in a file name is handled or missed
/usr/bin/grep -E '^- \*\*R-(ARCH-01|ARCH-03|ERR-0[1-4]|CLEAN-0[2345]|CLEAN-09|PROTO-05|STYLE-0[12])\*\*' docs/constraints.md | /usr/bin/grep -ciE 'name (contains|containing|with) a space'   # expect: 0
# Bindings unchanged
/usr/bin/grep -cE '^- \*\*R-(ARCH-01|ARCH-03|ERR-0[1-4]|CLEAN-03|CLEAN-05|CLEAN-09|PROTO-05)\*\* — .* — test: `tests/test_repo_shape\.sh`$' docs/constraints.md   # expect: 10
/usr/bin/grep -cE '^- \*\*R-(STYLE-0[12]|CLEAN-0[24])\*\* — .* — test: `tests/test_style\.sh`$' docs/constraints.md   # expect: 4
# This phase's verify.md
/usr/bin/grep -cE '^#+ What was built' docs/phases/21-scaffold-filename-splitting/verify.md   # expect: 1 or more
/usr/bin/grep -cE '^#+ .*[Cc]heck it' docs/phases/21-scaffold-filename-splitting/verify.md    # expect: 1 or more
# Nothing left behind
git status --porcelain | /usr/bin/grep -cE 'zz|__pycache__'   # expect: 0
# Project gates (.claude/workflow/toolchain.json: test, lint; no typecheck tool configured)
OPTIONAL_TOOLS=1 sh tests/test_style.sh   # expect: exit 0
python3 tests/test_rule_traceability.py   # expect: exit 0
python3 tests/test_checks_are_live.py     # expect: exit 0
make test                                 # expect: exit 0
make lint                                 # expect: exit 0
```

## Out of scope

- **A newline in a file name** — POSIX `sh` has no NUL-delimited read; already recorded in
  R-ARCH-02. Step 4 measures it for `test_repo_shape.sh` and records the result in `notes.md`
  only; no clause gains a newline sentence in this phase (the row excludes it).
- **Other file-name forms `git ls-files` quotes** (non-ASCII under `core.quotePath`, `"`, tab)
  and switching `test_style.sh` to `git ls-files -z` — not in the row; record in `notes.md` if
  met.
- **`$SYSROOT` splitting on an SDK path with a space**, and every other form the affected
  rules' scope clauses name — not in the row.
- **Changing any finder's pattern, exclusion, the extension filters, or the tool flags** —
  this phase changes how lists are passed, not what is matched.
- **Mutation-testing `test_style.sh`** (removing it from `NO_MUTATE`) — not in the row.
- **`tests/test_boundaries.sh`** — fixed in 20-scaffold-check-harness-fixes.
- **Editing closed phases' directories** (`20-scaffold-check-harness-fixes/` and earlier).
