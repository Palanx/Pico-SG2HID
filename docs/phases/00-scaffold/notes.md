# Phase 00-scaffold — notes

## Outcome

- base: `db97117` (branch `chore/00-scaffold`)
  Was `0ee8abd`. Bumped 2026-08-31 after `db97117` committed the belay 35fbcb0 update
  on its own: the seven package-owned files it carries are tracked here, so leaving the
  base behind that commit would keep them inside this phase's file set — in the diff the
  independent review reads and in the closure test's reachability check. The phase's own
  work is unchanged and still uncommitted; only the baseline it is measured from moved.

Six new checks under `tests/`, all picked up by `make test` with no Makefile edit:

| file | rules |
|---|---|
| `tests/test_rule_traceability.py` | R-PROC-01 |
| `tests/test_repo_shape.sh` | R-ARCH-01, R-ARCH-03, R-ERR-03, R-ERR-04, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-PROTO-05 |
| `tests/test_boundaries.sh` | R-ARCH-02 |
| `tests/test_phase_docs.sh` | R-PROC-02 |
| `tests/test_secrets.sh` | R-SEC-01 |
| `tests/test_tool_versions.sh` | R-TOOL-01, R-TOOL-02 |

Fourteen rules moved from `planned: 00-scaffold` to `test:` in `docs/constraints.md`;
`grep -c 'planned: 00-scaffold'` is now 0. Each test carries its `RULE <id>` marker, and
`tests/test_rule_traceability.py` fails the build if either side drifts.

`STYLE_OPTIONAL` is now `OPTIONAL_TOOLS` across `Makefile` and `tests/test_style.sh`, so
one flag governs every tool outside the C++23 + `python3` floor.

Behaviour verified by hand, all eight caught by rule id and the tree restored afterwards:
`new` under `src/`, a Pico SDK header in `core`, `core` including `hal`, `try/catch`,
`.value()`, an unprefixed `bool`, a bare `TODO`, and inheritance in `core`.

`tests/test_style.sh` also runs `clang-tidy` with `-std=c++23` now, matching ADR-0008.

### Round 2 — 2026-08-31, after the first validation failed

The first validation returned seven `contradicts` and six missing pointers. All are closed;
`make test` is green and every acceptance criterion passes.

Code:

- `tests/test_tool_versions.sh` — `first_int` became `ver_num` (major*100+minor), so
  R-TOOL-01's `python3 >= 3.8` floor is actually enforced; a fourth rejection case pins it
  (a stub reporting 3.7.9 must fail a floor of 3.8). `resolve( )` now returns early for
  `arm-none-eabi-*`, which makes its own comment true and guarantees R-TOOL-01 and R-TOOL-02
  judge the same binary. The R-TOOL-02 probe dropped `-fno-exceptions -fno-rtti -std=c++23`
  and now passes exactly the two flags the spec names.
- `tests/test_repo_shape.sh` — R-ARCH-01 gained its second clause (hosted-only standard
  headers, anchored on `>` so `<string_view>` is not read as `<string>`); R-CLEAN-09 gained
  the default-specifier forms with `enum class E : uint8_t` excluded. Rejection cases 8 -> 10,
  accept cases 4 -> 7.
- `tests/test_secrets.sh` — added the `gitleaks git` history scan R-SEC-01's third clause
  requires, and corrected a comment claiming `make lint` runs this file. It does not:
  `lint` runs `tests/test_style.sh` and nothing else.
- `tests/test_boundaries.sh` — marker said `§Layering`; R-ARCH-02 is declared in `§Invariants`.
- `docs/phases/00-scaffold/verify.md` — the sample failure now matches what the check emits
  (full path, no space after the line number).

Catalogue: R-ERR-03 split. It now reads "No `throw`, `try` or `catch` anywhere under `src/`"
and keeps `test:`; the build-flags clause became **R-ERR-05**, `planned: 03-pio-bus`.
Operator decision, taken 2026-08-31. Still fourteen rules moved to `test:` this phase —
R-ERR-05 is new debt, not a fifteenth binding.

## Deviations

- **Spec said eight greps for `tests/test_repo_shape.sh`; there are eight checks but two
  scanning modes.** `hits()` strips line comments before searching, so `new` inside a
  comment is not read as an allocation. R-CLEAN-05 is a rule *about* comments, so comment
  stripping deleted the text it looks for and the check could never have fired. Added
  `raw_hits()` for that one rule. Found by its own rejection case on the first run — the
  real-tree run passed happily.
- **Added false-positive ("accept") cases the spec did not ask for**: 4 in
  `test_repo_shape.sh`, 1 in `test_phase_docs.sh`, 1 in `test_secrets.sh`. A rule that
  fires on legitimate code gets switched off by whoever it annoys, and then protects
  nothing. The four in `test_repo_shape.sh` pin down real ambiguities: `= delete` is legal
  C++ and must not read as `delete`, `is_`/`m_has_` bools are correct, and a
  `TODO(09-guitar-observe)` is exactly what R-CLEAN-05 wants.
- **Two self-reference bugs, both fixed by assembling strings at runtime.** The
  traceability fixtures contained literal `RULE R-X-01` text, which planted real markers in
  the checker's own source and made it report itself. `test_secrets.sh` contained a literal
  fake GitHub token, which *is* a detectable secret, so the check failed against its own
  file. Both found by running, not by reading.
- **`resolve()` added to `tests/test_tool_versions.sh`**, not in the spec: `clang-tidy`
  ships keg-only on macOS and is not on `PATH`, so R-TOOL-01 would have skipped it forever.
  It probes the same LLVM prefixes `test_style.sh` already did. Deliberately **not** applied
  to `arm-none-eabi-g++` — R-TOOL-02 is a claim about what is first on `PATH`, and probing
  elsewhere would hide the exact trap it exists to catch.
- **`grep -c` bug in `test_phase_docs.sh`**: `grep -c ... || echo 0` prints two zeroes,
  because `grep -c` prints `0` *and* exits 1 when there are no matches. Cosmetic, fixed.
- **Spec step 1 was already half-done.** `-UNDEBUG` landed in `CXXFLAGS` with the ADR-0008
  change before this phase started; the spec was amended to say "verify it is still there"
  before implementation began, so this is recorded, not discovered.
- **No missing Context pointers.** Everything needed was reachable from the spec. The one
  file read that the pointers did not name is `docs/templates/notes.md`, which the spec
  does point at.

- **Validation 2026-08-31 — spec gaps the independent review could not resolve (missing
  pointers).** Given only `CLAUDE.md`, `spec.md` and the diff, the reviewer could not decide
  six things from the spec alone. Each needs a pointer in `spec.md` before re-validation:
  (a) the Goal says "**six** test files" and lists `test_rule_traceability.py` as a separate
  bullet, while Plan steps 4–8 enumerate **five** shell files — which count is authoritative;
  (b) the Goal says "fourteen rules" while Context pointers and Out of scope both say
  "eleven" (fourteen is what landed, and `PHASES.md`'s coarse acceptance row still says
  eleven); (c) the `lint` target is never defined in the spec, so whether the two
  `OPTIONAL_TOOLS`-dependent checks can break `make lint` on a machine without `gitleaks` or
  the ARM compiler is undecidable; (d) whether R-SEC-01's "or history" clause must be covered
  before the rule may leave `planned:` — the check runs `gitleaks dir`, never `gitleaks git`;
  (e) which subtrees and file extensions each `test_repo_shape.sh` grep covers — R-CLEAN-03
  and R-CLEAN-05 state no scope in the catalogue but are checked only under `src/` and only
  for `*.cpp`/`*.h`; (f) whether the `PHASES.md` status flip belongs to the phase's own commit
  or to the workflow command that drove it.
- **Validation 2026-08-31 — the `resolve()` header comment in `tests/test_tool_versions.sh`
  is false, and the Deviations entry above repeats it.** `check_version` calls
  `resolve "$cv_bin"` for every tool, `arm-none-eabi-g++` included. The behaviour is close to
  harmless — `resolve` only appends LLVM prefixes, so the ARM binary still comes from `PATH` —
  but R-TOOL-01 could in principle version-check a different binary than R-TOOL-02 probes.

- **Validation round 2 — `spec.md` amended in six places, all recorded here.** (1) The
  eleven/fourteen drift: Context pointers and Out of scope said eleven, the Goal said
  fourteen; fourteen is what the catalogue held, so the two stragglers were corrected and the
  splitting rationale reworded ("eleven of the fourteen are the same shape"). (2) The Goal's
  "Six test files" became "Five shell test files" — the sixth was `test_rule_traceability.py`,
  already its own bullet. (3) The `Makefile` pointer now states what `make lint` runs
  (`tests/test_style.sh` only), which is what made the reviewer's `make lint` question
  undecidable. (4) Plan step 5 now fixes the greps' scope — `*.cpp`/`*.h` under `<root>/src`,
  `src/core` only for the two `core`-scoped rules, `tests/` deliberately outside — and states
  that a two-clause rule needs both clauses checked. (5) Plan step 7 now says R-SEC-01 is two
  scans, tree and history. (6) Plan step 10 now says `verify.md` is a durable operator
  reference, not a session report, and that its sample output must match what the checks emit.
  A seventh addition, after Context pointers, states that `PHASES.md` status flips are written
  by the workflow commands and are not a file this phase's Plan needs to name.
- **Acceptance criteria updated to match the counts they assert**: `test_repo_shape.sh` now
  expects 10 rejection and 7 accept cases (was 8/—), `test_tool_versions.sh` expects 4
  rejection cases (was silent), `test_secrets.sh` expects tree *and* history. No criterion was
  deleted.
- **`docs/constraints.md` amended**: R-ERR-03 narrowed, R-ERR-05 added. Outside this phase's
  original Plan, which only ever said "flip `planned:` to `test:`" — the split was the
  operator's answer to a half-binding validation found.

- **Validation round 2 — `docs/index/` was in the file set and unreachable from the spec.**
  `docs/index/_overview.md` and `docs/index/tests.md` are regenerated by
  `scripts/build-index.sh`, which `/validate-phase` step 4 runs when the index is stale — so
  they land in the diff of whichever phase made it stale, this one. The spec named neither.
  Fixed by extending the workflow-written-files paragraph after the Context pointers to cover
  `docs/index/` alongside `docs/phases/PHASES.md`. Same class of gap as the PHASES.md one the
  first validation found, and found the same way: by listing the file set and asking what the
  spec says about each entry.

- **Validation round 3 — four more missing pointers.** (a) `find_arch01`'s hosted-only
  header list is not derivable from the spec: the catalogue says "no `#include <` of a
  hosted-only standard header" and names no set, so whether `<memory>`, `<new>`, `<random>`,
  `<locale>` belong on it — and whether omitting `<chrono>`, `<optional>`, `<variant>`,
  `<algorithm>`, `<functional>` is a gap — cannot be judged from the spec. (b) The
  R-ERR-03/R-ERR-05 split is invisible to the spec: the Goal enumerates only "fourteen rules
  moved from `planned:` to `test:`" and Plan step 9 describes only flips, so *creating* a rule
  is unsanctioned by the document even though the Context pointers already reference R-ERR-05.
  (c) `PHASES.md`'s row still says "the eleven rules"; `CLAUDE.md` forbids editing rows, but
  the spec never says what to do when a row's coarse acceptance text drifts from the phase's
  Goal. (d) Plan step 8 asks for one rejection case in `test_tool_versions.sh` while the
  Acceptance criteria line asks for four — the spec contradicts itself, and the same ambiguity
  would let a one-case implementation claim compliance.

## Debt

- `find_clean03` scans line by line, so a single line declaring both a compliant and a
  non-compliant boolean (`bool m_is_ok = true, flag = false;`) is exempt as a whole. Found in
  validation round 4, left in place: one declaration per line is the house style everywhere
  else, and the fix is the same `clang-query` upgrade as the entry below.
- `belay-debt:` in `tests/test_repo_shape.sh` — the eight checks are greps, not parsed
  C++. Line comments are stripped (except for R-CLEAN-05, deliberately); block comments
  and string literals are stripped by neither, so a forbidden token inside `/* */` or a
  string literal is a false positive. Ceiling accepted because it catches the realistic
  violation, which is someone writing the forbidden thing. Upgrade path: `clang-query`,
  which needs the `compile_commands.json` that `.clang-tidy` also wants — so both upgrades
  land together, in `03-pio-bus` at the earliest.
- ~~R-TOOL-01 and R-TOOL-02 never run against the real ARM compiler.~~ **Closed during
  this phase.** The operator added the cask's `bin` to `PATH`; both now pass for real:
  `arm-none-eabi-g++ 15` clears the floor of 12, and R-TOOL-02 compiles a `<cstdint>` TU
  for `cortex-m0plus` with `/Applications/ArmGNUToolchain/15.3.rel1/arm-none-eabi/bin/arm-none-eabi-g++`.
  One caveat that is about tooling, not the repo: Claude Code snapshots the shell
  environment at session start, so an agent session opened before the `PATH` edit still
  sees the two `skip:` lines. A fresh session, or an explicit `export`, sees them pass.
- **R-ERR-05 is owed to `03-pio-bus`.** `Firmware builds pass -fno-exceptions -fno-rtti` is
  `planned:` because the firmware build does not exist yet. Nothing checks it until that phase
  writes the CMake config; until then the flags are a claim, not a rule.
- - The `R-TOOL-01` floor for `arm-none-eabi-g++` is GCC 12, inferred from when libstdc++
  gained `<expected>`. Only 15.3.1 has been measured. Anything in between is unverified —
  recorded in ADR-0008 §Verification.

## For later phases

- **`03-pio-bus`**: R-ERR-05 (`-fno-exceptions -fno-rtti` on firmware builds) is bound to
  this phase and must move from `planned:` to `test:` when the CMake config lands — the
  traceability meta-test will not let the phase close otherwise.
- **`03-pio-bus`**: the toolchain is ready, but do not undo how. The Homebrew cask only
  symlinked a partial set of binaries into `/opt/homebrew/bin` (`gcc-15.3.1`, `gdb`,
  `gfortran`, `gstack`), because the now-removed formula owned the conflicting names —
  `arm-none-eabi-gcc`, `g++`, `as`, `ld`, `objcopy` and `size` were never linked. The Pico
  SDK needs `objcopy` and `size` to produce a `.uf2`, so re-linking the cask would not have
  been enough. What works is the cask's own `bin` on `PATH`, which is what
  `~/.zshrc` now does; all four binaries verified reachable. That `PATH` line pins
  `15.3.rel1`, so a cask upgrade renames the directory and breaks it — the symptom will be
  `arm-none-eabi-g++ not found`, and R-TOOL-02 turns it into a named failure rather than a
  confusing build error.
- **`01-ps2-codec`**: `#embed` is available at C++23 in *both* compilers (measured while
  writing ADR-0008). It could let `tests/vectors/*.hex` be embedded directly instead of
  parsed at runtime, which strengthens R-PROTO-05 — the literal would stop needing code to
  read it. Worth evaluating when that phase is expanded; not decided.
- **`01-ps2-codec`**: R-CLEAN-04 (no magic numbers) is still `planned: 01-ps2-codec` and
  will need `readability-magic-numbers` tuned against real code, plus an exclusion for
  `tests/` — test files legitimately contain literal expected bytes. Turning it on blind
  against an empty tree proves nothing, which is why it was left out of this phase.
- **Any phase adding a rule**: the pattern to copy is a check function that takes the tree
  root as an argument, plus a rejection case and, where the rule could plausibly misfire, an
  accept case. Checks that only look at the real repo cannot be shown to work while the
  repo is nearly empty — and three of this phase's bugs were invisible any other way.
- **`02-wiring`**: `AlignArrayOfStructures` in `.clang-format` aligns a plain C array and
  gives up on `std::array`'s doubled braces, so `src/core/pins.h` must use a plain array for
  the pin table to be scannable in columns. Already recorded in `docs/constraints.md`
  §Observed conventions; repeated here because it is a constraint on that phase's first file.

## Validation — 2026-08-31

- criteria: 12 passed / 0 failed; the 8-case adversarial block also 8/8 (each exits non-zero
  naming its rule id, tree restored, `make test` back to 0)
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean — but vacuous: the file set contains no `.cpp`/`.h`, by design
- index: was stale (stamped `8bbc1e7`), rebuilt to `0ee8abd`
- independent review: contradicts — seven findings, all reproduced by hand (below);
  undecidable — six missing pointers, recorded in Deviations
- closure test: fail — the `undecidable` findings are missing pointers by definition, and the
  spec's own eleven/fourteen and five/six counts contradict each other
- verdict: returned to implementation

### The seven `contradicts` findings, each verified against the code

1. **R-TOOL-01's `python3` floor is not enforced.** The catalogue says `python3 >= 3.8` and
   the file's own comment says "python3 3.8 for the meta-test", but `check_version` is called
   with a floor of `3` and `first_int` compares the major only. Python 3.0 passes. The rule
   left `planned:` with its stated floor unchecked.
2. **R-ARCH-01 is half-bound.** The rule has two clauses; `find_arch01( )` matches only the
   hardware-header one. Verified: `#include <iostream>` in `src/core/` reports `ok: R-ARCH-01`.
3. **R-CLEAN-09 misses the default-specifier form.** The rule forbids inheritance in `core`
   "at all"; the pattern requires an explicit `public|private|protected` or `virtual`.
   Verified: `struct A : B { };` in `src/core/` reports `ok: R-CLEAN-09`.
4. **R-ERR-03 is half-bound, and the other half is out of scope.** The rule is "no
   `throw`/`try`/`catch` under `src/` **and** firmware builds pass `-fno-exceptions
   -fno-rtti`". The second clause lives in the firmware build, which Out of scope assigns to
   `03-pio-bus`. Either the rule splits or the binding states what it does not cover.
5. **`arm_compiles( )` adds flags the spec did not name.** Plan step 8 fixes
   `-mcpu=cortex-m0plus -mthumb`; the probe also passes `-fno-exceptions -fno-rtti
   -std=c++23`. `-std=c++23` is the GCC 13+ spelling (GCC 12 accepts only `-std=c++2b`) —
   *not verified against a real GCC 12 here* — so a compiler at R-TOOL-01's own floor of 12
   would fail R-TOOL-02 and be reported as "a cross-compiler with no target C library",
   which is the wrong diagnosis. The `-std=` flag is not needed to prove `<cstdint>` resolves.
6. **`verify.md` prints a sample failure the code cannot produce.** It promises
   `src/core/scratch.cpp:1: auto* p = new int;`; the actual line is
   `/Users/palanx/Git/pico-sg2hid/src/core/scratch.cpp:1:auto* p = new int;` — absolute path,
   no space after the line number. `src_files( )` finds under `$ROOT` and `hits( )` prefixes
   that path. For an operator-facing document under R-PROC-02 the expected reading has to match.
7. **The marker in `tests/test_boundaries.sh` names the wrong section.** Plan step 9 fixes
   `# RULE <id> — docs/constraints.md §Invariants — <one line>`; that file says `§Layering`.
   R-ARCH-02 is declared in §Invariants. The other five files follow the spec form.

The reviewer also reported `notes.md` as missing from the diff. It is not: this file was
withheld from the review on purpose, as `/validate-phase` step 5 requires. Dismissed.

### Taste, not blocking (from validation 2026-08-31)

- Taste, not blocking: `test_boundaries.sh` and `test_secrets.sh` write `/tmp/bsweep.$$` and
  `/tmp/gl.$$` while every other temp path in the phase goes through `mktemp -d`.
- Taste, not blocking: the five new `.sh` files land mode `100644` while `tests/test_style.sh`
  is `100755`. The Makefile runs them as `sh "$t"`, so nothing breaks, but an operator running
  the acceptance commands by hand gets a different experience per file.

## Implementation — 2026-08-31 (round 2)

- acceptance criteria: 8/8 commands exit 0; `grep -c 'planned: 00-scaffold'` is 0;
  `STYLE_OPTIONAL` is 0 in both files; `verify.md` and `notes.md` are non-empty
- project gates: test pass, lint pass, typecheck gap (no tool configured)
- deviations: 3, all recorded above — six `spec.md` amendments, the acceptance-criteria count
  updates, and the R-ERR-03 / R-ERR-05 catalogue split
- status: stays `in-progress`; `/validate-phase 00-scaffold` is the next step

## Validation — 2026-08-31 (round 2)

- criteria: 12 passed / 0 failed; the adversarial block 8/8 (each case exits non-zero naming
  its rule id, tree restored, `make test` back to 0). The counts the criteria now assert were
  checked literally: repo_shape 10 rejection / 7 accept, tool_versions 4 rejection,
  traceability 9 rejection
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean, and still vacuous — the file set holds no `.cpp`/`.h` by design
- index: fresh (`0ee8abd`)
- independent review: **did not run** — the subagent was killed by a session rate limit
  (resets 19:40 America/Santiago) before it read its inputs. Not `skipped: no subagent`: the
  dispatch path works, this session ran out of budget. The seven `contradicts` the round-1
  review found are the reason this is recorded as a gap rather than waved through
- closure test: the mechanical half passes — all four sections present and non-blank, and
  every file in the set is now reachable from the spec (`docs/index/` was not, and was fixed
  this round; see Deviations). The third bullet — "an `undecidable` finding IS a missing
  pointer" — is **unverified**, because the review that produces those findings never ran
- verdict: **returned to the operator, not to implementation.** No gate failed. One gate did
  not execute. Status stays `in-progress`; `done` needs either the round-2 review after the
  limit resets, or an explicit operator decision to accept a self-declared closure test

## Validation — 2026-08-31 (round 3)

- criteria: 12 passed / 0 failed; adversarial block 8/8 (verified this session on the same
  test files)
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean (still vacuous — no `.cpp`/`.h` in the file set)
- index: `build-index.sh --check` says `index fresh (0ee8abd)`, and **it is wrong** — see
  finding 1
- independent review: contradicts — five findings, all reproduced (below); undecidable —
  four missing pointers, recorded in Deviations
- closure test: fail — four `undecidable` findings are missing pointers by definition
- verdict: returned to implementation

### The five `contradicts` findings, each verified against the tree

1. **`docs/index/` is content-stale and the freshness check cannot see it.** The index was
   rebuilt during validation round 1, before round 2 changed three test files.
   `tests/test_repo_shape.sh` 146 -> 157 lines, `tests/test_secrets.sh` 60 -> 71,
   `tests/test_tool_versions.sh` 150 -> 178; `_overview.md`'s `Lines: 850` is the sum of the
   stale numbers. `build-index.sh --check` still reports `index fresh (0ee8abd)` because it
   stamps against the HEAD commit, and HEAD has not moved — every change this phase made is
   uncommitted. **A `--check` that passes on a stale index is a hole in validation step 4,
   not just a stale file here.** Worth a `/belay-feedback`.
2. **`verify.md`'s R-TOOL-01 sample no longer matches the check.** It prints
   `ok:   R-TOOL-01: arm-none-eabi-g++ 15 (min 12)`; round 2 changed the echo to
   `$cv_label $cv_min+ ($cv_out)`, so the check emits
   `ok:   R-TOOL-01: arm-none-eabi-g++ 12+ (15.3.1)` — floor and found version in opposite
   positions, and the `(min N)` form no longer exists. Round 2 fixed the R-ARCH-03 sample and
   missed this one.
3. **`verify.md` carries three session-scoped sections that round 2's own spec amendment
   outlawed.** Plan step 10 now says the file is a durable operator reference and that
   anything true only while the phase was written belongs in `notes.md`. "Three real bugs this
   approach caught", "The ARM toolchain check, now proven" ("While this phase was being
   finished you put the ARM compiler on the `PATH`") and the note about Claude Code reading
   the shell environment once per session all fail that test. Self-inflicted: the spec line
   was added in round 2 and `verify.md` was never reconciled against it.
4. **`verify.md` miscounts the checks.** It says "fourteen of them are programs ... and a
   fifteenth program checks that the tie itself has not come loose". R-PROC-01 is one of the
   fourteen and `tests/test_rule_traceability.py` is its binding — the traceability parser is
   not a fifteenth thing on top of the fourteen.
5. **`tests/test_boundaries.sh` sweeps less than Plan step 4 says, and conflates two failure
   modes.** The Plan says "every file under `src/`"; `sweep( )` filters to `*.cpp`/`*.h`
   (step 5 scopes its greps explicitly, step 4 does not). Separately, the spec's pointer says
   the hook "exits 2 on a violation", but `if ! "$HOOK" "$f"` treats *any* non-zero exit as a
   violation and prints `FAIL: R-ARCH-02: forbidden dependency direction` — so a crashing or
   misconfigured hook is reported as a layering breach.

The reviewer again reported `notes.md` missing from the diff; again withheld on purpose per
step 5. Dismissed.

## Implementation — 2026-08-31 (round 3)

All five `contradicts` findings closed and all four missing pointers written into `spec.md`.
`make test` green; every acceptance criterion re-run; the adversarial block 9/9 (the eight
documented cases plus one new one, below).

Code:

- `tests/test_boundaries.sh` — `sweep( )` now visits **every** file under `src/`, not only
  `*.cpp`/`*.h`, which is what Plan step 4 says; the hook decides for itself what an import
  line is, so filtering by extension was the wrapper inventing a scope the rule does not have.
  Exit codes are now kept apart: the hook returns 2 and only 2 for a layering breach, so
  `sweep` returns 1 for a real violation and 2 for "the hook did not run", and the two print
  different `FAIL:` lines. A second rejection case pins that branch — a stub hook that exits 3
  over a tree that *does* breach the layer must be reported as a broken hook, not as a breach.
  The sweep's temp file moved from `/tmp/bsweep.$$` to `mktemp`, closing half of the round-1
  taste note (`test_secrets.sh` still uses `/tmp/gl.$$`).
- The widened sweep was verified adversarially: `src/core/x.hpp` including `hal/bus.h` now
  fails `make test` naming R-ARCH-02. Before this change that file was invisible.

`docs/phases/00-scaffold/verify.md`:

- The R-TOOL-01 sample was `arm-none-eabi-g++ 15 (min 12)`; the check has emitted
  `arm-none-eabi-g++ 12+ (15.3.1)` since round 2 — floor first, version found in brackets.
  Corrected, and the R-TOOL-02 sample is now the single long line the check actually prints
  rather than a hand-wrapped two-line version.
- The three session-scoped sections are gone, per Plan step 10's "durable operator reference,
  not a session report": "Three real bugs this approach caught" (already in this file under
  Deviations, now referenced from `verify.md` in one sentence), "The ARM toolchain check, now
  proven" and the note that Claude Code snapshots the shell environment once per session
  (recorded under Debt here). What replaced them is durable: what the two toolchain checks
  print and why R-TOOL-02 compiles a file instead of reading a version number.
- The "fifteenth program" miscount is fixed. R-PROC-01 *is* one of the fourteen and
  `tests/test_rule_traceability.py` is its binding; there is no fifteenth thing.

`docs/index/` rebuilt — it was content-stale from round 1 and `--check` could not see it.

## Deviations (round 3)

- **Four `spec.md` amendments, one per undecidable finding.** (a) Plan step 5 now gives
  R-ARCH-01's hosted-only header list as a *criterion* plus a floor set, and says explicitly
  that the list is open — a header nobody has written yet is not a gap in this phase. (b) The
  Goal now sanctions splitting a two-clause rule, names the R-ERR-03 / R-ERR-05 split as the
  one instance, and states that a split leaves the count at fourteen bindings moved. (c) The
  workflow-written-files paragraph now says the `PHASES.md` coarse row's "eleven rules" is
  stale, that rows are append-only so it stays stale, and that this spec's Goal is
  authoritative where the two disagree. (d) Plan step 8 now asks for four rejection cases and
  names them, matching the Acceptance criteria line instead of contradicting it.
- **One rejection case added beyond the spec** (`test_boundaries.sh`, the broken-hook case).
  Splitting exit 2 from every other non-zero exit is new logic, and this phase's own rule is
  that a branch nobody has seen fire is not a check.
- **`build-index.sh --check` reports a content-stale index as fresh.** It stamps against the
  HEAD commit, so while a phase's work is uncommitted — which is the entire window in which
  `/validate-phase` runs — the check passes no matter how far the index has drifted. That is a
  hole in validation step 4, not a fact about this repo; it belongs upstream via
  `/belay-feedback`. Worked around here by rebuilding the index unconditionally.

## Deviations (round 4)

- **Validation 2026-08-31 round 4 — four more missing pointers.** (a) Whether `.claude/hooks/boundary-check.sh` and `.claude/workflow/boundaries.rules` are tracked repository content. `tests/test_boundaries.sh` hard-fails (`exit 1`, not `skip:`) when the hook is not executable, while the `PHASES.md` row promises `make test` runs green from a clean clone with only clang++ and python3. The spec lists both files under Context pointers but never says whether they ship with the repo, so whether a missing one should be `FAIL` or a `skip:` under `OPTIONAL_TOOLS` is undecidable. (b) The definition of tool "presence" for R-TOOL-01. `resolve( )` searches keg-only LLVM prefixes for `clang-format`/`clang-tidy` but deliberately returns early for `arm-none-eabi-*`; the spec says only "if absent, skip" and never defines absent as *on `PATH`* or *discoverable anywhere*, so the asymmetry cannot be judged as intent or as papering over the same shadowing trap R-TOOL-02 exists to catch. (c) The intended scope of R-ERR-04's grep. `find_err04` matches every `.value(` under `src/`, including `std::optional::value()` and any user type with a `value()` accessor, while the catalogue text is narrower ("on a `std::expected`"). The spec says only that the check "must be a grep, not a comment", which cannot distinguish receivers, so whether the over-broad match is the accepted price or a defect is undecidable. (d) The hook's full exit-code table. `tests/test_boundaries.sh` now implements a three-way contract (0 clean / 1 violation / 2 hook error) with precedence and a dedicated rejection case, but the spec states only "exits 2 on a violation" and asks for one rejection case — whether a non-0/non-2 exit is a hook error, a violation, or undefined is not derivable.

- **Validation round 4 — `docs/phases/00-scaffold/spec.md` is in the file set and unreachable from the spec.** The workflow-written-files paragraph covers `docs/phases/PHASES.md` and `docs/index/` and stops there, but `/validate-phase` mandates spec amendment as the fix for every `undecidable` finding, so an amended spec is in the diff of every phase that reaches round 2. Exactly the same class of gap as the `docs/index/` one round 2 found, and the third time this paragraph has been the fix. Until the paragraph names `spec.md`, the closure test cannot pass on any phase that has been through the loop once.

## Validation — 2026-08-31 (round 4)

- criteria: 12 passed / 0 failed
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean (still vacuous — no `.cpp`/`.h` in the file set by design)
- index: fresh, and content-fresh this time — `docs/index/tests.md` reports 935 lines and the
  tree holds 935. Round 3's content-staleness did not recur
- independent review: contradicts — four findings, two of them blocking and both reproduced
  against the tree (below); undecidable — four missing pointers, recorded in Deviations above
- closure test: **fail** — four `undecidable` findings are missing pointers by definition, and
  `spec.md` is in the file set while the spec names only `PHASES.md` and `docs/index/`
- verdict: returned to implementation — but see the iteration-4 escape below, which is the
  more important half of this record

### The four `contradicts` findings, each checked against the tree

1. **Three accept cases Plan step 5 names outright are missing.** Step 5: "Freestanding
   headers stay legal and need an accept case: `<cstdint>`, `<array>`, `<span>`, `<expected>`."
   `tests/test_repo_shape.sh` ships two `accept` calls against `find_arch01` — `<cstdint>`
   (:151) and `<string_view>` (:152). `<array>`, `<span>` and `<expected>` are never
   accept-tested. `<string_view>` is a legitimate extra (step 5 asks for the closing-`>`
   anchor) but is not a substitute. This is a real code gap and the only one this round:
   three lines. **CONFIRMED.**
2. **`spec.md` contradicts itself about `test_repo_shape.sh`'s counts.** Line 131 (step 5's
   check line): "eight `ok:` lines and eight rejection cases confirmed". Line 187 (Acceptance
   criteria): "8 rules ok, 10 rejection cases, 7 accept cases". The code matches line 187
   (`rejected -eq 10`, `accepted -eq 7`). Round 2 raised the counts in step 5's body and in the
   Acceptance block and left step 5's own check line at the old numbers. A spec bug, not a code
   bug — and one the amendment loop introduced. **CONFIRMED.**
3. **`spec.md` is edited by the phase's own diff and no Plan step names it.** Recorded in
   Deviations above. The reviewer's stronger reading — that the acceptance criteria were
   edited toward the code and so are not an independent gate — is structurally true of the
   `/validate-phase` loop, which prescribes spec amendment as the remedy for `undecidable`.
   That is the loop working as designed, but the spec nowhere says so, which is precisely why
   a starved reviewer reads it as goalpost-moving. **CONFIRMED as a missing pointer**, not as
   an implementation failure.
4. **`verify.md` line 44 states a fact about the authoring session.** "Three of the checks were
   silently broken when they were first written…" against step 10's "anything true only while
   this phase was being written belongs in `notes.md` instead". Recorded as **weak**: the
   sentence is the rationale for why every check carries a rejection case, which is durable,
   and it already delegates the detail to `notes.md`. It stops being precise if a fourth check
   is ever fixed. Not blocking on its own; noted so the next round can decide rather than
   rediscover.

### Taste, not blocking (round 4)

- `docs/constraints.md` orders the rules 01, 02, 03, 05, 04 — R-ERR-05 was inserted between
  R-ERR-03 and R-ERR-04 rather than appended.
- R-ERR-05's rule text carries its own dated changelog ("Split out of R-ERR-03 on
  2026-08-31: …") inside the `<rule text>` slot of the ADR-0005 grammar.
- `tests/test_secrets.sh` still writes `/tmp/gl.$$` rather than `mktemp` — the other half of
  the round-1 taste note; and the tree scan's output is overwritten by the history scan before
  either is removed.
- `tests/test_boundaries.sh` asserts the broken-hook case as `( … ; [ $? -eq 2 ] )` followed by
  `if [ $? -eq 0 ]`; a direct `if ( … )` reads straighter.
- The `belay-debt:` comment marker in `tests/test_repo_shape.sh` is used but defined nowhere in
  `CLAUDE.md` or the spec.
- `find_clean03`'s exclusion is per line, so one line declaring both `bool m_is_ok` and
  `bool flag` is silently exempt.

### Iteration-4 escape — the spec is what is failing, not the code

`/validate-phase` §Failure modes: "Expected convergence is 1–2 iterations … if you're on
iteration 3+, the spec is wrong — stop and say so, and route it." This is round 4. Stating it,
with the evidence rather than the rule:

- Round 1: seven `contradicts`, all genuine code defects (an unenforced `python3` floor, two
  half-bound rules, a probe with unsanctioned flags). The gate did its job.
- Round 3: five `contradicts` — two real scope bugs, three document drift.
- Round 4: **one** code defect, three lines wide (finding 1). The other three findings are the
  document: a spec that contradicts itself (finding 2), a spec that cannot account for its own
  amendment (finding 3), and a sentence in `verify.md` (finding 4).

The code has converged. The spec has not, and the reason is mechanical: `spec.md` has been
patched in place eleven times across three rounds, and round 3's patch to step 5's body left
step 5's check line stale — the amendment loop is now manufacturing the defects the next round
reports. Two of this round's four `undecidable` findings ((a) and (d)) are also about a file
the spec can only point at, never own: `.claude/hooks/boundary-check.sh` belongs to the
workflow package, so its exit-code contract and its clean-clone availability are not facts
`spec.md` can assert without the operator deciding them first.

**Recommendation — an operator decision, not taken here.** The cut is sound: the phase built
the right thing, all twelve acceptance criteria pass, and the adversarial block catches every
rule by id. So this is a wrong *spec*, not a wrong cut, and the routing the command prescribes
is status → `pending` and `/expand-phase 00-scaffold` — rewriting the spec once, coherently,
against the code that exists, instead of patching it a twelfth time. Against that: the spec is
substantially correct, and a rewrite discards eleven rounds of hard-won pointers to fix three
sentences plus three missing accept cases. The cheaper path is one more implementation round
with an explicit brief — add the three accept cases, reconcile step 5's check line with the
Acceptance block, extend the workflow-written-files paragraph to cover `spec.md`, and answer
(a)–(d) as pointers — and to accept that the closure test will be self-declared if the review
budget runs out again. Both are defensible; the choice of which belongs to the operator, which
is why status stays `in-progress` and nothing was rewritten.

### Round 4 amendment — `spec.md` now accounts for itself

The workflow-written-files paragraph went from two paths to three: `docs/phases/00-scaffold/spec.md`
joins `docs/phases/PHASES.md` and `docs/index/`. The bullet states why — `/validate-phase`
prescribes a spec amendment as the fix for every `undecidable` finding, so a phase that goes
round the loop once carries its own spec in its diff from then on — and that the pairing is the
Deviations entry, not a Plan step. Closes round 4's second missing pointer. The other blocking
finding (three absent accept cases in `tests/test_repo_shape.sh`) is code and is untouched.

### Upstream fix — the workflow-written-files paragraph moved into the package

Deleted that paragraph. Belay `c1d4334` makes the exemption structural: `/validate-phase`
step 6 now names `docs/phases/<id>/spec.md`, `notes.md`, `docs/phases/PHASES.md` and
`docs/index/` exempt from the reachability requirement, so a spec no longer has to argue its
own case — and it covers `notes.md`, which the hand-written version missed even though it is
in every phase's diff from round one. Filed as entry 1 in `~/.claude-belay/feedback/`, fixed
upstream, this repo re-installed at belay `35fbcb0`. The phase's scope is unchanged: the same
paths are still not Plan steps; the statement now lives where it is enforced.

## Implementation — 2026-08-31 (round 5)

Round 4 ended with an operator decision open: re-cut the spec (`pending` + `/expand-phase`)
or one more implementation round with an explicit brief. Running `/implement-phase` is the
second choice, so this round executed that brief and nothing wider. Round 4's blocking
findings are closed; the code defect was three lines, the rest was the document.

Code — one change, the only code defect round 4 found:

- `tests/test_repo_shape.sh` — the three accept cases Plan step 5 names outright but that
  were never written: `<array>`, `<span>`, `<expected>` against `find_arch01`. Accept cases
  7 -> 10, and the assertion at the bottom with them. Without these, R-ARCH-01's hosted-only
  list was only ever shown to reject; nothing proved a freestanding header survives it.

`docs/phases/00-scaffold/spec.md` — four amendments, one per round-4 `undecidable`, plus the
self-contradiction round 4 found:

- **(a) and (d), Plan step 4.** `.claude/hooks/boundary-check.sh` and
  `.claude/workflow/boundaries.rules` are tracked repository content — verified with
  `git ls-files .claude/hooks .claude/workflow`, both are listed — so a clean clone has them
  and a missing or non-executable hook is a hard `FAIL`, not an `OPTIONAL_TOOLS` skip. The
  `PHASES.md` clean-clone promise is about external tools, not files the repo ships. The
  step now also states the hook's full exit-code contract (0 clean / 2 and only 2 a breach /
  anything else means the hook did not run) and requires the second rejection case that
  pins the third branch, which the code already had and the spec did not sanction.
- **(b), Plan step 8.** "Absent" is now defined per tool family, with the asymmetry stated as
  intent: `clang-format`/`clang-tidy` resolve through `PATH` plus the keg-only LLVM prefixes
  Homebrew leaves unlinked; `arm-none-eabi-*` is `PATH` only, because R-TOOL-02 is a claim
  about the binary first on `PATH` and probing elsewhere would hide the shadowing trap.
- **(c), Plan step 5.** R-ERR-04's grep matches every `.value(` under `src/`, wider than the
  catalogue's "on a `std::expected`". Recorded as the accepted price with its reason: a grep
  cannot see the receiver's type, and `std::optional::value()` fails identically (it throws;
  under `-fno-exceptions` that is `abort`), so the wider match forbids nothing this project
  wants. A user type with a `value()` accessor is the trigger to narrow it.
- **Finding 2, the counts.** Step 5's check line still said "eight `ok:` lines and eight
  rejection cases" while the Acceptance block said 10 rejection / 7 accept. Step 5's line now
  carries the real numbers (8 ok, 10 rejection, 10 accept) and says explicitly that where a
  count appears twice the two must agree. The Acceptance block moved 7 -> 10 accept cases,
  which is the code change above, and `test_boundaries.sh` now states 2 rejection cases
  rather than one — the second has existed since round 3 and the criteria never counted it.

`docs/phases/00-scaffold/verify.md` — round 4's finding 4, the one it recorded as weak rather
than blocking. "Three of the checks were silently broken when they were first written" is a
fact about the authoring session and rots the moment a fourth is fixed. Reworded to the
durable claim it was standing in for — broken checks here have only ever been caught by their
own rejection cases — with the running list delegated to this file.

`docs/index/` rebuilt: `test_repo_shape.sh` grew by three lines and `--check` cannot see
content staleness while the work is uncommitted (round 3's deviation, filed upstream).

Verified this round:

- acceptance criteria: 12/12 exit 0. `make test` OK, `make lint` 0, `test_repo_shape.sh`
  8 ok / 10 rejection / 10 accept, `test_boundaries.sh` 2 rejection cases,
  `test_tool_versions.sh` 4 rejection cases and all four tools real (clang-format 23.1.0,
  clang-tidy 23.1.0, arm-none-eabi-g++ 15.3.1, python3 3.14.6), traceability 9/9,
  `grep -c 'planned: 00-scaffold'` 0, `STYLE_OPTIONAL` 0 in both files
- adversarial block: 9/9 — the eight documented cases plus the `.hpp` one from round 3. Each
  exits non-zero naming its rule id; `src/` removed afterwards and `make test` back to 0
- project gates: test pass, lint pass, typecheck gap (no tool configured — unchanged)

## Deviations (round 5)

- **Five `spec.md` amendments, four of them the mandated fix for a round-4 `undecidable`;
  the fifth reconciles step 5's check line with the Acceptance block.** Detailed above. The
  Acceptance block's accept-case count moved 7 -> 10 — the one place a count was raised
  toward the code rather than the reverse. Step 5's body already named those four
  freestanding headers as needing accept cases, so the body demanded 10 and the criteria
  line was the stale half.
- **Round 4's taste notes were left alone deliberately.** `/tmp/gl.$$` in
  `test_secrets.sh`, the file modes, the `( … ; [ $? -eq 2 ] )` idiom, the R-ERR-05 rule
  ordering and its inline changelog, and `find_clean03`'s per-line exclusion are all
  unchanged. Round 4's own reading is that the code has converged and the spec has not;
  churning working checks for style on iteration 5 adds diff for a reviewer to read and
  fixes nothing a rule cares about. `find_clean03`'s per-line hole (one line declaring both
  `bool m_is_ok` and `bool flag` is exempt) is the only one with teeth and is recorded under
  Debt instead.
- **The escape recommendation was not re-litigated.** Round 4 laid out both routes and left
  the choice to the operator; invoking `/implement-phase` is that choice, so this round did
  the cheaper path and did not rewrite the spec.

## Validation — 2026-09-02 (round 5)

- criteria: 12 passed / 0 failed. Each checked against its stated expectation, not just its
  exit code: `make test` OK, `make lint` 0, traceability 9/9, boundaries 2 rejection cases,
  repo_shape 8 ok / 10 rejection / 10 accept, phase_docs 1 rejection case, secrets tree +
  history + rejection case, tool_versions 4/4 with five `R-TOOL` ok lines,
  `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both files, both docs non-empty.
  Adversarial block re-run this session: 9/9, each exiting non-zero naming its rule id, tree
  restored, `make test` back to 0
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: clean, and still vacuous by design — `boundaries.rules` holds 13 active
  `deny` lines, so the rules are not inert, but the file set contains no `.cpp`/`.h` at all
- index: fresh and content-fresh — `--check` says `index fresh (db97117)` and the numbers
  agree with the tree (`test_repo_shape.sh` 160/160, tests total 938/938). Round 3's
  content-staleness did not recur; the round-4 rebuild added `docs/index/scripts.md`
- independent review: **contradicts** — one confirmed finding, reproduced twice (below);
  undecidable — three missing pointers, recorded in Deviations
- closure test: **fail** — three `undecidable` findings are missing pointers by definition.
  The mechanical half passes: all four sections present and non-blank, and all ten
  non-exempt files in the set are reachable from the spec
- upstream: none new. `.claude/workflow/installed` is present and no finding this round
  belongs to a file it names — the confirmed defect is in this repo's own test wrapper, not
  in `.claude/hooks/boundary-check.sh`. The round-3 `build-index.sh --check` hole is already
  filed
- verdict: **returned to implementation** — a `contradicts` is a code failure, so this goes
  to `/implement-phase 00-scaffold`, with the three spec pointers written in the same round

### The confirmed `contradicts` finding

**`tests/test_boundaries.sh` reports a hook that cannot run as a successful rejection.**
Spec Plan step 4 (as amended in round 5): the hook's exit codes are a three-way contract,
`0` clean, `2` and only `2` a breach, any other non-zero means the hook itself did not run.
`sweep( )` encodes that correctly and the main run prints two distinct `FAIL:` lines for it.
The positive rejection case then discards the distinction:

```sh
if sweep "$tmp" >/dev/null 2>&1; then   # true only on rc 0
    echo "  FAIL: ... core including hal was accepted"
else
    echo "  ok:   R-ARCH-02 rejection case (core -> hal is refused)"
fi
```

rc 2 — "the hook did not run" — lands in the `else` and is announced as proof that the layer
is enforced. Reproduced twice: a stub `sweep` with a hook that always exits 3 takes the `ok`
branch, and a copy of the real `tests/test_boundaries.sh` with `HOOK` pointed at that stub
**exits 0 and prints all three `ok:` lines**. On today's tree `src/` holds no files, so the
main sweep returns 0 with `(no sources yet)`, the first rejection case is satisfied by the
broken hook, and the second passes by construction. A hook that cannot run therefore produces
a fully green R-ARCH-02 — the exact "a check that does nothing looks identical to a check
that works" failure the Goal exists to make impossible. The fix is the shape the second
rejection case already uses: `sweep "$tmp" >/dev/null 2>&1; [ $? -eq 1 ]`.

The reviewer's other `contradicts` — `notes.md` absent from the diff — is dismissed for the
fifth time: this file is withheld from the review on purpose, as step 5 requires.

### Note on the escape recommendation recorded in round 4

Round 4 concluded "the code has converged; the spec has not". **That conclusion is now
falsified.** Round 5's review found a real, blocking code defect — not document drift — in a
file round 3 had already rewritten, and it is the worst class of defect this phase can carry:
a check that passes while enforcing nothing. The iteration count is not by itself evidence
that only the spec is left to fix. Two things follow: the phase is not over-iterating for
nothing, and a starved independent review is still paying for itself on round 5.

## Deviations (round 5, validation)

- **Validation 2026-09-02 round 5 — three missing pointers.** (a) Whether `std::string_view`
  is legal under R-ARCH-03. `find_arch03` greps the bare alternation `std::string`, so
  `std::string_view sv;` under `src/` fires `FAIL: R-ARCH-03` — verified. The spec rules on
  exactly this trap one clause earlier, for R-ARCH-01's include check ("anchor the match on
  the closing `>` so `<string_view>` is not read as `<string>`") and mandates the matching
  accept case, but prescribes no pattern and no accept case for R-ARCH-03. The two halves now
  disagree with each other: the phase's own accept case declares `#include <string_view>`
  legal in `core` while R-ARCH-03 rejects using it. The spec must say whether a type that
  allocates nothing is in scope for a no-dynamic-allocation rule, and the grep must be
  anchored (`std::string\b` does not match `std::string_view`, since `_` is a word character)
  with an accept case to pin it.
  (b) Whether `.claude/hooks/boundary-check.sh` honours `CLAUDE_PROJECT_DIR`. The whole
  rejection-case design in `tests/test_boundaries.sh` rests on it — the hook stays at its real
  path and only the project dir moves. The spec's pointer describes the interface as "takes
  one file path, exits 2 on a violation" and says nothing about how the hook finds its rules
  file, so a reviewer cannot tell whether the fixture's `boundaries.rules` copy is load-bearing
  or dead weight. It is load-bearing: `.claude/hooks/lib/common.sh:22` sets
  `ROOT="${CLAUDE_PROJECT_DIR:-$PWD}"` and the hook reads `$ROOT/.claude/workflow/boundaries.rules`.
  The code is right; the pointer is missing.
  (c) A version floor for `gitleaks`. `tests/test_secrets.sh` hard-codes the `dir` and `git`
  subcommands, both sanctioned by Plan step 7, but R-TOOL-01's floor list covers only
  `clang-format`, `clang-tidy`, `arm-none-eabi-g++` and `python3`, and nothing probes gitleaks
  (8.30.1 here). A gitleaks old enough not to know `dir`/`git` exits non-zero and the check
  prints `FAIL: R-SEC-01: gitleaks found secrets in the tree` — a misdiagnosis of exactly the
  class Plan step 4 goes to such lengths to prevent for the boundary hook. The spec must
  either give gitleaks a floor or state that the misdiagnosis is accepted.

- **The hook's own header narrows the contract round 5 wrote into Plan step 4.**
  `.claude/hooks/boundary-check.sh` says a third exit code "is not produced, and none may be
  added: edit-gate-adapter.sh and scripts/check.sh treat any non-zero as a violation". So the
  package guarantees 0/2; the wrapper's third branch defends against a hook that is broken
  rather than one that reports a third code deliberately. The amendment stands — a bad
  interpreter or a missing dependency still produces a non-zero exit the wrapper must not read
  as a breach — but the wording should say "cannot run", not "returns a third code".

### Taste, not blocking (round 5)

- **The strongest of them: one of the ten accept cases is vacuous.**
  `accept "= delete is legal C++" find_arch03 src/core/x.h 'Bus( const Bus& ) = delete;'`
  never exercises the `=[[:space:]]*delete` exclusion it is named for — the base pattern
  requires `delete` followed by whitespace and an identifier, and the input has `delete;`.
  Verified: the base alternation matches that line 0 times. Remove the exclusion entirely and
  the case still passes. By this phase's own thesis that is not a check.
- `R-ERR-03`'s `try` grep is line-anchored (`\btry[[:space:]]*\{`), so a brace on the next
  line is missed; the paired `catch (` still catches the realistic case.
- `R-CLEAN-03` sees only `bool name;` / `bool name =` declarations — parameters and return
  positions are invisible. The spec prescribes no pattern, so this is not a miss against it.
- `tests/test_boundaries.sh` prints no `rejection cases: n/n` line, unlike the other three
  files; the Acceptance criteria assert a count that only two `ok:` lines confirm.
- `verify.md` §3 overwrites the same two scratch filenames four times with the cleanup only
  after the block, so an operator following it literally sees two of the four rules fire.
  §2 has the right shape (break, run, restore) and §3 does not repeat it.
- `tests/test_secrets.sh` still writes `/tmp/gl.$$` rather than `mktemp`, and the tree scan's
  output is overwritten by the history scan before either is read.
- `PATH="$stub_dir:$PATH" check_version …` — a variable assignment prefixing a shell
  *function* call persists in the current shell under POSIX rules. Harmless only because all
  four stub cases run after the real probes; nothing states that ordering.
- `tests/test_rule_traceability.py`'s marker sits in the module docstring without the `#`
  prefix Plan step 9 specifies. The checker matches the substring, and `#` inside a docstring
  would be odd Python.
