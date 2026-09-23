---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "tests/test_style.sh"
---

# Tech debt log

## Unwritten spelling and file-scope clauses for seven grep-bound rules (reviewed 2026-09-23)

Files: `tests/test_repo_shape.sh` (`find_arch01`, `find_arch03`, `find_clean03`, `find_clean05`,
`find_clean09`, `find_err03`, `find_err04`, `hits( )`, `raw_hits( )`, `src_files( )`,
`core_files( )`), `docs/constraints.md` (R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05,
R-CLEAN-09, R-ERR-03, R-ERR-04)

Every `find_*` regex has spellings it does not match, so every grep-bound rule owes a **spelling
clause** in its own text naming them (`docs/phases/01-ps2-codec/spec.md` §What a `test:` binding
does and does not promise). Seven `00-scaffold` rules carry none: R-ARCH-01, R-ARCH-03,
R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-ERR-03, R-ERR-04. R-CLEAN-03 and R-CLEAN-05 also owe a
**file-scope clause**: their text is unscoped while the check reads `src_files( )` only. None of
the missed forms has been measured, so no specific form is claimed; one is known — `try` with `{`
on the next line is not seen by `find_err03`. A second is measured for R-ERR-03 and already
written into its clause (2026-09-23): a `'` outside a character literal (digit separator
`1'000`) can make `strip_line_comments( )` cut a line inside a string and hide the code after
it. Nothing breaks today: every check is live and
green over the current tree; the rules only assert full coverage they do not have.

Deliberately deferred, not dropped: `13-scaffold-check-gaps` changes `hits( )` (string-literal
handling) and `src_files( )` (adds `.hpp`, `.cc`, `.inl`), which back all seven. A clause written
before 13 lands describes a scanner and a file list that stop existing. **Evaluate after 13 is
`done`**: re-read the finders as 13 left them, then schedule a clause-only row via
`/plan-feature` ("Scheduling a fix to a phase already done"), shaped like
`12-scaffold-scope-clauses`.

Fix, with its cost: documentation only, no check changes. Per rule, extract the finder into a
scratch directory, feed it one fixture line per spelling, write what it misses into the rule's
text as a `**Scope, recorded <date> because the check reads less than this text says:**` clause.
Phase 12 took one session for four clauses; seven spelling plus two file-scope clauses is one
full phase.

What already works: the measurement method is round 28 in `docs/phases/01-ps2-codec/notes.md`;
the rule → finder → list/scanner assignment is recomputed with the four greps under
§The assignment, measured against the files rather than recalled in
`docs/phases/01-ps2-codec/spec.md`. Phase 12's Plan step 4 extracted `hits( )` by line range
instead of sourcing the script, because sourcing `tests/test_repo_shape.sh` runs its suite.

Where it was found: the round-15 and round-28 audits of `01-ps2-codec`
(`docs/phases/01-ps2-codec/notes.md` §For later phases), carried forward by
`12-scaffold-scope-clauses` and left out of `13-scaffold-check-gaps` on 2026-09-23 because 13
changes the checks these clauses would describe.

## File lists that stop at `.cpp` and `.h` (reviewed 2026-09-23)

Files: `tests/test_repo_shape.sh` (`core_files( )`, `core_headers( )`), `tests/test_style.sh`
(`tidy_sources()`, `sources()`), `docs/constraints.md` (R-ARCH-01, R-CLEAN-09, R-ERR-01,
R-ERR-02, R-STYLE-01, R-STYLE-02, R-CLEAN-02, R-CLEAN-04)

`13-scaffold-check-gaps` widens `src_files( )` to `.cpp`, `.h`, `.hpp`, `.cc` and `.inl`, and
leaves the other four file lists alone because its row names only `src_files( )`.
`core_files( )` still lists `.cpp`/`.h` and `core_headers( )` `.h` only, so a `.hpp`, `.cc` or
`.inl` under `src/core/` is unseen by R-ARCH-01, R-CLEAN-09, R-ERR-01 and R-ERR-02.
`tidy_sources()` (R-STYLE-02, R-CLEAN-02, R-CLEAN-04) and `sources()` (R-STYLE-01) hand
clang-tidy and clang-format `.cpp`/`.h` only. Nothing breaks today: every C++ file in the
repo is `.cpp` or `.h`. It stops being true the first time a file with another extension is
added — the Pico SDK and TinyUSB examples `03-pio-bus` onward will copy from use `.c`/`.h`,
but nothing forbids `.hpp`. No rule text states these file lists, so the rules assert
coverage they do not have.

Fix, cheapest first: (a) a rule that allows only `.cpp` and `.h` under `src/`, checked by one
`find` — one rejection case, and every other list becomes correct by definition; (b) widen
the four lists to match `src_files( )`, with a rejection case per extension per finder (the
liveness harness demands one per alternation branch) and a clang-format/clang-tidy probe per
extension — roughly one session; (c) a scope clause per rule, documentation only.

Deliberately deferred, not dropped: **evaluate after 13 is `done`**, together with the
spelling-clause entry above — 13 changes `src_files( )` into the one-line `grep -E`
alternation shape these lists would copy.

What already works: `13-scaffold-check-gaps` spec Plan step 3 is the pattern for (b) — a
one-line body with the extensions as one `grep -E` alternation, so
`tests/test_checks_are_live.py` generates a mutant per extension; it also found that `.h`
needs its own rejection case because no `src_files( )`-backed case planted one.

Where it was found: expanding `13-scaffold-check-gaps` on 2026-09-23 (spec §Out of scope).
