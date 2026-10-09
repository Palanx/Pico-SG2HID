---
paths:
  - "Makefile"
  - "tests/test_ps2_codec.py"
  - "tests/test_bus_frame.py"
  - "tests/test_bus_trace.py"
  - "tests/test_pin_table.py"
  - "tests/test_emulator.py"
  - "tests/test_trace_shift.py"
  - "tests/test_rule_traceability.py"
  - "tests/test_checks_are_live.py"
  - "tests/test_repo_shape.sh"
  - "src/core/ps2_frame.cpp"
  - "src/core/link.cpp"
  - "tests/ps2_codec_cases.cpp"
  - "docs/constraints.md"
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

## `test_rule_traceability.py`'s own logic is never mutated (reviewed 2026-10-08)

Files: `tests/test_rule_traceability.py`, `tests/test_checks_are_live.py`

The liveness harness mutates `.sh` check files only (`property_neutering( )` and
`property_alternation( )` skip every name that does not end in `.sh`); its `belay-debt:`
docstring points here. The upgrade was once owned by `01-ps2-codec`, triggered by "a rule whose
check is not a grep". That trigger has fired since (R-PROTO-02..09 and the pin-table and emulator
rules are checked by `.py` files), and on 2026-09-14 `01-ps2-codec` explicitly released the debt to
no phase (its `notes.md` §Debt).

The gap is narrow. Of the eight `tests/test_*.py` files, six prove
themselves live by mutating the code they check. `test_ps2_codec.py`, `test_bus_frame.py`,
`test_bus_trace.py`, `test_pin_table.py` and `test_emulator.py` do it through their `MUTATIONS`
lists, and `test_trace_shift.py` through in-memory rejection cases. One is the harness itself.
That leaves `test_rule_traceability.py` (R-PROC-01, nine functions) as the only check whose
internals nothing mutates.

Nothing breaks today. Each of its failure modes has a rejection case on a generated fixture repo
(`CASES`, 13 entries, a `floor 13` the file enforces). A wiring case proves through `run_all( )`
that a reported problem reaches the exit code. The harness's accounting property proves the real
run reports R-PROC-01. The file last changed on 2026-09-24 (`f2d82ad`). What is missing is the
stronger form: a dropped branch inside `check( )` that no case exercises would not be caught,
which is exactly the gap neutering and alternation close for the `.sh` checks.

Fix, cheapest first:
- Give `test_rule_traceability.py` a hand-written in-memory mutation list over its own `check( )`
  source, the `test_trace_shift.py` shape: no build, no subprocess. It costs one list to maintain
  by hand, and it covers what the list names, not what it forgets.
- Teach the harness to neuter Python functions (stdlib `ast`: replace a `def`'s body with
  `return` of an empty value) for `.py` checks without a `MUTATIONS` list. This is derived rather
  than listed, like the `.sh` properties, and its mutants go through the
  `27-live-mutant-cache` cache. It costs a second extractor in the harness, and a first run where
  the neutered helpers that no case reaches will surface as survivors to triage.

Where it was found: the debt sweep after `27-live-mutant-cache` closed, 2026-10-08.

## Grep checks have no type information (reviewed 2026-10-08)

Files: `tests/test_repo_shape.sh`, `docs/constraints.md` (R-ERR-01, R-ERR-02)

`tests/test_repo_shape.sh` checks C++ with line-anchored `grep`s, after stripping `//` comments.
Its two `belay-debt:` comments and the scope clauses of R-ERR-01 and R-ERR-02 record what that
misses:
- block comments and string literals are never stripped, which produces false positives;
- `'` read as a quote can hide code after a `//` inside a string, which produces a miss;
- a return type spelled other than at the start of the line (`static`, qualified, trailing
  `->`, on its own line) is not seen;
- a fallible return other than the three recognised spellings is not seen (`id_from_byte`
  returns `std::optional` and is not reached);
- a function declared only in an anonymous namespace is outside R-ERR-02.

All of these named `03-pio-bus` as owner. That phase was superseded by `23-firmware-build` and
`24-pio-bus`, and both closed saying the upgrade is still unowned (23's and 24's `notes.md`).

Nothing breaks today. The constraints text measured each blind form one fixture at a time, and none
of them occurs in `src/core/` now. The false positives fail loudly instead of passing quietly. The
risk is a future `src/core/` edit written in one of the unseen forms.

The input the fix needs now exists. `make firmware` writes `build/pico/compile_commands.json` (since
`23-firmware-build`), and `clang-query` ships with the Homebrew LLVM that `make lint` already
requires (`/opt/homebrew/opt/llvm/bin/clang-query`).

Fix, cheapest first:
- Leave the greps and keep the scope clauses. This costs nothing, and the clauses already state
  every gap.
- Replace R-ERR-01 and R-ERR-02's greps with `clang-query` matchers over the host compile of
  `src/core/` (`make test` compiles it, but no database is written for it). This costs a host
  compile database or explicit flags, a new tool in `make test` (R-TOOL-01 floor, `skip:` when
  absent), and rewriting their rejection cases and scope clauses. It buys type-aware matching,
  which closes the last three gaps listed above.
- Port every `test_repo_shape.sh` finder to `clang-query`. This is the same cost multiplied by
  every rule, and it also closes the comment and string gaps. It is a phase, through
  `/plan-feature`.

## Test drivers copy the `Makefile`'s `CXXFLAGS` by hand (reviewed 2026-10-08)

Files: `tests/test_ps2_codec.py`, `tests/test_bus_frame.py`, `tests/test_bus_trace.py`,
`tests/test_pin_table.py`, `tests/test_emulator.py`, `Makefile`

Each of the five drivers builds its mutated copies with its own `CXXFLAGS` list. That list is a hand
copy of the `Makefile`'s `CXXFLAGS ?= -std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc`.
Nothing ties the copies to the original. If the `Makefile` gains a flag, the drivers keep building
their mutants without it.

Nothing breaks today: all five lists equal the `Makefile`'s line (checked 2026-10-08). The
consequence of drift is mild. A mutant is compiled under different warnings than the real build,
so a mutation that only a new flag would reject passes in the driver. `04-trace-mode` and
`24-pio-bus` recorded this and did not schedule it.

Fix, cheapest first:
- One shared constant in a small `tests/` module that the five drivers import. This costs five
  edits, and the copy still exists, just once.
- Ask `make` for the value (`make -s print-CXXFLAGS` with a one-line target). This costs one
  `Makefile` target and a subprocess per driver, and it removes the copy. It is also the
  "build flags" input the `MUTATIONS`-cache fix above would hash.

## Two `decode` / `step` contracts are specified and asserted by nothing (reviewed 2026-10-08)

Files: `src/core/ps2_frame.cpp`, `src/core/link.cpp`, `tests/ps2_codec_cases.cpp`

`01-ps2-codec` left two contracts with no case. It named `03-pio-bus` as the owner, and that phase
was superseded without taking them.
- `decode( )` in `src/core/ps2_frame.cpp` accepts a buffer longer than the frame and ignores the
  extra bytes. No case passes a longer buffer. The obstacle was R-PROTO-05: expected bytes must be
  literals under `tests/vectors/`, so the input would be a vector plus padding while the expected
  bytes still come from the vector.
- `add_saturating( )` in `src/core/link.cpp` makes `us_in_state` saturate at `UINT32_MAX` instead
  of wrapping. No case reaches the boundary, which needs a caller that stops polling for over
  71.6 minutes.

Nothing breaks today. The 06-hil-digital poller always hands `decode( )` the exact frame length,
and no caller pauses for 71 minutes. A wrap would turn a stuck `Negotiating` link into one that
times out 71 minutes late, not never.

Fix: two cases in `tests/ps2_codec_cases.cpp`. The first is a vector plus one padding byte,
expecting the vector's frame. The second calls `step` twice with `elapsed_us = UINT32_MAX` and
asserts `us_in_state` did not roll over. Cheap, with no new test shape beyond the padding.

## `make test` runs its 16 test files one after another (reviewed 2026-10-08)

Files: `Makefile`, `tests/test_checks_are_live.py`

The `test` recipe walks `$(CPP_BINS)`, `$(SH_TESTS)` and `$(PY_TESTS)` in three serial `for`
loops. Measured 2026-10-08 at `853a8a3`, with a warm mutant cache: `make test` takes 55.4 s. Of
that, 11.3 s is the liveness harness, which already runs its accounting runs and mutants in
parallel. The other ~44 s is the remaining files back to back: `test_repo_shape.sh` 7.4 s,
`test_style.sh` 5.6 s, `test_ps2_codec.py` 5.5 s, `test_bus_frame.py` 5.0 s, then the rest. Run
in parallel, the floor would be about the slowest file plus the harness.

Nothing breaks: this is only wall-clock time. The operator expects it may never be worth doing,
because a warm run is under a minute.

What already works: every check is safe to run alongside the others. That was confirmed
2026-10-08 when the harness's accounting runs were parallelised. Each check works in a temp
directory outside the tree, the `.py` suites' tree copies skip `.git`, `build` and `mut_*`, and no
test imports a repo module during `make test`, so none writes `__pycache__` into the tree.

Fix, cheapest first:
- Background each file in the recipe, write its output to `build/test-logs/<name>.log`, `wait`,
  then print the logs in the current order and fold their exit codes. This costs a longer recipe
  and per-file exit-code bookkeeping in POSIX `sh`, and it keeps the `--- <file>` output order
  people read.
- One phony target per test file under `make -j`. This needs `--output-sync` to keep each file's
  output together. That flag arrived in GNU Make 4.0, and macOS ships 3.81 (`/usr/bin/make`), so
  this costs a Homebrew `gmake` dependency, which `make test`'s "a C++23 compiler, `python3`,
  bash >= 4" promise does not allow today.
- Either way: the harness re-runs every check itself, so running it alongside them doubles the
  CPU load at peak. Start the harness first, or last on its own.
