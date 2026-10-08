---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "Makefile"
  - ".claude/workflow/toolchain.manual.json"
  - "tests/test_checks_are_live.py"
  - "tests/test_ps2_codec.py"
  - "tests/test_bus_frame.py"
  - "tests/test_bus_trace.py"
  - "tests/test_pin_table.py"
  - "tests/test_emulator.py"
  - "tests/test_trace_shift.py"
---

# Tech debt log

## Every mutant runs on every `make test` (reviewed 2026-10-08)

Files: `tests/test_checks_are_live.py`, `tests/test_ps2_codec.py`, `tests/test_bus_frame.py`,
`tests/test_bus_trace.py`, `tests/test_pin_table.py`, `tests/test_emulator.py`,
`tests/test_trace_shift.py`, `Makefile`, `tests/fixtures/`

The `Makefile` collects every `tests/test_*.py` into `PY_TESTS`, so `make test` runs two kinds of
mutation suite every time, whether or not anything they cover changed:
- `test_checks_are_live.py` generates a mutant for every check function (neutering) and every
  pattern alternative (alternation), writes each one to `tests/mut_*.sh`, and runs it.
- `test_ps2_codec.py`, `test_bus_frame.py`, `test_bus_trace.py`, `test_pin_table.py` and
  `test_emulator.py` each carry a hand-written `MUTATIONS` list. Each mutation is applied to a
  copy of the source, rebuilt and run.
- `test_trace_shift.py` carries hand-written rejection and accept cases instead. They mutate
  four source texts in memory and re-run the check function: no copy on disk, no build, no
  subprocess.

Nothing about correctness breaks. The cost grows with every check, rule and mutation added, and
it is paid at every call: each Plan step check that runs `make test` (06-hil-digital's spec has
six), each `/validate-phase` round, and every manual run. One edited check re-runs every mutant
of every unrelated check. Measured 2026-10-08 at `62b412a` (warm build): `make test` takes
403 s, and `test_checks_are_live.py` alone 355.7 s (89 %); the five `MUTATIONS` suites together
about 20 s.

Fix (scheduled as `27-live-mutant-cache`, which narrows it to `test_checks_are_live.py`): run each mutant only when something it depends on changed.
- Keep a per-mutant result cache keyed on a content hash of the mutant's inputs. For
  `test_checks_are_live.py`: the mutated check text, every helper it sources, the fixtures its
  rejection cases read, and `test_checks_are_live.py` itself. For a `MUTATIONS` list: the
  mutated source file, the test file, and the build flags. `test_trace_shift.py`'s in-memory
  cases need no key: they cost no build and no subprocess. A hit with a "killed" result skips
  the mutant; a miss runs it and records the result. Never cache a survivor.
- The cache is per-clone and gitignored, never committed.
- Always run, uncached: property 1 (accounting), which reads the whole repo and is cheap, and
  `bootstrap( )`, the harness's own floor.
- `make test FULL=1`, and CI, ignore the cache. A full run that finds a survivor the cache
  recorded as killed means a key is missing a dependency. That makes the full run the
  measurement of whether the cache can be trusted.
- To confirm while expanding: whether a neutered or alternation mutant can be killed by the real
  run's output rather than by its rejection cases. If it can, its key also depends on the repo
  content that real run reads, and that dependency belongs in the key.

Cost: one cache layer shared by seven files, plus a key definition per mutation kind. A key
that misses a dependency trusts a stale "killed" between full runs.
