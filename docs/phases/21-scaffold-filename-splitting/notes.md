# Phase 21-scaffold-filename-splitting — notes

## Outcome

- `tests/test_repo_shape.sh`: `hits( )` and `raw_hits( )` now take the file list as ONE
  argument and read it with `printf '%s\n' "$list" | while IFS= read -r f`. All ten finders
  pass their list quoted (`"$( src_files "$1" )"`, `"$( core_files "$1" )"`,
  `"$( core_headers "$1" )"`); each is still a one-line body whose first single-quoted strings
  are its pattern and exclusion. Three new rejection cases, one per list, each in a path with a
  space: `find_err03 'src/core/a b.cpp'`, `find_clean09 'src/core/a b.hpp'`,
  `find_err02 'src/core/a b.h'`. Rejection cases 74 → 77. The `hits( )` header comment
  describes the new call shape.
- `tests/test_style.sh`: `format_files( )` (one clang-format run per line) replaces
  `echo "$files" | xargs clang-format`; the real run now captures its output once instead of
  running clang-format twice. `tidy_files( )` wraps the clang-tidy loop, reading one line per
  path; flags unchanged, `$( tidy_lang )` / `$SYSROOT` still unquoted on purpose. A scratch dir
  (`mktemp -d`, `trap … EXIT`) holds copies of `.clang-format` and `.clang-tidy`; `style_case( )`
  runs one rejection case per block on `"$case_tmp/a b.cpp"` and prints
  `ok:   R-STYLE-0N rejection case: file name with a space` or a `FAIL:` line. Header comment
  names the cases.
- Both new `test_style.sh` cases were shown to bite: with `tidy_files`/`format_files` reverted to
  `for f in $1`, both print `FAIL:`.
- `docs/phases/21-scaffold-filename-splitting/verify.md` written for the operator.
- Acceptance: probe 1 `1 1 1`; probe 2 `1 0` (with the amended fixture, see Deviations);
  probe 3 `1 0`; all greps at their expected values; `OPTIONAL_TOOLS=1 sh tests/test_style.sh`,
  `test_rule_traceability.py`, `test_checks_are_live.py` (76/76 alternations, no `find_*` under
  `no check pattern`), `make test`, `make lint` all exit 0; nothing left behind.
- Step 4, newline measured once: `src/core/a<LF>b.cpp` containing `void f( ) { throw 1; }` →
  `sh tests/test_repo_shape.sh` prints `ok:   R-ERR-03` — still missed, as R-ARCH-02's text
  already records for this class of defect. File deleted; no clause edit (row excludes it).

## Deviations

- **Probe 2's fixture amended in `spec.md`.** The spec's probe wrote `int  x=1;` and expected
  `1 0`. clang-format emits one `clang-format-violations` line per violation site, and that line
  has three (cols 4, 7, 8), so the corrected script printed `3 0` — correct behaviour, wrong
  expected count. Changed the probe's fixture to `int  x = 1;` (one violation): `1 0` after this
  phase, `0 2` before (measured by stashing `tests/test_style.sh`), which is exactly the before/
  after the spec's Goal table states. Checked the other statements of this fact: the Goal table
  row 4 says only "a misformatted `src/core/a b.cpp`" (no count), Plan step 2's check says
  "probe 2 → `1 0`" (still true), and Plan step 2's in-script case keeps `int  x=1;` because it
  asserts presence (`grep -q`), not a count.
- **R-STYLE-02 rejection case gated on sources — fixed in round 2.** Round 1 put the case
  inside the `else` of `if [ -z "$files" ]` (that branch defined `$SYSROOT` and `tidy_files( )`),
  so on a tree with no checkable sources it was silently skipped — narrower than Plan step 2's
  "run only when that block's tool was found", and unlike R-STYLE-01's case, which already ran
  after its `fi`. Moved the `SYSROOT` / `note:` / `tidy_files( )` setup above the sources test and
  the case after its `fi`; no flag or wording changed. Consequence: the SDK `note:` now also prints
  on a tree with no sources. Checked with a scratch copy redefining `tidy_sources( ) { :; }`:
  prints `(no checkable sources yet)` and the case's `ok:` line. Re-ran the mutation (both loops
  back to `for f in $1`): both cases `FAIL:`. Full acceptance re-run: 18/18.

- **Validation 2026-09-24, independent review — undecidable, resolved in round 2.** `verify.md`
  told the operator that `sh tests/test_style.sh` prints exactly "four lines, all starting
  `ok:`", which no spec statement backs. Reworded to what Plan step 2's check asserts: no
  `FAIL:` line, two `rejection case: file name with a space` lines, exit 0; other lines
  (`note:`) may appear. Spec unchanged.

- **Validation 2026-09-24, independent review — code-side, fixed in round 2.** Round 1 wrote
  `elif out=$( format_files "$files" ) && [ -n "$out" ]`, so R-STYLE-01's verdict took the exit
  status of the LAST `clang-format --Werror` in the loop: a tree whose last file was
  misformatted printed `ok:`. Reproduced (appended `int  y = 2;` to `tests/vectors/unknown_id.h`,
  the last file in `sources()`) → `ok:   R-STYLE-01`. Fixed: `elif out=$( format_files … );
  [ -n "$out" ]`, and `format_files( )` now ends in `return 0` with a comment, the same shape as
  `hits( )`. Same reproduction after the fix → `FAIL: R-STYLE-01` naming that file; reverted.
  Property generalised: *a check's verdict is decided by its output, never by the exit status
  of a per-file loop.* Enumerated every site that consumes a file-list loop:
  `test_style.sh` R-STYLE-01 real run (the defect), R-STYLE-02 real run (`out=$( tidy_files … )`
  as its own statement, then `[ -n "$out" ]` — holds), `style_case( )` (greps output — holds);
  `test_repo_shape.sh` `hits( )` / `raw_hits( )` (`return 0`) and `report( )` (`[ -n "$2" ]`)
  — hold; `test_boundaries.sh` `sweep( )` (out of this row's scope, fixed in phase 20) not
  edited. No other instance. The rejection case could not have caught it because it asserts
  `format_files`'s output, not the real run's `elif`; no new case added — the verdict is now
  the same `[ -n ]` test the case exercises.

## Debt

- None new. Newline in a file name remains open for all three check scripts — already recorded
  in R-ARCH-02 and out of this row's scope.

## For later phases

- `Makefile` `test` target loops `for t in $(SH_TESTS)` unquoted — splits a test-script name
  with a space. Not a checked-source list, no test file has one, and no rule covers it; left
  alone deliberately. Owner if it ever matters: whichever row next edits the `Makefile` test
  target.
- Other forms `git ls-files` quotes (non-ASCII under `core.quotePath`, `"`, tab) were not met
  in this phase; `test_style.sh` still reads `git ls-files` output, so those names would arrive
  quoted and not exist. Unmeasured.
- Independent review round 2, taste (non-blocking): in `tests/test_style.sh`, `mktemp -d` /
  `trap` / `cp` of the two configs run at top level even when neither tool is installed, and a
  `mktemp` failure exits 1 with no `FAIL:` line; `tidy_files( )` is defined inside the `else`
  branch while `format_files( )` is top-level; `style_case( )` requires `a b.cpp:` and the
  diagnostic on the same line (stricter than Plan step 2's wording, same as the probes).
  `verify.md` uses bare `grep` where the spec uses `/usr/bin/grep`, and says no tracked file
  has a newline in its name — checked at validation: `git ls-files -z` and `git ls-files` both
  list 125 paths.

## Validation — 2026-09-24
- criteria: 18 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: contradicts (code-side): R-STYLE-01's real run reports `ok` when the last file in `sources()` is misformatted — `elif out=$( format_files "$files" ) && [ -n "$out" ]` takes the exit status of the last `clang-format --Werror` in the `while` loop, so `&&` short-circuits. Reproduced: appended `int  y = 2;` to `tests/vectors/unknown_id.h` (the last file in the list) → `ok:   R-STYLE-01`; reverted. Conflicts with Goal row 4 and "every result line's prefix and wording are unchanged" (the old check failed on any output). Code-side because no Deviations entry records it and the old `| grep -q .` behaviour tested output only. The new rejection case does not catch it, because `style_case` inspects output, not the `elif`. | undecidable: `verify.md`'s "four lines, all starting `ok:`" has no spec statement behind it (see Deviations)
- closure test: fail: undecidable review finding (missing pointer for verify.md's expected-output claim)
- findings: 2
- spec size: 13414 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation

## Validation — 2026-09-24 (round 2)
- criteria: 18 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 13414 (+0 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
