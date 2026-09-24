# Phase 15-scaffold-file-lists — notes

## Outcome

- `tests/test_repo_shape.sh`: `core_files( )` is a one-line `grep -E` alternation over
  `.cpp|h|hpp|cc|inl` under `src/core/`; `core_headers( )` over `.h|hpp`. The comment above
  `core_headers( )` states that `.cc` and `.inl` are excluded for the same reason as `.cpp`.
  Five new rejection cases: `find_clean09` on `x.cpp`, `x.hpp`, `x.cc`, `x.inl`, and
  `find_err02` on `x.hpp`. `tests/test_checks_are_live.py` reports 76/76 alternations caught.
- `tests/test_style.sh`: `sources()` lists the five extensions; `tidy_sources()` lists the
  five `src/` pathspecs plus `tests/*.cpp`; `tidy_lang()` returns `-xc++` for `*.h|*.inl`. The
  header comment (scope, `-xc++` note), the `tidy_lang` comment, the inline `-xc++` comment
  in the tidy loop and the R-STYLE-01 `Fix:` hint name the new extensions.
- `.clang-tidy`: "Scope:" line and the `-xc++` sentence updated.
- `docs/constraints.md`: R-STYLE-01 names five extensions; R-STYLE-02, R-CLEAN-02, R-CLEAN-04
  name the five `src/` pathspecs and state `.c` as the only unchecked extension under `src/`;
  R-ERR-02 scans `.h` and `.hpp` and states `.cc`/`.inl` are excluded and a declaration in a
  `.inl` is outside the binding. The `R-STYLE-02's scope is` paragraph and the one after it,
  and the §Observed conventions clang-tidy-flags finding, updated; the finding carries the
  `.inl` failure text, re-measured this session (Homebrew LLVM, `clang-tidy` on a `.inl`
  under `src/core/` without `-x` → `unable to handle compilation, expected exactly one
  compiler job`; with `-xc++` → clean).
- `tests/test_phase_docs.sh`: header no longer claims no phase is done.
- `docs/phases/15-scaffold-file-lists/verify.md`: operator checks, no hardware.
- Acceptance: every §Acceptance criteria line printed its expected value; `make test` exit 0,
  `make lint` exit 0, `test_rule_traceability.py` and `test_checks_are_live.py` exit 0, no
  `zz_probe` left.

## Deviations

- R-CLEAN-04's clause gained "(the three middle patterns added 2026-09-23)" because its scope
  clause is dated 2026-09-15 and the list changed after that date. Spec did not ask for it.
- R-STYLE-02's clause, beyond naming `.c`, keeps the pre-existing statement that non-C++
  files and C++ files outside `src/` (other than `tests/*.cpp` and included headers) are
  unchecked — reworded into one sentence since the old `(.hpp, .cc, .inl, .c)` list it hung
  off was removed.
- Two comments in `tests/test_style.sh` not named by Plan step 4 (the `tidy_lang` doc line and
  the inline `-xc++ on headers only` comment in the tidy loop) were updated: both would
  otherwise contradict `tidy_lang()`'s new `.inl` case. Same file, inside scope.

- Validation 2026-09-23 (undecidable): the spec never points to the text of R-ARCH-01,
  R-CLEAN-09 and R-ERR-01, so a reviewer holding only the spec could not judge
  `verify.md`'s paraphrase of them in the `core_files( )` row. Missing pointer. Fixed by
  widening the existing §Context pointers `docs/constraints.md` entry (id list and grep) to
  the three ids; no sentence added. Reconciliation checked: the Goal table's "rules it
  backs" column already names the same ids and is unchanged; no other spec statement
  asserts which rule lines to read.

## Debt

- None new. The pre-existing `belay-debt:` above `core_headers( )` (anonymous-namespace
  functions, clang-query upgrade in `03-pio-bus`) is unchanged.

## For later phases

- `03-pio-bus` (candidate row, per spec §Out of scope): no C++ check reads `.c`. A `.c` under
  `src/` — likely once SDK-shaped files arrive — is seen by no rule check, and R-ARCH-01,
  R-CLEAN-09 and R-ERR-01 do not say so (R-STYLE-02, R-CLEAN-02, R-CLEAN-04 now do).
- `16-scaffold-spelling-clauses`: the file lists its clauses describe are now five extensions
  for `core_files( )`/`src_files( )` and `.h`/`.hpp` for `core_headers( )`.
- Unowned: `CORE_SRC` in the `Makefile` is `src/core/*.cpp` only, so a `.cc` under
  `src/core/` would not be linked into host tests (spec §Out of scope; build question, no row).
- Review taste (non-blocking, 2026-09-23): in the §Observed conventions clang-tidy-flags
  finding, the sub-bullet label still reads "`-xc++`, headers only:" while its text now
  covers `.inl`. Some edited lines in `docs/constraints.md`, `.clang-tidy` and the
  `tidy_lang` comment were not reflowed. R-STYLE-02's closing sentence is dense.
- Review taste, round 2 (non-blocking, 2026-09-23): R-ERR-02's new `.cc`/`.inl` sentence
  sits before "Measured 2026-09-11", so it reads as measured on that date. R-CLEAN-04's
  "the three middle patterns" means positions 3–5 of six; naming them would be clearer.
  `verify.md` probes use plain `grep`, not `/usr/bin/grep` (fine for the operator's shell).
- After validation: propose closing the tech-debt entry "File lists that stop at `.cpp` and
  `.h`" in `.claude/rules/tech-debt.md` — the operator decides.

## Validation — 2026-09-23
- criteria: 20 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: undecidable: `verify.md` table row for `core_files( )` paraphrases R-ARCH-01, R-CLEAN-09, R-ERR-01 ("no platform headers / no inheritance / no bare status returns"), but the spec names only their ids and never points to their text, so the reviewer could not judge the paraphrase. (Checked outside the review: the paraphrase matches `docs/constraints.md` lines for all three — spec gap, not a `verify.md` defect.)
- closure test: fail: the undecidable finding above is a missing pointer — the §Context pointers `docs/constraints.md` entry names R-STYLE-01, R-STYLE-02, R-ERR-02, R-CLEAN-02, R-CLEAN-04 but not R-ARCH-01, R-CLEAN-09, R-ERR-01, whose text `verify.md` restates
- findings: 1
- spec size: 13122 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-09-23 (round 2)
- criteria: 20 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean — note: the reviewer prompt carried one sentence beyond the command's text (a hunk restating content the spec points to by name is not undecidable merely because the reviewer cannot open it), so this review did not itself judge `verify.md`'s paraphrase of R-ARCH-01, R-CLEAN-09, R-ERR-01; that paraphrase was checked by hand against `docs/constraints.md` in round 1
- closure test: pass
- findings: 0
- spec size: 13167 (+45 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
