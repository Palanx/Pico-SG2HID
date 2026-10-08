# Phase 27-live-mutant-cache — cache killed liveness mutants between runs

<!-- Written by /expand-phase, immediately before implementation, never earlier
     (P4). Amendable during implementation ONLY together with a Deviations
     entry in notes.md. -->

## Goal

`tests/test_checks_are_live.py` (the harness) runs one subprocess per neutered and per
alternation mutant on every `make test`: 355.7 s of a 403 s run, measured 2026-10-08 at
`62b412a`. After this phase, a mutant that was killed by a rejection, accept, false-positive or
wiring case, and whose inputs are byte-identical since, is not run again — its "killed" result
is served from a per-clone cache.

The mutated check files are the six `tests/test_*.sh` files the harness mutates today:
`test_boundaries.sh`, `test_firmware_flags.sh`, `test_phase_docs.sh`, `test_repo_shape.sh`,
`test_secrets.sh`, `test_tool_versions.sh` (`test_style.sh` is `NO_MUTATE`; a file the
accounting property reports `unproven:` is not mutated at all, as today).

Observable behaviour after this phase:

- **Key.** Each mutant's key is a SHA-256 over: the harness file's text; the mutant's full text
  (which contains the whole check file, its inline helpers included — no check file sources
  another); and the content of the *named paths* of the original check file. A named path is a
  token in the check file's text matching `(\.claude|docs|scripts|src|tests|tools)/[A-Za-z0-9_./-]+`,
  with trailing `.` characters stripped, that exists under the repo root: a regular file
  contributes its relative path and content; a directory contributes every regular file under
  it, recursively, except basenames matching `mut_*.sh`. Tokens that name nothing are ignored.
  Paths enter the hash in sorted order.
- **What is cached.** A mutant is recorded as killed only when it exited non-zero **and** its
  output holds at least one `FAIL:` line whose text matches the harness's `CASE_LINE` pattern —
  that is, a case killed it. A mutant killed only by its real run's lines (which read repo
  content outside the key — e.g. a dropped exclusion alternative that now matches something in
  `src/`), or that exited non-zero with no case `FAIL:` line, is not recorded, and runs again
  next time. A survivor is never recorded.
- **Where.** One empty file per recorded key, named by the key's hex digest, in
  `build/live-mutant-cache/` (already covered by `.gitignore`'s `build/`). A hit is the file
  existing.
- **Always uncached:** property 1 (accounting) and `bootstrap( )`.
- **`FULL`.** The harness reads the environment variable `FULL`. Unset or empty: the cache is
  used. Exactly `1`: no lookup is made, every mutant of the six mutated check files runs, and
  kills are still recorded. Any other value: the harness prints one `FAIL:` line naming the value and exits non-zero before
  running properties 1–3. `make test FULL=1` passes it through.
- **Report.** In cached mode the harness prints, after `bootstrap( )`, one line per mutated
  check file: `  ok:   cache: <file> <hits>/<total> served from cache`, totals summed over
  neutering and alternation. With `FULL=1` it prints the single line
  `  ok:   cache: bypassed (FULL=1)` instead. A `FULL=1` survivor whose key was in the cache
  gets its normal survivor `FAIL:` line plus the words `(cached as killed: a key misses a
  dependency)`.

Known limit, accepted by the debt entry and stated here so the reviewer does not re-find it:
the key does not cover the versions of external tools on `PATH` (`gitleaks`, `bash`, the
compilers). `make test FULL=1` is the measurement of whether a key is complete.

## Context pointers

- `tests/test_checks_are_live.py` — the whole change lands here: `CASE_LINE`, `run( )`,
  `mutate_and_run( )` (today discards the mutant's output), `run_mutants( )`,
  `mutation_score( )`, `property_neutering( )`, `property_alternation( )`, `bootstrap( )`,
  `main( )`.
- `Makefile` — the `test` recipe's `PY_TESTS` loop; `FULL` must reach the harness's environment.
- `.gitignore` — `build/` already covers the cache directory; no edit expected.
- `.claude/rules/tech-debt.md` — the entry "Every mutant runs on every `make test`": the fix
  this phase narrows to the harness, and the 403 s / 355.7 s baseline.
- `docs/phases/PHASES.md` — the `## Feature: incremental liveness mutants` section paragraph:
  scope edge and non-goals.
- `tests/fixtures/incomplete_check.sh` — the bootstrap fixture; it has exactly one survivor,
  used by an acceptance criterion.
- `tests/test_repo_shape.sh` lines 114–125, 236–245, 436–438 — how a check prints real-run
  `FAIL:` lines versus case `FAIL:` lines; the distinction the "what is cached" rule reads.
- `docs/phases/26-trace-shift-derived/verify.md` — a recent operator guide for a host-only
  phase, the shape step 5's `verify.md` follows.

## Plan

1. **Keep the mutant's output.** `mutate_and_run( )` returns `(label, survived, output)`;
   `run_mutants( )` keeps returning survivors in submission order (bootstrap and the properties
   unchanged in behaviour). Add `killed_by_case( output )`: true iff `output` has a line matching
   `RESULT` with kind `FAIL` whose text matches `CASE_LINE`. — touches
   `tests/test_checks_are_live.py` — check:
   `python3 -c "import sys; sys.path.insert(0,'tests'); import test_checks_are_live as t; assert t.killed_by_case('  FAIL: find_x rejection case did not fire on: y'); assert not t.killed_by_case('  FAIL: R-ARCH-02: forbidden dependency direction'); assert not t.killed_by_case('  ok:   R-ARCH-02 rejection case')"`
   → exit 0.

2. **Key and cache.** Add `CACHE_DIR` (`<root>/build/live-mutant-cache`), `named_paths( text )`
   (the rule in Goal → Key, returning sorted repo-relative file paths), and
   `mutant_key( path, mutant )` (hex digest per Goal → Key; `named_paths` is applied to the
   original file at `path`). — touches `tests/test_checks_are_live.py` — check:
   `python3 -c "import sys; sys.path.insert(0,'tests'); import test_checks_are_live as t; p=t.named_paths(open('tests/test_boundaries.sh').read()); assert '.claude/hooks/include-check.sh' in p and 'docs/constraints.md' in p and p==sorted(p); assert t.mutant_key('tests/test_boundaries.sh','a')!=t.mutant_key('tests/test_boundaries.sh','b')"`
   → exit 0.

3. **Use the cache in the two properties.** `mutation_score( )` (the path neutering and
   alternation share; `bootstrap( )` calls `run_mutants( )` directly and stays uncached) skips
   jobs whose key file exists, runs the rest, records each run that is killed and
   `killed_by_case`, and accumulates per-file hit/total counts. Parse `FULL` per Goal → `FULL`
   at the top of `main( )`; print the cache lines per Goal → Report. — touches
   `tests/test_checks_are_live.py` — check: run
   `OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py` twice; both exit 0; the second
   prints one `cache:` line per mutated file and the per-file hits are not lower than the first
   run's.

4. **Pass `FULL` through `make`.** The `PY_TESTS` loop runs `FULL="$(FULL)" OPTIONAL_TOOLS=1
   python3 "$$t"`. — touches `Makefile` — check:
   `make -n test FULL=1 | grep -c 'FULL="1"'` → ≥ 1.

5. **Measure and teach.** Run `make test` once to fill the cache, then time a second
   unchanged run (`/usr/bin/time -p make test`); record the second run's `real` seconds, the
   cache lines it printed, and the 403 s baseline in `notes.md`. Write `verify.md` for the
   operator: what the cache is, where it lives, that deleting `build/` or running
   `make test FULL=1` is always safe, how to read the `cache:` lines, and when to run
   `FULL=1` (before closing a phase whose diff touches a tool on `PATH`). — touches
   `docs/phases/27-live-mutant-cache/notes.md`, `docs/phases/27-live-mutant-cache/verify.md` —
   check: `grep -c '403 s' docs/phases/27-live-mutant-cache/notes.md` → ≥ 1;
   `sh tests/test_phase_docs.sh` → exit 0.

## Acceptance criteria

```
rm -rf build/live-mutant-cache && make test 2>&1 | tail -n 1     # expect: OK (fills the cache)
make test > build/run2.log 2>&1; tail -n 1 build/run2.log         # expect: OK
grep -c 'cache: test_' build/run2.log                              # expect: >= 1
awk '/cache: test_/ { split( $4, a, "/" ); if ( a[1] != a[2] ) n++ } END { print n + 0 }' build/run2.log   # expect: 0 (every mutated file fully served)
grep -c '403 s' docs/phases/27-live-mutant-cache/notes.md         # expect: >= 1
cp tests/test_phase_docs.sh build/pd.bak && printf '\n# cache probe\n' >> tests/test_phase_docs.sh && OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py > build/probe.log; mv build/pd.bak tests/test_phase_docs.sh; awk '/cache: test_phase_docs.sh/ { split( $4, a, "/" ); print ( a[1] == 0 && a[2] > 0 ) }' build/probe.log   # expect: 1
awk '/cache: test_/ && !/test_phase_docs/ { split( $4, a, "/" ); if ( a[1] != a[2] ) n++ } END { print n + 0 }' build/probe.log   # expect: 0 (no other file re-ran)
cp docs/phases/PHASES.md build/ph.bak && printf '\n' >> docs/phases/PHASES.md && OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py > build/probe2.log; mv build/ph.bak docs/phases/PHASES.md; awk '/cache: test_phase_docs.sh/ { split( $4, a, "/" ); print ( a[1] == 0 && a[2] > 0 ) }' build/probe2.log   # expect: 1 (a named path changed, so that file's mutants re-ran)
python3 -c "import sys,tempfile,os; sys.path.insert(0,'tests'); import test_checks_are_live as t; t.CACHE_DIR=tempfile.mkdtemp(); f='tests/fixtures/incomplete_check.sh'; jobs,_=t.alternation_jobs(f); s=t.run_mutants(jobs); assert len(s)==1; m=[j for j in jobs if j[3]==s[0][1]][0]; t.mutation_score(jobs); assert not os.path.exists(os.path.join(t.CACHE_DIR,t.mutant_key(f,m[1])))"   # expect: exit 0 (the survivor is not recorded)
python3 -c "import sys; sys.path.insert(0,'tests'); import test_checks_are_live as t; assert t.killed_by_case('  FAIL: find_x rejection case did not fire on: y'); assert not t.killed_by_case('  FAIL: R-ARCH-02: forbidden dependency direction')"   # expect: exit 0 (a real-run-only kill is not cacheable)
FULL=1 OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py > build/full.log; echo rc=$?        # expect: rc=0
grep -c 'cache: bypassed (FULL=1)' build/full.log                 # expect: 1
grep -c 'served from cache' build/full.log                        # expect: 0
FULL=yes OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py; echo rc=$?                          # expect: one FAIL line naming yes, rc=1
make -n test FULL=1 | grep -c 'FULL="1"'                          # expect: >= 1
make test FULL=1 2>&1 | tail -n 1                                 # expect: OK
git check-ignore -q build/live-mutant-cache/x                     # expect: exit 0
git diff --quiet main -- tests/test_phase_docs.sh; echo rc=$?    # expect: rc=0 (the probe edit was restored)
ls tests/mut_*.sh tests/fixtures/mut_*.sh 2>/dev/null | wc -l    # expect: 0 (no mutant left behind)
make lint                                                         # expect: exit 0
make typecheck                                                    # expect: exit 0
python3 tests/test_rule_traceability.py                           # expect: exit 0
sh tests/test_phase_docs.sh                                       # expect: exit 0
```

## Out of scope

- Caching the five `MUTATIONS`-list suites (`test_ps2_codec.py`, `test_bus_frame.py`,
  `test_bus_trace.py`, `test_pin_table.py`, `test_emulator.py`) or `test_trace_shift.py` — no
  phase; the feature paragraph leaves them uncached.
- Putting tool versions or `PATH` into the key — no phase; `FULL=1` is the backstop.
- Changing `scripts/check.sh`, adding CI, or orphan-`mut_*.sh` handling beyond today's
  `sweep_orphans( )` — no phase; not wanted.
- Editing `.claude/rules/tech-debt.md` — the entry update is proposed after validation and
  waits for the operator.
- Editing the six mutated check files, `tests/test_style.sh`, `NO_MUTATE`, `CASE_LINE`, or the
  three properties' pass/fail criteria — no phase; the harness reaches the same verdicts, only
  faster.
