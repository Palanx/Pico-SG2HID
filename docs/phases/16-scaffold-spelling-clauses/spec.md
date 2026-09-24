# Phase 16-scaffold-spelling-clauses — spelling and file-scope clauses for seven grep-bound rules

## Goal

Seven rules in `docs/constraints.md` §Invariants are bound `test:` to a `find_*` grep in
`tests/test_repo_shape.sh`. Every regex has spellings of its subject that it does not match, yet
six of these rule texts carry no scope text at all. The seventh, R-ERR-03, has a clause that covers
its scanner (`strip_line_comments( )`) and says nothing about its regex. Two of the seven, R-CLEAN-03
and R-CLEAN-05, are also written for the whole repository while their check reads `src_files( )`
only. After this phase, each of the seven rule lines states in its own text what its finder was
**measured** not to report. Nothing else changes: no check, no test case, no source file, no
binding.

A **spelling clause** names the forms of the rule's subject that the finder does not report. Each
named form is one fixture line that was fed to the extracted finder and produced no output. A
**file-scope clause** names the files the check reads and the files the rule's text covers that
it does not read. A form that was not measured is not named. Forms that are reported, including
loud false positives, may be named, but the clause does not have to name them.

| rule | finder (scanner, file list) | clause owed | written as |
|---|---|---|---|
| R-ARCH-01 | `find_arch01( )` (`hits`, `core_files`) | spelling | new `**Scope, recorded <date> because the check reads less than this text says:**` clause |
| R-ARCH-03 | `find_arch03( )` (`hits`, `src_files`) | spelling | new clause, same heading |
| R-CLEAN-03 | `find_clean03( )` (`hits`, `src_files`) | spelling + file scope | new clause, same heading |
| R-CLEAN-05 | `find_clean05( )` (`raw_hits`, `src_files`) | spelling + file scope | new clause, same heading |
| R-CLEAN-09 | `find_clean09( )` (`hits`, `core_files`) | spelling | new clause, same heading |
| R-ERR-03 | `find_err03( )` (`hits`, `src_files`) | spelling | **extends** the existing 2026-09-23 clause with dated sentences; no second heading. It must name the one form already known: "`try` with `{` on the next line". |
| R-ERR-04 | `find_err04( )` (`hits`, `src_files`) | spelling | new clause, same heading |

`<date>` is the day of the measurement. The file lists, as `15-scaffold-file-lists` left them:
`src_files( )` reads `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` under `src/`, and `core_files( )`
reads the same five extensions under `src/core/`.

## Context pointers

- `CLAUDE.md` — §Rules are bound to tests. This phase changes rule text only, so no test moves.
- `docs/constraints.md` — the seven rule lines are where every edit lands
  (`/usr/bin/grep -nE '^- \*\*R-(ARCH-01|ARCH-03|CLEAN-03|CLEAN-05|CLEAN-09|ERR-03|ERR-04)\*\*' docs/constraints.md`).
  R-ERR-02 and R-PROTO-05 are the house examples of a measured spelling clause ("Measured one
  declaration per form: … are all outside the binding and none is reported"), and R-CLEAN-04's
  is the house example of a file-scope clause. Match their register.
- `tests/test_repo_shape.sh` — `src_files( )`, `core_files( )`, `strip_line_comments( )`,
  `hits( )`, `raw_hits( )` and the seven `find_*` one-liners. Each regex is what gets measured.
- `docs/phases/01-ps2-codec/spec.md` §"What a `test:` binding does and does not promise" and its
  subsection §"The assignment, measured against the files rather than recalled" — define the three
  clause kinds and assign each of the seven the clauses in §Goal's table.
- `docs/phases/15-scaffold-file-lists/notes.md` §For later phases — the file lists these clauses
  describe, and the `.c` gap this phase leaves to its owner (§Out of scope).
- `docs/phases/12-scaffold-scope-clauses/spec.md` and `notes.md` — the precedent phase: clause
  heading, per-rule grep checks, measurement in a scratch directory outside the repo.
- `.claude/rules/tech-debt.md` — the entry "Unwritten spelling and file-scope clauses for seven
  grep-bound rules" is the origin of this phase.
- `tests/test_rule_traceability.py` — `RULE_LINE`: a rule line stays
  `- **R-…** — <text> — test: <path>` on a single line. The clause goes inside `<text>` and must not
  contain ` — test: `.
- `docs/phases/12-scaffold-scope-clauses/verify.md` — the house shape for a no-hardware `verify.md`.

## Plan

**How to measure.** Every step from 2 to 8 uses this method. Do not source
`tests/test_repo_shape.sh`, because sourcing it runs the whole suite. Extract the helpers and
finders into a scratch directory **outside the repo** (`$S` below is that directory) and run one
fixture per form:

```
{ sed -n '/^src_files( )/,/^# report /p' tests/test_repo_shape.sh | sed '$d'
  /usr/bin/grep -E '^find_[a-z0-9]+\( *\)' tests/test_repo_shape.sh; } > "$S/finders.sh"
mkdir -p "$S/t/src/core"
printf '%s\n' 'try' '{' > "$S/t/src/core/x.cpp"      # one fixture per run
sh -c ". '$S/finders.sh'; find_err03 '$S/t'"            # no output = not reported
```

For the two `src_files( )` rules, measure file scope the same way. Place a violating file outside
the list, for example `$S/t/tests/x.cpp`, `$S/t/src/core/x.c` or a `TODO` in `$S/t/tests/x.sh`,
and confirm that nothing is reported.

**Minimum fixture set.** Measure at least these forms, plus any others the regex suggests. The
list only says what to feed the finder. It does not predict any result except the one already
known, and the clause records whatever the measurement shows.

| finder | forms to feed, at least |
|---|---|
| `find_arch01` | `# include "pico/stdlib.h"` (space after `#`); `#include "pico.h"`; `#include "RP2040.h"`; `#include <stdio.h>`; `#include <cstdio>`; `#include <chrono>`; `#include <atomic>`; `#include HEADER_MACRO` |
| `find_arch03` | `delete[] p;`; `auto p = new (buf) T;`; `void* p = ::operator new( 4 );`; `void* p = calloc( 1, 4 );`; `void* p = realloc( p, 8 );`; `auto p = std::make_unique<T>( );`; `std::wstring s;`; `std::map<int, int> m;`; `using std::vector; vector<int> v;`; `auto* p = new` / `T;` split across two lines |
| `find_clean03` | `bool flag{ true };`; `bool flag( true );`; `bool a, flag;`; `void f( bool flag );`; `bool check( );`; `std::atomic<bool> flag;`; `auto flag = true;`; `bool is_ok = true; bool flag = false;` on one line; `bool flag : 1;` |
| `find_clean05` | `// todo: fix`; `// FIXME: fix`; `// TODO(): fix`; `// TODO(later): fix`; `// TODO (09-guitar-observe): x`; `/* TODO */`; a bare `TODO` in `tests/x.sh`, `tests/x.cpp`, `src/core/x.c` (file scope) |
| `find_clean09` | `struct A final : B { };`; `struct alignas( 4 ) A : B { };`; `struct [[nodiscard]] A : B { };`; `struct A` / `: B { };` split across two lines; `enum struct E : std::uint8_t { };` (a possible loud false positive); `void poll( ) override;` |
| `find_err03` | `try` / `{` on separate lines (**known: not reported**); `catch` / `( ... )` split across two lines; `std::rethrow_exception( e );`; `std::throw_with_nested( e );`; `void f( ) try {` |
| `find_err04` | `auto v = r->value( );`; `.value` / `( )` split across two lines; `auto v = std::move( r ).value( );`; `auto v = opt.value( );` on a `std::optional` (a possible loud false positive) |

1. **Extract and confirm the method.** Build `$S/finders.sh` as above. Check: the known form
   (`try` / `{`) produces no output from `find_err03`, and `try {` on one line produces one
   `…/x.cpp:1:try {` line.
2. **R-ARCH-01 spelling clause.** Measure `find_arch01`, then add the new clause before
   ` — test:`. It touches `docs/constraints.md`. Check:
   `/usr/bin/grep -E '^- \*\*R-ARCH-01\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_arch01( )'`
   → one line. `python3 tests/test_rule_traceability.py` → exit 0.
3. **R-ARCH-03 spelling clause.** Same shape. Check: the step-2 grep with `R-ARCH-03` /
   `find_arch03( )` → one line, and traceability → exit 0.
4. **R-CLEAN-03 spelling + file-scope clause.** The file-scope part names `src_files( )`, its five
   extensions, and what the unscoped text covers that the check does not read: everything outside
   `src/`, and `.c` under it. Check: the step-2 grep with `R-CLEAN-03` / `find_clean03( )`, plus
   `| /usr/bin/grep -F 'src_files( )'` → one line, and traceability → exit 0.
5. **R-CLEAN-05 spelling + file-scope clause.** Same shape as step 4. Check: the step-4 grep with
   `R-CLEAN-05` / `find_clean05( )` → one line, and traceability → exit 0.
6. **R-CLEAN-09 spelling clause.** Check: the step-2 grep with `R-CLEAN-09` / `find_clean09( )`
   → one line, and traceability → exit 0.
7. **R-ERR-03 spelling extension.** Append dated sentences to the existing clause. They name
   `find_err03( )`'s regex misses, including the exact phrase "`try` with `{` on the next line".
   Check: `/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -F '`{` on the next line'`
   → one line.
   `/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -o '\*\*Scope, recorded' | wc -l`
   → 1 (extended, not duplicated). Traceability → exit 0.
8. **R-ERR-04 spelling clause.** Check: the step-2 grep with `R-ERR-04` / `find_err04( )` →
   one line, and traceability → exit 0.
9. **No scratch in the repo, checks untouched.** Check:
   `git status --porcelain --untracked-files=all` lists only `docs/constraints.md`, files under
   `docs/phases/16-scaffold-spelling-clauses/`, `docs/phases/PHASES.md` and `docs/index/`.
   `git diff --name-only main -- src tests .clang-tidy .clang-format Makefile` → empty.
10. **`verify.md` for the operator.** It touches
    `docs/phases/16-scaffold-spelling-clauses/verify.md`. It covers what changed (documentation
    only, no behaviour), why a rule that states less than its check measures matters, and how to
    re-run one measurement by hand: the known `try` / `{` form, with the commands from "How to measure" at the
    top of §Plan and the expected empty output. No physical steps; the file says so. Check:
    `test -s docs/phases/16-scaffold-spelling-clauses/verify.md` → exit 0.
11. **Full gates.** Check: `make test` → exit 0; `make lint` → exit 0.

## Acceptance criteria

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.

```
/usr/bin/grep -E '^- \*\*R-ARCH-01\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_arch01( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-ARCH-03\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_arch03( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-CLEAN-03\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_clean03( )' | /usr/bin/grep -F 'src_files( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-CLEAN-05\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_clean05( )' | /usr/bin/grep -F 'src_files( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-CLEAN-09\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_clean09( )' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -F '`try` with `{` on the next line' | wc -l    # expect: 1
/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -o '\*\*Scope, recorded' | wc -l    # expect: 1 (extended, not duplicated)
/usr/bin/grep -E '^- \*\*R-ERR-04\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'find_err04( )' | wc -l    # expect: 1
/usr/bin/grep -cE '^- \*\*R-(ARCH-01|ARCH-03|CLEAN-03|CLEAN-05|CLEAN-09|ERR-03|ERR-04)\*\* — .* — test: `tests/test_repo_shape\.sh`$' docs/constraints.md    # expect: 7 (bindings unchanged)
python3 tests/test_rule_traceability.py       # expect: exit 0
git diff --name-only main -- src tests .clang-tidy .clang-format Makefile | wc -l    # expect: 0
test -s docs/phases/16-scaffold-spelling-clauses/verify.md    # expect: exit 0
make test                                     # expect: exit 0
make lint                                     # expect: exit 0
```

## Out of scope

- **Fixing what the clauses record.** Widening a regex, adding a rejection case, or moving to
  `clang-query`. A clause records a gap; closing one is check-harness work. The `clang-query`
  upgrade is owed by `03-pio-bus` (R-ERR-02's text).
- **`.c` under `src/` for R-ARCH-01, R-ARCH-03, R-CLEAN-09, R-ERR-03 and R-ERR-04.** No list reads
  `.c`, and these five texts do not say so. That gap is the file list's, not the regex's.
  `15-scaffold-file-lists/notes.md` §For later phases gives it to a candidate row beside
  `03-pio-bus`. R-CLEAN-03 and R-CLEAN-05 name `.c` only because their file-scope clause is owed
  here anyway.
- **Scope clauses for R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02 and R-PROC-01** belong to
  `14-scaffold-nongrep-clauses`.
- **R-ERR-01, R-ERR-02 and R-PROTO-05** already carry their clauses (§"The assignment" table in
  `01-ps2-codec/spec.md`).
- **Closed phases are not edited.** That covers `docs/phases/00-scaffold/`, `01-ps2-codec/` (including its assignment
  table's "not written" cells), `12-…`, `13-…` and `15-…`.
- **Closing the tech-debt entry** in `.claude/rules/tech-debt.md`. That is proposed after
  validation and decided by the operator.
