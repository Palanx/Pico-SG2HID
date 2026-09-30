# Phase 22-scaffold-typecheck-transitive — notes

## Outcome

- `Makefile`: new `typecheck` target (and `CORE_HDRS := $(wildcard src/core/*.h)`, `.PHONY`,
  header comment). For each header it pipes `#include "<header>"` into
  `$(CXX) $(CXXFLAGS) -fsyntax-only -xc++ -`, keeps going after a failure, prints
  `typecheck: <header> does not compile on its own` per failing header and exits non-zero if
  any failed. Not wired into `make test` (`make -n test | grep -c fsyntax-only` → 0).
  All six current `src/core/*.h` pass.
- `.claude/workflow/toolchain.manual.json`: `"typecheck": "make typecheck"`.
  `scripts/check.sh typecheck` now prints `== typecheck: make typecheck` / `PASS`, no gap line.
- `tests/test_boundaries.sh`: `INCLUDE_HOOK` (overridable, defaults to
  `.claude/hooks/include-check.sh`) beside `HOOK`; the executable check covers both;
  `sweep( )` runs both hooks on every path under the same 0/2/other contract. Two new
  rejection cases: the transitive chain (core → unlayered header → hal) must return 1; an
  `INCLUDE_HOOK` stub exiting 3 on a clean tree must return 2. Floor 3 → 5.
  `INCLUDE_HOOK=/usr/bin/true` makes the chain case FAIL and the file exit 1 (step 5).
- `docs/constraints.md` R-ARCH-02: scope clause names both hooks, drops the transitive include
  from the misses, restates the no-layer miss (judged only through a layered includer via the
  reverse lookup), records the bash >= 4 requirement (re-measured this session: `/bin/bash`
  3.2.57 exits 0 on the chain fixture, bash 5.3 exits 2).
- `docs/phases/22-scaffold-typecheck-transitive/verify.md`: operator verification.

## Deviations

- Spec step 4 asked the "hook exited N" line to name which hook; it now prints the hook's path
  (`hook <path> exited N on <file>`). Not a result line, so the "keep every result line's
  wording" statement is unaffected.
- The chain case's FAIL line carries a hint `(one known cause: include-check.sh run by a bash
  older than 4 exits 0 here)` — not in the spec; added because the constraints text tells the
  reader that this is how a bash 3.2 environment shows up.
- `docs/constraints.md` R-ARCH-02: besides the spec's edits, "The hook still errs loudly on
  every conditional other than `#if 0`" became "The boundary hook …", since the clause now
  names two hooks and that sentence is about `boundary-check.sh` only. Checked the rest of the
  bullet for other bare "the hook" references: none remain ambiguous.
- No missing Context pointers: every file read or changed was named in the spec.
- Validation 2026-09-29 (independent review, undecidable): the spec said case (b)'s stub is
  "built as the existing `HOOK` stub case builds its stub", which the reviewer could not check
  from the spec and diff. That clause was deleted; the stub's requirement (exits 3) stands on its own.
  Reconciliation: no other spec sentence describes the stub.
- Validation 2026-09-29 (independent review, undecidable): R-ARCH-02 now says the reverse lookup works
  "by basename", which the spec did not state. The claim is true (`.claude/hooks/include-check.sh`
  header, line 21: "C-family files that include it by basename"), so the spec's step 4 now says
  "reverse lookup by basename, per its header comment". Reconciliation: Context pointers name the
  hook's header comment; no other spec sentence describes the lookup.
- Round 2, 2026-09-30 — finding "verify.md omits four acceptance-criteria commands" (contradicts,
  code-side). Treated as an instance of the property "every Acceptance criteria command appears
  in verify.md with its expected output" (Plan step 6). Enumerated all 12 criteria against
  verify.md: 8 present (`make typecheck`, the `bad.h` and `good.h` one-liners,
  `scripts/check.sh typecheck`, `sh tests/test_boundaries.sh`, the `INCLUDE_HOOK=/usr/bin/true`
  run, `make test`, `make lint`); 4 missing, exactly the reviewer's four. Added all four as
  verify.md steps 7 and 8; the old step 7 is now step 9. No spec change.
- Round 2: the floor comment in `tests/test_boundaries.sh` ("when a third case is added") was
  stale with a floor of 5; now "when another case is added". Comment only, from the reviewer's
  taste remarks.

## Debt

- None.

## For later phases

- `.claude/rules/tech-debt.md` "No typecheck gate configured": its first fix bullet is done
  (with the include-from-stdin form, not the header-as-main-file form the entry names). The
  entry's update is to be proposed to the operator after `/validate-phase` passes — not
  written by this phase (spec §Out of scope). The `make firmware` bullet stays open for
  `03-pio-bus`.
- `03-pio-bus`: `make typecheck` covers `src/core/*.h` only; SDK layers still rely on
  `make firmware`.
- Reviewer taste remarks (validation 2026-09-29), not blocking; all three fixed in round 2
  on the operator's order (2026-09-30): the stale floor comment; the "**Since phase 22
  (2026-09-29)**" prefix in R-ARCH-02 (dropped, the sentence's content kept); verify.md's "type
  `sh` first if your shell is zsh" line (deleted — the verify commands give the same results
  under zsh, measured 2026-09-30).
- Reviewer taste remarks (validation 2026-09-30), not blocking: the `hook <path> exited N`
  line would read better with a short name (`boundary`/`include`) than an absolute path;
  rejection case (a) checks only `sweep`'s return code, not that `run_all( )` prints the
  `FAIL: R-ARCH-02: forbidden dependency direction` line plus the include chain (Goal row 3);
  the R-ARCH-02 rewrite adds a positive sentence that lengthens the clause.

## Validation — 2026-09-29
- criteria: 12 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: contradicts (code-side): verify.md omits four acceptance-criteria commands (`grep -c 'common/y.h' docs/constraints.md`, `grep -n 'include-check.sh' docs/constraints.md`, `python3 tests/test_rule_traceability.py`, `make -n test | grep -c 'fsyntax-only'`) — spec Plan step 6 requires "the commands from the Acceptance criteria below with what each should print"; no Deviations entry records the omission | undecidable: case (b) stub "built as the existing HOOK stub case" (the pointed-to case is not in the diff) | undecidable: "reverse lookup by basename" in R-ARCH-02 is not stated in the spec
- closure test: fail: two undecidable findings (spec amended, see Deviations)
- findings: 3
- spec size: 12068 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation

## Validation — 2026-09-30
- criteria: 12 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean
- closure test: pass
- findings: 0
- spec size: 12068 (+0 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
