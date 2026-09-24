# Phase 20-scaffold-check-harness-fixes — notes

## Outcome

All five check gaps are closed (six spellings; spec §Goal rows 1–6). Each has a rejection
case, and each is gone from its rule's scope clause.

- `tests/test_boundaries.sh` — `sweep( )` reads `find`'s output with
  `| { while IFS= read -r f; …; exit "$sweep_bad"; }` and returns the pipeline's status. The
  0/1/2 contract and the `2 )` precedence are unchanged. The hook's stdin is now `/dev/null`,
  so it can never read the path list. New rejection case: `src/core/a b.cpp` including
  `hal/bus.h` makes `sweep` return 1. The floor is now `3/3`. Probe 1 → `1`.
- `tests/test_rule_traceability.py`:
  - `MARKER` captures `\d+`.
  - The `test` branch tests for the marker with `RULE <id>(?!\d)`, and reports
    `bound to '<value>', which is not a file` before reading a path that exists but is not
    a regular file.
  - `ANY_RULE_LINE` is `^[ \t]*[-*] \*\*R-.*$`. `RULE_LINE` is unchanged, so a `*` or
    indented rule is reported as unparsable.
  - Four new `CASES`. The floor is 13, and the count words changed to match.
  - Probe 2 → four `reported`; `13/13`.
- `tests/test_tool_versions.sh` — `ver_num( )` strips everything up to the first `version`
  (`${vn_t#*version}`) when the text contains it, then reads the first `N` or `N.N` as
  before. The header comment was updated. There are two new rejection cases and one accept
  case. The floor is now `6/6`. Probe 3 → `rejected rejected ok ok`. `Python 3.14.6`,
  `15.3.1`, `15`, `23` and `3.8` still parse to 314, 1503, 1500, 2300 and 308.
- `tests/test_checks_are_live.py` — docstring count only: "nine" → "thirteen". Run
  result: `neutered: 35/35`, `alternations: 76/76`, exit 0.
- `docs/constraints.md`:
  - R-ARCH-02: `sweep( )` is described as "one per line and unsplit". The space sentence
    is replaced by the measured newline miss, and the symlink sentence is kept.
  - R-PROC-01: the `ANY_RULE_LINE` sentence is rewritten. The `R-X-011` item and the
    `IsADirectoryError` sentence are deleted.
  - R-TOOL-01: the `ver_num( )` sentence is rewritten, and the first-number item is
    deleted.
  - The `recorded` dates stay 2026-09-24, because that is the day of the edit. The
    bindings are unchanged.
- `docs/phases/20-scaffold-check-harness-fixes/verify.md` — new.
- Acceptance: every probe and grep in spec §Acceptance criteria gives its expected value.
  `make test` → 0. `make lint` → 0. There is no `zz`/`__pycache__` left.

## Deviations

- **Spec step 3 said:** run the new `ver_num( )` cases "through
  `check_version clang-format 23`". **Done:** they run through a stub named
  `banner-format`. **Why:** the R-TOOL-01 wiring case further down needs the
  `clang-format` stub in `$stub_dir` to still report 14. Overwriting that stub made the
  wiring case fail (`run_all left fail=0`, measured). The banners and the floor are the same
  as the spec's. Probe 3 still goes through the name `clang-format`.
- **Spec step 1 said:** "measure a filename containing a newline once; if still missed,
  step 5 names it". **Measured:** missed. A tree whose only source is `src/core/a<NL>b.cpp`
  including `hal/bus.h` makes `sweep` return 0. R-ARCH-02's clause now names it in place of
  the space sentence. I also rewrote the clause's first sentence
  (`prints, one per line and unsplit, to`). It states the same fact about how `sweep( )`
  receives paths, so it was reconciled in the same edit. I checked the rest of R-ARCH-02,
  R-PROC-01 and R-TOOL-01, `docs/index/` and the other non-closed docs for statements of the
  five forms. The only other mentions are the superseded rows 17–19 and row 20 in
  `PHASES.md`, which are history and are left as they are.
- **Also edited, beyond the spec's list:** the `main( )` comment in
  `test_rule_traceability.py` said "breaks when a tenth case is added". It now says
  "another case", because "tenth" was stale once there were thirteen cases. I also added
  comments above `ANY_RULE_LINE` and `MARKER` saying why each is shaped the way it is.

- **Spec amended at validation (2026-09-24):** Plan step 3 said the form-6 cases run
  "through `check_version clang-format 23`". The review returned this as `contradicts`. It is
  spec-side: the `banner-format` deviation above records the measured reason. Step 3 now says
  the cases use a stub name other than `clang-format`. Checked for other statements of the
  same fact: Goal row 6 names only `ver_num( )`, and probe 3 runs its own harness through
  `clang-format`, which stays true. Neither needed a change, and nothing else is made
  redundant.

- **Validation round 1, code-side finding (2026-09-24):** verify.md items 2–3 gave only the
  gate lines (`13/13`, `6/6`), not probes 2 and 3 as Plan step 6 requires. **Done:** item 2
  now carries probe 2's `python3` block (expected: four `reported`), item 3 carries probe 3's
  `sh -c` block (expected: `rejected rejected ok ok`), each with its pre-phase output. The
  gate lines stay. **Enumeration** of the property "verify.md gives each of probes 1–3 with
  expected output": probe 1 was already item 1 (unchanged); probes 2 and 3 were the only
  misses. Both blocks were run as a reader copies them (list indentation stripped) and gave
  the expected output, leaving no `zz`/`__pycache__`.

## Debt

- None new. `sweep( )` still splits a name that contains a newline. This is recorded in
  R-ARCH-02's clause, not left in the source as `belay-debt:`. Closing it needs NUL-separated
  paths, and POSIX `sh` has no `read -d ''`.

## For later phases

- **needs a row: 00-scaffold** — `tests/test_repo_shape.sh` has the same defect this phase
  fixed in `sweep( )`. Every finder passes `$( src_files "$1" )`, `$( core_files "$1" )` or
  `$( core_headers "$1" )` unquoted. A file under `src/` whose name contains a space is
  therefore split. `hits( )` and `raw_hits( )` skip both halves with `[ -f ]`, and the file
  is never read.
  - Measured 2026-09-24: `void f( ) { throw 1; }` in `src/core/zz probe.cpp` → `ok: R-ERR-03`.
    The same file as `src/core/zzprobe.cpp` → `FAIL: R-ERR-03`.
  - Affected, by reading the source: R-ARCH-01, R-ARCH-03, R-ERR-01..04, R-CLEAN-03,
    R-CLEAN-05, R-CLEAN-09 and R-PROTO-05. None of their clauses mention it.
  - `tests/test_style.sh` feeds `sources()` to `xargs clang-format`, which also splits on
    whitespace. R-STYLE-01 fails on such a file, but with
    `src/core/zz: No such file or directory`: loud, with the wrong diagnosis.
  - The Goal of this phase survives this, so it is deferred rather than fixed here. The code
    was delivered by a `done` phase, so it needs `/plan-feature`.
  - Scheduled 2026-09-24 as `21-scaffold-filename-splitting` (PHASES.md). That row also
    covers the clang-tidy loop `for f in $files` in `tests/test_style.sh` (R-STYLE-02,
    R-CLEAN-02, R-CLEAN-04), which has the same split and was found by reading.

- **Reviewer taste, validation round 2 (2026-09-24), not blocking:** `ver_num( )` matches the
  substring `version` (`${vn_t#*version}`), not the whole word, so `subversion` would also
  match; no known banner tells the readings apart. `verify.md` probe 1 uses bare `grep`
  without anchors, unlike the spec's `/usr/bin/grep '^…$'`.

## Validation — 2026-09-24
- criteria: 23 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: contradicts (spec-side): Plan step 3 named `check_version clang-format 23`, but the cases use the `banner-format` stub — evidence: the Deviations entry records the measured wiring-case failure; spec amended. contradicts (code-side): Plan step 6 says verify.md gives "probes 1–3 as hand checks with their expected output", but verify.md items 2–3 give only the gate lines (`13/13`, `6/6`), not probe 2's four-form `python3` block (four `reported`) or probe 3's four-banner `sh -c` block (`rejected rejected ok ok`) — evidence: no Deviations entry, and the spec states the intent
- closure test: pass
- findings: 2
- spec size: 15512 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation

## Validation — 2026-09-24 (round 2)
- criteria: 23 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 15512 (+0 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
