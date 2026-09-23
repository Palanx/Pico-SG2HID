# Phase 12-scaffold-scope-clauses — scope clauses for four `00-scaffold` bindings

## Goal

Four rules in `docs/constraints.md` §Invariants are bound `test:` to checks that measure less
than the rule's sentence says, and the sentence does not say so. After this phase, each of those
four rule lines carries, in its own text, a **scope clause** — a sentence beginning
`**Scope, recorded <date>:**` that names what the check reads, what it does not read, and what
that gives up. Nothing else changes: no check, no test, no source file, no binding.

The four rules and the part of their check each clause describes:

| rule | check | what the clause must state |
|---|---|---|
| R-STYLE-02 | `tests/test_style.sh`, clang-tidy over `tidy_sources()` | **file scope:** only `src/core/*.cpp`, `src/core/*.h` and `tests/*.cpp` are handed to clang-tidy, plus any header under `src/` or `tests/` that one of them includes (`HeaderFilterRegex`). Every other layer under `src/` (`hal`, `usb`, `app`, `emu`) and every non-C++ file are unchecked. The patterns are git pathspecs that read recursively, as R-CLEAN-04's text already records — the clause may point there rather than repeat it. |
| R-CLEAN-02 | same invocation as R-STYLE-02 | the same **file scope**, plus its consequence for this rule specifically: functions in shell (`tests/*.sh`) and Python (`tests/*.py`) are never measured. Name the known instance: `reject( )` and `accept( )` in `tests/test_repo_shape.sh` take four positional parameters and nothing reports them. |
| R-PROC-02 | `tests/test_phase_docs.sh` | **existence, not content:** the check proves that every `PHASES.md` row whose id is `NN-…` and whose status is exactly `done` has a non-empty `docs/phases/<id>/verify.md` (`[ -s … ]`). It reads none of the file. "Written for a non-specialist", "what was built", "what it should do" and "exact physical steps and expected readings" are judged by no check — a one-line file passes. |
| R-ERR-03 | `tests/test_repo_shape.sh`, `find_err03` via `hits( )` | **comment handling:** `hits( )` deletes everything from `//` to end of line before matching and does nothing else. Consequences, measured at expansion (re-measure, see Plan step 4): a `throw`/`try {`/`catch (` inside a `/* … */` comment or a string literal **is reported** (a loud false positive, not a hole); code after a `//` that sits inside a string literal on the same line — `"http://x"; throw E;` — **is not seen** (a silent miss). |

## Context pointers

- `CLAUDE.md` — §Rules are bound to tests: a rule edit and its test move together; this phase changes rule text only, so no test moves.
- `docs/constraints.md` — the four rule lines (`grep -nE '^- \*\*R-(STYLE-02|CLEAN-02|PROC-02|ERR-03)\*\*' docs/constraints.md`) are where every edit lands. R-CLEAN-04 and R-ERR-02 are the house examples of a written scope clause; match their register.
- `docs/phases/01-ps2-codec/notes.md` §"For later phases — added by the round-15 audit" — the origin of this phase: names the four rules and the gap in each.
- `docs/phases/01-ps2-codec/notes.md` §"For later phases" → "Owed by `00-scaffold`, found 2026-09-22" — the `reject( )`/`accept( )` four-parameter instance R-CLEAN-02's clause names.
- `docs/phases/01-ps2-codec/spec.md` §"What a `test:` binding does and does not promise" — defines the three clause kinds (file-scope, comment, spelling) and the convention that a rule with no scope text asserts full coverage.
- `tests/test_style.sh` — `tidy_sources()` (the file list behind R-STYLE-02 and R-CLEAN-02).
- `.clang-tidy` — `HeaderFilterRegex`, the naming options and `readability-function-size` thresholds the check actually applies.
- `tests/test_phase_docs.sh` — `missing_verify( )`, the whole of R-PROC-02's check.
- `tests/test_repo_shape.sh` — `hits( )`, `src_files( )` and `find_err03( )`, R-ERR-03's check.
- `tests/test_rule_traceability.py` — `RULE_LINE`: a rule line must stay `- **R-…** — <text> — test: <path>` on one line; the clause goes inside `<text>` and must not contain ` — test: `.
- `docs/phases/00-scaffold/verify.md` — the house shape for `verify.md`.

## Plan

1. **R-STYLE-02's scope clause.** Append the clause from §Goal's table to the rule's text, before
   ` — test:` — touches `docs/constraints.md` — check:
   `/usr/bin/grep -E '^- \*\*R-STYLE-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'tidy_sources()'`
   → one line; `python3 tests/test_rule_traceability.py` → exit 0.
2. **R-CLEAN-02's scope clause.** Same shape, with the shell/Python consequence and the
   `reject( )`/`accept( )` instance — touches `docs/constraints.md` — check:
   `/usr/bin/grep -E '^- \*\*R-CLEAN-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'tidy_sources()' | /usr/bin/grep -F 'accept( )'`
   → one line; traceability → exit 0.
3. **R-PROC-02's scope clause** — touches `docs/constraints.md` — check:
   `/usr/bin/grep -E '^- \*\*R-PROC-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'non-empty'`
   → one line; traceability → exit 0.
4. **R-ERR-03's scope clause.** Before writing, re-measure the two consequences by sourcing
   `hits( )` and `find_err03( )` out of `tests/test_repo_shape.sh` in a scratch directory
   (outside the repo) and feeding it one fixture file per form: `/* throw E; */` and
   `const char* s = "throw";` must be reported; `const char* u = "http://x"; throw E;` must not.
   If a result differs from §Goal's table, write the measured result and record the difference in
   `notes.md` — touches `docs/constraints.md` — check:
   `/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F '/*'`
   → one line; traceability → exit 0; `git status --porcelain` shows no scratch files in the repo.
5. **`verify.md` for the operator** — touches `docs/phases/12-scaffold-scope-clauses/verify.md` —
   what changed (four sentences of documentation, no behaviour), why a rule stating less than it
   promises matters, and how to check it by hand: open `docs/constraints.md`, find each of the four
   rules, confirm each now says what its check does not look at; run `make test` and see it pass.
   No physical steps — this phase touches no hardware, and the file says so. Check:
   `test -s docs/phases/12-scaffold-scope-clauses/verify.md` → exit 0.
6. **Full gates** — check: `make test` → exit 0; `make lint` → exit 0.

## Acceptance criteria

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.

```
/usr/bin/grep -E '^- \*\*R-STYLE-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'tidy_sources()' | wc -l   # expect: 1
/usr/bin/grep -E '^- \*\*R-CLEAN-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'tidy_sources()' | /usr/bin/grep -F 'accept( )' | wc -l   # expect: 1
/usr/bin/grep -E '^- \*\*R-PROC-02\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F 'non-empty' | wc -l   # expect: 1
/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -F '**Scope, recorded' | /usr/bin/grep -F '/*' | wc -l   # expect: 1
/usr/bin/grep -cE '^- \*\*R-(STYLE-02|CLEAN-02|PROC-02|ERR-03)\*\* — .* — test: `tests/test_(style|phase_docs|repo_shape)\.sh`$' docs/constraints.md   # expect: 4 (bindings unchanged)
python3 tests/test_rule_traceability.py       # expect: exit 0
git diff --name-only main -- src tests .clang-tidy .clang-format Makefile | wc -l   # expect: 0
test -s docs/phases/12-scaffold-scope-clauses/verify.md   # expect: exit 0
make test                                     # expect: exit 0
make lint                                     # expect: exit 0
```

## Out of scope

- **Fixing what the clauses describe** — widening `tidy_sources()`, making `hits( )` strip block
  comments or string literals, checking `verify.md` content, and refactoring `reject( )`/`accept( )`
  to three parameters. A scope clause records a gap; closing one is check-harness work — no phase
  owns it yet (see `01-ps2-codec/notes.md` §For later phases, "Whoever next touches the check harness").
- **Spelling clauses** — R-ERR-03 owes one (e.g. `try` with `{` on the next line is not seen), as
  do R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09 and R-ERR-04. Writing one of seven
  here would leave the other six looking settled. No phase owns them yet.
- **Extensions `src_files( )` does not list** — `.hpp`, `.cc`, `.inl` under `src/` are not scanned
  (measured at expansion). It affects every rule `src_files( )` backs, not R-ERR-03 alone. No phase
  owns it yet.
- **Scope clauses for R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02, R-PROC-01** — owed per
  `01-ps2-codec/spec.md`, not named by this phase's row. No phase owns them yet.
- **The prose paragraph under R-STYLE-01/02** in `docs/constraints.md` ("R-STYLE-02's scope is
  `src/core/` and `tests/` only…") — left as written; the rule line now carries the precise clause.
- **Editing `docs/phases/00-scaffold/` or `docs/phases/01-ps2-codec/`** — closed phases are not edited.
