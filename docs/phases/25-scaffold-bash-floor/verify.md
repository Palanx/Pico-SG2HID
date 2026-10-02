# Phase 25-scaffold-bash-floor — how to check this yourself

Written for someone who does not write shell scripts. **This phase touches no hardware**: no
Pico, no guitar, no wiring, no multimeter. There are no physical steps — only commands to paste
into a terminal opened in the repository root.

## What was built

`make test` already needed bash version 4 or newer; this phase makes that need checked up
front and written down.

**Why the bash version matters.** bash is the program that runs shell scripts. One script the
tests run, `.claude/hooks/include-check.sh` (the checker that follows `#include` chains between
layers), uses `local -A`: it declares an *associative array*, a table that maps names to values
(here, "file already visited → yes"). bash learned that in version 4 (2009). macOS still ships
bash 3.2 at `/bin/bash`, for licensing reasons, and in 3.2 `local -A` is an error. The script's
first line is `#!/usr/bin/env bash`, which means "run me with whichever `bash` comes first on
`PATH`" — so what matters is not which bashes are installed, but which one your shell finds
first. Until now, a 3.2 found first showed up only as one confusing failure in the middle of
`make test` (the R-ARCH-02 chain case).

**What changed.**

- `tests/test_tool_versions.sh` (rule R-TOOL-01, "every tool meets its minimum version") gained
  a fifth check: the `bash` first on `PATH` must report version 4 or higher. It looks only at
  `PATH` — exactly where `include-check.sh` will look — never at other install locations.
- The same file now proves that check can fail: it builds a fake `bash` that prints macOS's 3.2
  banner and requires the check to reject it. The count of such proofs went from 6 to 7.
- Rule R-TOOL-01's text in `docs/constraints.md` names `bash` >= 4 and why.
- `CLAUDE.md`, `README.md` and `docs/constraints.md` no longer say `make test` needs only a C++
  compiler and `python3`; each names bash >= 4.

## Check it yourself

1. **Which bash do you have?**

   ```
   command -v bash
   bash --version | head -1
   ```

   **Expected:** a path (on this machine `/opt/homebrew/bin/bash`) and a line starting
   `GNU bash, version 5.` (any 4.x or 5.x is fine). If you see `/bin/bash` and `version 3.2`,
   install a newer one with `brew install bash` and open a new terminal.

2. **The real bash passes.** `OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh; echo "rc=$?"`
   **Expected:** no `FAIL:` line; a line starting `  ok:   R-TOOL-01: bash 4+ (GNU bash, version
   5…`; the line `  ok:   rejection cases: 7/7`; both `wiring case` lines start `ok:`; `rc=0`.

3. **An old bash is refused.** This puts a fake bash that claims to be 3.2.57 first on `PATH`
   for one run, then deletes it:

   ```
   d=$( mktemp -d ) && printf '#!/bin/sh\necho "GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)"\n' > "$d/bash" && chmod +x "$d/bash" && PATH="$d:$PATH" OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh > "$d/out" 2>&1; echo "rc=$?"; grep 'FAIL: R-TOOL-01: bash' "$d/out"; rm -rf "$d"
   ```

   **Expected:** `rc=1` and one line
   `  FAIL: R-TOOL-01: bash is below the floor of 4 (GNU bash, version 3.2.57(1)-release …)`.
   Before this phase the same command printed `rc=0` and no such line.

4. **The new check has its own label.** `grep -c '^# LIVE R-TOOL-01: bash$' tests/test_tool_versions.sh`
   → `1`. Then `python3 tests/test_checks_are_live.py; echo "rc=$?"` → `rc=0`: the harness
   that proves every check can fire accounts for the new one.

5. **The rule's text matches the test.**
   - `grep -c "four .probe. calls" docs/constraints.md` → `0` (it now says five).
   - `python3 tests/test_rule_traceability.py; echo "rc=$?"` → `rc=0`.
   - `grep -c 'any toolchain beyond a C++23 compiler and `python3`' docs/constraints.md` → `1`:
     rule R-PROC-04 is deliberately unchanged — it limits *toolchains* (compilers), and bash is
     an interpreter.

6. **The prose names bash.** `grep -n 'bash >= 4' CLAUDE.md README.md docs/constraints.md`
   → at least one line in each of the three files: the sentence about what `make test` needs.

7. **Nothing else broke.** `make test` → last line `OK`; `make lint` → exit 0.
