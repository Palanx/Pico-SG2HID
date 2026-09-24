# Phase 16-scaffold-spelling-clauses — notes

## Outcome

- `docs/constraints.md`: R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09 and R-ERR-04
  each gained a `**Scope, recorded 2026-09-24 because the check reads less than this text says:**`
  clause naming their finder, its scanner and file list, and the forms measured not to be
  reported. R-CLEAN-03 and R-CLEAN-05 also carry a `**File scope:**` part naming `src_files( )`,
  its five extensions, and that everything outside `src/` and `.c` under it is unchecked.
  R-ERR-03's 2026-09-23 clause was extended with **Its regex, measured 2026-09-24:** — one
  `**Scope, recorded` heading still. Bindings unchanged (all seven still `test: tests/test_repo_shape.sh`).
- `docs/phases/16-scaffold-spelling-clauses/verify.md`: no-hardware operator check, including a
  by-hand re-run of the `try` / `{` measurement.
- Measurement: 85 one-line fixtures, one fresh tree per fixture, against the finders extracted
  per spec §Plan "How to measure" into a scratchpad directory outside the repo, run with
  `PATH=/usr/bin:/bin` so `grep` is BSD `/usr/bin/grep`. Results (`missed` = not reported):
  - find_arch01 — missed: `# include "pico/stdlib.h"`, `"pico.h"`, `"RP2040.h"`, `<stdio.h>`,
    `<cstdio>`, `<cstdlib>`, `<chrono>`, `<atomic>`, `<functional>`, `HEADER_MACRO`.
    Reported: `"pico/stdlib.h"`, `"hardware/gpio.h"`, `<vector>`, `<iostream>`.
  - find_arch03 — missed: `delete[] p`, placement `new (buf) T`, `::operator new( 4 )`, `calloc`,
    `realloc`, `std::make_unique`, `std::make_shared`, `std::wstring`, `std::u8string`,
    `std::map`, `new` / `T;` split. Reported: `new T`, `new int`, `new T[ 4 ]`,
    `using std::vector; vector<int> v;`.
  - find_clean03 — missed: `bool flag{ true }`, `bool flag( true )`, `bool a, flag`, parameter,
    `bool check( )`, `std::atomic<bool>`, `auto flag = true`, bit-field,
    `bool is_ok = true; bool flag = false;` on one line (exclude pattern drops the line),
    `bool flag;` in `tests/x.cpp` and `src/core/x.c`. Reported: `bool flag;`, `bool flag = true;`,
    `const bool flag = true;`, `bool m_flag;`.
  - find_clean05 — missed: `// todo:`, `// FIXME:`, `// TODO():`, `// TODO(later):`, compliant
    `// TODO(09-guitar-observe):`, bare `TODO` in `tests/x.sh`, `tests/x.cpp`, `src/core/x.c`,
    `README.md`. Reported: `// TODO (09-guitar-observe):` (space), `/* TODO */`, `// TODO: fix`.
  - find_clean09 — missed: `final`, `alignas( 4 )`, `[[nodiscard]]` before the name, `struct A` /
    `: B` split, bare `override`, `enum class E : …`, `enum E : …`. Reported:
    `enum struct E : std::uint8_t` (loud false positive), `struct A : B`, `class A : public B`,
    template base, `struct A : ns::B`.
  - find_err03 — missed: `try` / `{` split (the known form), `catch` / `( ... )` split,
    `std::rethrow_exception`, `std::throw_with_nested`. Reported: `try {`, `void f( ) try {`,
    `catch(...)`.
  - find_err04 — missed: `r->value( )`, `r . value( )`, `.value` / `( )` split, `value_or`.
    Reported: `r.value( )`, `std::move( r ).value( )`, `opt.value( )` on `std::optional` (loud
    false positive), temporary `std::expected{…}.value( )`, `r.error( ).value( )`.
- Acceptance: every criterion in spec §Acceptance criteria returned its expected value;
  `make test` exit 0, `make lint` exit 0. Working tree after the run: only
  `docs/constraints.md`, `docs/phases/PHASES.md` and this phase's directory.

## Deviations

- The `README.md` fixture for find_clean05 and the extra reported-form fixtures (compliant
  forms, `const bool`, template base, etc.) go beyond the spec's minimum fixture set, which the
  spec permits ("plus any others the regex suggests"). Not written into the rule texts except
  where they help separate missed from reported.
- None otherwise. No file outside the spec's Context pointers was read or changed.

## Debt

- None added. The recorded gaps themselves are the debt; closing them is out of scope (spec
  §Out of scope) and owned by the `clang-query` upgrade in `03-pio-bus`.

## For later phases

- `03-pio-bus` (the `clang-query` upgrade): the per-finder missed lists above are a ready test
  corpus — each missed line is a rejection case the AST-based check should turn into a report.
- R-ERR-04 and R-CLEAN-09 each have a measured loud false positive (`std::optional::value( )`,
  `enum struct … : T`). Harmless today — neither form occurs under `src/` — but whoever first
  writes one will see a FAIL that the rule's words do not justify; the clause now says so.
- Independent review (validation 2026-09-24), taste only, not blocking:
  - `verify.md` step 3 nests backticks in `` ` — test: `tests/test_repo_shape.sh`` ` and renders broken.
  - `verify.md` "Why…" pairs R-CLEAN-03 with the bare-`TODO` example, which only fits R-CLEAN-05.
  - R-ARCH-01's clause restates the finder's prefix list; it goes stale if the regex changes and no
    test ties the two. (R-ERR-04's `value_or` was re-measured at validation: not reported, as stated.)

## Validation — 2026-09-24
- criteria: 14 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap (`workflow gap: no 'typecheck' tool configured — the project was NOT checked.`)
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 13342 (first)
- upstream: none
- not-ours: none
- verdict: done
- extra: every form named in the seven clauses (missed and reported) re-measured independently
  with the spec's "How to measure" extraction under `PATH=/usr/bin:/bin`; 61 fixtures, all matched
  the clause text.
