# Phase 12-scaffold-scope-clauses — notes

## Outcome

- `docs/constraints.md` — the rule lines for R-STYLE-02, R-CLEAN-02, R-PROC-02 and R-ERR-03 each
  gained a `**Scope, recorded 2026-09-23 because the check reads less than this text says:**`
  clause inside the rule text, before ` — test:`, in R-CLEAN-04's register. Bindings unchanged.
  - R-STYLE-02: `tidy_sources()` file scope + `HeaderFilterRegex` headers; `hal`/`usb`/`app`/`emu`
    and non-C++ files unchecked; points at R-CLEAN-04 for the pathspec note instead of repeating it.
  - R-CLEAN-02: same file scope; shell and Python functions never measured; names `reject( )` and
    `accept( )` in `tests/test_repo_shape.sh` (four positional parameters, unreported).
  - R-PROC-02: existence, not content (`[ -s … ]` in `missing_verify( )`); a one-line file passes.
  - R-ERR-03: `hits( )` strips only `//`-to-EOL; `/* … */` and string-literal hits are reported
    (false positive); code after an in-string `//` is not seen (silent miss).
- `docs/phases/12-scaffold-scope-clauses/verify.md` — operator check, no physical steps.
- R-ERR-03's two consequences were re-measured 2026-09-23 (Plan step 4) by extracting
  `src_files( )`, `hits( )` and `find_err03( )` from `tests/test_repo_shape.sh` into the session
  scratchpad (outside the repo) and running one fixture per form: `/* throw E; */` → reported;
  `const char* s = "throw";` → reported; `const char* u = "http://x"; throw E;` → not reported.
  All three match §Goal's table; no scratch file entered the repo (`git status --porcelain`).
- Acceptance: all ten criteria pass — the four clause greps → 1 each; bindings grep → 4;
  traceability → exit 0; `git diff --name-only main -- src tests .clang-tidy .clang-format Makefile`
  → 0; `verify.md` non-empty; `make test` → exit 0; `make lint` → exit 0.

## Deviations

- None. The spec's Context pointers were sufficient; no file outside them was read or changed.
  Extraction for step 4 used line ranges rather than sourcing the whole script, because sourcing
  `tests/test_repo_shape.sh` runs its suite; the measured functions are byte-identical to the file.

## Debt

- None introduced. The clauses record existing gaps; closing them is out of scope (spec §Out of scope).

## For later phases

- needs a row: 00-scaffold — the gaps these four clauses now record are still open and unowned:
  `tidy_sources()` does not reach `hal`/`usb`/`app`/`emu` (owed before `03-pio-bus` adds code
  there, else R-STYLE-02/R-CLEAN-02/R-CLEAN-04 silently stop covering new firmware);
  `hits( )` misses code after an in-string `//` (R-ERR-03 silent miss); `reject( )`/`accept( )`
  exceed R-CLEAN-02's three-parameter limit; R-PROC-02 checks no content. Run `/plan-feature`
  ("Scheduling a fix to a phase already done") to give them an owner.
- The other owed clauses listed in spec §Out of scope (spelling clauses for seven rules; scope
  clauses for R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02, R-PROC-01; `.hpp`/`.cc`/`.inl` under
  `src/` unscanned by `src_files( )`) remain unowned — same route.
- Taste from the 2026-09-23 independent review (not findings): the R-STYLE-02 clause pins
  `tests/test_style.sh:68`, which goes stale on the next edit to that file (R-CLEAN-04 already
  carries the same pin); `verify.md` describes R-STYLE-02/R-CLEAN-02 scope as `src/core/` plus
  `tests/` C++ without the included-header extension — deliberate simplification for the reader.

## Validation — 2026-09-23
- criteria: 10 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap
  - `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 9261 (first)
- upstream: none
- not-ours: none
- verdict: done
