---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
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
on the next line is not seen by `find_err03`. Nothing breaks today: every check is live and
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
