---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "Makefile"
  - ".claude/workflow/toolchain.manual.json"
  - "src/core/**"
  - "tools/trace_decode.py"
  - "src/hal/ps2_master.pio"
  - "tests/vectors/trace_session.rendered"
  - "tests/test_checks_are_live.py"
  - "tests/test_ps2_codec.py"
  - "tests/test_bus_frame.py"
  - "tests/test_bus_trace.py"
  - "tests/test_pin_table.py"
  - "tests/test_emulator.py"
  - "tests/test_trace_shift.py"
  - "tests/test_phase_docs.sh"
  - "tests/test_rule_traceability.py"
  - ".claude/commands/validate-phase.md"
---

# Tech debt log

## `SHIFT_US` is a hand copy of the PIO cycle budget (reviewed 2026-10-02)

Files: `tools/trace_decode.py`, `src/hal/ps2_master.pio`, `src/core/ps2_protocol.h`,
`tests/vectors/trace_session.rendered`

`tools/trace_decode.py` subtracts `SHIFT_US = 37` from each byte's elapsed time to get the `ack`
delay. The 37 is copied by hand from two sources: the cycle-budget comment in
`src/hal/ps2_master.pio` ("37 cycles per byte without the ACK wait") and the one cycle per µs
that `kBusClockHz = 250000` in `src/core/ps2_protocol.h` gives at 4 PIO cycles per bit. Nothing
ties these values together. If the PIO program gains or loses an instruction, or the bus clock
changes, every `ack` delay the decoder prints shifts silently, and `make test` stays green.
`trace_session.rendered` is computed from the same 37, so it moves with the copy and cannot
catch the drift.

Nothing breaks today. `ps2_master.pio` has not changed since 24-pio-bus, and 04-trace-mode's
acceptance criteria check it with `git diff --quiet main -- src/hal/ps2_master.pio`. The ATT→ACK
bench capture (firmware 51f1291, 2026-10-01) measured 36–38 µs per byte and `ack` delays of
0–1 µs, so 37 matches the hardware. The point where this stops holding is the first edit to
`ps2_master.pio` or to `kBusClockHz`.

Fixes, cheapest first:
- A host check, in `tests/test_bus_trace.py`, that reads the "So <n> cycles per byte"
  sentence from the `.pio` comment and `kBusClockHz` from the header, and asserts
  `SHIFT_US == n * 1e6 / (4 * kBusClockHz)`. It is a few lines, but it trusts the comment
  to match the instructions.
- Count the instructions on the no-ACK path of the `.pio` program itself. This is exact, but
  it is a small parser for PIO assembly.
- Re-measure on the bench whenever either file changes, reading the 1-byte frame's `us` in an
  ATT→ACK capture (`docs/phases/04-trace-mode/verify.md` explains how). This costs no code, but
  it relies on someone remembering to do it.

What already works: the bench `awk` criterion in `docs/phases/04-trace-mode/spec.md` counts
`ack` delays above 5 µs in a decoded ATT→ACK capture. A non-zero count means `SHIFT_US` is too
small. A count of zero does not prove `SHIFT_US` is not too large, because the delay is clamped
with `max(0, …)`.

Where it was found: 04-trace-mode, validation round 2. The independent reviewer could not
check 37 from the spec and the diff alone.

## Every mutant runs on every `make test` (reviewed 2026-10-08)

Files: `tests/test_checks_are_live.py`, `tests/test_ps2_codec.py`, `tests/test_bus_frame.py`,
`tests/test_bus_trace.py`, `tests/test_pin_table.py`, `tests/test_emulator.py`,
`tests/test_trace_shift.py`, `Makefile`, `tests/fixtures/`

The `Makefile` collects every `tests/test_*.py` into `PY_TESTS`, so `make test` runs two kinds of
mutation suite every time, whether or not anything they cover changed:
- `test_checks_are_live.py` generates a mutant for every check function (neutering) and every
  pattern alternative (alternation), writes each one to `tests/mut_*.sh`, and runs it.
- `test_ps2_codec.py`, `test_bus_frame.py`, `test_bus_trace.py`, `test_pin_table.py`,
  `test_emulator.py` and `test_trace_shift.py` each carry a hand-written `MUTATIONS` list.
  Each mutation is applied to a copy of the source, rebuilt and run.

Nothing about correctness breaks. The cost grows with every check, rule and mutation added, and
it is paid at every call: each Plan step check that runs `make test` (06-hil-digital's spec has
six), each `/validate-phase` round, and every manual run. One edited check re-runs every mutant
of every unrelated check. A SIGTERM mid-run also leaves `tests/mut_*.sh` orphans, because the
unlink is in a `finally` and `sweep_orphans( )` only runs at the next start. Not measured yet:
the wall time of `make test`, and the share of it that the mutation suites take. Measure both
before cutting anything.

Fix (the scope of a dedicated phase): run each mutant only when something it depends on changed.
- Keep a per-mutant result cache keyed on a content hash of the mutant's inputs. For
  `test_checks_are_live.py`: the mutated check text, every helper it sources, the fixtures its
  rejection cases read, and `test_checks_are_live.py` itself. For a `MUTATIONS` list: the
  mutated source file, the test file, and the build flags. A hit with a "killed" result skips the
  mutant; a miss runs it and records the result. Never cache a survivor.
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

## belay's gate carry-over never fires here (reviewed 2026-10-08)

Files: `.claude/commands/validate-phase.md`, `tests/test_phase_docs.sh`,
`tests/test_rule_traceability.py`, `tests/test_ps2_codec.py`

Since belay `7025e5b`, `/validate-phase` step 2 skips `scripts/check.sh` when the tree
fingerprint, which leaves out only the phase's `spec.md` and `notes.md`, matches the previous
passing round. The skip is guarded: if `git grep -lE 'spec\.md|notes\.md|docs/phases' -- ':!*.md'`
prints anything, the gates run anyway. Here it prints three files:
- `tests/test_phase_docs.sh`, which really reads `docs/phases/*/verify.md` and `PHASES.md`
  (R-PROC-02).
- `tests/test_rule_traceability.py`, which really reads `PHASES.md`.
- `tests/test_ps2_codec.py:121`, where the match is only a comment ("see spec.md §Out of scope").

So every round runs the full suite, mutants included. A round that only amended the spec pays
the same as one that changed code.

Nothing breaks: the guard errs toward running.

Fix, cheapest first:
- Reword the comment in `test_ps2_codec.py`. That removes the false hit but saves nothing,
  because the two real readers still trip the guard. They read `PHASES.md` and `verify.md`,
  never `spec.md` or `notes.md`, but belay has no way for a project to declare that. Do not hide
  the paths from the grep to get past the guard: the guard exists because such a gate may read
  the excluded files.
- The saving that is actually available is the mutant cache in the entry above. With it, a
  spec-only round re-runs the cheap tests and hits the cache for every mutant.
- If belay later lets a project declare which workflow files its gates read, declare these two
  gates and the carry-over starts firing. That is belay's to add, not this project's.

Where it was found: designing belay's carry-over against this repo's validation rounds.

