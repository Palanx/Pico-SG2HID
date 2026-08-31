# Phase 00-scaffold — Make every rule that needs no product code real

## Goal

After this phase, `make test` runs a suite that turns fourteen rules currently marked
`planned:` into rules with deterministic tests, and the rule catalogue can no longer
drift from its tests without the build failing.

Concretely, what exists afterwards that does not exist now:

- Six test files under `tests/` that check the *shape of the repository* — layer
  boundaries, `core` purity, no heap, no exceptions, boolean naming, TODO references, no
  inheritance in `core`, no `.value()` on `std::expected`, test-vector provenance,
  per-phase `verify.md`, absence of secrets, and minimum tool versions.
- `tests/test_rule_traceability.py`, which parses `docs/constraints.md` §Invariants under
  ADR-0005's grammar and fails on any drift in either direction: a `test:` path that does
  not exist or lacks its `RULE <id>` marker, a marker naming an undeclared id, a `manual:`
  rule that has a marker anyway, a `planned:` phase that is missing or already `done`, a
  duplicate id, an unparsable rule line, or an id `CLAUDE.md` mentions that the catalogue
  does not declare.
- **A negative self-test on every check.** This is the load-bearing part of the phase and
  the reason it is not just fourteen greps. Almost every check here passes vacuously today —
  there is no code under `src/` yet — so a check that is silently broken is
  indistinguishable from a check that is passing. Each check therefore builds a
  deliberately-bad tree in a temp directory and asserts that the check *rejects* it. A
  check without its rejection case is not finished.
- Fourteen rules in `docs/constraints.md` moved from `planned: 00-scaffold` to
  `test: <path>`, each test file carrying the matching `RULE <id>` marker comment.
- `docs/phases/00-scaffold/verify.md`, the operator-facing procedure (R-PROC-02).

Observable behaviour: `make test` exits 0 on the clean repo, and exits non-zero — naming
the rule and the offending path — when any of the fourteen violations is introduced.

## Context pointers

- `CLAUDE.md` — the session contract; §"Rules are bound to tests" states the binding this phase implements.
- `docs/constraints.md` — the rule catalogue. §Invariants is the file this phase's meta-test parses, and the eleven `planned: 00-scaffold` lines are the work list. The grammar is in the section preamble.
- `docs/adr/0005-rule-test-traceability.md` — why the binding exists and the exact three-binding grammar, including what each failure mode must report.
- `docs/adr/0002-hardware-free-core.md` — the layering this phase's boundary and purity checks enforce, and why `core` must not reach the SDK.
- `docs/adr/0007-error-model.md` — why `-fno-exceptions` is a rule and not a preference (R-ERR-03); its decoding half is superseded by ADR-0009.
- `docs/adr/0008-cpp23.md` — the C++23 decision, the measurements behind it, and the two-toolchain `PATH` trap R-TOOL-02 exists to catch.
- `docs/adr/0009-std-expected.md` — why `.value()` is forbidden and must be a grep (R-ERR-04), not a comment.
- `.claude/workflow/boundaries.rules` — the machine-readable layer edges; `tests/test_boundaries.sh` drives the existing hook with this file, it does not reimplement it.
- `.claude/hooks/boundary-check.sh` — takes one file path, exits 2 on a violation. Read its interface before wrapping it; it is a per-file gate, so the sweep is the wrapper's job.
- `tests/test_style.sh` — the pattern every new check follows: `RULE` markers in the header, `skip_or_fail` for a missing external tool, one `fail` accumulator, a `FAIL:` line naming the rule. Copy its shape.
- `Makefile` — `test` globs `tests/test_*.cpp|.sh|.py`; new files are picked up with no edit. The `test` target is where `-UNDEBUG` and the optional-tools env var are set.
- `docs/phases/PHASES.md` — the phase table the traceability test reads to validate `planned:` targets, and whose `status` column drives R-PROC-02.
- `docs/templates/notes.md` — the shape of the `notes.md` this phase must leave behind.

No dependency phases, so there are no `notes.md` to absorb.

## Plan

1. **Generalise the optional-tool flag** — touches `tests/test_style.sh`, `Makefile`.
   Rename `STYLE_OPTIONAL` to `OPTIONAL_TOOLS` so one flag covers every check that needs a
   tool outside the C++23 + `python3` floor (R-PROC-04). `-UNDEBUG` already landed in
   `CXXFLAGS` with the ADR-0008 change, so `assert` cannot be compiled out; verify it is
   still there rather than adding it again.
   — check: `make test` → exit 0, still prints the `ok:` lines from `test_style.sh`;
   `grep -c STYLE_OPTIONAL Makefile tests/test_style.sh` → `0` in both;
   `grep -c UNDEBUG Makefile` → `1`.

2. **Write `tests/test_rule_traceability.py`** — touches `tests/test_rule_traceability.py`.
   Parse `docs/constraints.md` with the ADR-0005 grammar, cross-check against
   `docs/phases/PHASES.md` and `CLAUDE.md`, and walk the repo for `RULE <id>` markers
   (skipping `.git/`, `build/`, `docs/`, and `.claude/commands/`). Each of the eight
   failure modes listed in the Goal reports its own message naming the rule id and the
   file. Pure `python3`, no third-party imports.
   — check: `python3 tests/test_rule_traceability.py` → exit 0 on the current tree.

3. **Give step 2 its rejection cases** — touches `tests/test_rule_traceability.py`. For
   each of the eight failure modes, build a temp copy of the catalogue with exactly that
   defect and assert the parser reports it. The parser must therefore take the paths it
   reads as parameters, not hardcode them.
   — check: `python3 tests/test_rule_traceability.py` → exit 0, and its output states how
   many rejection cases ran (one per failure mode, so at least 8).

4. **Write `tests/test_boundaries.sh`** (R-ARCH-02) — touches `tests/test_boundaries.sh`.
   Run `.claude/hooks/boundary-check.sh` over every file under `src/`; any exit 2 fails
   the sweep. Then the rejection case: write a temp `src/core/x.cpp` that includes
   `"hal/bus.h"` and assert the hook rejects it.
   — check: `sh tests/test_boundaries.sh` → exit 0, output confirms the rejection case ran.

5. **Write `tests/test_repo_shape.sh`** (R-ARCH-01, R-ARCH-03, R-ERR-03, R-ERR-04,
   R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-PROTO-05) — touches `tests/test_repo_shape.sh`.
   Eight greps
   over a *root passed as an argument*, so the same function serves the real tree and the
   temp bad tree. One `RULE` marker per rule in the header, one `FAIL:` line per rule
   naming the offending path, and one rejection case per rule.
   — check: `sh tests/test_repo_shape.sh` → exit 0 with eight `ok:` lines and eight
   rejection cases confirmed.

6. **Write `tests/test_phase_docs.sh`** (R-PROC-02) — touches `tests/test_phase_docs.sh`.
   For every row in `docs/phases/PHASES.md` whose status is `done`, assert
   `docs/phases/<id>/verify.md` exists and is non-empty. Rejection case: a temp phase table
   with a `done` row and no `verify.md`.
   — check: `sh tests/test_phase_docs.sh` → exit 0 (no `done` phases yet, and the rejection
   case proves that is not why it passed).

7. **Write `tests/test_secrets.sh`** (R-SEC-01) — touches `tests/test_secrets.sh`. Run
   `gitleaks` over the working tree; missing `gitleaks` follows the `OPTIONAL_TOOLS`
   convention from step 1. Rejection case: a temp file holding a non-allowlisted fake
   token — **not** the AWS documentation example key `AKIAIOSFODNN7EXAMPLE`, which gitleaks
   allowlists and which will make a broken check look like a passing one.
   — check: `sh tests/test_secrets.sh` → exit 0 and confirms the rejection case ran.

8. **Write `tests/test_tool_versions.sh`** (R-TOOL-01, R-TOOL-02) — touches
   `tests/test_tool_versions.sh`. For each of `clang-format`, `clang-tidy`,
   `arm-none-eabi-g++` and `python3`: if absent, skip under `OPTIONAL_TOOLS`; if present,
   parse its version and fail below the floor in R-TOOL-01. Then R-TOOL-02: compile a
   two-line TU that includes `<cstdint>` for `-mcpu=cortex-m0plus -mthumb` with whichever
   `arm-none-eabi-g++` is first on `PATH`, and fail if it cannot — that is the exact trap
   ADR-0008 §Context records, and a version number alone does not catch it. Rejection case:
   assert the version parser rejects a stubbed-out `--version` reporting an old release.
   — check: `sh tests/test_tool_versions.sh` → exit 0 on this machine, naming each tool and
   the version it found.

9. **Flip the fourteen rules and add the markers** — touches `docs/constraints.md` and the
   six new test files. Each `planned: 00-scaffold` becomes ``test: `tests/<file>` `` and
   each test file carries `# RULE <id> — docs/constraints.md §Invariants — <one line>`.
   — check: `python3 tests/test_rule_traceability.py` → exit 0, and
   `grep -c 'planned: 00-scaffold' docs/constraints.md` → `0`.

10. **Write `docs/phases/00-scaffold/verify.md` and `notes.md`** — touches both files.
   `verify.md` is for a non-specialist: what was built, why a test that passes on an empty
   repo is worth anything, and a hands-on procedure — break one rule on purpose, watch
   `make test` name it, put it back. `notes.md` records deviations and any debt taken.
   — check: `sh tests/test_phase_docs.sh` → exit 0; both files exist and `verify.md` names
   at least one rule the operator can break by hand.

## Acceptance criteria

```
make test                                          # expect: exit 0, "OK"
make lint                                          # expect: exit 0
python3 tests/test_rule_traceability.py            # expect: exit 0, reports >= 8 rejection cases
sh tests/test_boundaries.sh                        # expect: exit 0, rejection case confirmed
sh tests/test_repo_shape.sh                        # expect: exit 0, 8 rules ok, 8 rejection cases
sh tests/test_phase_docs.sh                        # expect: exit 0, rejection case confirmed
sh tests/test_secrets.sh                           # expect: exit 0, rejection case confirmed
sh tests/test_tool_versions.sh                     # expect: exit 0, names each tool and its version
grep -c 'planned: 00-scaffold' docs/constraints.md # expect: 0
grep -c 'STYLE_OPTIONAL' Makefile tests/test_style.sh   # expect: 0 in both files
test -s docs/phases/00-scaffold/verify.md          # expect: exit 0
test -s docs/phases/00-scaffold/notes.md           # expect: exit 0
```

Adversarial check — run by hand, and the point of the phase. Each of these must make
`make test` exit non-zero **and name the rule id**, and the tree must be restored after:

```
mkdir -p src/core   # git does not track empty directories: absent in a fresh clone
printf '#include "pico/stdlib.h"\n' > src/core/x.h            # expect: FAIL naming R-ARCH-01
printf '#include "hal/bus.h"\n'     > src/core/x.h            # expect: FAIL naming R-ARCH-02
printf 'auto* p = new int;\n'       > src/core/x.cpp          # expect: FAIL naming R-ARCH-03
printf 'try { } catch ( ... ) { }\n'> src/core/x.cpp          # expect: FAIL naming R-ERR-03
printf 'bool flag = true;\n'        > src/core/x.cpp          # expect: FAIL naming R-CLEAN-03
printf '// TODO: fix\n'             > src/core/x.cpp          # expect: FAIL naming R-CLEAN-05
printf 'struct A : public B { };\n' > src/core/x.h            # expect: FAIL naming R-CLEAN-09
printf 'auto v = e.value( );\n'     > src/core/x.cpp          # expect: FAIL naming R-ERR-04
rm -f src/core/x.h src/core/x.cpp && make test                # expect: exit 0 again
```

## Out of scope

- **Any PS2 protocol logic, type, or constant.** `Ps2Frame`, `DecodeStatus`, `LinkState`,
  the `std::expected` signatures of ADR-0009, and the vectors under `tests/vectors/` belong
  to `01-ps2-codec`. This phase must not
  create `src/core/*.cpp` — the checks are written against a tree that has no product code
  yet, and that is deliberate.
- **`src/core/pins.h` and any GPIO number.** Belongs to `02-wiring`; R-SAFETY-01..03 stay
  `planned:` and this phase does not touch them.
- **`CMakeLists.txt`, `PICO_SDK_PATH`, and any firmware build.** Belongs to `03-pio-bus`;
  `make firmware` keeps failing with its current message. The ARM toolchain itself is *not*
  out of scope any more — step 8 probes it for R-TOOL-01/02 — but this phase never builds
  with it.
- **A C++ test harness, assertion library, or test framework.** `<cassert>` plus the
  Makefile's per-file compile-and-run is the harness. The `-UNDEBUG` in `CXXFLAGS` is what makes it
  trustworthy. Adding a framework is the over-build this phase is shaped to avoid.
- **Enforcing R-CLEAN-04 (magic numbers).** Belongs to `01-ps2-codec`:
  `readability-magic-numbers` needs real code to tune its exclusions against, and turning
  it on blind against an empty tree proves nothing.
- **Installing a git `pre-commit` hook.** The security gate currently runs as a Claude
  `PreToolUse` hook only. Wiring it into `.git/hooks/` is an operator decision that has
  been raised and not yet answered; it is not this phase's to take.
- **Splitting this phase.** It was considered — eleven rules is a lot for one row — and
  rejected: ten of the eleven are the same shape (a grep over a path root plus its
  rejection case) and share one helper. The only substantial piece is the traceability
  parser. Two rows would duplicate the harness setup for no gain.
