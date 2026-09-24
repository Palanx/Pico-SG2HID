# Phase 14-scaffold-nongrep-clauses — how to check this yourself

Written for someone who does not write C++, shell or firmware. **This phase touches no
hardware**: no Pico, no guitar, no wiring, no multimeter. There are no physical steps.

## What was built

Nothing that runs. Five rules in `docs/constraints.md` gained text, and no program, test or
source file changed. The five are:

- R-ARCH-02 — code in one layer (say `src/core/`) never includes a layer it may not depend on.
- R-SEC-01 — no passwords, tokens or keys anywhere in the repository or its history.
- R-TOOL-01 — the tools the checks use are new enough.
- R-TOOL-02 — the ARM cross-compiler on `PATH` can actually compile for the Pico's CPU.
- R-PROC-01 — every rule is tied to exactly one test, and the ties are consistent.

Each now carries a paragraph starting **`Scope, recorded 2026-09-24`** that lists what its check
was measured *not* to see.

## Why a rule that says more than its check matters

Every rule here is enforced by a script (not a simple text search, which is why these five were
handled separately from the seven in phase 16). The project's convention is that **a rule with no
scope text promises full coverage**. None of these five had any. In fact each check has blind
spots, and each one below was measured by feeding the real check a small example:

- R-ARCH-02: `#include <hal/bus.h>` (angle brackets) in `src/core/` is not reported;
  `#include "hal/bus.h"` (quotes) is.
- R-SEC-01: on a machine without `gitleaks` installed, `make test` skips the secret scan and
  still says OK.
- R-TOOL-01: a tool whose version banner starts with an unrelated number (`x86_64-…`) is read
  as that number.
- R-TOOL-02: the check compiles but never links, so a compiler that can compile and not link
  passes.
- R-PROC-01: a rule written with `*` instead of `-` as its bullet is not seen at all.

This phase closes none of the gaps. It makes each rule say how far its check reaches, so no one
relies on protection that isn't there.

## Check it yourself

1. Open `docs/constraints.md` and search for each of the five ids. Each is one long line
   starting with `- **R-…**`.
2. On each line find **`Scope, recorded 2026-09-24`**. Confirm it names a function with `( )`
   after it: `sweep( )`, `scan( )`, `check_version( )`, `arm_compiles( )`, `check( )`.
3. Confirm each line still ends with the same test file it had before (for example
   `tests/test_boundaries.sh` for R-ARCH-02).
4. Re-run one measurement yourself, the angle-bracket include. In a terminal at the
   repository root:

   ```
   S=$(mktemp -d)
   mkdir -p "$S/.claude/workflow" "$S/src/core"
   cp .claude/workflow/boundaries.rules "$S/.claude/workflow/"
   printf '%s\n' '#include <hal/bus.h>' > "$S/src/core/x.cpp"
   CLAUDE_PROJECT_DIR="$S" .claude/hooks/boundary-check.sh "$S/src/core/x.cpp"; echo $?
   ```

   **Expected:** `0` and nothing else — the check does not see it. Then write the quoted form
   and run the last command again:

   ```
   printf '%s\n' '#include "hal/bus.h"' > "$S/src/core/x.cpp"
   CLAUDE_PROJECT_DIR="$S" .claude/hooks/boundary-check.sh "$S/src/core/x.cpp"; echo $?
   ```

   **Expected:** a `BOUNDARY VIOLATION` message and then `2`. Delete `$S` afterwards
   (`rm -rf "$S"`); it is outside the repository.
5. Run `make test`. **Expected:** no `FAIL:` line, and `echo $?` right after prints `0`.

If step 2 finds a rule without the `Scope, recorded` text, step 4 prints different numbers, or
step 5 prints a `FAIL:` line, the phase is not correct.
