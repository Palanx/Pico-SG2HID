# Phase 20-scaffold-check-harness-fixes — how to check this yourself

Written for someone who does not write C++, shell or firmware. **This phase touches no
hardware**: no Pico, no guitar, no wiring, no multimeter. There are no physical steps — only
three commands to paste into a terminal.

## What was built

Phase 14 wrote down five ways three of the project's safety checks could be fooled. This phase
fixes all five, adds a test case for each that proves the check now catches it, and deletes each
from the rule's list of known blind spots in `docs/constraints.md`.

| check | what fooled it | why it mattered | what it does now |
|---|---|---|---|
| `tests/test_boundaries.sh` (R-ARCH-02: `src/core/` may not include hardware code from `src/hal/`) | a source file whose **name has a space**, like `a b.cpp` | the script split the name into `a` and `b.cpp`, neither exists, so the file was never looked at — a forbidden include in it passed | reads each file name whole |
| `tests/test_rule_traceability.py` (R-PROC-01: every rule is tied to a test file that names it) | a test file saying `RULE R-X-011` counted as naming `R-X-01` | a rule could look tested when the only file mentioning it is about a different rule | reads the whole number; `R-X-011` is its own id |
| same | a rule written with `*` instead of `-`, or indented | the check never saw it, so a rule with no test at all passed | reports it as a badly written rule line |
| same | a rule tied to a **folder** instead of a file | the check crashed instead of saying what was wrong | reports "is not a file" |
| `tests/test_tool_versions.sh` (R-TOOL-01: tools are new enough) | a version banner starting with an unrelated number, like `x86_64-apple clang-format version 14.0.6` | the check took the first number it saw (86) as the version, so a too-old tool (14) passed a minimum of 23 | reads the number after the word `version` |

One blind spot found while doing this is still open and is now written in R-ARCH-02's text: a
file name containing a **line break** (possible on Linux and macOS, very unusual) is still
split in two. No file in this project has one.

## Check it yourself

Open a terminal in the repository root. Type `sh` first if your shell is zsh, so the commands
below run as written. Each command cleans up after itself.

1. **A space in a file name no longer hides a forbidden include.**

   ```
   printf '#include "hal/bus.h"\n' > 'src/core/zz probe.cpp'; sh tests/test_boundaries.sh | grep -c 'FAIL: R-ARCH-02: forbidden dependency direction'; rm -f 'src/core/zz probe.cpp'
   ```

   **Expected:** `1` — the check reported the breach. Before this phase it printed `0`.
   Then run `sh tests/test_boundaries.sh` alone. **Expected:** the last line is
   `  ok:   rejection cases: 3/3`.

2. **The four rule-catalogue tricks are reported.** Run `python3 tests/test_rule_traceability.py`.

   **Expected:** three lines, all starting `ok:`, the middle one
   `  ok:   R-PROC-01 rejection cases: 13/13`. Thirteen deliberately broken catalogues are
   fed to the check and it must object to every one; four of them are this phase's.

   To watch those four directly, paste this whole block (it builds each broken catalogue in a
   throwaway folder, runs the real check on it, and deletes the folder):

   ```
   PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
   import sys, tempfile, shutil; sys.path.insert(0, "tests"); import test_rule_traceability as t
   M = "RU" "LE"
   forms = [
       ("longer-id marker", "- **R-X-01** — text — test: `tests/t.sh`\n", {"tests/t.sh": f"# {M} R-X-011\n"}),
       ("star bullet",      "* **R-X-01** — text — test: `tests/missing.sh`\n", {}),
       ("indented bullet",  "- intro\n  - **R-X-01** — text — test: `tests/missing.sh`\n", {}),
       ("directory path",   "- **R-X-01** — text — test: `tests/sub`\n", {"tests/sub/x.sh": f"# {M} R-X-01\n"}),
   ]
   for label, body, files in forms:
       d = tempfile.mkdtemp()
       try:
           print(label + ":", "reported" if t.check(d, *t.make_repo(d, body, files=files)) else "missed")
       except Exception as e:
           print(label + ":", "crashed", type(e).__name__)
       finally:
           shutil.rmtree(d)
   PY
   ```

   **Expected:** four lines, each ending `reported`. Before this phase they read `missed`,
   `missed`, `missed`, `crashed IsADirectoryError`.

3. **A banner with a leading number is read correctly.** Run
   `OPTIONAL_TOOLS=1 sh tests/test_tool_versions.sh`.

   **Expected:** every line starts `ok:` (or `skip:` for a tool not installed), including
   `  ok:   R-TOOL-01 accept case (a leading number before "version" is not the version)` and
   `  ok:   rejection cases: 6/6`. The accept case matters as much as the rejections: it proves
   the fix did not just start refusing every banner that begins with a number.

   To feed it the banners yourself, paste this block. It makes a fake `clang-format` in a
   throwaway folder that prints one banner, asks the real version check whether that clears the
   minimum of 23, and prints `ok` or `rejected` — four banners, in order: two old tools (14)
   with a misleading leading number, then two new ones (23):

   ```
   sh -c '
   S=$( mktemp -d ) || exit 1
   sed -n "/^ver_num( )/,/^}/p;/^resolve( )/,/^}/p;/^check_version( )/,/^}/p" tests/test_tool_versions.sh > "$S/fns.sh"
   . "$S/fns.sh"
   for b in "x86_64-apple clang-format version 14.0.6" "clang-format 99 version 14.0.6" "x86_64-apple clang-format version 23.1.0" "Homebrew clang-format version 23.1.0"; do
       printf "#!/bin/sh\necho \"%s\"\n" "$b" > "$S/clang-format"; chmod +x "$S/clang-format"
       if PATH="$S:$PATH" check_version clang-format 23 --version >/dev/null 2>&1; then printf "ok "; else printf "rejected "; fi
   done; echo; rm -rf "$S"'
   ```

   **Expected:** `rejected rejected ok ok`. Before this phase it printed `ok ok ok ok` — the
   two too-old tools were let through.

4. **The whole suite.** Run `make test`. **Expected:** it ends without any `FAIL:` line and the
   command's exit status (`echo $?` right after) is `0`. It takes a few minutes.
