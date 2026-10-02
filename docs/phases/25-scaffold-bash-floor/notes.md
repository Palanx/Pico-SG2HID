# Phase 25-scaffold-bash-floor — notes

## Outcome

- base: 010c915 (work uncommitted in the working tree)
- `tests/test_tool_versions.sh`: a fifth R-TOOL-01 probe, `probe bash 4 --version`, with its
  own `# LIVE R-TOOL-01: bash` label. `resolve( )` returns early for `bash` as it does for
  `arm-none-eabi-*`, so the probe sees only the `bash` first on `PATH` — the one
  `include-check.sh`'s `#!/usr/bin/env bash` runs under. Header comment says five probes;
  the floors comment names `local -A` as the reason for 4.
- Same file: a seventh rejection case — a `bash` stub printing
  `GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)` must fail a floor of 4. The stub
  is removed right after, so the wiring cases see the real bash. The `rejected` floor is 7.
- Measured on this machine (bash 5.3.15 at `/opt/homebrew/bin/bash`): the real run prints
  `ok:   R-TOOL-01: bash 4+ (GNU bash, version 5.3.15(1)-release …)` and `rejection cases: 7/7`,
  exit 0. With the 3.2 stub first on `PATH`, the script prints
  `FAIL: R-TOOL-01: bash is below the floor of 4 (…3.2.57…)` and exits 1.
- `docs/constraints.md` R-TOOL-01: text names `bash` >= 4 and why; Scope says five `probe`
  calls, `bash` left the unprobed list (now: `$(CXX)`, `git`, `gitleaks`), and a sentence
  records that `resolve( )` takes `bash` from `PATH` only. Binding unchanged.
- Prose naming bash >= 4 as a `make test` requirement: `CLAUDE.md` §Architecture (drops "and
  nothing else"; line count unchanged), `docs/constraints.md` §Style paragraph, `README.md`
  (adds a `brew install bash` hint). R-PROC-04's text unchanged.
- `docs/phases/25-scaffold-bash-floor/verify.md` written (R-PROC-02).
- Acceptance: every criterion in spec §Acceptance criteria run in order and passed, including
  `make test` (last line `OK`) and `make lint` (exit 0).

## Deviations

- None. The README sentence's wording is free per Plan step 5; the added `brew install bash`
  parenthetical stays inside it. Swept the whole repo (outside `docs/phases/`) for other
  prose stating the old requirement: only R-PROC-04 remains, which is out of scope by design.

## Debt

- None. No `belay-debt:` comments added.

## For later phases

- Taste from the 2026-10-02 independent review (not blocking, no owner):
  `verify.md` step 3 states `rc=1` where the spec promises only non-zero; the bash sentence in
  `resolve( )`'s comment trails a block written about R-TOOL-02; README keeps "only" in
  "needs only a C++23 compiler, `python3` and bash >= 4".

## Validation — 2026-10-02
- criteria: 11 passed / 0 failed
- project gates: test pass, lint pass, typecheck pass (no `workflow gap:` line)
- boundary sweep: not swept: no file in the set is under a declared layer (`scripts/check.sh --files` rc=0)
- independent review: clean (settled: none)
- closure test: pass
- findings: 0
- spec size: 8981 (first)
- upstream: none
- not-ours: none
- verdict: done
