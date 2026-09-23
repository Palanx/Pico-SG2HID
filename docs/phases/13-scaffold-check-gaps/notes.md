# Phase 13-scaffold-check-gaps — notes

## Outcome

- base: 9245db1
- **G1** — `tidy_sources()` in `tests/test_style.sh` now lists `src/*.cpp`, `src/*.h` (git
  pathspecs, recursive) and `tests/*.cpp`. A naming violation planted in each of `src/hal`,
  `src/usb`, `src/app`, `src/emu` is reported by `make lint` (probe prints `4`). Scope comments
  in `tests/test_style.sh` and `.clang-tidy` restated, including the expected
  `clang-diagnostic-error` on SDK-including files until `03-pio-bus`.
- **G2** — `hits( )` in `tests/test_repo_shape.sh` no longer uses `sed 's|//.*||'`; it calls a
  new `strip_line_comments( )` (awk) that cuts at the first `//` outside a `"…"` string
  (honouring `\"`) and outside a `'…'` character literal, keeping literal contents. New cases:
  one rejection (`"http://x"; throw E;`), two accepts (comment after a string; `'"'` in a char
  literal). `accepted -ge` floor 23 → 25. `belay-debt:` header comment rewritten.
- **G3** — `check_version( )` (`tests/test_tool_versions.sh`) takes `<binary> <min> <flag>`;
  output unchanged, so the four `LIVE R-TOOL-01:` labels still prefix one line each.
  `reject( )`/`accept( )` (`tests/test_repo_shape.sh`) take `<finder> <path> <content>`; the
  finder name is `report( )`'s label; each accept label became a `#` comment above its call.
  64 rejection and 23 accept cases preserved before the G2/G5 additions.
- **G4** — `missing_verify( )` (`tests/test_phase_docs.sh`) also reports a `done` phase whose
  `verify.md` lacks a `^#+ What was built` or a `^#+ .*[Cc]heck it` line. Three new rejection
  cases (one-line file, only-built heading, only-check heading) through `reject_verify( )`,
  counted against `3`; false-positive fixture now carries both headings. The three `done`
  phases pass unedited.
- **G5** — `src_files( )` is `find "$1/src" -type f | grep -E '\.(cpp|h|hpp|cc|inl)$'`; four
  rejection cases (`.h`, `.hpp`, `.cc`, `.inl`) under `find_err03`. Liveness harness: 69/69
  alternations, 35/35 neuterings caught.
- **Rule text** (`docs/constraints.md`) — R-STYLE-02, R-CLEAN-02, R-CLEAN-04 scope now
  `src/*.cpp`/`src/*.h`/`tests/*.cpp`, layer lists and `tests/test_style.sh:68` pins removed;
  R-CLEAN-02 drops the `reject( )`/`accept( )` instance, keeps shell/Python unmeasured;
  R-ERR-03 drops the silent-miss sentence, keeps the false-positive one; R-PROC-02 names the
  two headings and narrows its clause to "headings checked, prose not". Prose paragraph under
  R-STYLE-01/02 rewritten. Bindings unchanged.
- `docs/phases/13-scaffold-check-gaps/verify.md` — operator hand checks, no hardware.
- Every acceptance criterion in the spec run at the end, all at expected values;
  `make test`, `make lint`, traceability and liveness exit 0.

## Deviations

- **`reject_verify( )` counts its cases** (spec step 8 named the cases, not the helper). The
  first liveness run reported `neutering reject_verify is not caught` — a neutered helper just
  prints nothing. Added `verify_rejected` and a `-ne 3` FAIL line, same shape as
  `rejected`/`accepted` in `tests/test_repo_shape.sh`. In scope: `make test` would otherwise
  fail.
- **R-STYLE-02 / R-CLEAN-02 / R-CLEAN-04 now name the extensions they do not see**
  (`.hpp`, `.cc`, `.inl`, `.c`) where they used to name the layers — replacing "every other
  layer is unchecked" with nothing would have claimed coverage `tidy_sources()` does not have.
  Consistent with the tech-debt entry "File lists that stop at `.cpp` and `.h`", which remains
  open.
- **Liveness run once for steps 3, then once for steps 4 + 8 together**, not after each of 4
  and 8 separately — the harness takes ~2.5 min; the two steps touch different files.
- Spec pointers were sufficient; no file outside them was read or changed during
  implementation.
- **Operator-ordered after implementation (2026-09-23), outside the Plan:** (1) `PHASES.md`
  row `03-pio-bus` gained `13-scaffold-check-gaps` in its `depends` column, so
  `/expand-phase 03-pio-bus` reads this file. That bends `/plan-feature`'s rule, which
  allows `depends` edits only when a dependency is superseded; the operator chose it
  explicitly. (2) `docs/constraints.md` §Observed conventions gained the finding "An
  SDK-including file anywhere under `src/` fails `make lint`". Both are records for 03,
  not changes to this phase's checks.
- **Spec amended after validation round 1 (2026-09-23):** the review returned the
  §Observed conventions hunk in `docs/constraints.md` as `undecidable`. Plan step 6 now says
  to record the SDK-lint fact, mislabelled FAIL line included, as a finding there, and lists
  `docs/constraints.md` among the files it touches. Extended step 6's existing sentence
  instead of adding a step. Reconciliation: I checked the Context pointer for
  `docs/constraints.md` ("every rule-text edit lands here"). A finding is not a rule-text
  edit, so that pointer is still true and was left alone. No other spec statement asserts
  or contradicts the fact.
- **Operator-ordered after validation (2026-09-23), no gate logic changed:** `check_version( )`'s
  `cv_label` renamed `cv_name`. The `belay-debt:` header in `tests/test_repo_shape.sh` and
  R-ERR-03's clause now record a measured miss: `strip_line_comments( )` treats every `'` as a
  quote, so after a digit separator it can cut a line inside a string. With
  `int n = 1'000; u = "a'"; v = "http://x"; throw E;`, the `throw` goes unreported.

## Debt

- `strip_line_comments( )` does not understand raw strings (`R"(…)"`), `/* … */`, or
  line-continuation backslashes — ceiling recorded in the `belay-debt:` header comment of
  `tests/test_repo_shape.sh` and in R-ERR-03's clause; upgrade path is the `clang-query`
  upgrade `03-pio-bus` carries.

## For later phases

- **`03-pio-bus`**: the first `.cpp`/`.h` under `src/hal`, `src/usb`, `src/app` or `src/emu`
  that includes a Pico SDK or TinyUSB header now fails `make lint`. Measured 2026-09-23 with a
  one-line `src/hal/x.cpp` holding `#include "pico/stdlib.h"`:
  `'pico/stdlib.h' file not found [clang-diagnostic-error]`, printed under
  `FAIL: R-STYLE-02 / R-CLEAN-02 / R-CLEAN-04: naming, function-size or magic-number
  violations` — a misleading label. 03's spec must include a Plan step that adds the SDK
  and TinyUSB include flags to the clang-tidy invocation in `tests/test_style.sh`, with
  `make lint` green over its own `src/hal/` files as an acceptance criterion. Recorded
  where 03's expansion reads it: this row became a dependency of `03-pio-bus` in
  `PHASES.md`, and the fact is in `docs/constraints.md` §Observed conventions.
- **Tech-debt entries "Unwritten spelling and file-scope clauses…" and "File lists that stop
  at `.cpp` and `.h`"** both say "evaluate after 13 is `done`". Their premise holds as
  written: `src_files( )` is now the one-line `grep -E` alternation the second entry names,
  and `hits( )` now keeps string contents, so a spelling clause for R-ERR-03 must describe
  `strip_line_comments( )`. The operator declined folding either in (2026-09-23).
- `tests/test_phase_docs.sh` header still says "Today no phase is done" — stale since
  `00-scaffold` closed; comment only, the Goal does not touch it. needs a row: 00-scaffold —
  stale header comment in `tests/test_phase_docs.sh` (fold into the next `00-scaffold` fix row).
- Taste from the 2026-09-23 independent reviews (non-blocking): `mv_f` in `missing_verify( )`
  is a cryptic name.

## Validation — 2026-09-23
- criteria: 22 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: undecidable: `docs/constraints.md` §Observed conventions hunk (new finding "An SDK-including file anywhere under `src/` fails `make lint`…", removal of "None yet; …") — the spec's Plan never names §Observed conventions, nor says whether the FAIL-line mislabel is accepted
- closure test: fail: `docs/constraints.md` §Observed conventions edit unreachable from the spec (the review's undecidable finding)
- findings: 1
- spec size: 16523 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation

## Validation — 2026-09-23
- criteria: 22 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 16733 (+210 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
