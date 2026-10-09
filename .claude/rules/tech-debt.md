---
paths:
  - "tests/test_rule_traceability.py"
  - "tests/test_checks_are_live.py"
  - "docs/phases/*/notes.md"
---

# Tech debt log

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

## belay's step-5 reviewer is starved by instruction only (reviewed 2026-10-09)

Files: `docs/phases/*/notes.md` (the `- independent review:` line of every `## Validation` block)

`/validate-phase` step 5 hands its reviewer four inputs and tells it to use nothing else. The
subagent still has the session's tools and can read the whole repo. In `07-analog-mode` round 2
the reviewer ran a `diff -q` of its `CLAUDE.md` copy against the repo's, outside those inputs; it
recorded the call on the review line and did not use it.

Nothing breaks while the reviewer keeps to its inputs, or reports when it does not. A reviewer
that reads the rest of `notes.md` or a dependency's notes judges the diff against what the session
meant, which is the bias the starvation exists to remove.

When a `- independent review:` line, or anything else in a validation round, shows the reviewer
reading outside its four inputs: run `/belay-feedback` with that line, and say whether the outside
read fed a verdict. The fix is belay's (restrict the reviewer's tools, or have the review line
record every outside read), not this project's.
