# Phase 13-scaffold-check-gaps — close five check gaps `12-scaffold-scope-clauses` recorded

## Goal

Phase 12 wrote into five rules' text what their checks do not see. This phase changes the
checks so four of those gaps close, fixes the shell functions that break R-CLEAN-02, and in
the same edit removes or narrows each rule's scope clause so the text states what the check
now reads. After it:

| gap | check after this phase | rule text after this phase |
|---|---|---|
| G1 — `tidy_sources()` never reaches `src/hal`, `src/usb`, `src/app`, `src/emu` | `tidy_sources()` hands clang-tidy every `*.cpp` and `*.h` under `src/` (any depth) plus `tests/*.cpp`. A naming violation in a `.cpp` under any of the four layers is reported by `make lint`. | R-STYLE-02, R-CLEAN-02 and R-CLEAN-04 no longer say the four layers are unchecked; their file scope reads `src/` (`.cpp`/`.h`) plus `tests/*.cpp`. The prose paragraph under R-STYLE-01/02 in `docs/constraints.md` stops saying the scope is `src/core/` and `tests/` only. |
| G2 — `hits( )` misses code after a `//` inside a string literal (R-ERR-03) | `hits( )` treats `//` as a comment only when it sits outside a string literal (`"…"`, with `\"` escapes) and outside a character literal (`'…'`). String and character contents are **kept**, not blanked — `#include "pico/stdlib.h"` must still reach R-ARCH-01. `const char* u = "http://x"; throw E;` under `src/` is reported by R-ERR-03. | R-ERR-03's clause drops the silent-miss sentence. It keeps the false-positive sentence: a `throw`/`try {`/`catch (` inside `/* … */` or inside a string literal is still reported. |
| G3 — shell functions in `tests/` with more than three parameters (R-CLEAN-02) | No shell function in `tests/*.sh` or `tests/fixtures/*.sh` reads a positional parameter past `$3`. Three functions change: `reject( )` and `accept( )` in `tests/test_repo_shape.sh`, `check_version( )` in `tests/test_tool_versions.sh` (not named by the row, caught by its criterion). A trailing `"$@"` list after named parameters counts as one parameter (`hits( )`, `raw_hits( )`, `probe( )`). No new check is added. | R-CLEAN-02's clause drops the `reject( )`/`accept( )` instance. It keeps the statement that shell and Python functions are never measured — that stays true. |
| G4 — R-PROC-02 accepts any non-empty `verify.md` | `missing_verify( )` reports a `done` phase whose `verify.md` is missing, empty, **or lacks either of two headings**: a heading line matching `^#+ What was built` and a heading line matching `^#+ .*[Cc]heck it`. The three `done` phases today (`00-scaffold`, `01-ps2-codec`, `12-scaffold-scope-clauses`) pass unedited. | R-PROC-02's text names the two required headings (it is the only place a future phase learns them). Its clause narrows: the check proves the two headings exist; whether the prose under them is written for a non-specialist, and whether it gives exact physical steps and expected readings, is judged by no check. |
| G5 — `src_files( )` lists only `*.cpp` and `*.h` | `src_files( )` lists `.cpp`, `.h`, `.hpp`, `.cc` and `.inl` under `src/` (any depth). A `throw` in a file of each of the three new extensions is reported by R-ERR-03. | No clause to change: phase 12 wrote none for G5. |

The four layer directories under `src/` hold no files today, so `make test` and `make lint`
stay green on the real tree.

## Context pointers

- `CLAUDE.md` — §Rules are bound to tests: a check edit and its rule text move in the same edit.
- `docs/phases/12-scaffold-scope-clauses/notes.md` §Outcome and §For later phases — origin of G1–G4 and how R-ERR-03's miss was measured.
- `docs/phases/12-scaffold-scope-clauses/spec.md` §Out of scope — origin of G5.
- `docs/constraints.md` — every rule-text edit lands here: rule lines R-STYLE-02, R-ERR-03, R-CLEAN-02, R-CLEAN-04, R-PROC-02 (`grep -nE '^- \*\*R-(STYLE-02|ERR-03|CLEAN-0[24]|PROC-02)\*\*' docs/constraints.md`) and the prose paragraph beginning `R-STYLE-02's scope is` under §Invariants.
- `tests/test_style.sh` — `tidy_sources()` (G1), and the header comment that states the scope as `src/core/` and `tests/` only.
- `.clang-tidy` — header comment §"Scope limit" states the same scope; `HeaderFilterRegex` is unchanged.
- `tests/test_repo_shape.sh` — `src_files( )` (G5), `hits( )` (G2), `reject( )` and `accept( )` with every call site (G3), the `belay-debt:` header comment on comment stripping, and the `accepted -ge` floor.
- `tests/test_tool_versions.sh` — `check_version( )`, `probe( )` and their call sites (G3).
- `tests/test_phase_docs.sh` — `missing_verify( )`, its rejection, wiring and false-positive cases (G4).
- `tests/test_checks_are_live.py` — every function in the `.sh` check files is neutered, and every single-quoted string in a **one-line** function body is split on `|` and mutated one alternative at a time; a surviving mutant fails `make test`. `test_style.sh` is exempt (`NO_MUTATE`). This is why the Plan puts G5's extensions in a one-line regex and requires hand-written cases for G2 and G4.
- `tests/test_rule_traceability.py` — a rule line stays `- **R-…** — <text> — test: <path>` on one line; clause text must not contain ` — test: `.
- `docs/phases/12-scaffold-scope-clauses/verify.md` — house shape for this phase's `verify.md`, and one of the three files G4's check must keep passing.

## Plan

`/usr/bin/grep` is spelled out because an agent session may shadow `grep` with `ugrep`.

1. **G3 — `check_version( )` to three parameters.** Every call site passes a label identical to
   the binary name; drop the label, so it becomes `check_version <binary> <min> <flag>` and
   messages name the binary. Update `probe( )`'s comment and every `probe`/`check_version`
   call site — touches `tests/test_tool_versions.sh` — check:
   `/usr/bin/grep -nE '\$\{?[4-9]' tests/test_tool_versions.sh | wc -l` → 0;
   `OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh` → exit 0.
2. **G3 — `reject( )` and `accept( )` to three parameters.** Both become
   `<finder> <relative-path> <content>`. `reject`'s rule-id argument goes: the finder name is
   passed to `report( )` as its label and printed in the FAIL line
   (`FAIL: <finder> rejection case did not fire on: <content>`). `accept`'s label becomes a
   `#` comment above its call; its FAIL line reads
   `FAIL: <finder> false-positive case fired on: <content>`. Every call site rewritten; no case
   dropped — touches `tests/test_repo_shape.sh` — check:
   `/usr/bin/grep -nE '\$\{?[4-9]' tests/*.sh tests/fixtures/*.sh | wc -l` → 0;
   `sh tests/test_repo_shape.sh` → exit 0 with the same `rejection cases:` and
   `false-positive cases:` counts as before the step.
3. **G5 — `src_files( )` lists five extensions.** Keep it a one-line body and express the
   extensions as one `grep -E` alternation (e.g. `find "$1/src" -type f | grep -E '\.(cpp|h|hpp|cc|inl)$'`),
   so `tests/test_checks_are_live.py` mutates each extension. Add one R-ERR-03 rejection case
   (content `throw Status::kBad;`) for each of `src/core/x.h`, `x.hpp`, `x.cc`, `x.inl` — `.h`
   included because no case of a `src_files( )`-backed finder plants a `.h` today, so dropping
   `h` from the alternation would survive —
   touches `tests/test_repo_shape.sh` — check: `sh tests/test_repo_shape.sh` → exit 0;
   `python3 tests/test_checks_are_live.py` → exit 0 (no surviving alternation mutant).
4. **G2 — `hits( )` ignores `//` inside literals.** Replace the `sed 's|//.*||'` step with a
   scanner (awk is sufficient) that cuts a line at the first `//` found outside a `"…"` string
   (honouring `\"`) and outside a `'…'` character literal, and otherwise leaves the line
   unchanged. Add to `tests/test_repo_shape.sh`: one rejection case
   `find_err03 src/core/x.cpp 'const char* u = "http://x"; throw E;'`; one accept case
   `find_err03 src/core/x.cpp 'const char* u = "http://x"; // throw later'` (a comment after a
   string is still stripped); one accept case whose planted C++ line is `char q = '"'; // throw`
   (a `"` in a character literal does not open a string) — in shell,
   `find_err03 src/core/x.cpp 'char q = '\''"'\''; // throw'`. Raise the `accepted -ge` floor to the new count.
   Update the `belay-debt:` header comment to say what is and is not stripped now — touches
   `tests/test_repo_shape.sh` — check: `sh tests/test_repo_shape.sh` → exit 0;
   `python3 tests/test_checks_are_live.py` → exit 0.
5. **G2 — R-ERR-03's clause narrowed.** Remove the silent-miss sentence; keep the
   false-positive sentence; re-date the clause — touches `docs/constraints.md` — check:
   `/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -cF 'is not seen'` → 0;
   `python3 tests/test_rule_traceability.py` → exit 0.
6. **G1 — `tidy_sources()` covers `src/`.** Change its `src/core/*.cpp` and `src/core/*.h`
   pathspecs to `src/*.cpp` and `src/*.h` (git pathspecs, recursive). Update the scope
   comments in `tests/test_style.sh` and `.clang-tidy` to state the new scope, and that a file
   under `src/hal`, `src/usb`, `src/app` or `src/emu` which includes a Pico SDK or TinyUSB
   header fails lint with `clang-diagnostic-error` until `03-pio-bus` supplies its include
   flags — touches `tests/test_style.sh`, `.clang-tidy` — check: the G1 probe in §Acceptance
   criteria prints `4`; `make lint` → exit 0.
7. **G1 and G3 — rule text.** R-STYLE-02, R-CLEAN-02 and R-CLEAN-04: file scope becomes
   `src/` `.cpp`/`.h` plus `tests/*.cpp` and included headers; drop "every other layer under
   `src/`" / "every future layer under `src/`"; drop the `tests/test_style.sh:68` line pins
   (they go stale on this edit). R-CLEAN-02 also drops the `reject( )`/`accept( )` sentence.
   Rewrite the prose paragraph beginning `R-STYLE-02's scope is` to the new scope — touches
   `docs/constraints.md` — check: the three clause greps for G1/G3 in §Acceptance criteria
   print `0`; traceability → exit 0.
8. **G4 — `missing_verify( )` checks the two headings.** A `done` phase is reported when its
   `verify.md` is missing, empty, has no line matching `^#+ What was built`, or has no line
   matching `^#+ .*[Cc]heck it`. Cases in `tests/test_phase_docs.sh`: the existing
   no-file rejection case stays; add rejection cases for (a) a one-line file, (b) a file with
   only the `What was built` heading, (c) a file with only the check heading; change the
   false-positive fixture to a file carrying both headings — touches `tests/test_phase_docs.sh`
   — check: `sh tests/test_phase_docs.sh` → exit 0; `python3 tests/test_checks_are_live.py` → exit 0.
9. **G4 — R-PROC-02's text.** State the two required headings in the rule; narrow the clause
   to: headings are checked, the prose under them is not — touches `docs/constraints.md` —
   check: `/usr/bin/grep -E '^- \*\*R-PROC-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'What was built'` → 1;
   traceability → exit 0.
10. **`verify.md` for the operator** — touches `docs/phases/13-scaffold-check-gaps/verify.md` —
    carries the two headings G4 now requires; explains each of the five gaps in plain terms and
    gives hand checks the operator can run: plant each probe from §Acceptance criteria, see the
    FAIL line, remove it. No hardware; the file says so. Check: both heading greps in
    §Acceptance criteria print `1` or more.
11. **Full gates** — check: `make test` → exit 0; `make lint` → exit 0; `git status --porcelain`
    shows no `zz_probe` file.

## Acceptance criteria

Probes create a file, run a check, and delete the file in one command line; run them from the
repo root.

```
# G1 — a naming violation in each of the four layers is reported by make lint
for d in hal usb app emu; do mkdir -p src/$d; printf 'int BadName( ) { return 0; }\n' > src/$d/zz_probe.cpp; done; make lint 2>&1 | /usr/bin/grep -E 'src/(hal|usb|app|emu)/zz_probe\.cpp:.*readability-identifier-naming' | /usr/bin/grep -oE 'src/[a-z]+/' | sort -u | wc -l; rm -f src/*/zz_probe.cpp   # expect: 4
# G2 — code after an in-string // is seen by R-ERR-03
printf 'const char* u = "http://x"; throw E;\n' > src/hal/zz_probe.cpp; sh tests/test_repo_shape.sh | /usr/bin/grep -c '^  FAIL: R-ERR-03$'; rm -f src/hal/zz_probe.cpp   # expect: 1
# G3 — no shell function in tests/ reads a parameter past $3
/usr/bin/grep -nE '\$\{?[4-9]' tests/*.sh tests/fixtures/*.sh | wc -l   # expect: 0
# G4 — a one-line verify.md on a done phase fails R-PROC-02 (file restored afterwards)
f=docs/phases/12-scaffold-scope-clauses/verify.md; cp "$f" "$f.bak"; printf 'see above\n' > "$f"; sh tests/test_phase_docs.sh | /usr/bin/grep -c '^  FAIL: R-PROC-02$'; mv "$f.bak" "$f"   # expect: 1
git diff --quiet docs/phases/12-scaffold-scope-clauses/verify.md   # expect: exit 0
# G5 — a throw in each new extension under src/ is reported by R-ERR-03
for e in hpp cc inl; do printf 'throw E;\n' > src/hal/zz_probe.$e; sh tests/test_repo_shape.sh | /usr/bin/grep -c '^  FAIL: R-ERR-03$'; rm -f src/hal/zz_probe.$e; done   # expect: 1 1 1 (one per line)
# Rule text moved with the checks
/usr/bin/grep -E '^- \*\*R-(STYLE-02|CLEAN-02|CLEAN-04)\*\*' docs/constraints.md | /usr/bin/grep -cE 'every (other|future) layer under `src/`|`hal`, `usb`, `app`, `emu`'   # expect: 0
/usr/bin/grep -E '^- \*\*R-CLEAN-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'accept( )'   # expect: 0
/usr/bin/grep -cF "R-STYLE-02's scope is \`src/core/\` and \`tests/\` only" docs/constraints.md   # expect: 0
/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -cF 'is not seen'   # expect: 0
/usr/bin/grep -E '^- \*\*R-ERR-03\*\*' docs/constraints.md | /usr/bin/grep -cF '**Scope, recorded'   # expect: 1 (false-positive half kept)
/usr/bin/grep -E '^- \*\*R-PROC-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'a one-line `verify.md` passes'   # expect: 0
/usr/bin/grep -E '^- \*\*R-PROC-02\*\*' docs/constraints.md | /usr/bin/grep -cF 'What was built'   # expect: 1
/usr/bin/grep -cE '^- \*\*R-(STYLE-02|CLEAN-02|CLEAN-04|PROC-02|ERR-03)\*\* — .* — test: `tests/test_(style|phase_docs|repo_shape)\.sh`$' docs/constraints.md   # expect: 5 (bindings unchanged)
# This phase's own verify.md satisfies the new R-PROC-02 check
/usr/bin/grep -cE '^#+ What was built' docs/phases/13-scaffold-check-gaps/verify.md   # expect: 1 or more
/usr/bin/grep -cE '^#+ .*[Cc]heck it' docs/phases/13-scaffold-check-gaps/verify.md   # expect: 1 or more
# No probe left behind
git status --porcelain | /usr/bin/grep -c zz_probe   # expect: 0
# Project gates
python3 tests/test_rule_traceability.py   # expect: exit 0
python3 tests/test_checks_are_live.py     # expect: exit 0
make test                                 # expect: exit 0
make lint                                 # expect: exit 0
```

## Out of scope

- **Spelling clauses for R-ARCH-01, R-ARCH-03, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-ERR-03, R-ERR-04**
  — tech-debt log entry, deliberately deferred until this phase is `done`; the operator
  declined folding it in (2026-09-23).
- **Scope clauses for R-ARCH-02, R-SEC-01, R-TOOL-01, R-TOOL-02, R-PROC-01** — `14-scaffold-nongrep-clauses`.
- **A permanent parameter-count check for shell or Python functions** — G3 is fixed by
  refactoring; R-CLEAN-02 still states that shell and Python are unmeasured. Python functions
  with four parameters (e.g. `make_repo( )` in `tests/test_rule_traceability.py`) are untouched.
  No phase owns this.
- **`core_files( )` and `core_headers( )` extensions** — they still list only `.cpp`/`.h`
  (resp. `.h`), so a `.hpp` under `src/core/` is unseen by R-ARCH-01, R-CLEAN-09, R-ERR-01 and
  R-ERR-02. The row names `src_files( )` only. No phase owns this.
- **`.hpp`/`.cc`/`.inl` in `tidy_sources()` or in R-STYLE-01's `sources()`** — clang-tidy and
  clang-format still read `.cpp`/`.h` only. No phase owns this.
- **Stripping `/* … */` comments or blanking string literals in `hits( )`** — blanking strings
  would hide quoted `#include` paths from R-ARCH-01; the false positives stay recorded in
  R-ERR-03's clause. Owner: the `clang-query` upgrade `03-pio-bus` already carries.
- **Include flags or a compile database for SDK-facing files** — `03-pio-bus`, per `.clang-tidy`'s upgrade-path note.
- **Editing `docs/phases/00-scaffold/`, `01-ps2-codec/` or `12-scaffold-scope-clauses/`** —
  closed phases are not edited; the G4 acceptance probe restores `12`'s `verify.md` byte for byte.
