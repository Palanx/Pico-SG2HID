# Phase 15-scaffold-file-lists — widen the four file lists that stop at `.cpp` and `.h`

## Goal

`13-scaffold-check-gaps` widened `src_files( )` to five extensions and left four other file
lists alone. This phase widens those four so every C++ check reads the extensions its rule
text claims, then narrows or restates each rule's text in the same edit. After it:

| list | file | lists after this phase | rules it backs | rule text after this phase |
|---|---|---|---|---|
| `core_files( )` | `tests/test_repo_shape.sh` | `.cpp`, `.h`, `.hpp`, `.cc`, `.inl` under `src/core/` (any depth) | R-ARCH-01, R-CLEAN-09, R-ERR-01 | unchanged — none of the three states a file list |
| `core_headers( )` | `tests/test_repo_shape.sh` | `.h`, `.hpp` under `src/core/` (any depth) | R-ERR-02 | "scans `src/core/*.h` only" becomes "scans `.h` and `.hpp` under `src/core/`"; the text states that `.inl`, like `.cpp` and `.cc`, is not scanned because it holds out-of-line definitions, and that a declaration placed in a `.inl` is outside the binding |
| `sources()` | `tests/test_style.sh` | tracked-or-new `*.cpp`, `*.h`, `*.hpp`, `*.cc`, `*.inl` | R-STYLE-01 | the rule names the five extensions |
| `tidy_sources()` | `tests/test_style.sh` | `src/*.cpp`, `src/*.h`, `src/*.hpp`, `src/*.cc`, `src/*.inl`, `tests/*.cpp` | R-STYLE-02, R-CLEAN-02, R-CLEAN-04 | each clause names the five `src/` pathspecs; the only file extension left unchecked under `src/` is `.c` |

`tidy_lang()` in `tests/test_style.sh` returns `-xc++` for `.inl` as well as `.h`. clang-tidy
with no `-x` flag rejects a `.inl` with `unable to handle compilation, expected exactly one
compiler job` (measured 2026-09-23, Homebrew LLVM 23.1.0). It infers `.hpp` and `.cc` as C++
unaided. clang-format formats all four new extensions with no flag.

The header comment of `tests/test_phase_docs.sh` stops saying that no phase is done.

Every C++ file in the repo today is `.cpp` or `.h`, so `make test` and `make lint` stay green
on the real tree.

## Context pointers

- `CLAUDE.md` — §Rules are bound to tests: a check edit and its rule text move in the same edit.
- `.claude/rules/tech-debt.md` — entry "File lists that stop at `.cpp` and `.h`": the debt this phase closes with fix (b).
- `docs/phases/13-scaffold-check-gaps/spec.md` Plan step 3 — the one-line `grep -E` alternation pattern this phase repeats, and why `.h` needed its own case.
- `docs/phases/13-scaffold-check-gaps/notes.md` §Deviations (the extension clauses written into R-STYLE-02, R-CLEAN-02, R-CLEAN-04) and §For later phases (the stale `test_phase_docs.sh` header).
- `tests/test_repo_shape.sh` — `src_files( )` (the shape to copy), `core_files( )`, `core_headers( )` and the comment above it, the `reject` cases for `find_clean09` and `find_err02`.
- `tests/test_style.sh` — `sources()`, `tidy_sources()`, `tidy_lang()`, the header comment (scope and the `-xc++` flag note), and the R-STYLE-01 FAIL line's `Fix:` hint, which names `'*.cpp' '*.h'`.
- `.clang-tidy` — header comment: "Scope:" line and the `-xc++` on a `.h` sentence.
- `tests/test_phase_docs.sh` — header comment, lines 7–9.
- `tests/test_checks_are_live.py` — mutates each `|`-alternative of the single-quoted string in a one-line function body, and neuters every function; a surviving mutant fails `make test`. `test_style.sh` is exempt (`NO_MUTATE`), so its lists are proven by the §Acceptance criteria probes instead.
- `tests/test_rule_traceability.py` — a rule line stays `- **R-…** — <text> — test: <path>` on one line; clause text must not contain ` — test: `.
- `docs/constraints.md` — rule lines R-ARCH-01, R-STYLE-01, R-STYLE-02, R-ERR-01, R-ERR-02, R-CLEAN-02, R-CLEAN-04, R-CLEAN-09 (`/usr/bin/grep -nE '^- \*\*R-(ARCH-01|STYLE-0[12]|ERR-0[12]|CLEAN-0[249])\*\*' docs/constraints.md`); the prose paragraph beginning `R-STYLE-02's scope is` and the one after it; the §Observed conventions finding beginning `**clang-tidy needs exactly two extra flags`.
- `docs/phases/13-scaffold-check-gaps/verify.md` — house shape for this phase's `verify.md` (R-PROC-02 requires a `What was built` heading and a `Check it` heading).

## Plan

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.

1. **`core_files( )` to five extensions.** Rewrite it as a one-line body in the `src_files( )`
   shape: `find "$1/src/core" -type f 2>/dev/null | grep -E '\.(cpp|h|hpp|cc|inl)$'`. Add four
   rejection cases `reject find_clean09 src/core/x.<ext> 'virtual void poll( );'` for `cpp`,
   `hpp`, `cc`, `inl`. `.cpp` is needed because every existing `core_files( )`-backed case
   plants a `.h`. Touches `tests/test_repo_shape.sh`. Check: `sh tests/test_repo_shape.sh` →
   exit 0.
2. **`core_headers( )` to `.h` and `.hpp`.** One-line body:
   `find "$1/src/core" -type f 2>/dev/null | grep -E '\.(h|hpp)$'`. Extend the comment above
   it: `.inl` is excluded for the same reason as `.cpp`. Add the rejection case
   `reject find_err02 src/core/x.hpp 'LinkState step( Link& link );'`. Touches
   `tests/test_repo_shape.sh`. Check: `sh tests/test_repo_shape.sh` → exit 0;
   `python3 tests/test_checks_are_live.py` → exit 0 (no surviving mutant for any new
   alternative).
3. **R-ERR-02's text.** Replace "scans `src/core/*.h` only" with the `.h`/`.hpp` scope, and
   state that `.inl` (definitions, like `.cpp` and `.cc`) is not scanned, so a declaration
   placed in a `.inl` is outside the binding. Touches `docs/constraints.md`. Check:
   `/usr/bin/grep -E '^- \*\*R-ERR-02\*\*' docs/constraints.md | /usr/bin/grep -cF '`.hpp`'` → 1;
   `python3 tests/test_rule_traceability.py` → exit 0.
4. **`sources()`, `tidy_sources()`, `tidy_lang()`.** Add `'*.hpp' '*.cc' '*.inl'` to
   `sources()`, and `'src/*.hpp' 'src/*.cc' 'src/*.inl'` to `tidy_sources()`. `tidy_lang()`
   returns `-xc++` for `*.inl` as well as `*.h`. Update the header comment's scope sentence
   and `-xc++` flag note, and the `Fix:` hint on the R-STYLE-01 FAIL line, to the five
   extensions. Touches `tests/test_style.sh`. Check: the R-STYLE-01 and `make lint` probes in
   §Acceptance criteria print `3`; `make lint` → exit 0.
5. **`.clang-tidy` header comment.** The "Scope:" line names the five extensions under `src/`,
   and the `-xc++` sentence says `.h` or `.inl`. Touches `.clang-tidy`. Check: `make lint` →
   exit 0.
6. **Style rule text.** R-STYLE-01 names the five extensions. In R-STYLE-02, R-CLEAN-02 and
   R-CLEAN-04, each clause lists the five `src/` pathspecs, and the extension that stays
   unchecked is `.c` alone. R-STYLE-02's list `(.hpp, .cc, .inl, .c)` goes, as do R-CLEAN-02's
   "Every other file extension is unchecked" and R-CLEAN-04's "including every other file
   extension under `src/`". Rewrite the prose paragraph beginning `R-STYLE-02's scope is` to
   the new scope. In the paragraph after it, "`-xc++` on a header" becomes "on a `.h` or
   `.inl`". Touches `docs/constraints.md`. Check: the rule-text greps in §Acceptance criteria
   print their expected values; `python3 tests/test_rule_traceability.py` → exit 0.
7. **The §Observed conventions finding on clang-tidy flags.** The invocation reads
   `[-xc++ if <file> is a .h or .inl]`. Add one sentence: a `.inl` with no `-x` fails with
   `unable to handle compilation, expected exactly one compiler job`, and `.hpp` and `.cc`
   need no `-x` (measured 2026-09-23). Touches `docs/constraints.md`. Check:
   `/usr/bin/grep -cF 'expected exactly one compiler job' docs/constraints.md` → 1.
8. **Stale header in `tests/test_phase_docs.sh`.** Replace "Today no phase is done, so the real
   run passes without examining anything…" with a statement that stays true: the real run
   examines every `done` phase, and the rejection cases are what prove the check would fire.
   Comment only. Touches `tests/test_phase_docs.sh`. Check:
   `/usr/bin/grep -c 'Today no phase is done' tests/test_phase_docs.sh` → 0.
9. **`verify.md` for the operator.** Carries the two headings R-PROC-02 requires. In plain
   terms: what a file extension is, why a check that lists extensions can silently skip a
   file, what changed, and hand checks the operator can run (plant each probe from
   §Acceptance criteria, see the FAIL line, remove it). No hardware; the file says so. Touches
   `docs/phases/15-scaffold-file-lists/verify.md`. Check: both heading greps in §Acceptance
   criteria print `1` or more.
10. **Full gates.** Check: `make test` → exit 0; `make lint` → exit 0;
    `git status --porcelain` shows no `zz_probe` file.

## Acceptance criteria

Run from the repo root. Each probe creates a file, runs a check and deletes the file on one
command line.

```
# core_files( ): a virtual in each new extension under src/core/ is reported by R-CLEAN-09
for e in hpp cc inl; do printf 'virtual void poll( );\n' > src/core/zz_probe.$e; sh tests/test_repo_shape.sh | /usr/bin/grep -c '^  FAIL: R-CLEAN-09$'; rm -f src/core/zz_probe.$e; done   # expect: 1 1 1 (one per line)
# core_headers( ): an unmarked LinkState return in a .hpp is reported by R-ERR-02
printf 'LinkState step( Link& link );\n' > src/core/zz_probe.hpp; sh tests/test_repo_shape.sh | /usr/bin/grep -c '^  FAIL: R-ERR-02$'; rm -f src/core/zz_probe.hpp   # expect: 1
# sources(): a misformatted file of each new extension fails R-STYLE-01
for e in hpp cc inl; do printf 'int  x;\n' > src/core/zz_probe.$e; sh tests/test_style.sh | /usr/bin/grep -c '^  FAIL: R-STYLE-01'; rm -f src/core/zz_probe.$e; done   # expect: 1 1 1 (one per line)
# tidy_sources() + tidy_lang(): a naming violation in each new extension under src/ is reported by make lint
mkdir -p src/hal; for e in hpp cc inl; do printf 'int BadName( ) { return 0; }\n' > src/hal/zz_probe.$e; done; make lint 2>&1 | /usr/bin/grep -E 'src/hal/zz_probe\.(hpp|cc|inl):.*readability-identifier-naming' | /usr/bin/grep -oE 'zz_probe\.[a-z]+' | sort -u | wc -l; rm -f src/hal/zz_probe.*; rmdir src/hal   # expect: 3
# Rule text moved with the lists
/usr/bin/grep -E '^- \*\*R-STYLE-01\*\*' docs/constraints.md | /usr/bin/grep -cF '`.inl`'   # expect: 1
/usr/bin/grep -E '^- \*\*R-(STYLE-02|CLEAN-02|CLEAN-04)\*\*' docs/constraints.md | /usr/bin/grep -cF 'src/*.inl'   # expect: 3
/usr/bin/grep -E '^- \*\*R-(STYLE-02|CLEAN-02|CLEAN-04)\*\*' docs/constraints.md | /usr/bin/grep -ciE '`\.hpp`, `\.cc`, `\.inl`, `\.c`|every other file extension'   # expect: 0
/usr/bin/grep -E '^- \*\*R-ERR-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'scans `src/core/*.h` only'   # expect: 0
/usr/bin/grep -E '^- \*\*R-ERR-02\*\*' docs/constraints.md | /usr/bin/grep -cF '`.hpp`'   # expect: 1
/usr/bin/grep -cF "R-STYLE-02's scope is every \`.cpp\` and \`.h\` under" docs/constraints.md   # expect: 0
/usr/bin/grep -cF 'expected exactly one compiler job' docs/constraints.md   # expect: 1
/usr/bin/grep -cE '^- \*\*R-(STYLE-01|STYLE-02|CLEAN-02|CLEAN-04|ERR-02)\*\* — .* — test: `tests/test_(style|repo_shape)\.sh`$' docs/constraints.md   # expect: 5 (bindings unchanged)
# Stale header gone
/usr/bin/grep -c 'Today no phase is done' tests/test_phase_docs.sh   # expect: 0
# This phase's verify.md satisfies R-PROC-02
/usr/bin/grep -cE '^#+ What was built' docs/phases/15-scaffold-file-lists/verify.md   # expect: 1 or more
/usr/bin/grep -cE '^#+ .*[Cc]heck it' docs/phases/15-scaffold-file-lists/verify.md   # expect: 1 or more
# No probe left behind
git status --porcelain | /usr/bin/grep -c zz_probe   # expect: 0
# Project gates
python3 tests/test_rule_traceability.py   # expect: exit 0
python3 tests/test_checks_are_live.py     # expect: exit 0
make test                                 # expect: exit 0
make lint                                 # expect: exit 0
```

## Out of scope

- **`.c` files.** None of the five lists includes `.c`, and neither does `src_files( )`. A `.c`
  under `src/` is seen by no C++ check, and R-ARCH-01, R-CLEAN-09 and R-ERR-01 say nothing
  about that. No phase owns this; carry it to `notes.md` §For later phases as a candidate
  row for `03-pio-bus`, the first phase that may add SDK-shaped `.c` files.
- **Spelling and file-scope clauses for R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05,
  R-CLEAN-09, R-ERR-03, R-ERR-04.** Owned by `16-scaffold-spelling-clauses`, which depends on
  this phase.
- **Scope clauses for R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02, R-PROC-01.** Owned by
  `14-scaffold-nongrep-clauses`.
- **`CORE_SRC` in the `Makefile`** (`src/core/*.cpp` only). A `.cc` under `src/core/` would not
  be linked into the host tests. That is a build question, not a rule check. No phase owns it.
- **`tests/*.hpp`, `tests/*.cc` or other extensions under `tests/` in `tidy_sources()`.** The
  row matches `src_files( )`, which covers `src/` only.
- **Liveness coverage of `tests/test_style.sh`.** It stays in `NO_MUTATE`, and the
  §Acceptance criteria probes are what prove its lists.
- **Editing the tech-debt log.** After validation, propose that the "File lists that stop at
  `.cpp` and `.h`" entry be marked closed, and wait for the operator.
- **Editing `docs/phases/00-scaffold/`, `01-ps2-codec/`, `12-…/` or `13-…/`.** Closed phases are
  not edited.
