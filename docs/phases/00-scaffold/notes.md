# Phase 00-scaffold — notes

## Outcome

- base: `db97117` (branch `chore/00-scaffold`)
  Was `0ee8abd`. Bumped 2026-08-31 after `db97117` committed the belay 35fbcb0 update
  on its own: the seven package-owned files it carries are tracked here, so leaving the
  base behind that commit would keep them inside this phase's file set — in the diff the
  independent review reads and in the closure test's reachability check. The phase's own
  work is unchanged and still uncommitted; only the baseline it is measured from moved.
  **Held at `db97117` for round 8 (2026-09-02), deliberately.** Rounds 6 and 7 are now
  committed (`1a48543`, `b35395a`), so bumping the base to `HEAD` would shrink the review
  diff to this round's four files and hide the rest of the phase from the independent
  reviewer — which is precisely how round 7 found the `test_secrets.sh` defect in a file
  round 7 never touched. The base moves for package commits, not for the phase's own work.
  Held at `db97117` again for round 9, for the same reason and with the same known cost:
  seven package-owned files ride along in the file set. That cost is filed upstream
  (`~/.claude-belay/feedback/pico-sg2hid.md`, `commands/validate-phase.md`, open) rather
  than paid again by moving the base and blinding the reviewer to the phase's own work.

Seven new files under `tests/` (six checks plus the round-9 liveness harness), all picked up by `make test` with no Makefile edit:

| file | rules |
|---|---|
| `tests/test_rule_traceability.py` | R-PROC-01 |
| `tests/test_repo_shape.sh` | R-ARCH-01, R-ARCH-03, R-ERR-03, R-ERR-04, R-CLEAN-03, R-CLEAN-05, R-CLEAN-09, R-PROTO-05 |
| `tests/test_boundaries.sh` | R-ARCH-02 |
| `tests/test_phase_docs.sh` | R-PROC-02 |
| `tests/test_secrets.sh` | R-SEC-01 |
| `tests/test_tool_versions.sh` | R-TOOL-01, R-TOOL-02 |
| `tests/test_checks_are_live.py` | none — it checks the six above (added round 9) |

plus `tests/fixtures/incomplete_check.sh`, the harness's bootstrap floor, which is data the harness reads and deliberately not a `tests/test_*` file: the Makefile glob would otherwise run it as a check, and it is designed to be incomplete.

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

### Round 8 validation — the five confirmed `contradicts`, and what they share

Recorded here rather than only in the round-8 validation record because `/expand-phase`
reads this section first, and the re-expansion has to answer them as one question, not five.
Every mutation below was run against the tree as it stands; each removes the named substring
from the named file and leaves the whole suite exiting **0**. Reproduce with:

```
python3 - <<'EOF'
import pathlib, subprocess
f, old, new = 'tests/test_repo_shape.sh', '<substring from the table>', ''
src = pathlib.Path(f).read_text(); assert old in src
pathlib.Path('tests/mut.sh').write_text(src.replace(old, new, 1))
print(subprocess.run(['sh','tests/mut.sh']).returncode)   # 0 == the mutation survived
EOF
rm -f tests/mut.sh
```

| # | file | substring removed | result |
|---|---|---|---|
| F1 | `test_repo_shape.sh` | `hardware/\|` | rc=0 — a `hardware/gpio.h` include in `core` becomes invisible |
| F1 | `test_repo_shape.sh` | `vector\|` | rc=0 — and the spec's own adversarial line `#include <vector>` stops firing |
| F1 | `test_repo_shape.sh` | `\|\bcatch[[:space:]]*\(` | rc=0 — the one case still matches via `\btry` |
| F1 | `test_repo_shape.sh` | `:[[:space:]]*(public\|private\|protected)[[:space:]]\|` | rc=0 — both cases still match via the `(struct\|class)…:` form |
| F3 | `test_repo_shape.sh` | the whole `report R-ERR-04 "$( find_err04 "$1" )"` line | rc=0, 7 `ok:` lines — the rule is simply not run |
| F3 | `test_repo_shape.sh` | `if [ -n "$2" ]; then` → `if false; then` | rc=0 — **all eight** rules unenforced |
| F2 | `test_secrets.sh` | `scan git "$ROOT"` → `scan dir "$ROOT"` | rc=0 — the history clause silently unbound |
| F2 | `test_secrets.sh` | the entire real `if scan git "$ROOT" …` block | rc=0 — the scan does not run at all |

The control matters as much as the mutations: removing `iostream\|` **is** caught (rc=1),
because that alternation is one of the two `find_arch01` has a case for. The suite catches
exactly the alternations that were enumerated by hand and nothing else — which is the shape
of the defect, not an accident of which ones were tried.

- **F1 — the §Goal claim outran the code.** Round 8 wrote "every alternation in a pattern
  needs a case that fires on it alone" into §Goal and "deleting any one alternation from any
  finder must make this file exit non-zero" into Plan step 5, then delivered cases for the
  seven alternations round 7 had happened to name. `find_arch03` has 7 alternations and 7
  cases; `find_arch01` has 31 and 2; `find_err03` has 3 and 1; `find_clean09` has 3 and 3 but
  two of them reach the same alternation. Step 5 enumerates 18 names, so `18 >= 18` conforms
  to the enumeration while contradicting the check line directly above it.
- **F2 — binding the function is not binding the call.** Round 8 routed both scans through
  `scan()` and proved the *function* cannot be gutted. Nothing proves the function is ever
  called on `$ROOT`. The comment left in the file ("Delete the `scan git` call above and this
  is what stops it going unnoticed") is false as written.
- **F3 — the same defect one layer up, in every file.** The rejection and accept machinery
  calls the finders directly (`$2 "$reject_tmp"`), so it validates a *pattern against a
  fixture* and never that the pattern is run against the real tree. `run_all`, `report()` and
  `check_version`'s call sites are unprotected in all six files.
- **F4 — `verify.md` now teaches this as a guarantee.** Its round-8 paragraph tells a
  non-specialist that a shared function is "what makes an `ok:` line evidence that something
  ran". Per F3 it is evidence that the *function* works. R-PROC-02 is about the operator
  being able to judge these files.
- **F5 — `ok:` on a shortfall survives in the two files round 8 did not touch.**
  `tests/test_tool_versions.sh:175` and `tests/test_rule_traceability.py:270` print
  `ok:   … rejection cases: n/m` unconditionally, before the comparison that sets the
  failure. Round 8 fixed the two files it was already editing and wrote the principle into
  step 5's check line, where it reads as scoped to `test_repo_shape.sh`.

**What the five share.** Every one is the same sentence: *a check can report success while
the thing it names never ran.* Rounds 4 through 8 each found one instance and fixed that
instance. F1, F2 and F5 are instances; F3 is the class. A sixth round of enumerated cases
buys one more instance and leaves the class intact.


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
- `belay-debt:` in `tests/test_checks_are_live.py` — the `.py` check under `tests/` is held
  to the accounting property only. Its internals are never mutated, so its nine failure modes
  rest on nine hand-written rejection cases, which round 9 established is the weaker form.
  Upgrade when a rule arrives whose check is not a grep; owner `01-ps2-codec`.
- The liveness harness discovers check functions by naming convention (`find_*`, `check_*`,
  `sweep`, `scan`, `missing_verify`). A check function named outside it is not mutated. The
  mitigation is that the discovered set is printed on every run, so the omission is visible —
  but it is a convention, not a guarantee, and a future check that invents a new name gets no
  coverage until someone reads that line. Owner: whichever phase adds such a check.
- `tests/fixtures/incomplete_check.sh` is the harness's floor, not a regress: it proves the
  harness still detects a gap, not that it detects *every* gap. Nothing tests the fixture
  itself. That is the accepted end of the tower.

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
- **Whoever re-expands `00-scaffold`** — the bullet above ("the pattern to copy is a check
  function that takes the tree root as an argument, plus a rejection case…") is **known to be
  insufficient** as of round 8, and following it verbatim in `01-ps2-codec` would reproduce
  this phase's defect in new code. It makes a pattern testable against a fixture. It does not
  make the *check* testable: nothing in that pattern fails when the call in `run_all` is
  deleted, when `report()`'s failure branch is gutted, or when the real scan is pointed
  somewhere else. See §Deviations "Round 8 validation" for the eight mutations that
  demonstrate it. The re-expansion's central question is that wiring layer — what ties a
  check to the real run — and the answer has to be one mechanism, not a longer list of
  rejection cases. Two shapes worth weighing when the spec is rewritten, neither yet chosen:
  a self-test that mutates the suite's own files and requires a non-zero exit (the thing
  every round has done by hand, made into a check), or an accounting property the suite
  asserts about itself — every declared rule id produced a result line this run, so a
  deleted call site is a missing rule rather than a silent pass.
- **Whoever re-expands `00-scaffold`** — the §Goal must not promise more than the Plan
  delivers. "A negative self-test on every check" has been in §Goal since round 1 and has
  never been true at the granularity the same document claims; that gap is where the
  `contradicts` findings of rounds 4, 7 and 8 all came from. Either lower the Goal to what
  the phase actually ships and record the rest as declared debt with an owning phase, or add
  the Plan step that raises the code to it. Leaving the two out of step is what makes the
  phase unvalidatable — the reviewer is handed a spec that its own diff cannot satisfy.
- **Any phase adding a rule** — a rule's binding is only as good as the weakest link in
  `catalogue → check → call site → real tree`. This phase bound the first three links and
  never the fourth, and `make test` stayed green throughout. When `01-ps2-codec` moves
  R-PROTO-01..04 to `test:`, the question to ask of each is not "does the check catch a bad
  vector" but "what would have to break for this check to stop running without anyone
  noticing".

- **Any phase adding a check** — three requirements, all of which round 9 established by
  finding their absence, and all of which `tests/test_checks_are_live.py` or the check file
  itself will now enforce:
  1. Declare every rule the file checks as `# RULE <id>` in its header. The harness requires
     each to produce a result line, so a call site that disappears becomes a build failure.
  2. If one rule is checked by more than one independent real-run call, declare
     `# LIVE <id><literal prefix of that call's result line>` for each. R-SEC-01 (working
     tree, history) and R-TOOL-01 (four tool probes) are the worked examples. Without this,
     deleting one of the calls is invisible.
  3. Route rejection and accept cases **through** the file's reporting function, and give
     each rule a wiring case driving the real aggregate. A case that calls the finder
     directly leaves both the verdict logic and the exit-code path load-bearing for nothing —
     holes 3 and 4 of round 9.
- **Any phase adding an alternative to an existing check pattern** — the harness will fail
  the build on the next run naming that alternative, because nothing tests it yet. That is
  working as intended; add the rejection case it names. Cost per alternative is one line, and
  it is deliberate: a forbidden form nobody demonstrates is a forbidden form nobody enforces.
- **`01-ps2-codec` onwards** — `make test` now takes about a minute (67 generated mutants,
  run in a thread pool) against ~2s before. The spec caps it at two minutes. When that starts
  to bind, the lever is the number of mutants, not the parallelism: mutating only the check
  files a commit touched would cut it, at the cost of the guarantee being per-commit rather
  than absolute. Not needed yet.

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

## Implementation — 2026-09-02 (round 6, after re-expansion)

Round 4 laid out two routes and round 5 falsified its own "the code has converged" reading.
The operator took the escape route this time: status back to `pending`, `/expand-phase`
rewrote `spec.md` once from the row's Goal and from this file rather than patching it a
twelfth time, and this round implemented the result. The rewritten spec carries a **Plan
step 0** listing the five edits the code owed; that is what this round did, and nothing
wider.

Structural change in the spec, worth knowing before reading the diff: **counts are now
floors over a named enumeration** (`rejection cases: 11 (floor 10)`), because an exact count
written in a Plan step and again in the Acceptance block is what produced three separate
findings across rounds 2-5. A floor cannot contradict its own list.

Code:

- `tests/test_boundaries.sh` — round 5's confirmed `contradicts`. The positive rejection
  case was `if sweep "$tmp"; then FAIL else ok`, so `sweep`'s rc 2 ("the hook did not run")
  was announced as proof that the layer is enforced. It now asserts the exact code
  (`rc -eq 1`) and reports what it got. Verified the way round 5 found it: a copy of the
  file with `HOOK` pointed at a stub exiting 3 now **exits 1** and prints
  `core including hal returned 2, expected 1`, where before it printed three `ok:` lines and
  exited 0. The file also prints `rejection cases: n/n` and asserts it, which the other three
  files did and this one did not — round 5's taste note that the acceptance criteria asserted
  a count nothing printed. The second case's `( … ; [ $? -eq 2 ] )` / `if [ $? -eq 0 ]` pair
  became a plain `if ( … )` while that block was being touched.
- `tests/test_repo_shape.sh` — R-ARCH-03's `std::string` alternation is anchored
  (`std::string([^_[:alnum:]]|$)`), so `std::string_view` no longer fires a
  no-dynamic-allocation rule that the phase's own R-ARCH-01 accept case already declares
  legal in `core`. Two cases pin it in opposite directions: an accept case
  (`std::string_view sv;`) and a **new rejection case** (`std::string s;`), because anchoring
  a pattern can silently kill the match it exists for and nothing would have caught that.
  Rejection 10 -> 11, accept 10 -> 11.
- `tests/test_repo_shape.sh` — the `=[[:space:]]*delete` exclusion is gone; see Deviations.
- `tests/test_secrets.sh` — a gitleaks too old for the `dir`/`git` subcommands used to exit
  non-zero and be reported as `FAIL: R-SEC-01: gitleaks found secrets in the tree`, the same
  class of misdiagnosis Plan step 4 goes to lengths to prevent for the boundary hook. Now
  each subcommand is probed with `gitleaks <sub> --help` (0 when known, 1 when not, measured
  on 8.30.1) and a failure says `too old to scan with`. Verified with a stub gitleaks that
  rejects both subcommands: `FAIL: … has no 'dir' subcommand`, exit 1. Deliberately **no**
  R-TOOL-01 version floor for gitleaks — the floor would need a release number nobody here
  has measured, and the capability probe answers the question the floor was for. The two
  scans also stopped sharing `/tmp/gl.$$`: each gets its own `mktemp`, so the history scan no
  longer erases the tree scan's findings before they are printed. That closes the remaining
  half of the round-1 taste note.
- `docs/phases/00-scaffold/verify.md` §3 — four "try a few more" cases wrote two filenames
  with a single cleanup after the block, so an operator pasting them literally saw two of the
  four rules fire. Now one break/run/restore per rule, matching §2 and the spec's adversarial
  block, and it says why the order matters.

Verified this round:

- acceptance criteria 12/12: `make test` OK, `make lint` 0, traceability 9/9, boundaries
  `rejection cases: 2/2`, repo_shape 8 ok / 11 rejection / 11 accept, phase_docs 3 ok lines,
  secrets tree + history + rejection + accept, tool_versions four tools real
  (clang-format 23.1.0, clang-tidy 23.1.0, arm-none-eabi-g++ 15.3.1, python3 3.14.6) and 4/4
  rejection cases, `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both files, both docs
  non-empty
- adversarial block 12/12, each producing a `FAIL:` line **that names its rule**, tree
  restored, `make test` back to 0. The first run of that check was itself wrong and is worth
  recording: it grepped the output for the bare rule id, which also matches the `ok:` line
  the same check prints, so a case that failed for an unrelated reason would have read as a
  pass. Re-run with `grep -E "FAIL:.*<id>"`. Same failure mode as the phase's own thesis, one
  level up
- the accept case for `= delete;` is load-bearing now, checked by breaking it: with the
  pattern tightened to a bare `delete` word match the case fires and the file fails
- `docs/index/` rebuilt (stamp `c8d5eff`); the test files changed line counts and `--check`
  cannot see content staleness while the work is uncommitted (round 3's deviation, already
  filed upstream)

## Deviations (round 6)

- **Plan step 0c's suggested input does not work, and the exclusion it defends is
  unreachable.** The spec offered `Bus& operator=( const Bus& ) = delete;` as an input the
  base pattern would match; it does not — the alternation requires `delete` followed by
  whitespace and an identifier, and every `= delete` form ends `delete;`. There is no legal
  C++ line that matches the base pattern and needs the `=[[:space:]]*delete` exclusion, so
  the exclusion was dead code defending against a match that cannot happen. Took the step's
  other branch: the exclusion is removed and the accept case kept, which now exercises the
  base pattern's own "whitespace plus identifier" requirement — tighten that to a bare
  `delete` word match and the case fires. `spec.md` step 0c amended to say this instead of
  the input that does not work.
- **One acceptance criterion was impossible as written, and was amended rather than
  deleted.** The adversarial block's positive case ran
  `printf '#include <string_view>\nstd::string_view sv;\n' > src/core/x.h ; make test` and
  expected exit 0. It cannot pass: clang-tidy on this machine resolves **no** standard header
  without a `compile_commands.json`, so any scratch file containing `std::` or an
  `#include <…>` fails R-STYLE-02 with `error: 'string_view' file not found
  [clang-diagnostic-error]`. That is a tooling result and says nothing about R-ARCH-03. The
  criterion now runs `sh tests/test_repo_shape.sh` over the same file — which is where
  R-ARCH-03's accept case lives — and the spec states the limitation and points here.
  Measured invocation in §For later phases.
- **`docs/phases/PHASES.md`'s coarse row was corrected during the re-expansion**: "the eleven
  rules marked `planned: 00-scaffold`" -> "the fourteen rules". Rounds 1-5 left it stale on
  the reading that rows are append-only, and it produced an `undecidable` finding twice.
  `CLAUDE.md`'s append-only rule is about a wrong *cut* — a cut is superseded, never edited —
  and `/expand-phase` §Failure modes explicitly allows sharpening a row whose coarse text has
  drifted. The cut itself is unchanged. Flagged to the operator, who can have it reverted.
- **No missing Context pointers.** Everything this round needed was reachable from the
  rewritten spec. Files touched that no Plan step names: `docs/index/` only, which
  `/validate-phase` step 6 exempts.

## For later phases (added round 6)

- **`01-ps2-codec` — R-STYLE-02 will fail on the phase's very first header, and it is not a
  naming problem.** `tests/test_style.sh` invokes `clang-tidy --quiet <file> -- -std=c++23
  -Isrc`. Homebrew's clang-tidy 23 finds no standard header that way: a file containing
  `#include <cstdint>` reports `error: 'cstdint' file not found [clang-diagnostic-error]`,
  and `WarningsAsErrors: '*'` turns that into a `FAIL: R-STYLE-02` line. Measured 2026-09-02
  on this machine: `-isysroot $(xcrun --show-sdk-path)` alone does **not** fix it; adding the
  toolchain's own libc++ headers does —

      clang-tidy --quiet <file> -- -std=c++23 -Isrc \
        -isysroot "$(xcrun --show-sdk-path)" \
        -I/opt/homebrew/opt/llvm/include/c++/v1

  ...runs clean on `#include <cstdint>` + `using Byte = std::uint8_t;`. Two things follow.
  (1) `01-ps2-codec` must fix the invocation before it can pass `make lint`. Three flags are
  enough **on this machine**, and that is the catch: the third is
  `-I/opt/homebrew/opt/llvm/include/c++/v1`, a Homebrew-on-Apple-Silicon path. Hardcoding
  it fixes one laptop and breaks `make lint` on every other, which contradicts this
  phase's own goal of a green run from a clean clone. There is no CI to catch that today.
  So the choice — hardcoded flags versus a generated `compile_commands.json` versus
  detecting the include root — is owed by that phase and is not settled by this note. (2) `docs/constraints.md` §Style and the header
  of `.clang-tidy` both say clang-tidy can run where "the flags are trivial — `src/core/` and
  `tests/`"; that is false today for any file using the standard library, which is every file
  that phase will write. It is a **finding** in `CLAUDE.md`'s sense — the obvious reading of
  the config is wrong, and it cost a measurement to establish — and belongs in
  `docs/constraints.md` §Observed conventions with its date. Not written there this round:
  amending the catalogue's prose is outside this phase's Plan, which only flips bindings.
  Proposed to the operator instead.
- **`03-pio-bus`** — unchanged from earlier rounds: R-ERR-05 must move from `planned:` to
  `test:` when the CMake config lands, and the `clang-query` upgrade for
  `tests/test_repo_shape.sh` rides along with the `compile_commands.json` that phase produces.

- escaped to /expand-phase: spec re-expanded — **recorded retroactively 2026-09-02.** No
  gate wrote this line at the time: the escape was applied out of band between this round
  and round 6 (status set back to `pending` by hand, then `/expand-phase 00-scaffold`),
  before `/validate-phase` either performed or recorded that flip. It is written here in the
  vocabulary belay `5aa5a3c` counts, so the iteration budget resets at this point rather
  than running from round 1: round 6 is validation #1 against the re-expanded spec.

## Validation — 2026-09-02 (round 6)

- criteria: 12 passed / 0 failed. Each checked against its stated expectation, not just its
  exit code: `make test` OK, `make lint` 0, traceability 9/9, boundaries `rejection cases:
  2/2`, repo_shape 8 rule `ok:` lines + 11 rejection (floor 10) + 11 accept (floor 11),
  phase_docs rejection case confirmed, secrets tree + history + rejection + accept,
  tool_versions 4/4 with five `R-TOOL` ok lines, `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0
  in both files, both docs non-empty. Adversarial block re-run this session: 12/12, each
  producing a `FAIL:` line **naming its rule** (grepped as `FAIL:.*<id>`, not the bare id —
  the bare id also matches the `ok:` line the same check prints), tree restored, `make test`
  back to 0. Positive case green
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
  (`scripts/check.sh` must be run as `./scripts/check.sh`, not `sh scripts/check.sh`: it is
  `#!/usr/bin/env bash` and uses process substitution, which dash rejects at line 99.)
- boundary sweep: **not swept: no file in the set is under a declared layer.** Checked both
  other reasons a sweep comes back clean: `boundaries.rules` holds 13 active `deny` lines, so
  the rules are live, and the declared prefixes are `src/core|hal|usb|app|emu/` — the phase's
  file set is `tests/`, `docs/`, `Makefile` and package files, none of which any prefix
  covers. Ordinary for a test-only phase, and no code change could alter it
- index: fresh (`c8d5eff`) **and content-fresh**, verified against the tree rather than
  trusting the stamp: all seven `tests/` line counts in `docs/index/tests.md` match
- independent review: **contradicts — one finding, reproduced (below); undecidable — two
  missing pointers, recorded in Deviations.** The reviewer also ran its own replica of every
  finder, mutation-tested all eleven accept cases (each flips to a failure when its exclusion
  or anchor is removed — no vacuous accept case survives, which was round 5's strongest taste
  note) and confirmed the traceability checker reports 0/9 when its `check()` is stubbed out
- closure test: **fail** — two `undecidable` findings are missing pointers by definition. The
  mechanical half passes: all four sections present and non-blank, and all ten non-exempt
  files in the set are named in the spec
- upstream: **`.claude/commands/validate-phase.md`** — `/belay-feedback` recommended. Both
  undecidables are about files belay `c1d4334` made *structurally* exempt in step 6
  (`docs/phases/PHASES.md`, `docs/index/`). The exemption is real, but it lives in a command
  file the starved reviewer never receives, so every phase in this workflow will keep
  producing these two findings forever — the statement was moved to where it is enforced and
  away from where it is read. The round-3 `build-index.sh --check` content-staleness hole is
  already filed
- verdict: **returned to implementation** — a `contradicts` is a code failure, so this goes to
  `/implement-phase 00-scaffold`, with the two spec pointers written in the same round

### The confirmed `contradicts` finding

**`tests/test_boundaries.sh:13` assigns `HOOK` unconditionally, so Plan step 0's own
verification command reports the opposite of the truth.** Spec step 0's check line says: with
`HOOK` pointed at a stub that always exits 3, the file must exit non-zero. Reproduced:

```
$ HOOK=/tmp/stub3 sh tests/test_boundaries.sh
  ok:   R-ARCH-02 (no sources yet)
  ok:   R-ARCH-02 rejection case (core -> hal is refused)
  ok:   R-ARCH-02 rejection case (a hook that cannot run is not a violation)
  rejection cases: 2/2
rc=0
```

`HOOK="$ROOT/.claude/hooks/boundary-check.sh"` discards the environment value, so the stub is
never used and the run is the ordinary green one.

Two things keep this from being a repeat of round 5's defect, and both matter for the fix.
The *substance* of step 0a is delivered: `sweep` really does return 1 for a breach and 2 for a
hook that cannot run, both in-file rejection cases assert those exact codes, and the file does
exit non-zero when the hook is genuinely broken — which is how it was verified during
implementation, with a `sed`-patched copy rather than an environment variable. What is broken
is that the spec prescribes an *externally driven* verification the code cannot support, so
anyone running the check literally concludes the check is dead when it is not. The fix belongs
on the code side because it makes the spec's check real:
`HOOK="${HOOK:-$ROOT/.claude/hooks/boundary-check.sh}"` — one line, and the second rejection
case's subshell override keeps working unchanged.

### Deviations found by validation (round 6) — two missing pointers

- **(a) The spec does not say whether `docs/phases/PHASES.md` may be edited, and this round
  edited its row.** `/expand-phase` corrected the coarse acceptance text from "the eleven
  rules" to "the fourteen rules" alongside the status flips. No Plan step names the file;
  `CLAUDE.md` says a cut "is superseded by new rows, never edited or deleted"; the spec's own
  header says the code is what changes where the two disagree, and the stale row is not one of
  step 0's five owed edits. All three readings are defensible from the documents given, which
  is the definition of undecidable. The spec must state the rule: a row's *cut* is
  append-only, a row's coarse acceptance *text* may be corrected when it has drifted from the
  spec's Goal (`/expand-phase` §Failure modes allows exactly that), and status flips are
  written by the workflow commands.
- **(b) The spec does not say the repo index is regenerated inside the phase.**
  `docs/index/_overview.md`, `scripts.md` and `tests.md` are in the diff and no Plan step
  mentions them. `/validate-phase` step 6 exempts `docs/index/` — but that exemption is
  invisible to a reviewer holding only `CLAUDE.md` and `spec.md`. One line in the spec fixes
  it for this phase; the general case is the `upstream:` note above.
  The reviewer additionally observed that `docs/index/scripts.md` reports
  `scripts/build-index.sh` growing 231 -> 276 lines while that file appears nowhere in the
  diff. That is an artefact of how the review input was cut, not a defect: five package files
  (`.claude/commands/expand-phase.md`, `.claude/commands/validate-phase.md`,
  `.claude/hooks/boundary-check.sh`, `.claude/workflow/belay-version`,
  `scripts/build-index.sh`) changed between the base `db97117` and now via `chore(belay):`
  commits `7f1b2c9`, `98da73a` and `c8d5eff`, and were excluded from the reviewer's diff as
  not being this phase's work. Recorded so the next round does not chase it.

### Taste, not blocking (round 6)

- **The strongest, and it is a correctness problem in prose rather than a preference:**
  `verify.md` §3 still explains the hazard that step 0e removed — "two of these write the same
  filename, so if you paste all four at once only the last one is still on disk". Every line
  now ends in its own `rm -f`, so pasting all four works fine and all four fire. A false
  sentence in the operator-facing document R-PROC-02 exists to keep honest; worth one edit
  next round.
- Plan step 5's check line says `tests/test_repo_shape.sh` prints "eight `ok:` lines". It
  prints ten: the two count lines carry the same `ok:` prefix. The Acceptance criteria wording
  ("8 rules ok") is right; a mechanical `grep -c 'ok:'` against the Plan's number would not be.
- The `belay-debt:` comment in `tests/test_repo_shape.sh` names its upgrade path but not its
  owning phase; §Out of scope names `03-pio-bus` as the earliest landing point.
- `tests/test_rule_traceability.py` parses the whole of `docs/constraints.md` rather than
  restricting to §Invariants as the Goal says. It can only over-report, never under-report —
  a `- **R-…**` line added to another section would be flagged unparsable.

## Implementation — 2026-09-02 (round 7, after the round-6 validation)

Round 6's validation returned one `contradicts`, two `undecidable` missing pointers and one
taste note it classed as "a correctness problem in prose". All four are closed. No check
changed what it catches this round; the adversarial block was re-run anyway because
`tests/test_boundaries.sh` was touched.

- **The `contradicts` — `tests/test_boundaries.sh:13`.** `HOOK` is now
  `HOOK="${HOOK:-$ROOT/.claude/hooks/boundary-check.sh}"`, with a two-line comment saying
  why it is overridable. Spec step 0's check line is now literally runnable and reports the
  truth: `HOOK=<stub exiting 3> sh tests/test_boundaries.sh` prints
  `FAIL: R-ARCH-02 rejection case did not fire — core including hal returned 2, expected 1`,
  `rejection cases: 1/2`, and exits 1. Unset, the file is green at `2/2`. The substance of
  step 0a was already correct — only the externally driven verification of it was impossible.
- **Missing pointer (a) and (b) — one new spec subsection.** `spec.md` §"Files this phase
  writes that no Plan step names", placed before §Context pointers, states both: that
  `docs/phases/PHASES.md` status flips are written by the workflow commands and a row's
  coarse acceptance *text* may be corrected when it has drifted from the Goal while the
  row's *cut* stays append-only; and that `docs/index/` is regenerated inside the phase and
  is structurally exempt in `/validate-phase` step 6. Both were undecidable from
  `CLAUDE.md` + `spec.md` alone, which is precisely what the subsection removes. The general
  fix — the exemption living in a command file the starved reviewer never receives — stays
  the `upstream:` item filed in round 6.
- **The prose defect in `verify.md` §3.** The sentence claiming two of the four lines share
  a filename and that pasting them together silences three of them was false once step 0e
  gave every line its own `rm -f`. Replaced with what the block actually does, and the claim
  was then verified rather than asserted: pasting all four at once produces
  `FAIL: R-ARCH-01`, `FAIL: R-ARCH-02`, `FAIL: R-CLEAN-03`, `FAIL: R-CLEAN-05` — all four
  fire.
- **One gap found while verifying that paste, not reported by validation.** §3's first two
  lines put an `#include` in a `.h`, so each also prints
  `FAIL: R-STYLE-02 / R-CLEAN-02 … error: '<header>' file not found [clang-diagnostic-error]`
  — the clang-tidy-without-`compile_commands.json` limitation §Acceptance criteria already
  documents. §2's example (`new int;`, no include) never shows it, so the doc had no reason
  to mention it and an operator working through §3 literally met an unexplained second
  failure. One paragraph added to §3 naming it, saying to ignore it, and pointing at
  `03-pio-bus` as when it goes away. R-PROC-02 is about the operator being able to judge the
  output, and an unexplained `FAIL:` defeats that.
- **Two round-6 taste notes closed in passing**, both single-line and neither changing
  behaviour: Plan step 5's check line said `test_repo_shape.sh` prints "eight `ok:` lines"
  when it prints ten (the two count lines carry the same prefix) — reworded to "one `ok:`
  line per rule (eight rules, plus the two count lines…)"; and the `belay-debt:` comment in
  `tests/test_repo_shape.sh` now names `03-pio-bus` as the earliest phase that can land the
  clang-query upgrade, matching §Out of scope.
- **The remaining round-6 taste note is deliberately not closed.**
  `tests/test_rule_traceability.py` still parses the whole of `docs/constraints.md` rather
  than restricting to §Invariants. It can only over-report — a `- **R-…**` line outside
  §Invariants would be flagged unparsable, never missed — so narrowing it trades a
  fail-loud behaviour for a fail-quiet one. Left as is on purpose.

Acceptance: 12/12 criteria pass. Adversarial block re-run: 12/12 caught, each by a
`FAIL:` line naming its rule id, tree restored and `make test` back to `OK` after every
case and at the end.

## Deviations (round 7)

- **`spec.md` was amended, not just implemented against.** The two `undecidable` findings
  were spec gaps by construction — no code change could close them — so the round adds one
  subsection to `spec.md` and corrects one stale count in Plan step 5. Recorded here because
  `/implement-phase` requires an amended spec to say so in the notes. Nothing in the Goal,
  the Plan's substance or the Acceptance criteria changed; no criterion was deleted or
  weakened.
- **One edit nobody asked for: the clang-tidy paragraph in `verify.md` §3.** It came out of
  verifying the sentence that *was* asked for. Recorded as a deviation rather than folded in
  silently, since it is the only change this round that no finding named.
- Files touched that the spec's Plan steps name: `tests/test_boundaries.sh` (step 0a),
  `tests/test_repo_shape.sh` (comment only), `docs/phases/00-scaffold/verify.md` (steps 0e,
  10), `docs/phases/00-scaffold/notes.md` (step 11). Plus `docs/phases/00-scaffold/spec.md`
  itself, per the first bullet.

## Validation — 2026-09-02 (round 7)

- criteria: 12 passed / 0 failed. Each checked against its stated expectation, not just its
  exit code: `make test` OK, `make lint` 0, traceability 9/9, boundaries `rejection cases:
  2/2`, repo_shape 8 rule `ok:` lines + 11 rejection (floor 10) + 11 accept (floor 11),
  phase_docs rejection + false-positive confirmed, secrets four `ok:` lines,
  tool_versions 4/4 with five `R-TOOL` ok lines, `planned: 00-scaffold` 0, `STYLE_OPTIONAL`
  0 in both files, both docs non-empty. The negative half of the adversarial block was run
  here (`std::string_view sv;` → repo_shape exit 0, R-ARCH-03 does not fire); the twelve
  positive cases were run in this session's implementation half, after the last code
  change, 12/12 caught by a `FAIL:` line naming the rule — and the independent reviewer
  re-ran all twelve itself and agrees
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** Both other
  reasons a sweep comes back clean were checked, not assumed: `boundaries.rules` holds 13
  active `deny` lines, so the rules are live; the declared prefixes are
  `src/{core,hal,usb,app,emu}/` and this phase's file set is `tests/`, `docs/`, `Makefile`
  and package files, which none of them covers. Ordinary for a test-only phase
- index: was **STALE** (`tests/test_boundaries.sh`, `tests/test_repo_shape.sh` changed after
  the last build) — rebuilt, stamp `5e4f119`, `--check` now clean
- independent review: **contradicts — one finding, reproduced and found worse than reported
  (below); undecidable — four, three of them real spec gaps.** The reviewer also
  mutation-tested every check: all 8 repo_shape finders, all 9 traceability failure modes,
  both boundaries mappings, all 4 tool_versions cases and phase_docs' `missing_verify` fail
  when gutted, and all 11 accept-case exclusions are load-bearing. Step 0b and 0c were
  confirmed genuinely fixed rather than cosmetically
- closure test: **fail** — three undecidables are missing pointers by definition. The
  mechanical half passes: all four `notes.md` sections present and non-blank, and all ten
  non-exempt files in the set are named in the spec
- upstream: **`.claude/commands/validate-phase.md`** (in `.claude/workflow/installed`) —
  `/belay-feedback` recommended. The iteration-3+ escape says to count the `## Validation`
  sections in `notes.md`; there are six, so read literally the escape fires again. But it
  already fired once at round 5, and the round-6 spec *is* its output — against that spec
  this is validation #2, inside the expected 1-2 convergence, and round 6 returned a
  one-line code defect rather than a spec collapse. The counter never resets after a
  re-expansion, so any phase that escapes once is permanently in escape territory and would
  be re-expanded forever. Judged by the current spec's iteration count instead, and stated
  here rather than applied silently. Round 6's `upstream:` item (the step-6 exemption living
  in a command file the starved reviewer never receives) is unchanged and still open
- verdict: **returned to implementation** — a `contradicts` is a code failure, so this goes
  to `/implement-phase 00-scaffold`, with the three spec pointers written in the same round

### The confirmed `contradicts` finding

**`tests/test_secrets.sh` reports success while enforcing nothing.** It is the only one of
the six files whose rejection case does not run through the same code path as the real
scan. The real run inlines `gitleaks dir "$ROOT"` (line 39) and `gitleaks git "$ROOT"`
(line 51); the rejection case separately inlines `gitleaks dir "$tmp"` (line 70). Nothing
asserts that either real scan runs, points at this repository, or can catch anything.

Reproduced here, and the result is worse than the reviewer's: stubbing the history scan
alone leaves the file green, and so does stubbing **both** real scans —

```
$ sed -e 's|^if gitleaks dir "$ROOT".*|if true; then|' \
      -e 's|^if gitleaks git "$ROOT".*|if true; then|' tests/test_secrets.sh > mut.sh
$ sh mut.sh
  ok:   R-SEC-01 (working tree)
  ok:   R-SEC-01 (history)
  ok:   R-SEC-01 rejection case (a planted token is reported)
  ok:   R-SEC-01 false-positive case
rc=0
```

The file prints `ok:` for a working-tree scan and a history scan that no longer exist.
This is the exact defect the §Goal names — "a check that is silently broken is
indistinguishable from a check that is passing" — surviving inside the phase whose whole
purpose is to make it impossible, and it survived six validation rounds because every
round read the four `ok:` lines as evidence that four things were checked.

Spec §Goal (the "negative self-test on every check" bullet) and §Plan step 7, which makes
the history scan a first-class requirement with its own argument ("a secret deleted in the
next commit is still in the repository") and then binds it to nothing. The mitigating
reading, recorded so the next round can weigh it: step 7 says "**One** rejection case" and
the code delivers exactly one, so the count conforms while the Goal's structural rule does
not. Every other file in this phase routes its rejection case through the shared function
(`sweep`, `check`, `missing_verify`, `check_version`, `find_*`) — that is what makes those
non-vacuous, and it is what this file is missing.

Fix: extract the scan into a function taking a root, the way `sweep` does, and make the
rejection case a temp git repo with a token committed and then deleted, so `gitleaks git`
is the thing under test rather than an unasserted line.

### Deviations found by validation (round 7) — three missing pointers, one artefact

- **(a) Per-rule or per-alternation self-test?** Seven sub-clauses of
  `tests/test_repo_shape.sh` can be deleted with the file still green (verified by
  mutation, rc=0, zero `FAIL:` lines): `\bthrow\b` (R-ERR-03), `\bvirtual\b` (R-CLEAN-09),
  `\bmalloc\(`, `\bfree\(`, `std::vector`, `std::function` and
  `\bdelete[[:space:]]+[A-Za-z_]` (R-ARCH-03). The last is notable: step 0c reworked that
  exact alternation and kept the `= delete;` accept case, which pins the pattern's *shape*
  when present but not its *presence*. The Goal says "a negative self-test on every check"
  and Plan step 5 enumerates ten rejection cases, none covering these tokens — so the code
  conforms to the step while leaving seven forbidden tokens undemonstrated. The spec must
  say which granularity it means; the honest answer is probably per-alternation, since a
  silently-deleted alternation is the same failure as a silently-broken check.
- **(b) Two `docs/constraints.md` hunks that are not binding flips.** The §Style paragraph
  correction ("That scope is necessary but **not sufficient**…") and the §Observed
  conventions bullet "clang-tidy resolves no standard header without a compile database
  (measured 2026-09-02)". Plan step 9 authorizes touching that file only to flip bindings
  and add markers, and §Acceptance criteria explicitly routes that measurement to
  "`notes.md` §For later phases" — while `CLAUDE.md` §Conventions says a verified finding
  goes to §Observed conventions. Two documents give two destinations; the spec must pick
  one. (R-ERR-05's catalogue entry is *not* part of this finding — the Goal authorizes the
  R-ERR-03 split explicitly.)
- **(c) Is the `docs/index/` exemption structural or scoped?** Round 7's new spec
  subsection justifies it as carrying "the six new test files", but `docs/index/scripts.md`
  reports `scripts/build-index.sh` growing 231 → 276 lines — a package change, not a test
  file. The exemption is meant to be structural; the sentence that states it is not.
- **(d) Not a spec gap: the reviewer could not verify Plan step 11** because
  `docs/phases/00-scaffold/notes.md` was withheld from its diff. That is `/validate-phase`
  step 5 forbidding `notes.md` as an input, working as designed — the reviewer is starved on
  purpose. Recorded so the next round does not "fix" it by feeding the reviewer more.

### Taste, not blocking (round 7)

- The five new `.sh` files are mode `100644` while `tests/test_style.sh` is `100755`.
  Harmless — `make test` invokes them via `sh` — and §Out of scope already names it.
- `class/` and `device/` are very broad prefixes in the R-ARCH-01 hardware-header
  alternation for a match on `#include "…"`.
- `ver_num` takes the first number anywhere in a `--version` banner; a distro prefix
  carrying a digit would misparse. Fine for the four tools actually probed.
- `report`'s `head -8` silently truncates a long violation list.
- §Goal says each of the nine traceability messages names "the id and the file"; the
  duplicate-id and unparsable-line messages name neither. Prose looseness, not a defect.

## Standing note for round 8 — the escape counter, and the frozen package

Belay is frozen at `9e1e7b2` for the rest of this phase. Package fixes found from here go
to `~/.claude-belay/feedback/` and land after the phase closes, never during it: six
mid-phase re-installs meant rounds 6 and 7 were judged against different versions of
`/validate-phase`, which is the failure `scripts/installs-stale.sh` warns about — a
mid-session update mutates the gates a running session is being judged by (P3).

**The one package fix taken before freezing.** Round 7 filed an `upstream:` finding: the
iteration-3+ escape counted every `## Validation` section, so a phase that escapes once is
permanently in escape territory — with seven rounds here it would have fired on every round
from now on and demanded re-expanding a spec that is converging. Fixed in belay `5aa5a3c`
and applied before the freeze, not during a round: the counter now resets at the most recent
`escaped to /expand-phase` verdict. That verdict did not exist when this phase escaped, so
it is recorded retroactively at the end of round 5's record above.

That makes round 8 validation **#3** against the re-expanded spec, which is the escape's
own threshold — so if round 8 fails, the escape fires and it is right to. Do not treat this
note as permission to override it. What the fix bought is a counter that measures the
current spec instead of the phase's lifetime; it did not buy an exemption. Round 7 returned
one code defect plus three missing pointers, and three failed validations against one spec
is precisely the signal the escape exists to raise.

## Implementation — 2026-09-02 (round 8, after the round-7 validation)

Round 7 returned one confirmed `contradicts` and three missing pointers. All four are
closed. One check changed what it enforces (`tests/test_secrets.sh`), one gained seven
rejection cases (`tests/test_repo_shape.sh`), and the rest are spec and doc work.

- **The `contradicts` — `tests/test_secrets.sh` enforced nothing.** Both real scans are now
  routed through one `scan <subcommand> <root> <output>` function, and so is every case, the
  way `sweep` and the `find_*` helpers already worked. The rejection cases are what changed
  substance: the working-tree case is unchanged in spirit, and a **second, history-only case**
  now exists — a temp git repo where the fake token is committed and then deleted, asserting
  both halves at once (`scan dir` comes back clean, `scan git` still fires). That is the
  first thing in this phase that binds the rule's "or history" clause to anything.
  Mutation-tested rather than asserted, both mutations run from `tests/` so `ROOT` resolves:
  - `scan` body → `return 0`: two rejection FAILs, `rejection cases: 0 (floor 2)`, rc=1.
  - `gitleaks "$1"` → `gitleaks dir`, i.e. the history scan silently becomes a second tree
    scan: the history rejection case FAILs, `rejection cases: 1 (floor 2)`, rc=1. Under the
    old file this mutation was invisible.
- **A `.git` guard, found while writing the above.** `gitleaks git` over a directory that is
  not a repository exits 1 — the same code as "found a secret" — so a source export with no
  `.git` would have been reported as a leak. Its own `FAIL:` line now, the same treatment
  step 0d gave the too-old-gitleaks case. Verified: `gitleaks git /tmp/not-a-repo` → 1,
  `gitleaks git <clean repo>` → 0.
- **Missing pointer (a) — the self-test granularity is per alternation.** §Goal now says so,
  and Plan step 5 enumerates eighteen rejection cases instead of ten. Seven added, one per
  alternation validation proved deletable: `malloc(`, `free(`, `std::vector`,
  `std::function`, `delete p` (R-ARCH-03), `throw` (R-ERR-03), `virtual` (R-CLEAN-09). Each
  was re-mutated after the fact — deleting any one of the seven now gives rc=1 with two
  `FAIL:` lines (the case, and the floor). The file reports 18 against a floor of 18.
- **Missing pointer (b) — one destination for a verified finding, and it is the catalogue.**
  `CLAUDE.md` §Conventions was already unambiguous; the spec was the document disagreeing
  with it. Plan step 9 now authorizes the two non-binding `docs/constraints.md` hunks
  explicitly (a finding to §Observed conventions, and a §Style paragraph a check in this
  phase proved wrong), and §Acceptance criteria no longer routes the clang-tidy measurement
  to `notes.md` — `notes.md` keeps the working invocation, the catalogue keeps the finding.
  Nothing moved on disk: both hunks were already where they belong.
- **Missing pointer (c) — the `docs/index/` exemption is structural.** The round-7 sentence
  justified it by "the six new test files", which reads as a scope and is why the reviewer
  could not decide `scripts.md`'s 231 → 276 line change. Rewritten: the whole directory is
  generated output, exempt whatever changed in it and whoever changed it, and a diff of
  `docs/index/` is never evidence about a phase.
- **Two count lines that lied.** `test_repo_shape.sh` printed `ok:   rejection cases: N
  (floor M)` even when `N < M`, then set `fail=1` — a shortfall announced under an `ok:`
  prefix, in the phase whose §Goal is that a broken check must not look like a passing one.
  Both count lines in `test_repo_shape.sh` and both in the new `test_secrets.sh` now print
  `FAIL:` when the floor is not met. Visible in the mutation output above.
- **`verify.md` gained one paragraph**, on the failure mode round 7 found: a healthy
  rejection case vouching for a real scan that has been disconnected, and the structural
  repair (real run and rejection case call the same function). R-PROC-02 is about the
  operator being able to judge these files, and this is the thing that fooled six rounds of
  readers. The stale sentence claiming rejection cases had caught every broken check was
  corrected in the same edit — this one was caught by mutation, not by a rejection case.

Acceptance: 12/12 criteria pass. Adversarial block re-run in full because both changed files
are in it: 12/12 caught, each by a `FAIL:` line naming its rule id, tree restored and
`make test` back to `OK` after every case and at the end; the negative half
(`std::string_view sv;` → `test_repo_shape.sh` rc=0) passes.

## Deviations (round 8)

- **`spec.md` was amended again.** Three of the four findings were missing pointers, which
  no code change can close. §Goal gained the granularity rule, §"Files this phase writes that
  no Plan step names" had its `docs/index/` bullet rewritten, and Plan steps 5, 7 and 9 and
  §Acceptance criteria were updated to match what the code now does. No criterion was
  deleted or weakened; two floors went up (repo_shape rejection 10 → 18, secrets rejection
  and accept 1 → 2 each).
- **Two edits no finding named.** The `.git` guard in `test_secrets.sh` and the four count
  lines that printed `ok:` on a shortfall. Both came out of implementing the findings, both
  are the phase's own §Goal applied to itself, and both are recorded here rather than folded
  in silently.
- Files touched: `tests/test_secrets.sh` (step 7), `tests/test_repo_shape.sh` (step 5),
  `docs/phases/00-scaffold/spec.md` (per the first bullet),
  `docs/phases/00-scaffold/verify.md` (step 10), `docs/phases/00-scaffold/notes.md` (step 11).
  Nothing outside the Plan's named set.
- **Round-7 taste notes: none taken.** All five are still open and still not blocking; none
  changes what a rule catches, and this round was already carrying a `contradicts` plus three
  spec gaps. `head -8` truncation and the broad `class/`/`device/` prefixes are the two most
  likely to become real, and they become real when `src/` has files — `01-ps2-codec`.

## Validation — 2026-09-02 (round 8)

- criteria: 12 passed / 0 failed. Each against its stated expectation: `make test` OK,
  `make lint` 0, traceability 9/9, boundaries `rejection cases: 2/2`, repo_shape 8 rule
  `ok:` lines + 18 rejection (floor 18) + 11 accept (floor 11), phase_docs rejection +
  false-positive, secrets both scans + 2 rejection + 2 accept, tool_versions 4/4 with five
  `R-TOOL` ok lines, `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both files, both docs
  non-empty. The adversarial block was run in this session's implementation half after the
  last code change: 12/12 caught by rule id, negative half clean
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** Both other
  reasons were checked, not assumed: 13 active `deny` lines, declared prefixes
  `src/{core,hal,usb,app,emu}/`, and the set contains no `src/` file. Ordinary for a
  test-only phase
- index: was **STALE** (`tests/test_repo_shape.sh`, `tests/test_secrets.sh`) — rebuilt,
  stamp `8a339bd`, `--check` now clean
- independent review: **contradicts — five findings, all reproduced by mutation (below);
  undecidable — two, both real spec gaps.** One further reported `contradicts` (the
  `CLAUDE.md` hunk) was checked and is not a code defect: `git log -S` puts that line in
  `694c902`, a `chore(belay):` commit, and the spec's claim that `CLAUDE.md` already states
  the rule is true. It is the file-set artefact described under `upstream:`
- closure test: **fail** — two undecidables are missing pointers by definition. The
  mechanical half passes: four sections present and non-blank, and all eleven non-exempt,
  non-package files in the set are named in the spec
- upstream: **`.claude/commands/validate-phase.md`** (in `.claude/workflow/installed`) —
  `/belay-feedback` recommended, three items, all about the file set. (i) The set cannot
  separate a `chore(belay):` package-install commit from phase work, so seven package-owned
  files sit in this phase's diff — six named in `.claude/workflow/installed`, plus
  `.claude/workflow/belay-version`. Step 6 exempts four paths "and nothing else", so read
  literally they fail reachability, and the prescribed fix (name them in the Plan) would make
  the spec claim this phase wrote `docs/templates/CLAUDE.bootstrap.md`. Excluded explicitly
  here rather than silently, which is what round 7 did — its "all ten non-exempt files"
  count already applied this exclusion without stating it. The `- base: <ref>` workaround
  only removes package noise when every package commit precedes the base; here they
  interleave with phase work, so no single ref exists that keeps the phase's own history in
  the diff and the package's out of it. (ii) The reviewer is not told the diff is
  path-filtered, so the deliberate absence of `notes.md` reads as evidence that Plan step 11
  was skipped — finding 7, and the same confusion round 7 recorded. `CLAUDE.md` now asserts
  a phase diff always carries those files, which makes the filtered diff look like a defect.
  (iii) `.claude/workflow/installed` omits `.claude/workflow/belay-version`, which
  `install.sh` writes. Round 6's `upstream:` item (the step-6 exemption living in a command
  file the starved reviewer never receives) is unchanged and still open
- verdict: **escaped to /expand-phase: spec re-expanded.** This is validation #3 against the
  spec written in round 6 — the `## Validation` sections following the escape verdict
  recorded above are rounds 6, 7 and this one — which is the iteration-3+ threshold. Status
  moved to `pending` by this command, per its own failure-mode rule. The standing note for
  round 8 said the escape would be right to fire if this round failed, and the findings below
  are why it is: round 8 closed a defect in one file and one rule-set and wrote a *universal*
  claim into the Goal that the code satisfies for one finder out of eight

### The five confirmed `contradicts`

All reproduced here by mutation, each run from `tests/` so `ROOT` resolves to the repository.

**1. "One case per alternation" holds for `find_arch03` and nothing else.** Round 8 added the
Goal sentence "every alternation in a pattern needs a case that fires on it alone" and the
step 5 check line "deleting any one alternation from any finder must make this file exit
non-zero", then delivered cases for the seven alternations round 7 happened to name. Surviving
mutations, all `rc=0`:

| finder | alternations | cases | example surviving deletion |
|---|---|---|---|
| `find_arch01` | 31 (7 include prefixes + 24 headers) | 2 | `hardware/`; also `vector`, which silently disables the spec's own adversarial line 370 |
| `find_err03` | 3 | 1 | `\bcatch[[:space:]]*\(` — the single case still matches via `\btry` |
| `find_clean09` | 3 | 3 | the whole `:[[:space:]]*(public\|private\|protected)` alternation — both cases still match via the `(struct\|class)…:` form |
| `find_arch03` | 7 | 7 | none — the only finder that satisfies the rule |

The spec is complicit: step 5 enumerates 18 names and lists "R-ARCH-01 hardware header" and
"R-ERR-03 `try`/`catch`" as single cases, so `18 = 18` conforms to the enumeration while
failing the check line and the Goal bullet in the same file. Two sentences of one spec cannot
both be satisfied — which is the defect, not the count.

**2. `tests/test_secrets.sh`: the real history scan is still unbound, and the file now says
otherwise.** Round 8 bound the `scan` *function*; nothing binds the *call*. Changing the real
run's `scan git "$ROOT"` to `scan dir "$ROOT"`, or deleting that block outright, leaves the
file green (`rc=0` both). The in-file comment "Delete the `scan git` call above and this is
what stops it going unnoticed" is false as written, and step 7's sentence "without it the
`git` scan can be silently turned into a second `dir` scan with the file still green" is true
only of the mutation inside the function, not of the one at the call site. Round 8 verified
the first and wrote the claim as if it covered both.

**3. The wiring layer is untested in every file — the same defect class, one level up.** The
rejection and accept machinery calls the finders directly, so it proves the *pattern* works
and never that the pattern is run against `$ROOT`. Deleting `report R-ERR-04 "$( find_err04
"$1" )"` from `run_all` leaves 7 `ok:` lines and `rc=0`; gutting `report()`'s failure branch
(`if [ -n "$2" ]` → `if false`) disables all eight rules and still exits 0. The same holds for
`check_version` in `tests/test_tool_versions.sh`. This is what findings 1 and 2 are instances
of, and it is the finding the phase most needs: eight rounds have each fixed one instance.

**4. `verify.md` overstates the guarantee to the operator.** Round 8's new paragraph tells a
non-specialist that "the real run and the rejection case must call the same function … which
is what makes an `ok:` line evidence that something ran". Per finding 3 a shared function
makes the `ok:` line evidence that the *function* works, not that the real run invoked it.
R-PROC-02 is about the operator being able to judge these files; this sentence teaches
something false.

**5. `ok:` on a shortfall, still, in the two files round 8 did not touch.**
`tests/test_tool_versions.sh:175` prints `  ok:   rejection cases: $rejected/4` and
`tests/test_rule_traceability.py:270` prints `  ok:   R-PROC-01 rejection cases: {passed}/{n}`
unconditionally, before the comparison that sets the failure. Both still exit non-zero, so the
build is not fooled and the reader is. Round 8 fixed exactly the two files it was already
editing and wrote the principle into step 5's check line, where it reads as scoped to
`test_repo_shape.sh`.

### The two `undecidable` findings

- **(a) Plan step 11 cannot be verified, and now looks skipped.** The review diff carries no
  `notes.md` and no `spec.md` hunk. That is `/validate-phase` step 5 starving the reviewer on
  purpose plus this session supplying `spec.md` whole as its own input — but nothing tells the
  reviewer the diff is path-filtered, and `CLAUDE.md` line 18 states that a phase's diff
  always carries both files. Round 7 recorded the same confusion as "not a spec gap, working
  as designed"; it has now recurred against a `CLAUDE.md` that contradicts it, so it is filed
  upstream rather than dismissed a second time.
- **(b) Which check that this phase writes proved the §Style paragraph wrong?** Round 8's new
  step 9 authorizes a §Style correction "when a check this phase writes proves it wrong". The
  clang-tidy measurement behind that paragraph comes from `tests/test_style.sh`, which this
  phase did not write — `git diff` shows its only change in this range is the
  `STYLE_OPTIONAL` → `OPTIONAL_TOOLS` rename. The authorization as written does not reach the
  hunk it was written for. The §Observed conventions entry is unambiguously authorized by the
  same step; only the §Style paragraph is unclear.

### Taste, not blocking (round 8)

Carried forward for whoever re-expands this phase; none changes what a rule catches.

- `\b` is used throughout `test_repo_shape.sh` while step 0b chose the portable
  `([^_[:alnum:]]|$)` form for one pattern on portability grounds. Inconsistent.
- `sweep`'s `for f in $( find … )` word-splits on paths containing spaces; same in the
  `hits`/`raw_hits` argument expansion.
- `PATH="$stub_dir:$PATH" check_version …` — a prefix assignment on a *function* call
  persists under POSIX `sh`, so `PATH` keeps a deleted directory for the rest of the run.
- The duplicate-id, unparsable-line and `planned:` traceability messages name no file, and
  the unparsable one names no id, against §Goal's "each with its own message naming the id
  and the file".
- `SKIP_DIRS` adds `node_modules`, which Plan step 2 does not list.
- `find_clean09`'s `enum[[:space:]]+class` exclusion is line-based, so one line carrying both
  an enum and a real base clause is excluded wholesale.
- `test_secrets.sh` exits 0 having enforced nothing when `gitleaks` is absent under
  `OPTIONAL_TOOLS` — authorized by step 7, but R-SEC-01's `test:` binding is then vacuous on
  a machine without the tool.
- The five new `.sh` files are mode `100644` while `tests/test_style.sh` is `100755`
  (carried from round 7, still not blocking).
- `report`'s `head -8` silently truncates a long violation list (carried from round 7).

## Implementation — 2026-09-03 (round 9, against the re-expanded spec)

The round-8 escape sent this phase back to `/expand-phase`, and the re-expansion made one
change of substance: the coverage question stopped being a human's to remember and became a
machine's to answer. This round built that machine, and it immediately found four holes —
three of which no round had ever named, and one of which was a claim in the new spec itself.

### The mechanism: `tests/test_checks_are_live.py`

One new file that checks no rule. It reads the other checks' source and generates mutations
from it, rather than from a list anybody maintains — which is the whole difference from
rounds 7 and 8, where a human enumerated cases and covered one check function out of eight.

- **accounting** — every rule id a check file declares in its header must produce a result
  line when that file runs for real, in both directions. A deleted call site becomes a
  missing rule instead of a silent pass.
- **neutering** — replacing any check function with one that finds nothing must make its
  file fail. 12 functions discovered by naming convention (`find_*`, `check_*`, `sweep`,
  `scan`, `missing_verify`); the discovered set is printed, so a function the convention
  misses is visible rather than quietly unmutated.
- **alternation** — removing any one alternative from any check function's pattern, at any
  nesting depth, must make its file fail. 55 generated. This is what rounds 7 and 8 reached
  for by hand.
- **bootstrap** — the harness cannot verify itself without a regress, so
  `tests/fixtures/incomplete_check.sh` ships with a two-alternative pattern and a case for
  only one, and the harness must report exactly that hole every run.

### Four holes it found, none of which reading had found in eight rounds

1. **31 alternatives of `find_arch01` had no case; so did 3 of `find_clean09` and 2 of
   `find_err03`.** First run: `alternations: 16/52 caught`, naming all 36. Closed by 39 new
   rejection cases. The three interesting ones are `: public Base` / `: private Base` /
   `: protected Base` on a **continuation line** — the only form the access-specifier
   alternative can catch alone, because `struct A : public B` on one line is also caught by
   the `(struct|class) Name :` alternative. That is precisely the gap round 8's reviewer
   predicted and no hand-written case had covered.
2. **`find_err03` was not being mutated at all, and the harness said `52/52`.** Its pattern
   contains a literal `\{`, the brace matcher counted it as a nesting brace, walked off the
   end of the file, and dropped the function from the set — silently. Fixed by making the
   matcher quote-aware **and by making an unparsable definition a hard failure**: the bug was
   not the miscount, it was that a check the harness could not parse was skipped without a
   word. A harness with that hole is the defect it exists to catch.
3. **Gutting `report()` left everything green.** The rejection and accept cases called the
   finders directly, so the function deciding *whether output counts as a violation* was
   load-bearing for nothing. Fixed structurally: `report()` no longer sets `fail` itself, it
   returns a verdict, and every one of the 55 rejection and 11 accept cases now runs through
   it. Gutting it now fails on the first case.
4. **Dropping one `|| fail=1` printed the `FAIL:` line and still exited 0.** The verdict
   reached the screen and not the exit code. The harness cannot see this — it only reads
   exit codes — so it needed a check in the file: **eight wiring cases**, one per rule,
   driving the real `run_all` over a tree carrying exactly that rule's violation and
   requiring both the rule's name in the output and a failed run.

Holes 3 and 4 are why the harness is not the whole answer, and the spec now says so.

### The `LIVE` label, and a substring that matched the wrong line

R-SEC-01 is two independent scans behind one rule id, and R-TOOL-01 is four tool probes, so
accounting on the id alone cannot see one of them disappear. A check file now declares
`# LIVE <id><rest>`, where the rest is the literal prefix of the result line that proves that
call happened. The first implementation required the label to appear anywhere in the output
and **passed while the real history scan was deleted**: `(history)` also occurs inside
`ok:   R-SEC-01 false-positive case (history)`. The rule is now *prefix of exactly one result
line*. Verified by deleting the real `scan git "$ROOT"` block (harness fails, naming
`R-SEC-01 (history)`) and by deleting one `check_version` call (fails, naming
`R-TOOL-01: clang-tidy`).

That near-miss is worth keeping in view: it is the same shape as everything else this phase
has found. The check looked right, ran, printed `ok:`, and was answering a different question
than the one asked.

### Runtime

`make test` went from ~2s to ~56s, because it now runs 67 mutants. Two changes kept it under
the spec's two-minute criterion: `tests/test_repo_shape.sh` uses one scratch directory
instead of `mktemp -d` per case (66 per run × 67 runs), and the harness runs its mutants in a
thread pool. 82s → 24s for the harness; 56s for the whole suite, measured.

### Step 0's six owed edits, all closed

0a the false universal comment in `test_repo_shape.sh`; 0b the false comment in
`test_secrets.sh` about what protects the history scan; 0c `ok:` prefixing a shortfall in
`test_tool_versions.sh` and `test_rule_traceability.py`; 0d `verify.md`'s claim that a shared
function makes an `ok:` line evidence; 0e the §Style correction now names the check that
produced it (`tests/test_style.sh`); 0f five traceability messages now name the id and the
file, which broke three of its own rejection cases — they asserted the old text, and were
updated in the same edit.

### Acceptance, measured

13/13 criteria pass. `make test` 56s against the spec's 2-minute cap.

**Adversarial block: 12/12**, each caught by a `FAIL:` line naming its rule id, tree restored
and `make test` back to `OK` after every case and at the end. Negative half clean
(`std::string_view sv;` → `test_repo_shape.sh` rc=0).

**Liveness block: 5/5** — this is the part rounds 1-8 could not do, so the messages are
recorded verbatim:

```
alternative with no case    FAIL: R-ARCH-01 rejection case did not fire on: #include "hardware/gpio.h"
deleted call site           FAIL: R-ERR-04 is reported but never reaches the exit code (run_all left fail=0)
gutted report()             FAIL: R-ARCH-01 rejection case did not fire on: #include "pico/stdlib.h"
dropped || fail=1           FAIL: R-ERR-04 is reported but never reaches the exit code (run_all left fail=0)
deleted real history scan   FAIL: test_secrets.sh declares the live call(s) R-SEC-01 (history) but the run produced no such line
```

Every one of those five was green on this tree at the start of the round.

Harness summary on the clean tree: `neutered: 12/12 caught`, `alternations: 55/55 caught`,
`bootstrap: fixture gap reported as expected (forbidden_beta)`, accounting `ok:` for all
seven check files. `tests/test_repo_shape.sh` reports 55 rejection cases, 11 accept cases and
`wiring cases: 8/8`.

## Deviations (round 9)

- **The harness is `tests/test_checks_are_live.py`, not `.sh` as the spec's Plan said.**
  Splitting a regex into its alternatives at every nesting depth is a parser, and the shell
  is the wrong tool for one. `python3` is inside the floor R-PROC-04 guarantees, so this
  costs no dependency. `spec.md` and its acceptance criterion were amended to match.
- **Three claims in the round-9 spec were wrong and the spec was corrected, not worked
  around.** (i) "Properties 2 and 3 also catch a gutted shared helper for free" — they do
  not; that is holes 3 and 4 above, and §Plan step 6 now carries the two structural
  requirements that do catch them. (ii) The `LIVE` label rule said "appear verbatim in the
  output"; it is now "prefix exactly one result line", with the reason. (iii) The liveness
  block's expected message for the `hardware/` mutation named the harness, but the case added
  for that alternative now catches it earlier and more directly, in `test_repo_shape.sh`
  itself. Each is recorded here because an amended spec requires it.
- **`tests/test_repo_shape.sh` lost its rejection-case floor.** The spec's §How counts are
  stated forbids a floor next to a universal claim about the same thing, and the harness is
  now the universal claim. It prints the count without a threshold.
- **Two files were restored by hand after a killed experiment.** A 2-minute tool timeout
  killed a mutation script mid-restore and left `tests/test_secrets.sh` without its real
  history scan — in the working tree and in the backup the script had made. The harness's own
  accounting property is what caught it (`declares the live call(s) R-SEC-01 (history) but
  the run produced no such line`) rather than a human noticing. Block reconstructed and
  verified. Worth stating plainly: for a few minutes this phase's tree contained exactly the
  defect it exists to prevent, and the new machinery is what found it.
- **`spec.md` step 5 amended during validation to name the fixture by path.** It said
  "touches the harness and `tests/fixtures/`" — the directory, with the file's content
  specified in the step but its path never written out. Reachable on a generous reading and
  ambiguous on a strict one, and this phase has lost rounds to exactly that gap, so the exact
  path is now in the step. No behaviour changed; the file was already there.
- **Files touched, all named in the Plan:** `tests/test_checks_are_live.py` (new, steps 2-5),
  `tests/fixtures/incomplete_check.sh` (new, step 5), `tests/test_repo_shape.sh` (0a, 5, 6),
  `tests/test_secrets.sh` (0b, 2, 6), `tests/test_tool_versions.sh` (0c, 2),
  `tests/test_rule_traceability.py` (0c, 0f), `docs/constraints.md` (0e, 7),
  `docs/phases/00-scaffold/verify.md` (0d, 8), `docs/phases/PHASES.md` (step 9, plus status),
  `docs/phases/00-scaffold/spec.md` (per the second bullet), `docs/phases/00-scaffold/notes.md`
  (step 10).

## Validation — 2026-09-03 (round 9)

- criteria: 14 passed / 0 failed. Each against its stated expectation, not just its exit
  code: `make test` OK, `make lint` 0, harness `neutered: 12/12` + `alternations: 55/55` +
  `bootstrap: fixture gap reported as expected`, traceability 9/9, boundaries
  `rejection cases: 2/2`, repo_shape 55 rejection + 11 accept + `wiring cases: 8/8`,
  phase_docs rejection + false-positive, secrets both `LIVE` labels reporting,
  tool_versions six `ok:` lines, `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both files,
  both docs non-empty, `time make test` **58.4s** against the 2m criterion
- project gates: test pass, lint pass, typecheck gap —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** Both other
  reasons checked rather than assumed: 13 active `deny` lines, prefixes
  `src/{core,hal,usb,app,emu}/`, and 0 of the 26 files in the set match one
- index: was **STALE** (`test_rule_traceability.py`, `test_secrets.sh`,
  `test_tool_versions.sh`) — rebuilt, stamp `8a339bd`, `--check` clean
- independent review: **contradicts — three, all reproduced by mutation and all confirmed
  (below); undecidable — three, two of them real spec gaps.** The reviewer also found the
  bootstrap fixture weaker than §Plan step 5 assumes, which is recorded under
  §For later phases
- closure test: **fail** — two undecidables are missing pointers by definition. The
  mechanical half passes: four sections present and non-blank, and all thirteen non-exempt,
  non-package files in the set are named in the spec (`tests/fixtures/incomplete_check.sh`
  was named by path during this validation; see §Deviations)
- upstream: **`.claude/commands/validate-phase.md`** — unchanged from round 8, already filed
  and open (`~/.claude-belay/feedback/pico-sg2hid.md`, entry of 2026-09-02). Seven
  package-owned files ride in the file set again, and the reviewer again returned the
  `CLAUDE.md` hunk as a `contradicts` (finding 3): `git log -S` puts that line in `694c902`,
  a `chore(belay):` commit, so the phase never wrote it. Second consecutive round in which
  that artefact costs a review finding. No new entry — the open one covers it
- verdict: **returned to implementation.** A `contradicts` is a code failure, so this goes to
  `/implement-phase 00-scaffold`. This is validation **#1** against the round-9 spec (zero
  `## Validation` sections follow the `escaped to /expand-phase` verdict above), so the
  iteration-3+ escape does not apply and must not be invoked

### The three confirmed `contradicts`

**1. The harness breaks the clean-clone promise it was built under.** `make test` runs the
shell checks with `OPTIONAL_TOOLS=1` but runs `PY_TESTS` — where the harness lives — with
nothing, and `run()` passes no `env`, so every check file the harness executes inherits an
environment where a missing external tool is a *hard failure* rather than a skip. Measured on
this machine by removing only the directory holding `gitleaks` from `PATH`:

```
$ PATH=<without /opt/homebrew/bin> OPTIONAL_TOOLS=1 sh tests/test_secrets.sh
  skip: R-SEC-01: gitleaks not found (brew install gitleaks)      rc=0     <- make test
$ PATH=<without /opt/homebrew/bin> sh tests/test_secrets.sh
  FAIL: R-SEC-01: gitleaks not found (brew install gitleaks)      rc=1     <- the harness
$ PATH=<without /opt/homebrew/bin> python3 tests/test_checks_are_live.py
  FAIL: test_secrets.sh does not pass on the real tree (rc=1)
```

So `make test` fails on a clean clone that has clang++ and python3 and nothing else — which
is what the `PHASES.md` row promises, what `CLAUDE.md` §Architecture states, and what
§Goal's own "What this phase does NOT prove" bullet relies on when it says a skipped file
must be reported `unproven:` rather than failing. That branch is unreachable today: the file
fails before it can be reported unproven. The same holds for `test_style.sh` without LLVM and
`test_tool_versions.sh` without the ARM toolchain. **The round that added a check to prove
the checks are live is the round that made the suite unrunnable on the floor it promises.**

There is a second, independent half the reviewer separated correctly: `test_tool_versions.sh`
emits `skip:` lines for absent tools *and* an `ok:` for `python3`, so it is not "skipped" by
the harness's test (`not RESULT.search(out)`), and each absent tool then leaves its
`# LIVE R-TOOL-01: <tool>` label prefixing zero result lines. The accounting property has no
notion of a partially-skipped file.

**2. `LIVE` labels were given to the two rules with obviously-multiple calls, and the same
hole is open in two files that did not get one.** Accounting is satisfied when a declared id
appears in *any* result line — and in `test_boundaries.sh` and `test_phase_docs.sh` the
rejection-case lines themselves carry the rule id. Both reproduced, both leave the file *and*
the harness green:

```
delete the real sweep block from test_boundaries.sh   -> file rc=0, harness rc=0  SURVIVES
delete the real run from test_phase_docs.sh           -> file rc=0, harness rc=0  SURVIVES
```

`src/` is never swept and `PHASES.md` is never examined, and nothing anywhere says so. This
is F2 — round 8's central finding — reproduced verbatim in the two files that happened not to
need a label under the criterion §Plan step 2 wrote down. **That criterion is wrong.** "More
than one real-run call" is not the test; the test is "does any line that is *not* the real
run also carry this rule id", which is true of every check with a rejection case that names
its rule.

**2b. `tests/test_phase_docs.sh` bypasses the shared verdict, the one shape §Plan step 6
forbids.** `missing_verify` returns text, and `[ -n "$found" ]` is written three separate
times — real run, rejection case, accept case. Gutting only the real run's copy
(`if [ -n "$found" ]; then` → `if false; then`) leaves file and harness at rc=0. Step 6's
requirement was implemented in `test_repo_shape.sh` and nowhere else; the step says "every
rejection and accept case", not "every case in `test_repo_shape.sh`".

**3. `CLAUDE.md` in the diff.** Not a code defect: the hunk came from `694c902`, a
`chore(belay):` commit. Recorded above under `upstream:`.

### The three `undecidable` findings

- **(a) Is the exclusion pattern in scope for the alternation property?** `find_clean03` is
  `hits '<pattern>' '<exclusions>'` and `first_pattern()` deliberately takes only the first
  quoted string, so the exclusion's alternatives are never mutated. Confirmed live:
  dropping `can|` from `'bool[[:space:]]+(m_)?(is|has|can|should)_'` leaves file and harness
  green, and `bool can_fire = true;` silently becomes a false positive. §Goal says "any check
  function's pattern" and the Context pointer describes both strings; the spec must say
  which it means.
- **(b) Does step 6's wiring requirement reach the five rules outside `test_repo_shape.sh`?**
  The parenthetical names `run_all`, which exists only there, and the acceptance criteria
  mention wiring only as `8/8`. R-ARCH-02, R-PROC-01, R-PROC-02, R-SEC-01 and R-TOOL-01/02
  have none — so deleting `fail=1` from `test_secrets.sh`'s history-scan `else` branch prints
  `FAIL: R-SEC-01`, satisfies accounting with that very line, and exits 0.
- **(c) `verify.md` §3's clang-tidy claim is not checkable from the inputs.** Step 8 requires
  every command the page prints to produce the `FAIL:` line it promises; whether the third
  and fourth scratch files also trip clang-tidy cannot be decided without `tests/test_style.sh`,
  which the diff carries only as the two rename hunks.

### Taste, not blocking (round 9)

- **The bootstrap fixture cannot floor the properties it exists to floor.** `bootstrap()`
  re-implements the mutation loop inline and never calls `mutate_and_run`, `run_mutants`,
  `property_alternation`, `property_neutering` or `NO_MUTATE`. Two mutations leave it
  printing "fixture gap reported as expected" while the harness is blind: `return label, rc == 0`
  → `return label, False` (every mutant "caught", both properties print n/n), or adding
  `test_repo_shape.sh` to `NO_MUTATE` (alternation drops to zero jobs and `main()` prints
  nothing and fails nothing when `total == 0`). The code matches §Plan step 5 as written; the
  step is what is weak. This is the most valuable finding of the round after the three above.
- Mutants are judged by exit code alone, so "caught for the wrong reason" counts as caught —
  a syntactically broken mutant inflates n/n.
- Functions outside `FN_PREFIXES`/`FN_NAMES` are visible only by their absence from a printed
  list: `ver_num`, `resolve` and `arm_compiles` in `test_tool_versions.sh` are never mutated.
  Deleting `resolve`'s `arm-none-eabi-*` early return leaves the suite green while defeating
  the exact `PATH`-shadowing trap that file's header says the line exists for.
- `first_pattern` takes the first quoted string whatever its role. In `missing_verify` it
  grabs awk's `-F'|'`, yielding zero alternatives with no "no pattern found" report.
- The harness docstring's claim that gutting `report()` makes every mutant *survive* is
  backwards — it makes the base file fail, and the accounting `rc != 0` gate is the real
  catch. Same class of imprecise prose that step 0a existed to remove.
- `tests/test_repo_shape.sh:220` prints `ok:   rejection cases: $rejected` unconditionally
  and then tests it — **the exact shape step 0c bans**, reintroduced by this round when the
  floor was removed.
- An interrupted harness leaves `mut_*.sh` in `tests/` carrying real `RULE` markers.
- `R-ERR-05` sits between `R-ERR-03` and `R-ERR-04` in the catalogue, breaking numeric order.

## Implementation — 2026-09-03 (round 10, the two corrections named at the close of round 9)

Scope was fixed by the operator to one piece in one file plus the spec step that states its
criterion, with an explicit ban on fixing the other round-9 findings ("nada de arreglar
hallazgos en otros archivos"). Files touched: `tests/test_checks_are_live.py`,
`docs/phases/00-scaffold/spec.md` (§Plan step 2, plus two acceptance-criteria fixes forced by
this validation), `docs/index/` (regenerated), this file. Nothing else.

**1. The accounting criterion (round-9 `contradicts` 2 — F2, closed).** The property was
"does this rule id appear in some result line". Every check ships rejection and accept cases
whose lines name the rule, so the answer is yes whether or not the real run still happens —
which is why `sweep "$ROOT"` could be deleted from `test_boundaries.sh` and the real
`missing_verify` run from `test_phase_docs.sh` with the file *and* the harness green. The
criterion asks the question of the real run's own lines now: one regex, `CASE_LINE`, over the
output, resting on the house convention that a case line says which case it is (`rejection
case`, `false-positive case`, `accept case`, `wiring case`), and a declared id reported by
nothing but case lines is a deleted real run.

Deviation from §Plan step 2 as the operator stated it: step 2's true criterion ("does a line
that is not the real run carry this id?") is universally yes, which read literally means
every rule in every check needs a `LIVE` label — five check-file headers. The convention is
derived in the harness instead, so the fix closed all six files from one file and cost no
declaration. `LIVE` labels remain for the one residue the convention cannot reach — a rule
with several independent real calls (R-SEC-01, R-TOOL-01) — and must now pin a *real-run*
line, not merely a unique one.

**2. `bootstrap()` reimplemented the mutation loop inline (round-9 taste, promoted).** The
fixture could report its expected gap while the real properties were blind. Extracted
`alternation_jobs(path)`; `bootstrap()` and `property_alternation()` both drive it through
`run_mutants()`. `run_mutants()` no longer reports — a survivor is a defect for properties 2
and 3 and the *expected* result for the floor — so reporting moved to `mutation_score()`.
`mutate_and_run()` writes the mutant beside its own source rather than always under `tests/`.

**Verified by mutating a copy of the repository, which is the step nine rounds skipped.** Not
by running the file and reading its output:

```
delete the real `sweep "$ROOT"` block, test_boundaries.sh   before rc=0  ->  after rc=1
delete the real `missing_verify "$ROOT/…"` block, phase_docs before rc=0  ->  after rc=1
delete the real `scan git "$ROOT"` block, test_secrets.sh                     after rc=1
sabotage the harness: `return label, rc == 0` -> `return label, False`        after rc=1
```

The first two are the cases that survived round 9; the messages are
`R-ARCH-02, reported by nothing but its own cases — the real run is gone` and the same for
`R-PROC-02`. The fourth is the floor doing its job: the sabotaged harness still prints
`neutered: 12/12 caught` and `alternations: 55/55 caught` and fails only on
`bootstrap: expected exactly 1 uncovered alternative in the fixture, got 0`.

One premise in the operator's order was wrong and is corrected here rather than absorbed:
R-SEC-01 was **not** passing with its real run unbound. The `LIVE` prefix-exactly-one test
already caught that deletion, because `R-SEC-01 false-positive case (history)` does not
prefix-match `R-SEC-01 (history)`. What was broken is the criterion deciding *which* rules
get a label, which left the other four files resting on plain accounting.

## Deviations (round 10)

- **The `LIVE`-label-per-rule reading of step 2 was not implemented; the convention is
  derived in the harness.** Reason above. Step 2 now states the convention, names the two
  files whose deletion it catches, and keeps `LIVE` for the multi-call residue.
- **An acceptance criterion had never been runnable, and this validation is what found it.**
  The liveness block's second mutation is
  `s.replace('    report R-ERR-04   "$( find_err04   "$1" )"\n', '', 1)`. The line in
  `tests/test_repo_shape.sh` has carried `|| fail=1` since the wiring cases landed, so the
  replacement matched nothing: the criterion mutated no file, `make test` returned 0, and the
  0 was read as the criterion passing. A silent no-op that has been reported as a pass for at
  least two rounds. `spec.md` fixed to include `|| fail=1`; re-run gives rc=2. Its expected
  message was also stale — the R-ERR-04 **wiring case** fires first now
  (`R-ERR-04 is reported but never reaches the exit code (run_all left fail=0)`,
  `wiring cases: 7/8`), and the harness then fails the file; the comment says that.
- **`docs/index/` was stale and was rebuilt** (stamp `8a339bd` -> `26c04d8`), per step 4.
- **An interrupted `make test` leaves `tests/mut_*.sh` behind.** The harness unlinks each
  mutant in a `finally`, which a SIGTERM skips. A 9-minute tool timeout during the
  adversarial block orphaned seven; removed by hand. Round 9 logged the same thing under
  taste — it is now observed twice, and the fix is a trap or a `.gitignore` line, owned by no
  step of this spec.

## Validation — 2026-09-03 (round 10)

- criteria: **32 passed / 1 failed-then-fixed.** 14 main (`make test` OK, `make lint` 0,
  harness accounting for all seven files + `neutered: 12/12` + `alternations: 55/55` +
  `bootstrap: fixture gap reported as expected`, traceability 9/9, boundaries
  `rejection cases: 2/2`, repo_shape 55 rejection + 11 accept + `wiring cases: 8/8`,
  phase_docs rejection + false-positive, secrets both `LIVE` labels, tool_versions six `ok:`
  lines, `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both files, both docs non-empty,
  `time make test` **55.9s** against the 2m cap); 5 liveness — one of which was **not
  runnable** and is recorded under §Deviations, fixed in `spec.md` and then passing; 12
  adversarial, each exiting non-zero and naming its own rule id; 1 negative half (repo_shape
  quiet on `std::string_view`), tree green after every one
- project gates: test pass, lint pass, typecheck **gap** —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** Both other
  reasons checked rather than assumed: 13 active `deny` lines, prefixes
  `src/{core,hal,usb,app,emu}/`, and none of them prefixes any of the 26 files in the set
- index: was **STALE**, rebuilt, `--check` clean at `26c04d8`
- independent review: **contradicts — six, all six reproduced against the tree before being
  recorded; undecidable — two.** Detail below
- closure test: **fail.** Mechanical half passes — four sections present and non-blank, and
  every non-exempt, non-package file in the set is named in the spec's Plan or Context
  pointers. It fails on the two `undecidable` verdicts, which are missing pointers by
  definition
- upstream: **`.claude/commands/validate-phase.md`** — third consecutive round in which the
  path-filtered diff carries seven package-owned files and costs a review finding
  (`undecidable` 1 this time, `contradicts` 3 in round 9). Already filed and open
  (`~/.claude-belay/feedback/pico-sg2hid.md`, 2026-09-02); no new entry. Also new:
  `.claude/workflow/installed` does **not** name `.claude/workflow/belay-version`, so the
  spec's "files `installed` names are the package's" exemption cannot cover a file that rides
  in on every `chore(belay):` commit. Worth one line in the same open entry
- verdict: **returned to implementation.** Six `contradicts` are code failures, so this goes
  to `/implement-phase 00-scaffold`. This is validation **#2** against the round-9 spec (two
  `## Validation` sections follow the `escaped to /expand-phase` verdict of round 8), so the
  iteration-3+ escape does not apply and must not be invoked. Round 10 was deliberately
  scoped to two corrections and forbidden from touching the rest, so five of the six findings
  below are round-9 defects surviving by instruction, not by oversight

### What round 10 closed

- **Round-9 `contradicts` 2 (F2) is closed in all six checks at once.** Both surviving
  mutations now fail; see §Implementation for the before/after codes.
- The bootstrap floor can now fail, demonstrated by sabotage rather than asserted.

### The six confirmed `contradicts` — all still open

1. **§Out of scope parks parallelising; the harness parallelises.** "*parallelising is an
   optimisation nobody has yet needed*" against `run_mutants()`'s
   `ThreadPoolExecutor(max_workers=min(len(jobs), cpu_count*2))`, and
   `tests/test_repo_shape.sh`'s single shared `case_tmp` replacing per-case `mktemp -d`,
   justified in its own comment as wall-clock work. The code is defensible — 67 mutants
   serially is minutes, and an acceptance criterion caps the suite at two — but §Out of scope
   says not to, and the spec is what a cold session reads.
2. **§Plan step 4 says "top-level" alternatives; `alternatives()` splits at every nesting
   depth, on purpose.** Step 4's own prose argues for depth (31 alternatives in
   `find_arch01`, one top-level branch), so the word "top-level" is stale text inside a step
   that everywhere else demands the opposite. The code is right and the step is wrong; the
   step is what must change.
3. **§Plan step 3's naming convention is four names; the harness carries five, plus an
   invisible hole.** `FN_NAMES = ("sweep", "scan", "missing_verify")` adds a fifth name the
   step does not declare — the hand-written list step 3 exists to forbid. And
   `arm_compiles( )` in `tests/test_tool_versions.sh` — the whole of R-TOOL-02 — falls
   outside the convention, so it is never neutered and never alternation-mutated, and the
   harness prints only the set it *discovered*, so nothing in the output says so.
   `ver_num( )` and `resolve( )` are in the same position.
4. **§Plan step 0c's premise is false of the file in this diff.** 0c says
   `test_repo_shape.sh` already prints its shortfall as `FAIL:`; line 220 prints
   `ok:   rejection cases: $rejected` unconditionally and only then evaluates
   `[ "$rejected" -gt 0 ] || fail=1`. Round 9 reintroduced the exact shape 0c bans, when the
   floor was removed. The step-0 check as written is also false: the grep matches four files,
   three of which print inside a floor-passing branch.
5. **The clean-clone floor is still broken (round-9 `contradicts` 1, unfixed by
   instruction).** `Makefile:28` prefixes `OPTIONAL_TOOLS=1` onto the `SH_TESTS` loop only;
   `PY_TESTS` run bare, and the harness's `run()` passes no `env`. Re-measured today with
   `/opt/homebrew/bin` off `PATH`:
   `OPTIONAL_TOOLS=1 sh tests/test_secrets.sh` -> `skip:`, rc=0; the same file as the harness
   invokes it -> `FAIL:`, rc=1. So §Plan step 2's `unproven:` branch is unreachable under
   `make test`, and §Goal's "`make test` exits 0 on the clean repo" is false on the floor the
   `PHASES.md` row promises. The reviewer also separated the partial-skip half correctly:
   `SKIPPED.search(out) and not RESULT.search(out)` is false for `test_tool_versions.sh` when
   some tools resolve and others do not, so an absent `arm-none-eabi-g++` leaves its
   `# LIVE R-TOOL-01: arm-none-eabi-g++` label prefixing zero lines and the harness fails the
   file. Accounting has no notion of a partially-skipped file
6. **Three false statements in the tree, the class §Plan step 0 exists to delete.**
   `tests/test_secrets.sh:116` and `tests/test_repo_shape.sh:154` both name
   `tests/test_checks_are_live.**sh**`, a file that does not exist — the harness is `.py`, as
   §Goal says. `tests/test_boundaries.sh:13` attributes the `HOOK`-stub requirement to "*the
   spec's own step-0 check*"; step 0 contains no such check, it is in step 6.

**Round-9 `contradicts` 2b is also still open** and was re-verified today, though this
round's reviewer did not surface it: `tests/test_phase_docs.sh` writes `[ -n "$found" ]`
three separate times instead of routing every case through one shared verdict, so gutting
only the real run's copy (`if [ -n "$found" ]; then` -> `if false; then`) leaves the file at
rc=0 and the harness at rc=0. §Plan step 6 requires the shared-verdict shape of "*every
rejection and accept case*"; it exists in `test_repo_shape.sh` and nowhere else.

### The two `undecidable` findings

- **(a) Which paths in the diff belong to the belay package.** The spec's "*A third
  category*" exempts "*files that `.claude/workflow/installed` names*" but never writes the
  list out, and the reviewer holds only `CLAUDE.md`, the spec and the diff. It could not
  decide for `.claude/commands/{expand-phase,validate-phase}.md`,
  `.claude/hooks/boundary-check.sh`, `.claude/workflow/belay-version`,
  `docs/templates/CLAUDE.{adopted,bootstrap}.md`, `CLAUDE.md`, and especially
  `scripts/build-index.sh`, whose dependency-edge loop is rewritten in this diff. Missing:
  the package-owned paths written out inline. See the `upstream:` line — the manifest itself
  does not name `belay-version`, so even a spec that quoted `installed` verbatim would not
  cover the whole set.
- **(b) The accept-case floor of 11 has no enumeration to equal.**
  `false-positive cases: $accepted (floor 11)` fails below 11, but §"How counts are stated"
  requires every count to be a floor equal to the names enumerated in its Plan step, and no
  step enumerates accept cases — step 6 says only "*an accept case wherever the pattern could
  plausibly misfire*". §Acceptance criteria lists no accept floor either. Missing: the
  enumeration, or a statement that accept coverage carries no floor.

### Taste, not blocking (round 10)

- `scripts/build-index.sh` uses `[[ … ]]` and `${other##*/}` — bash-only — in a script the
  rest of the repo drives with `sh`. Package-owned; belongs upstream if anywhere.
- New `tests/*.sh` and `tests/fixtures/incomplete_check.sh` are mode `100644` while
  `tests/test_style.sh` is `100755`. §Out of scope already parks `.sh` modes.
- `verify.md` says `make test` "takes about a minute" against a 2m cap: true today (55.9s),
  and a number that drifts upward with every alternative a later phase adds.
- The large hand-enumerated `R-ARCH-01` rejection block is what §Plan step 4 predicted the
  harness would demand, not a finding.

## Deviations (round 10, post-validation spec amendments)

Four of the round-10 validation findings were spec text, not code, and were fixed in
`spec.md` on the operator's order rather than handed to `/implement-phase`. No code changed;
`make test` still exits 0. The other five findings — `contradicts` 1, 3, 5, 6 and round-9's
2b — are code and remain open, so the phase stays returned to implementation.

- **§Plan step 4 no longer says "top-level".** It now says every nesting depth, which is what
  the paragraph below it always argued for and what `alternatives()` implements. Added the
  reason inline: `find_arch01` is one top-level branch wrapping groups of 7 and 24 prefixes,
  so depth-0 splitting yields one mutant for thirty-odd forbidden forms. The code was right
  and the word was stale (`contradicts` 2).
- **§Plan step 0c's premise corrected and a third file added.** The draft named
  `test_repo_shape.sh` among the files that already print `FAIL:` on a shortfall; it does not,
  since round 9 removed its floor and left `ok:   rejection cases: $rejected` printing above
  `[ "$rejected" -gt 0 ] || fail=1`. 0c now owes that line too, and its check is no longer
  circular: it was "the grep count equals the number of files that print one inside a
  floor-passing branch", which compares a number to itself and cannot fail. Now `→ 4`, with
  the branch half stated as read-by-hand because a grep cannot see a branch — three of four
  pass it today (`contradicts` 4). **This leaves one code item owed**, which is the honest
  outcome: the rule is general and the file breaks it.
- **The package-owned paths are written out inline**, as a table with the provenance test
  (`git log --oneline -- <path>` shows `chore(belay):` commits only), replacing a pointer to
  `.claude/workflow/installed` that the reviewer cannot read. Eight rows, including
  `scripts/build-index.sh` — which round 10 singled out — and the two the manifest cannot
  cover: `.claude/workflow/belay-version`, which `install.sh` writes but never lists, and the
  one `CLAUDE.md` line, whose provenance is `694c902`, confirmed with `git log -S`
  (`undecidable` a).
- **The eleven accept cases are enumerated in §Plan step 6**, so the
  `false-positive cases: 11` floor equals a list, as §How counts are stated requires. Also
  corrected the acceptance-criteria paragraph that claimed "the two floors that remain" and
  named two of the five that actually remain — the nine traceability modes, the two boundary
  cases, the two secret-scan cases, the four tool probes and the eleven accept cases. Accept
  coverage is enumerated rather than generated because mutating what a check *finds* says
  nothing about what it must *ignore* (`undecidable` b).

## Implementation — 2026-09-03 (round 11, against the round-10 validation)

Six code findings were open: `contradicts` 1, 3, 5 and 6 from the round-10 review, round-9's
`contradicts` 2b, and the one item round 10's own spec fix created. Round-9's `undecidable`
(a) is closed too, because the fix for `contradicts` 3 ran straight into it. All six are
closed and each was verified by mutation or measurement, never by reading the output.

**`contradicts` 5 — the clean-clone floor.** `Makefile` gave `OPTIONAL_TOOLS=1` to the
`SH_TESTS` loop only, and the harness's `run()` passed no `env`, so every check the harness
re-ran as a subprocess treated a missing external tool as a hard failure. `run()` now sets
`OPTIONAL_TOOLS=1` unconditionally — the harness asks whether a check is wired to the tree,
never whether this machine has gitleaks — and the `PY_TESTS` loop sets it too. The
partial-skip half is closed with it: the old test was `SKIPPED.search(out) and not
RESULT.search(out)`, false for `test_tool_versions.sh` when some tools resolve and others do
not, which left an absent tool's `# LIVE R-TOOL-01: <tool>` label prefixing zero lines and
failed the file. **Any** `skip:` line now makes a file `unproven:`, and an unproven file is
excluded from both mutation properties — every mutant of a skipping file would skip too, exit
0 and count as a survivor, failing the build for the opposite of the real reason. Measured on
a simulated clean clone (`PATH` = a shim holding only `python3`, plus the system directories,
so no gitleaks and no `arm-none-eabi-g++`):

```
before   make test  rc=2   FAIL: test_secrets.sh does not pass on the real tree (rc=1)
                           FAIL: test_style.sh does not pass on the real tree (rc=1)
                           FAIL: test_tool_versions.sh does not pass on the real tree (rc=1)
after    make test  rc=0   4 skip: lines, 3 unproven: lines, OK
```

That simulation is now an acceptance criterion (§Acceptance criteria, clean-clone block), so
it cannot regress silently.

**`contradicts` 3 — the naming convention is gone, and the hole under it was worse than
reported.** The convention was replaced by "every function the file defines", after measuring
that neutering **each** of the 25 parsable functions in the five shell checks makes its file
exit non-zero — finders, shared helpers and case drivers alike, so there was nothing for a
convention to exclude. Then the 26th: `arm_compiles` was *unparsable*, not merely
out-of-convention. The brace walker was quote-aware but not comment-aware, and the comment
line "R-TOOL-01's own floor" opens a single-quote state that never closes, so the walker ran
to end of file and dropped the function. R-TOOL-02's entire check had therefore never been
mutated and the harness said nothing, because it printed only the set it *discovered* — the
defect this file exists to catch, sitting in this file, for the second time (the first was
`\{` in `find_err03`, recorded in round 9). The walker skips comments now. Neutering went
from **12/12 to 26/26**.

**Round-9 `undecidable` (a), reached by the same fix.** With every function mutated,
`first_pattern` was still taking only the first quoted string, so `find_clean03`'s exclusion
list was never touched. Confirmed live: dropping `can|` from `(is|has|can|should)_` left the
file and the harness green while `bool can_fire = true;` silently became a false positive.
`patterns()` now takes **both** quoted strings of a **one-line** body. One line is what keeps
the scope derived instead of listed: a finder is a one-liner by the house shape, and a
multi-line body is a helper whose quoted strings are `sed`/`printf` expressions —
`hits`'s own `s|//.*||` splits on `|` like a regex and produced two mutants that were
"caught" only because sed broke, which inflates the count while proving nothing. The change
immediately demanded two accept cases (`bool can_fire`, `bool should_retry`), which is step 4
working as written; alternations went **55/55 → 59/59** and the accept floor 11 → 13.

**Round-9 `contradicts` 2b — the shared verdict in `tests/test_phase_docs.sh`.** `[ -n
"$found" ]` was written out three times, so gutting only the real run's copy left file and
harness at rc=0. The file now has `report` (the shared verdict), `run_all` (finder → verdict →
return code) and one wiring case asserting that the verdict reaches the return code *and*
names the rule. Verified: `if [ -n "$2" ]; then` → `if false; then` now fails the rejection
case and the wiring case together.

**`contradicts` 6 — three false statements deleted.** Two comments named
`tests/test_checks_are_live.sh`, a file that does not exist (`tests/test_secrets.sh:116`,
`tests/test_repo_shape.sh:154`); `tests/test_boundaries.sh:13` attributed the `HOOK`-stub
requirement to §Plan step 0, which contains no such check — it is step 6, and the comment now
says so and says why the override exists.

**The item round 10's own spec fix created.** `tests/test_repo_shape.sh:220` printed
`ok:   rejection cases: $rejected` above `[ "$rejected" -gt 0 ] || fail=1`. Now inside the
branch, with a `FAIL:` line for a run that produced no case at all.

**`contradicts` 1 — §Out of scope was wrong, not the code, and the measurement decides it.**
§Out of scope said "parallelising is an optimisation nobody has yet needed" while an
acceptance criterion caps the suite at 2m0s. Serially (`workers = 1`) the harness alone takes
**3m11s** for its 85 mutants; with the thread pool the whole suite is **50.6s**. A mutant is a
subprocess that spends its life waiting on other subprocesses, so the pool is I/O concurrency
and the cap cannot be met without it. §Out of scope now says so with both numbers, keeps
everything past the cap out of scope, and names the shared `case_tmp` in
`tests/test_repo_shape.sh` for the same reason.

Harness on the clean tree: accounting `ok:` for all seven check files, `neutered: 26/26`,
`alternations: 59/59`, `bootstrap: fixture gap reported as expected (forbidden_beta)`.
`tests/test_repo_shape.sh`: 55 rejection cases, 13 accept cases, `wiring cases: 8/8`.

## Deviations (round 11)

- **Five spec amendments, all of them the spec being wrong rather than the code.** §Out of
  scope (parallelism, with the 3m11s/50.6s measurement); §Plan step 3 (the naming convention
  removed, with the "all 25 die" measurement that justifies removing it); §Plan step 4 (both
  quoted strings of a one-line finder, and why one-line is the scope rule); §Plan step 6's
  accept enumeration (11 → 13 names); and the §Acceptance criteria paragraph on which floors
  remain (11 → 13). Recorded here because an amended spec requires it.
- **Three acceptance criteria added, not amended.** The two round-9 survivors are permanent
  liveness cases now — delete the real `sweep` from `tests/test_boundaries.sh`, delete the
  real `run_all` call from `tests/test_phase_docs.sh`, each must fail `make test` — plus
  gutting `test_phase_docs.sh`'s shared verdict, plus the clean-clone block, plus a negative
  half for the two new accept prefixes. A finding that cost three rounds and is not in the
  acceptance criteria is a finding that returns.
- **`tests/test_phase_docs.sh` was rewritten rather than patched.** Routing every case through
  one verdict meant a `report`/`run_all` pair and a wiring case; the finder `missing_verify`
  is unchanged. The file is 98 lines against 74.
- **Files touched:** `tests/test_checks_are_live.py`, `tests/test_phase_docs.sh`,
  `tests/test_repo_shape.sh`, `tests/test_boundaries.sh`, `tests/test_secrets.sh`, `Makefile`
  (all named in the Plan: steps 1, 2, 3, 4, 6) and `docs/phases/00-scaffold/spec.md`.
- **Nothing was done about `contradicts` 1's sibling question**, whether a mutant judged only
  by exit code can be "caught for the wrong reason". The two `hits` mutants that raised it are
  no longer generated, so the concrete instance is gone; the general question is untouched and
  stays in §For later phases.

## For later phases (added round 11)

- **The mutant judgement is still exit-code-only.** A mutant that breaks a file syntactically
  counts as caught. Round 11 removed the two instances that existed (the `sed` expressions in
  `hits`) by scoping patterns to one-line bodies, but nothing stops a future check from
  reintroducing the class. The fix, if it is ever worth it, is to require a `FAIL:` line
  naming a rule rather than a non-zero exit — measured and rejected as unnecessary today
  because both instances produced 61 `FAIL:` lines and were therefore indistinguishable from
  a genuine catch by that test. Owner: the first phase whose check is not a grep.
- **An interrupted `make test` still leaves `tests/mut_*.sh` behind** — the harness unlinks in
  a `finally` that SIGTERM skips. Observed three rounds running. A `trap` in the Makefile or a
  `.gitignore` line closes it; neither is owned by any step of this spec.
- **`tests/test_boundaries.sh:111` prints `rejection cases: 2/2` with no `ok:`/`FAIL:`
  prefix**, so the liveness harness's accounting cannot see it and §Plan step 0c's rule does
  not reach it either. Not a defect today — the floor is asserted on the next line — but it is
  the only case count in the suite that is invisible to the harness.

## Validation — 2026-09-03 (round 11)

- criteria: **37 passed / 0 failed.** 14 main (`make test` OK, `make lint` 0, harness
  accounting for all seven files + `neutered: 26/26` + `alternations: 59/59` + `bootstrap:
  fixture gap reported as expected`, traceability 9/9, boundaries `rejection cases: 2/2`,
  repo_shape 55 rejection + 13 accept + `wiring cases: 8/8`, phase_docs rejection + wiring +
  false-positive, secrets both `LIVE` labels, tool_versions six `ok:` lines,
  `planned: 00-scaffold` 0, `STYLE_OPTIONAL` 0 in both, both docs non-empty, `time make test`
  **55.3s** against the 2m cap); 8 liveness mutations, each failing the build, including the
  two that survived round 9; 1 clean-clone block (rc=0, 4 `skip:`, 3 `unproven:`, zero "does
  not pass on the real tree"); 12 adversarial cases each naming its own rule; 2 negative-half
  cases. Tree green after every one
- project gates: test pass, lint pass, typecheck **gap** —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** Both other
  reasons checked rather than assumed: 13 active `deny` lines, prefixes
  `src/{core,hal,usb,app,emu}/`, 0 of the 26 files in the set match one
- index: was **STALE** at `26c04d8`, rebuilt to `d0a41d4`, `--check` clean
- independent review: **contradicts — two, both reproduced before being recorded;
  undecidable — four.** The first review of this round hung: it emitted `I'll read the three
  inputs.`, wrote 137 bytes and stopped. Killed and relaunched with the same three inputs
- closure test: **fail.** The mechanical half passes — four sections present and non-blank,
  and all 19 non-exempt files in the set are reachable from the Plan or from the
  package-provenance table added in round 10. It fails on four `undecidable` verdicts
- upstream: **none new.** No package-owned file misbehaved this round; the provenance table
  did its job and the reviewer raised no question about those paths for the first time in
  three rounds. The open entry (`~/.claude-belay/feedback/pico-sg2hid.md`,
  `commands/validate-phase.md`, 2026-09-02) still stands on its own terms
- verdict: **escaped to /expand-phase: spec re-expanded.** This is validation **#3** against
  the round-9 spec (`## Validation` sections at notes.md:1735 and :1952 follow the
  `escaped to /expand-phase` verdict at :1482), and a gate failed, so the iteration-3+ escape
  applies. **Status set to `pending` by this command**, per its own instruction not to leave
  the flip to the operator

### The two confirmed `contradicts` — and why they are the spec's fault, not the code's

Both are one finding, and it is the **third** consecutive round it has been returned under a
different verdict name. Round 9's reviewer raised it as `undecidable` (b) — "does step 6's
wiring requirement reach the five rules outside `test_repo_shape.sh`?" — and no round has
resolved it in the spec; round 10 was scoped away from it by the operator; round 11 fixed it
in `tests/test_phase_docs.sh` only, and said so. It has now come back as a `contradicts`
because §Goal says "**each rule** gets a wiring case" while §Plan step 6 illustrates the
requirement with `run_all`, which exists in one file.

1. **Wiring cases reach 9 of the 14 rules.** `tests/test_repo_shape.sh` has 8, one per rule;
   `tests/test_phase_docs.sh` has 1. `tests/test_boundaries.sh` (R-ARCH-02),
   `tests/test_secrets.sh` (R-SEC-01) and `tests/test_tool_versions.sh` (R-TOOL-01, R-TOOL-02)
   have none, and the consequence §Goal names is live in all three. Measured this round, each
   against a full `make test`:

   ```
   test_secrets.sh:69   `fail=1` -> `:`   in the tree-scan else branch     make test rc=0
   test_boundaries.sh:66 `fail=1` -> `:`  in the "forbidden direction" arm  make test rc=0
   test_tool_versions.sh:104 drop `|| fail=1` from the clang-tidy probe     make test rc=0
   ```

   Each prints its `FAIL:` line and exits 0. No case, no harness property and no acceptance
   criterion notices.
2. **The shared-verdict requirement reaches 2 of the 5 shell checks.** Only
   `tests/test_repo_shape.sh` and `tests/test_phase_docs.sh` define `report`; in the other
   three every case re-decides from a raw exit code (`sweep` → `rc -eq 1`, `if scan …`,
   `if check_version …`). §Goal's "every rejection and accept case runs **through** the shared
   `report` function" is false of three files.

### The four `undecidable` findings

- **(a) `tests/test_secrets.sh`'s two floors** (`rejection cases: … (floor 2)`,
  `false-positive cases: … (floor 2)`). §Acceptance criteria counts "the two secret-scan
  cases" among the floors that remain, and §How counts are stated requires a floor to equal a
  by-name enumeration in its Plan step. Step 6 enumerates none of this file's cases. Missing:
  the name list, and whether "two" is one floor or both.
- **(b) `tests/test_tool_versions.sh`'s `rejection cases: $rejected/4`.** §Acceptance criteria
  calls the remaining count "the four tool probes", but the probes are pinned by the four
  `# LIVE R-TOOL-01:` labels; the floor of 4 counts four rejection *stubs* that no step
  enumerates. Missing: which four the floor means, and their names.
- **(c) `tests/test_boundaries.sh` asserts `rejected -eq 2`** while §Acceptance criteria says
  `n >= 2`. Equality breaks on a third case; the spec does not say which was intended, and
  §Plan step 6 names the stub-hook case explicitly and the core→hal case only implicitly.
- **(d) Is `tests/test_style.sh` inside the accounting property?** §Goal counts "six checks
  under `tests/`" and does not include it; §Out of scope excludes only *mutating* it. But
  `check_files()` takes every `tests/test_*.{sh,py}`, so its three ids — R-STYLE-01,
  R-STYLE-02, R-CLEAN-02, none of them among the fourteen — must produce non-case result
  lines or the harness fails the file. Missing: a statement of the accounting property's file
  set.

### Why this is an escape and not a twelfth round

Every gate that judges the *code* passed: 37 acceptance criteria, both project gates, the
sweep, the index. What failed is the gate that judges whether the **spec** describes the
code, and the failure is concentrated in two places the spec has never stated precisely:
which files each structural requirement reaches (the two `contradicts`), and which counts are
floors over which enumerations (three of the four `undecidable`). The spec has been amended in
rounds 9, 10 and 11 — eleven separate amendments — and each amendment has produced the next
round's finding somewhere it did not reach. That is the condition the iteration-3+ escape
exists to stop.

What a re-expansion has to settle, so it is not rediscovered a fourth time:

1. **The reach of each structural requirement, per file, by name.** Wiring cases and the
   shared verdict either apply to all five shell checks — in which case three files owe work —
   or the requirement is scoped to the aggregate-shaped checks and §Goal must stop saying
   "each rule". Do not leave it as prose that names `run_all`.
2. **Every floor with its enumeration**, for `test_secrets.sh`, `test_tool_versions.sh` and
   `test_boundaries.sh`, or an explicit statement that those three carry no floor. §How counts
   are stated already demands this and three files do not satisfy it.
3. **The accounting property's file set**, explicitly: whether it is "the six checks" or
   "every `tests/test_*` the Makefile globs", and what happens to a check whose rules are
   outside the fourteen.
4. **Whether `notes.md` §For later phases (round 11) items are in scope**: the exit-code-only
   mutant judgement, the orphaned `mut_*.sh`, and `test_boundaries.sh:111`'s prefix-less count.

## Implementation — 2026-09-03 (round 12, against the re-expanded spec)

Two Plan steps were owed: **W** (Table 2's WC column, 9/14 → 14/14, plus Table 4's three
equalities → floors, plus `test_boundaries.sh`'s prefix-less count) and **T** (the orphaned
`mut_*.sh`). Steps 0-10 were already built and were not touched. Files changed:
`tests/test_boundaries.sh`, `tests/test_secrets.sh`, `tests/test_tool_versions.sh`,
`tests/test_rule_traceability.py`, `tests/test_checks_are_live.py`, and `spec.md` for three
amendments forced by the work. No new file.

**Before, measured, all five rows at exit 0** — the defect, reproduced first as §Plan step W
requires:

```
test_secrets.sh       drop the tree-scan fail=1        make test rc=0
test_secrets.sh       drop the history-scan fail=1     make test rc=0
test_boundaries.sh    drop the breach-arm fail=1       make test rc=0
test_tool_versions.sh drop a probe's || fail=1         make test rc=0
test_tool_versions.sh drop R-TOOL-02's fail=1          make test rc=0
test_rule_traceability.py  real run returns not-failed make test rc=0
```

**After: every one exits non-zero naming its rule.** WC is 14/14, carried by fifteen wiring
cases — R-SEC-01 has two, for the reason below.

### Two things the spec did not anticipate, both found by the measurement

- **A wiring case only bites when the flag it protects lives *inside* the aggregate.** The
  first `test_boundaries.sh` attempt put `|| fail=1` at the call site and had `run_all`
  return a code. The wiring case passed and deleting that flag *still* left the file at
  exit 0 — the defect moved instead of closing. `tests/test_repo_shape.sh` had it right all
  along: its `run_all` sets `fail` itself and the case captures `fail` from a subshell
  (`wiring_flag=$( fail=0; run_all … ; echo "$fail" )`). Every wiring case added here follows
  that shape. Caught because the step's check says to re-run the *before* mutation after the
  fix, not because anything in the spec said so.
- **A rule with N independent failure paths needs N wiring cases.** R-SEC-01 scans the tree
  and the history with a `fail=1` each. One fixture tripping both is satisfied by either flag
  alone: built that way first, it passed while each branch's flag was deleted in turn. It now
  has two cases with fixtures that trip exactly one clause each — a token in the working tree
  never committed, and a token committed then deleted. This is the same reason the rule
  carries two `# LIVE` labels, and the two mechanisms now agree.

The second finding is why `tests/test_tool_versions.sh` has **two** wiring cases and not five.
R-TOOL-01's four probes carried four `|| fail=1` at four call sites — four failure paths, so
four cases by the rule above. Collapsing them into one `probe( )` helper that owns the single
flag is cheaper than writing four cases and proves the same thing, so that is what was done.
R-TOOL-02's stub had to be chosen to fail *only* R-TOOL-02: an `arm-none-eabi-g++` whose
`-dumpversion` clears R-TOOL-01's floor of 12 and which still cannot compile. With the
existing `fake-gcc` stub both rules failed and R-TOOL-01's flag would have satisfied the case.

### Step T — the orphaned mutants

`sweep_orphans()` removes `tests/mut_*.sh` and `tests/fixtures/mut_*.sh` at harness start-up
and from a `SIGTERM`/`SIGINT` handler. Start-up covers a kill that never reached the harness;
the handler covers one that did. Confirmed twice, once deliberately and once by accident: a
9m20s tool timeout killed a `make test` mid-adversarial-block during this round's acceptance
run and left no orphans behind.

## Deviations (round 12)

- **Three spec amendments, all of them the spec's own instruction being followed.**
  (i) The §Acceptance criteria wiring block named `tests/test_secrets.sh:69` and
  `tests/test_boundaries.sh:66`; step W moved both lines, and the block said in that case the
  implementer updates it in the same edit. It is now anchored on **content**, not line
  numbers — the shape that failed silently in round 10 and again in round 11's before-measure,
  where a `sed` on line 116 hit an `echo` and reported a meaningless exit 0.
  (ii) Two mutations in the liveness block anchored on code step W replaced
  (`if scan git "$ROOT" "$hist_out"` and the inline `sweep "$ROOT"` block); re-anchored on
  the new shapes and re-run.
  (iii) L5's expected message was "missing LIVE label"; the history wiring case now fires
  first, and the comment says so.
- **The wiring block grew from five mutations to six**, because R-SEC-01 needs one per clause.
  §Acceptance criteria says so and names the reason.
- **Table 2's *today* column was left at its 5c1422c measurement.** It is dated in the spec
  and its stated purpose is "the gap step W closes"; a pointer was added saying the
  post-implementation state is recorded here. Updating it in place would erase the reason the
  step exists.
- **Files touched, all named in the Plan or the Context pointers:** the four check files and
  the harness (steps W and T), `spec.md` (the three amendments above). Nothing else.

## Debt (round 12)

- No new `belay-debt:` comment. The one item Table 6 puts out of scope — judging a mutant by
  more than its exit code — keeps its owner, `01-ps2-codec`, and is recorded in the spec's
  table rather than as a comment, because there is no single line of code to attach it to.

## For later phases (added round 12)

- **"A rule with N independent failure paths needs N wiring cases" is the general form**, and
  it is not written down anywhere except here and in the two files that obey it. The next
  phase that binds a rule whose check has more than one real-run call should read
  `tests/test_secrets.sh`'s wiring cases before writing its own; the cheaper alternative,
  taken for R-TOOL-01, is to collapse the paths into one flag first.
- **The suite is at 73s against a 2m cap** — up from 55.3s in round 11, because the new wiring
  cases run `gitleaks` twice more and the mutation set grew from 26 to 30 functions. Two more
  wiring cases of the `gitleaks` kind would put the cap in play. When it is threatened the
  answer is §Out of scope's: raise the cap in a re-expansion or cut mutants, not optimise.
- **`tests/test_style.sh` is the only file in the accounting set that nothing else covers**
  (Table 5). Its three rules are not among the fourteen and it is never mutated, so if the
  phase that owns clang-tidy ever changes it, accounting is the only thing that will notice.

## Validation — 2026-09-04 (round 12)

- criteria: **44 passed / 0 failed.** 14 main (`make test` OK, `make lint` 0, harness
  accounting for all seven Table 5 files + `neutered: 30/30` + `alternations: 59/59` +
  `bootstrap: fixture gap reported as expected`, traceability 9/9, boundaries
  `rejection cases: 2/2`, repo_shape 55 rejection + 13 accept + `wiring cases: 8/8`,
  phase_docs rejection + wiring + false-positive, secrets both `LIVE` labels and both wiring
  cases, tool_versions six `ok:` lines and two wiring cases, `planned: 00-scaffold` 0,
  `STYLE_OPTIONAL` 0 in both, both docs non-empty, `time make test` **74.1s** against the 2m
  cap); 8 liveness mutations; **6 wiring mutations, each exiting non-zero naming its own
  rule**; 4 floors (`n/2`, `n/4`, `n/9` all floors now, grep count 5); 1 clean-clone block
  (rc=0, 4 `skip:`, 3 `unproven:`, 0 "does not pass on the real tree"); 2 orphan checks
  (clean run and killed run, no `mut_*.sh` either time); 12 adversarial; 2 negative half
- project gates: test pass, lint pass, typecheck **gap** —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json — the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** 13 active
  `deny` lines, prefixes `src/{core,hal,usb,app,emu}/`, 0 of the 26 files in the set match
- index: was **STALE** at `d0a41d4`, rebuilt to `da1e8a6`, `--check` clean
- independent review: **contradicts — one, confirmed, and confirming it found a second
  instance of the same defect; undecidable — two, both the same missing statement**
- closure test: **fail.** Mechanical half passes — four sections non-blank, and all 20
  non-exempt files in the set are named in the spec, the four that steps W and T touch now
  carrying 11-21 mentions each where the pre-re-expansion spec named them zero times. It
  fails on the two `undecidable` verdicts
- upstream: **none.** No package-owned file misbehaved, and the provenance table added in
  round 10 again drew no reviewer question — second round running. The open entry
  (`~/.claude-belay/feedback/pico-sg2hid.md`, `commands/validate-phase.md`, 2026-09-02)
  stands on its own terms
- verdict: **returned to implementation**, and one spec edit alongside it. This is validation
  **#1** against the round-12 spec (the counter reset at the `escaped to /expand-phase`
  verdict of round 11), so the iteration-3+ escape does not apply

### What the round-12 spec bought

Worth recording because it is the first round in twelve where the review found nothing
structural. The reviewer checked and cleared, by name, the things that failed in rounds 4-11:
the fourteen `planned:` → `test:` flips are exactly Table 1's fourteen; every mutation anchor
in the liveness and wiring blocks matches the shipped source verbatim; the three equalities
Table 4 marks are floors and `wiring cases` stays `-eq 8` as that table says; the 59
alternation spans decompose per finder and each has a covering case; `test_boundaries.sh`
kept `sweep( )`'s `case` as its verdict instead of growing a `report( )`, which the Context
pointer explicitly forbids. Tables made those decidable; prose did not.

### The one `contradicts`, and the second instance found while confirming it

**`docs/phases/00-scaffold/verify.md` tells the operator to expect output the checks no
longer emit.** §Plan steps 0d and 8 require its sample output to match what the checks emit
today, which makes this an implementation failure, not a taste note.

- §4 says "Expect two lines: the catalogue is consistent, and `rejection cases: 9/9`."
  `tests/test_rule_traceability.py` prints **three** since §Plan step W added its wiring case
  to that file in this same round:
  ```
    ok:   R-PROC-01 (catalogue consistent in both directions)
    ok:   R-PROC-01 rejection cases: 9/9
    ok:   R-PROC-01 wiring case (the verdict reaches the exit code)
  ```
- **Found while confirming the above, not reported by the reviewer:** §3 quotes the collateral
  clang-tidy failure as `FAIL: R-STYLE-02 / R-CLEAN-02`. `tests/test_style.sh` separates those
  two ids with a **comma**, not a slash — `ok:   R-STYLE-02, R-CLEAN-02 (no checkable sources
  yet)` on the clean tree. An operator grepping for the quoted string finds nothing.

`verify.md` gained no wiring-case paragraph either, so the operator has no way to check by
hand the property this round exists to deliver. That is the same class and is owed with it.

### The two `undecidable`, which are one missing statement

Both come from `tests/test_style.sh` being the one file in Table 5 whose output the spec never
describes. The reviewer could not decide whether verify.md's §3 sample matches what that check
emits, nor whether Table 5's accounting ✓ for it holds, because the spec says only "a `FAIL:`
line naming the rule" (singular) while the file declares **three** ids and emits them across
**two** lines, the second carrying two ids at once:

```
  ok:   R-STYLE-01 (no C++ sources yet)
  ok:   R-STYLE-02, R-CLEAN-02 (no checkable sources yet)
```

That shape is exactly what the accounting property has to tolerate — one result line naming
two rules — and nothing in §Goal or §Plan says so. Table 5 asserts the ✓ as a measurement the
reviewer cannot reproduce. The fix is a row or a sentence stating `test_style.sh`'s
result-line shape and that accounting accepts several ids on one line; it is a spec edit, and
§Plan step 2's wording is where it belongs.

### Taste, not blocking (round 12)

- `Makefile`'s `OPTIONAL_TOOLS=1` on the `PY_TESTS` loop is inert today: no `.py` check reads
  the variable, and the harness sets it for its own children unconditionally. Harmless, and
  it becomes load-bearing the first time a `.py` check needs an external tool.
- Every wiring case runs its aggregate **twice** — once discarding output to read `fail`, once
  capturing text. Eight times in `test_repo_shape.sh`, and in `test_secrets.sh` that is four
  extra `gitleaks` pairs per run. With the suite at 74.1s against a 2m cap this is the first
  taste note here with a budget attached.
- `property_accounting` `continue`s after the first defect per file, so a file with two
  accounting defects surfaces them one round at a time.
- `src_files( )` / `core_files( )` are one-line bodies whose quoted strings are `find` globs,
  not check patterns; they fall inside §Plan step 4's stated scope and generate no jobs only
  because they contain no `|`.
- `R-ERR-05` still sits between R-ERR-03 and R-ERR-04 in the catalogue (logged round 9).
- `tests/fixtures/incomplete_check.sh` is mode 100644 (§Out of scope parks `.sh` modes).

## Implementation — 2026-09-04 (round 13, against the round-12 validation)

Two routes and one decision, in that order. Route 1 is a spec edit with no command behind
it; route 2 is this command; the decision is what stops route 2 from returning in round 14.

### Route 1 — the spec gap round 12 returned as two `undecidable`

`tests/test_style.sh` was the one file in Table 5 whose output no round had written down, so
its accounting ✓ was a measurement the reviewer could not reproduce. §Plan step 2 now carries
it as two rows, measured by **reading** the file rather than running it — most of its branches
need a tool absent or a source present, so no single run exhibits them:

- `R-STYLE-01` alone on **5** emission sites (`ok:` ×2, `FAIL:` ×2, one skip-or-fail).
- `R-STYLE-02` **and** `R-CLEAN-02` always together on **4** sites (`ok:` ×2, `FAIL:` ×1, one
  skip-or-fail). Nine sites, three ids, at most two lines per run.

And the consequence nothing in the spec had stated: **accounting accepts several ids on one
line.** `property_accounting` applies `RULE_IN_LINE` to each result line and counts every id
it finds, so one line discharges two declarations. A criterion reading "one line per declared
rule" would fail this file, which is why §Goal says *produces a result line* and not *produces
its own*. Table 5's row now points at those rows instead of asserting the ✓ bare.

### The premise round 12 got wrong, found while writing route 1

Round 12's validation recorded a second instance alongside the `verify.md` §4 defect:

> §3 quotes the collateral clang-tidy failure as `FAIL: R-STYLE-02 / R-CLEAN-02`.
> `tests/test_style.sh` separates those two ids with a **comma**, not a slash […] An operator
> grepping for the quoted string finds nothing.

**That is false, and the correction is worth more than the finding was.** The file uses both
separators, chosen by branch, not by file:

```
git stash list >/dev/null; mkdir -p src/core
printf '#include "pico/stdlib.h"\n' > src/core/scratch.h
sh tests/test_style.sh          # rc=1, and the FAIL line is verbatim what verify.md §3 quoted
rm -f src/core/scratch.h; rmdir src/core src
```

Comma on the two `ok:` lines, ` / ` on the `FAIL:` and the skip-or-fail one. §3 describes the
broken tree and quoted the `FAIL:` branch; round 12 checked it against the clean tree's `ok:`
line. So `verify.md` arrived at this round with **one** confirmed defect (§4's line count),
not two, plus the missing wiring-case paragraph.

It is still evidence for the decision below, and the strongest available: a reviewer holding
the literal output beside the document got it wrong anyway, because *which* literal line is
correct depends on a branch the document does not fix. "Keep the sample current" cannot be
executed by a reader who does not know which branch produced the sample.

### The decision — `verify.md` states properties, never a transcript

`verify.md` has gone stale on quoted output in **four rounds out of twelve** (rounds 2, 3, 9,
12; the entries are above). Every time, the round that broke it was a round that changed a
check — which is every round — and every time the fix applied was the instance: update the
sample. It is F2 at the level of the documentation.

Two options were on the table. **(b)** generate `verify.md` from the tree, so the round that
desynchronises it fails. **(a)** forbid literal output in that file and state properties
instead. (a) shipped. (b) buys the same property for the price of a generator plus a template,
and a generated operator document stops being written for the operator, which is the only
thing this file is for.

(a) is not "be careful": §Plan step 8 now bans, by name, a line count, a quoted result line, a
quoted path and the text a check prints after the rule id — and §Acceptance criteria carries
the grep that decides it. The grep extracts every `ok:`/`skip:`/`FAIL:` run up to the next
backtick and requires each to be a bare prefix (prose naming the shape) or `FAIL: <id>` with
nothing after the id. Measured against the file as it arrived: **3**. Against the file as it
ships: **0**. What the grep cannot see is a line *count* — "expect two lines" quotes nothing —
and that is exactly the half round 12 reported, so it is written down as read, not run.

It also removes a defect no round named. Round 2 fixed §2 by pinning the absolute path the
check prints, and what shipped was `/path/to/pico-sg2hid/…`: a **placeholder** inside a block
the surrounding sentence sold as real output. Neither a transcript nor a property, and the
shape no literal-output rule can ever settle — the moment a sample has to be
machine-independent it has stopped being the sample.

### Route 2 — `verify.md` rewritten under that rule

- **§2** — the fenced two-line sample is gone; it now says a line prefixed `FAIL:` naming
  `R-ARCH-03`, with the offending file, line and text indented under it, and tells the
  operator not to match the rest against anything in the document, with the reason.
- **§3** — the collateral clang-tidy failure is described (a second `FAIL:` line naming both
  ids, complaining a header was not found) instead of quoted, with a parenthesis explaining
  that the two ids always travel together and that the separator differs between the success
  and failure branches. That parenthesis is the operator-facing half of route 1.
- **§4** — no line count. It says one `ok:` line per thing proved, each naming `R-PROC-01`,
  and that the number grows as the check gains cases. Measured: three lines today, all three
  naming `R-PROC-01`.
- **§5, new — the wiring case**, which is the property this round of the phase exists to
  deliver and which the operator had no way to check by hand. It explains the split between
  a check *saying* a rule broke and *voting* that the run failed, then has the operator cut
  one wire and watch the suite fail anyway. Measured end to end: replacing
  `report R-ERR-04 "$( find_err04 "$1" )" || fail=1` with the same line minus `|| fail=1`
  gives `make test` rc=2, a `FAIL:` line naming `R-ERR-04` and saying its verdict never
  reached the exit code, and the file's wiring tally dropping below its floor. Restored, `OK`.
- **§What the toolchain checks print** — the two-line sample of `R-TOOL-01`/`R-TOOL-02` output
  is replaced by the property (floor first, version found in brackets after it), which is what
  the paragraph below it was already explaining in words.

## Deviations (round 13)

- **Three spec amendments, all ordered by the operator, none inferred from the tree.**
  (i) §Plan step 2 gained `test_style.sh`'s result-line rows and the "several ids on one line"
  statement; Table 5's prose now points at them. (ii) §Plan step 8 gained the no-transcript
  rule, its rationale and its grep, and §Acceptance criteria gained the grep itself.
  (iii) §Plan step 0d's clause "any sample output must match what the checks emit today" —
  the instance-level rule this decision replaces — now defers to step 8.
- **One of the two defects handed to this round is not a defect.** Recorded above with the
  reproduction. `verify.md` §3's quoted string was verbatim correct for the branch it
  describes; round 12's finding compared it against a different branch. The paragraph was
  rewritten anyway, because quoting *any* branch is what step 8 now forbids.
- **§5's wording was corrected after measuring it.** The first draft promised "the failing
  line names `R-ERR-04` and the words *wiring case*". The mutation produces **two** `FAIL:`
  lines — the rule's, then the tally's — and the words *wiring case* are on the second. Also
  corrected: a first draft claimed the suite prints one wiring-case line per rule. It prints
  eight lines for fourteen rules — one tally covering the eight in `test_repo_shape.sh`, one
  each for the rest, and **two** for `R-SEC-01`, whose check has two real-run calls.
- **A fourth spec amendment, forced by the first independent review of this round.** Step 8
  originally stated the decision as "properties, not transcripts" plus three example shapes.
  The reviewer returned four `undecidable` verdicts that are one thing: the rule moved the
  failure from *the quote is stale* to *the description is unverifiable*, because the spec
  tabulates the result lines of exactly one file — `test_style.sh`, route 1's own subject.
  Every other output claim in `verify.md` was unarbitrable from the spec alone.
  Two ways out, and the obvious one is wrong: tabulating all seven Table 5 files reproduces
  F2 one level up, in `spec.md`, where the same rounds would desynchronise it. What shipped
  instead is a **closed vocabulary, sourced item by item** — the rule id (accounting), the
  case-kind words (step 2's house convention, which by construction reaches a check written
  tomorrow), a count line and the number it is held to (Table 4), WC's two assertions
  (§Goal, Table 2), and `test_style.sh`'s two-ids-on-one-line (route 1's rows). Everything
  else about a check's output is banned **whether quoted or described**, because a described
  format goes stale as fast as a quoted one and the §Acceptance criteria grep only sees the
  quoted kind. The vocabulary is universal and machine-enforced, so it needs no per-file
  table and nothing to keep current.
- **Files touched:** `docs/phases/00-scaffold/spec.md` (routes 1 and the decision),
  `docs/phases/00-scaffold/verify.md` (route 2), this file. No check file changed;
  `tests/test_repo_shape.sh` was mutated and restored for the §5 measurement, and
  `git diff` over `tests/` is empty.

## Debt (round 13)

- No new `belay-debt:` comment. The one shortcut this round takes is stated in the spec
  rather than hidden: the no-transcript grep is a floor under the ban, not the whole of it —
  it cannot see a line count in prose, so that half stays a reading.

## For later phases (added round 13)

- **The no-transcript rule is written into this phase's spec, and R-PROC-02 makes every phase
  write a `verify.md`.** Nothing carries it forward: `docs/constraints.md` has no rule about
  what a `verify.md` may contain, and this one lives in `docs/phases/00-scaffold/spec.md`
  §Plan step 8 where the next phase will not read it. Either it becomes a rule with a binding
  (the grep is already written and is a two-line `test:`) or the next `/expand-phase` copies
  it into its own step. **That is an operator decision, not an implementation detail**, which
  is why it is here and not in `docs/constraints.md`.
- **Every wiring case still runs its aggregate twice** — once discarding output to read
  `fail`, once capturing text. Round 12 logged it as taste with a budget; this round measured
  the budget and left it alone on the operator's instruction. Eight double runs in
  `tests/test_repo_shape.sh`, and in `tests/test_secrets.sh` four extra `gitleaks` pairs per
  run. **Measured this round: `make test` real 1:10.5** (78.2s user, 179.5s system, 365% cpu)
  against the 2m cap in §Acceptance criteria, versus 74.1s recorded in round 12 — inside the
  noise of each other, and both a full 45s under the cap. The upgrade, when the cap is
  actually threatened, is one capture consumed twice, not an optimisation pass; and §Out of
  scope's standing answer applies first: raise the cap in a re-expansion or cut mutants.
- **`property_accounting` still `continue`s after the first defect per file** (logged round
  12), so a file with two accounting defects surfaces them one round at a time. Round 13 hit
  no instance of this; it stays logged because `test_style.sh` is the file most likely to
  produce one, being the only accounting-set member nothing else covers.

## Validation — 2026-09-04 (round 13)

- criteria: **16 passed / 0 failed.** `make test` OK, `make lint` 0, harness accounting for
  all seven Table 5 files (`test_style.sh` reported as 3 rule(s), which is route 1's rows
  confirmed by the machine) + `neutered: 30/30` + `alternations: 59/59` + bootstrap gap
  reported, traceability 9/9, boundaries 2/2, repo_shape 55 rejection + 13 accept +
  `wiring cases: 8/8`, secrets both floors, tool_versions 4/4, `planned: 00-scaffold` 0,
  `STYLE_OPTIONAL` 0 in both files, the new no-transcript grep **0**, orphan mutants 0.
  `time make test` real **1:12.4** against the 2m cap.
- project gates: test **pass**, lint **pass**, typecheck **gap** —
  `workflow gap: no 'typecheck' tool configured — the project was NOT checked. Fix: run
  /adopt-project (re-detect), or add the command to .claude/workflow/toolchain.manual.json —
  the project-owned file re-detection never overwrites.`
- boundary sweep: **not swept: no file in the set is under a declared layer.** 13 active
  `deny` rules exist; the phase's three files are all under `docs/`, and the declared layer
  prefixes are `src/core/`, `src/hal/`, `src/usb/`, `src/app/`, `src/emu/`.
- independent review: **contradicts** — see below. Three passes were run, and the count of
  findings is itself the result.
- closure test: **fail** — the `undecidable` verdicts below are missing pointers by
  definition, and the file set is otherwise clean (`spec.md` and `notes.md` are exempt;
  `verify.md` is named in §Plan step 8).
- upstream: none. `.claude/workflow/installed` is present and no file it names misbehaved.
- verdict: **returned to implementation** by the letter, and **the escape is what this round
  actually recommends** — see "Why the next round should not be round 14" below.

### Three review passes, and why the count is the finding

| pass | reviewed | contradicts | undecidable |
|---|---|---|---|
| 1 | step 8 as *"properties of the output, never a transcript"* + three example shapes | 4 | 4 |
| 2 | step 8 as a **five-row sourced vocabulary table** | 8 | 5 |
| 3 | step 8 as **one axis: what a check *decides* vs how it *formats*** | 3 | 6 |

Three formulations of the same clause in one round, each written to close the previous
pass's `undecidable` verdicts, and each one splitting on the same sentence. Pass 3 named the
place exactly:

> the offending path *is* what the line contains after the rule id, so a single fact is on
> both sides of the axis.

That is `verify.md` §2's core sentence — a result line naming `R-ARCH-03` and the file the
violation is in. `decides` licenses it (§Goal's *Observable behaviour* guarantees the id and
the path); `formats` bans it (the path is the text after the id). No wording of §Plan step 8
separates them, because **the authority the clause needs does not exist at §Goal.** §Goal
says what the suite must do; it has never said what `verify.md` may assert about it, and
step 8 has now been asked to invent that authority three times in one round.

### What was fixed, and what was left standing

Fixed, because each is a defect independent of the axis question:

- **pass 1** — §5 counted lines four lines below the sentence banning line counts (the grep
  cannot see it, which is why the ban is prose as well); §4 claimed the traceability check
  gains *lines* as it gains cases, when Table 4 says a tenth case moves `n/9` to `n/10`
  inside one line; §5 promised a `grep` for *wiring case* returning one line per rule when
  `wiring cases: 8/8` contains the substring and the real count is 8 — the identical hazard
  §Plan step 2 documents for `(history)`; §3 said the `skip:` branch was a failure.
- **pass 2** — `in red` (presentation, guaranteed nowhere); "`R-SEC-01` prints two" (a line
  count); "names `R-PROC-01` in each of them" (accounting guarantees *at least one* line per
  id, not one per proof); the `R-TOOL-01`/`R-TOOL-02` line composition, which was the deleted
  quote reconstituted as prose; round-numbered self-narration in a file step 8 requires to be
  durable.
- **pass 3** — §Plan step 0d still defined the requirement as "properties of the output",
  the formulation step 8 names as its own discarded first attempt; `verify.md` said "also
  print a **second** … line" (count and order) and said it unconditionally, when on the clean
  clone the spec's own §Acceptance block describes there is no clang-tidy and no such line at
  all; "the hole that survived **eight** attempts" contradicts §Goal's "rounds 1-11" and is
  session-scoped besides; Table 5's added sentence asserted an **ordering** ("the second
  naming two ids") that §Plan step 2's rows deliberately do not carry, since no single run
  exhibits every branch.

Left standing, and this is the finding, not an omission: **pass 3's U1, U2, U3 and U4.** They
are one question wearing four hats — by what authority does `verify.md` assert anything about
the suite's behaviour? A fourth wording of step 8 would close some and open others, which is
what the first three did.

### Why the next round should not be round 14

Round 13 was set up to stop `verify.md` going stale a third round running, and decision (a)
did stop the failure it was aimed at: the transcripts are gone, the grep that keeps them gone
is in §Acceptance criteria, and it reads **0** where it read 3. What (a) could not do is
supply an authority §Goal does not have, and three review passes is the evidence.

By the letter this is validation **#2** against the round-12 spec (the counter reset at round
11's `escaped to /expand-phase` verdict at :2270), so the iteration-3+ escape has not fired
and the status stays `in-progress`. By the substance, a fourth attempt at the same clause in
`/implement-phase` is exactly what the escape exists to prevent, and the question it would be
attempting — what an operator document may assert, and on whose authority — is a §Goal
question that `/expand-phase` owns.

**This is an operator decision and is deliberately not taken here.** Two routes:

1. `/expand-phase 00-scaffold` — re-expand with the missing requirement stated at §Goal:
   what `verify.md` may assert, sourced, so §Plan step 8 stops inventing it. Requires the
   status flipped to `pending` first, which `/validate-phase` only does at iteration 3+.
2. `/validate-phase 00-scaffold` again — validation #3, where the escape fires on its own if
   the fourth wording fails like the first three. One more round of cost to reach the same
   place, with the chance that a fourth wording holds.

The recommendation is (1). The cost of being wrong about it is one re-expansion; the cost of
being wrong about (2) is round 14 finding the same sentence.

## Implementation — 2026-09-04 (round 13, continued: the §Goal amendment)

The operator rejected both routes the validation offered and named a third that is better
than either: **the defect is one sentence, so amend §Goal in place** — a re-expansion would
rewrite six tables that had just passed review for the first time in thirteen rounds, and
`/implement-phase` cannot write a Goal.

- **§Goal gained a section, *What `verify.md` may assert about the suite, and on whose
  authority*.** The *Observable behaviour* paragraph is now declared to be the complete set of
  facts `verify.md` may state about a run; everything else about a check's output is out,
  quoted or described. It settles the four things the three step-8 wordings kept splitting on:
  the id and the path are **one fact** the clause guarantees together (so stating them
  together is the clause, not a layout); widening the clause is the only route to letting the
  operator be told something new; `make test`'s own result is inside the clause because it is
  the exit code; and `verify.md`'s references to its own sections are not claims about the
  suite.
- **The clang-tidy fact moved into *Observable behaviour* rather than becoming an exception to
  it.** §Plan step 8 mandates that `verify.md` teach the collateral R-STYLE-02 / R-CLEAN-02
  failure, and every previous wording had to carve it out as a special case — a rule that
  mandates a fact and bans stating it is not a rule. It is now a guaranteed observable with
  its condition attached (*where `clang-tidy` resolves*; where it does not, the check is
  skipped and the failure does not appear), which also closed the last review's finding that
  `verify.md` stated it unconditionally against a clean clone that has no clang-tidy.
- **§Plan step 8 shrank to an implementation of that clause** and now says so explicitly,
  including that it may not narrow or widen it. The three discarded wordings are named there
  in one line so a future round does not retry one of them. §Plan step 0d points at the clause
  too, instead of at whichever wording was current.
- **Step 8's own check no longer demands a prefix the clause does not license.** It read
  "each produces the `FAIL:` line it promises"; it now reads "a failing run naming the rule it
  promises", because §Goal guarantees the id and not the line that carries it.
- **`verify.md` §5 lost two sentences** that were on the allowed side of every previous
  wording and are still not in the clause: the wiring-case tally falling below its number, and
  the per-file shape of how the fourteen wiring cases are reported. What replaced them is the
  clause verbatim — cut any one of the fourteen wires and `make test` fails.

## Deviations (round 13, the §Goal amendment)

- **§Goal was amended, which no earlier round of this phase did.** Every previous spec
  amendment here touched a Plan step, a table or the Acceptance criteria. This one adds a Goal
  clause, on the operator's explicit instruction, after three independent reviews established
  that no Plan-step wording could ground the claim. Recorded because it is the largest kind of
  amendment this workflow allows short of a re-expansion.
- **Three step-8 wordings were written and discarded inside one round** — properties, a
  five-row sourced vocabulary table, a decides/formats axis — at the cost of two review
  passes. All three are named in the spec, not deleted silently, so round 14 does not retry
  one.
- **Process error, mine: `verify.md` was edited while review pass 2 was reading the diff it
  had been given.** Three of that pass's thirteen findings (its C3, U3, and the floor/equality
  half of U4) point at text that had already changed. The evidence of that pass is
  contaminated; passes 1 and 3 ran against frozen diffs. The rule for the next round is that
  the diff handed to the reviewer is frozen until the verdict returns.
- **The reviewer was wrong once, and it is worth recording which way.** Pass 3's U4 claimed
  the spec disagrees with itself in three places about whether `wiring cases: n/8` is a floor.
  It does not: Table 4 marks it `= 8` and §Acceptance criteria's floors block does not list
  it. The claim `verify.md` was making at the time had already been corrected to "has to equal
  eight" before that pass began.

## For later phases (added round 13, the §Goal amendment)

- **"The spec asserts something the reviewer cannot ground" is a shape, not an incident, and
  this is its second instance.** Round 11 failed on a universal stated in prose with no
  enumeration beside it; round 13 failed on a normative clause with no authority behind it.
  Both times the wrong instinct was to reword the Plan step — round 11 amended eleven times,
  round 13 three times in one sitting — and both times the fix was to move the claim to where
  a reviewer can establish it: round 11 to a table with one row per member, round 13 to a
  §Goal clause.
  **The rule, which is what the next phase needs and not the case:** before writing a
  quantified or normative statement in a spec, name in the same breath where a reviewer
  holding only `CLAUDE.md`, that spec and the diff establishes it. If the honest answer is
  "because this Plan step says so", the statement is not a Plan step's to make — it belongs in
  the Goal, or in a table with one row per member.
  `01-ps2-codec` is where this bites next: it quantifies over `tests/vectors/` ("every literal
  vector decodes correctly") and over protocol rules ("R-PROTO-01..04 move to `test:`"). Both
  are universals over sets the spec will have to enumerate, and its acceptance will make
  normative claims about what a decoder may do with a malformed frame, which is the same shape
  as this round's clause.

## Deviations (round 13, the §Goal amendment — second and third correction passes)

Two further review passes ran against the amended Goal. The clause held: neither pass found a
structural gap in it, and every finding was a first-order consequence of it that had not been
applied. What each pass forced:

- **The clause's scope was wrong in its first form** (pass 4). It declared *Observable
  behaviour* the complete set of facts `verify.md` may state "about a run", full stop, which
  read strictly makes `CLAUDE.md` §Teach, don't just deliver unsatisfiable and read loosely
  governs nothing. It now scopes itself to a run's **output**, and says in the same breath that
  facts about what a check *is*, *decides* or how it is *built* are outside it and are founded
  on a Plan step or a table like every other claim in this spec. "This check compares each tool
  against a floor" is the first kind; "its line puts the floor before the version found" is the
  second.
- **Four edges settled by name** (pass 4): `make test`'s `OK` is the exit code's rendering; a
  single check file's own exit status is inside, because step 8's procedures run checks
  directly; a line's **prefix** is outside, and the §Acceptance grep's whitelist of bare
  prefixes is the grep's tolerance and never a licence; the skip is inside for a tool
  `OPTIONAL_TOOLS` governs — not a universal over "tools", since §Plan step 6 makes the
  boundary hook a hard `FAIL` on purpose.
- **`verify.md` stopped naming result-line prefixes anywhere**, including in the historical
  narrative about the four holes, which had described them as printing `ok:` and `FAIL:`. Four
  sentences rewritten. The file now names no prefix at all, which is what the clause says.
- **`Observable behaviour` was widened once, on the declared route** (pass 5). Its naming
  guarantee was attached only to the hand-introduced-violation branch, while §5 tells the
  operator to expect the `fail=1`-removal branch to name `R-ERR-04` — measured true, and true
  by construction, since the wiring case whose assertion fails is what reports it (Table 2,
  WC). Widening the clause is settlement 2's only route and it was taken rather than deleting
  a fact the operator needs.
- **One lie to the operator, written in this round and caught by pass 5.** §4 said that adding
  a tenth traceability failure mode obliges the check to gain a case "before the build will
  pass again". False: Table 5 marks that file `mutated ✗` and Table 4 gives it a floor of
  `>= 9`, so a tenth mode with no case leaves the build green. It promised exactly the
  enforcement §Goal *What this phase does NOT prove* declares as debt owed to `01-ps2-codec`.
  It now says so.
- **"Four things this settles", followed by six bullets** — §How counts are stated's own defect,
  introduced inside the section written to prevent it. The number is gone; the list is the
  enumeration.
- **Table 5's ✓ for `test_style.sh` is now stated as conditional.** It was measured on a machine
  where both clang tools resolve. On the clean-clone floor R-PROC-04 promises, `OPTIONAL_TOOLS=1`
  turns both of that file's sites into skips, it emits no `ok:`/`FAIL:` line, and §Plan step 2's
  own rule makes it `unproven` — which is the correct outcome and not a ✓. Pass 5 found this;
  no earlier round had.

Review counts across the round, recorded because the trend is the evidence:
`4+4`, `8+5`, `3+6`, `5+8`, `5+3`. The `undecidable` column is the one that matters — it is the
spec failing, and it broke downward only after the authority moved to §Goal.

## For later phases (added round 13, correction passes)

- **`tests/test_style.sh`'s source shape is a dated measurement with nothing watching it.**
  §Plan step 2's rows record three ids across nine emission sites in two exclusive chains,
  measured by reading the file. §How counts are stated does not reach it: that rule governs
  counts a *check prints*, and these describe a file's source. Nothing in the suite fails the
  day `test_style.sh` gains a branch, and its accounting ✓ would silently start meaning
  something else. The phase that owns `clang-tidy` — `03-pio-bus`, the first with a
  `compile_commands.json` — is where this becomes cheap to fix, because that is the phase that
  will change the file. Until then the rows are true of the tree they name and of no other.
- **The clean-clone floor and the accounting ✓ are in tension for exactly one file.** On a
  machine with no LLVM, `test_style.sh` is `unproven`, and it is the only Table 5 member whose
  rules nothing else covers. That is not a defect — §Plan step 2 says `unproven` is the honest
  outcome — but it means the clean-clone run proves strictly less about R-STYLE-01, R-STYLE-02
  and R-CLEAN-02 than a developer machine does, and no phase currently owns closing that.

