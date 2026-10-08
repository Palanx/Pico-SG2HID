---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "Makefile"
  - ".claude/workflow/toolchain.manual.json"
  - "tests/test_ps2_codec.py"
  - "tests/test_bus_frame.py"
  - "tests/test_bus_trace.py"
  - "tests/test_pin_table.py"
  - "tests/test_emulator.py"
  - "tests/test_trace_shift.py"
---

# Tech debt log

## Every `MUTATIONS`-list mutant runs on every `make test` (reviewed 2026-10-08)

Files: `tests/test_ps2_codec.py`, `tests/test_bus_frame.py`, `tests/test_bus_trace.py`,
`tests/test_pin_table.py`, `tests/test_emulator.py`, `tests/test_trace_shift.py`, `Makefile`

The `Makefile` collects every `tests/test_*.py` into `PY_TESTS`, so `make test` runs these
mutation suites every time, whether or not anything they cover changed:
- `test_ps2_codec.py`, `test_bus_frame.py`, `test_bus_trace.py`, `test_pin_table.py` and
  `test_emulator.py` each carry a hand-written `MUTATIONS` list. Each mutation is applied to a
  copy of the source, rebuilt and run.
- `test_trace_shift.py` carries hand-written rejection and accept cases instead. They mutate
  four source texts in memory and re-run the check function: no copy on disk, no build, no
  subprocess.

Nothing about correctness breaks. The cost grows with every mutation added, and it is paid at
every call. Measured 2026-10-08 at `62b412a` (warm build): the five `MUTATIONS` suites together
take about 20 s of a 403 s `make test`. The liveness harness (`tests/test_checks_are_live.py`),
355.7 s of that run, is no longer part of this entry: `27-live-mutant-cache` caches its killed
mutants, and a second unchanged `make test` measured 90.75 s on 2026-10-08.

Fix, if the 20 s ever matters: reuse that phase's approach. Keep a per-mutation cache of killed
results keyed on a content hash of the mutated source file, the test file and the build flags;
never cache a survivor; keep it under the gitignored `build/`; let `make test FULL=1` bypass it.
`test_trace_shift.py`'s in-memory cases need no key: they cost no build and no subprocess.

Cost: a key definition per suite, and a key that misses a dependency trusts a stale "killed"
between full runs.
