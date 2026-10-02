# Phase 25-scaffold-bash-floor — bash >= 4 becomes a probed floor of R-TOOL-01

## Goal

`make test` needs bash >= 4 because `.claude/hooks/include-check.sh` (shebang
`#!/usr/bin/env bash`, so the `bash` first on `PATH`) uses `local -A`. Today nothing checks
that up front: on stock macOS, where `/bin/bash` is 3.2, the first sign is R-ARCH-02's
transitive-include rejection case failing mid-suite. After this phase:

- `tests/test_tool_versions.sh` carries a fifth R-TOOL-01 probe, `bash` with a floor of `4`
  and flag `--version`, taken from `PATH` only (no fallback location). It has its own
  `# LIVE R-TOOL-01: bash` label, so it prefixes exactly one real-run result line.
- With a `bash` first on `PATH` whose banner is
  `GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)`, the script prints a
  `FAIL: R-TOOL-01: bash is below the floor of 4` line and exits non-zero. With the real
  bash 5 on this machine it prints `ok:   R-TOOL-01: bash 4+ (…)`. Absent from `PATH`, it
  skips under `OPTIONAL_TOOLS=1` like the other four probes.
- A rejection case with a bash 3.2 stub raises the `rejected` floor from 6 to 7.
- R-TOOL-01's text in `docs/constraints.md` names `bash` >= 4 and why; its Scope clause says
  five probes and no longer lists `bash` among the tools no probe versions.
- No prose outside `docs/phases/` claims `make test` needs only a C++23 compiler and
  `python3`. The three sentences that say so name bash >= 4:

| File | Sentence |
|---|---|
| `CLAUDE.md` §Architecture | "`make test` needs a C++23 compiler and `python3` and nothing else — …" |
| `docs/constraints.md` §Style (the paragraph after R-STYLE-02) | "(host tests need only a C++23 compiler and `python3`)" |
| `README.md` | "`make test` needs only a C++23 compiler and `python3`." |

R-PROC-04's own text is not one of them and does not change: it bounds *toolchains*, and
bash is an interpreter (`docs/phases/23-firmware-build/notes.md` §For later phases). Phase
docs under `docs/phases/` are records of their time and are not edited.

## Context pointers

- `docs/phases/23-firmware-build/notes.md` — §For later phases, the "needs a row:
  22-scaffold-typecheck-transitive" entry: the operator's decision (2026-09-30) this phase
  implements and why R-PROC-04 is not violated.
- `tests/test_tool_versions.sh` — where the probe, label, rejection case and floor land.
  `ver_num( )` reads the bash banner after the word `version` (`3.2.57(1)` → 302, floor `4` →
  400). `resolve( )`'s early-return `case` is how a tool is held to `PATH` only.
- `tests/test_checks_are_live.py` — `property_accounting( )`: every `LIVE` label must prefix
  exactly one real-run result line; why the new label is needed.
- `tests/test_boundaries.sh` — `sweep( )` executes `include-check.sh` directly, so its
  interpreter is the `bash` that `env` finds on `PATH`; the rejection case's FAIL hint
  already names bash < 4.
- `docs/constraints.md` — R-TOOL-01 (§Invariants), the §Style paragraph naming R-PROC-04,
  R-ARCH-02's scope clause (records the bash >= 4 requirement; unchanged here), R-PROC-04.
- `CLAUDE.md`, `README.md` — the two other prose sentences in the table above.
- `tests/test_rule_traceability.py` — must stay green; R-TOOL-01's binding does not change.
- `tests/test_phase_docs.sh` — checks `verify.md` exists and is well-formed.
- `docs/phases/22-scaffold-typecheck-transitive/verify.md` — the shape of a verify file for a
  check-only phase.

## Plan

1. **Add the probe** — touches `tests/test_tool_versions.sh`:
   - header: add `# LIVE R-TOOL-01: bash` after the python3 label; "four independent probes"
     → five, "the other three" → four;
   - `resolve( )`: add `bash` to the early-return `case` pattern
     (`arm-none-eabi-*|bash )`) and extend its comment: bash is probed where
     `include-check.sh`'s `env` finds it, so it is never looked up elsewhere;
   - `run_all( )`: add `probe bash 4 --version` after the python3 probe; extend the
     §R-TOOL-01 floors comment with one line for bash (`local -A` in `include-check.sh`).
   Check: `OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh | grep 'R-TOOL-01: bash'` → one
   `ok:   R-TOOL-01: bash 4+ (GNU bash, version 5…` line; script exit 0.
2. **Add the rejection case** — touches `tests/test_tool_versions.sh`: a stub named `bash` in
   `$stub_dir` printing `GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)`;
   `PATH="$stub_dir:$PATH" check_version bash 4 --version` must fail, else a
   `FAIL: R-TOOL-01 rejection case did not fire — bash 3.2.57 passed a floor of 4` line and
   `fail=1`. Delete the stub right after the case (`rm -f "$stub_dir/bash"`) so the wiring
   cases' PATH is unchanged — wiring case (2) needs every R-TOOL-01 probe to pass. Raise the
   floor to 7 in the `-ge` test and both `/6` messages, and add "a bash below its floor" to
   the comment that names the cases.
   Check: `sh tests/test_tool_versions.sh` → `ok:   rejection cases: 7/7`, both wiring cases
   `ok:`, exit 0.
3. **Prove the case is live** — no file change. Check: the second Acceptance criterion
   below (bash 3.2 stub first on `PATH`) → a `FAIL: R-TOOL-01: bash` line, rc non-zero.
4. **Rewrite R-TOOL-01 in the same edit as its test** — touches `docs/constraints.md`:
   rule text adds "`bash` >= 4 (the interpreter `.claude/hooks/include-check.sh` gets from
   `PATH`; it uses `local -A`)"; Scope clause: "four `probe` calls" → five, remove `bash`
   from the list of unprobed tools, and add that `resolve( )` takes `bash` from `PATH` only,
   like `arm-none-eabi-*`.
   Check: `python3 tests/test_rule_traceability.py` → exit 0; `grep -c "four .probe. calls"
   docs/constraints.md` → 0.
5. **Correct the prose** — touches `CLAUDE.md`, `docs/constraints.md` (§Style paragraph),
   `README.md`: each sentence in the Goal table names bash >= 4 as a `make test` requirement
   (exact wording free; the string `bash >= 4` must appear in the sentence). `CLAUDE.md` stays
   under 150 lines.
   Check: the prose criterion below → `OK`.
6. **Write the operator's verification** — touches
   `docs/phases/25-scaffold-bash-floor/verify.md` (R-PROC-02): why bash 3.2 vs 4 matters
   (what `local -A` is, in one paragraph), how to see which bash is first on `PATH`
   (`command -v bash`, `bash --version`), and the Acceptance commands with what each prints.
   Check: `sh tests/test_phase_docs.sh` → exit 0.

## Acceptance criteria

```
OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh                 # expect: exit 0, a line starting "  ok:   R-TOOL-01: bash 4+", "ok:   rejection cases: 7/7"
d=$( mktemp -d ) && printf '#!/bin/sh\necho "GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)"\n' > "$d/bash" && chmod +x "$d/bash" && PATH="$d:$PATH" OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh > "$d/out" 2>&1; echo "rc=$?"; grep 'FAIL: R-TOOL-01: bash' "$d/out"; rm -rf "$d"
                                                                 # expect: rc= non-zero, one line "FAIL: R-TOOL-01: bash is below the floor of 4 (GNU bash, version 3.2.57…)"
grep -c '^# LIVE R-TOOL-01: bash$' tests/test_tool_versions.sh   # expect: 1
python3 tests/test_checks_are_live.py                            # expect: exit 0
python3 tests/test_rule_traceability.py                          # expect: exit 0
python3 -c "
import re
for f, pat in [('CLAUDE.md', r'\`make test\` needs[^.]*'), ('README.md', r'\`make test\` needs[^.]*'), ('docs/constraints.md', r'host tests need[^)]*')]:
    s = ' '.join(open(f).read().split())
    hits = re.findall(pat, s)
    assert hits and all('bash >= 4' in h for h in hits), (f, hits)
    assert 'and nothing else' not in ' '.join(hits), f
print('OK')"                                                     # expect: OK
grep -c 'any toolchain beyond a C++23 compiler and `python3`' docs/constraints.md
                                                                 # expect: 1 — R-PROC-04's text unchanged
test $( wc -l < CLAUDE.md ) -lt 150 && echo OK                   # expect: OK
sh tests/test_phase_docs.sh                                      # expect: exit 0
make test                                                        # expect: exit 0, last line "OK"
make lint                                                        # expect: exit 0
```

## Out of scope

- R-PROC-04's text — it bounds toolchains, not interpreters (operator's decision, 2026-09-30).
- Making `include-check.sh` run under bash 3.2, or any edit under `.claude/hooks/` — package-owned;
  goes through `/belay-feedback`.
- R-ARCH-02's scope clause — its bash >= 4 sentence stays true as written.
- Version floors for `make`, `git`, `cmake`, `sh` or the C++ compiler — no phase; not wanted.
- Editing earlier phases' `spec.md`/`notes.md`/`verify.md` that repeat the old prose — records
  of their time.
- `resolve( )`'s keg-only LLVM lookup diverging from R-STYLE-01's (recorded in R-TOOL-01's
  scope clause) — no phase.
