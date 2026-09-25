# Phase 21-scaffold-filename-splitting — how to check this yourself

Written for someone who does not write C++, shell or firmware. **This phase touches no
hardware**: no Pico, no guitar, no wiring, no multimeter. There are no physical steps — only
three commands to paste into a terminal.

## What was built

The project's safety checks are shell scripts. A shell script receives a list of file names as
one long piece of text, and if that text is used without quotes the shell cuts it at every
space. That cutting is called **word splitting**. For `src/core/a b.cpp` it produces two names,
`src/core/a` and `b.cpp`, and neither file exists. So a file whose name contains a space was
never looked at, and a forbidden thing inside it passed.

Phase 20 fixed this in one script (`tests/test_boundaries.sh`). This phase fixes the other two:

| script | rules affected | before | now |
|---|---|---|---|
| `tests/test_repo_shape.sh` | ten rules (no exceptions, no allocation, no inheritance in core, …) | a violation in `a b.cpp` passed silently | the file is read and the violation reported |
| `tests/test_style.sh` | R-STYLE-01 (layout), R-STYLE-02 (naming) and the two rules on the same line | the check did fail, but only said `No such file or directory` — the real problem was never shown | the file's real diagnostic is shown, and no missing-file error |

Each list is now read one whole line per file name. Each script gained test cases that plant a
violation in a file named `a b.…` and require the check to catch it — three in
`test_repo_shape.sh` (one per file list), two in `test_style.sh` (one per tool).

Still open, on purpose: a file name containing a **line break** is still split. The shell used
here cannot read file names any other way. It was measured in this phase and is written in
R-ARCH-02's text. No file in this project has one.

## Check it yourself

Open a terminal in the repository root. Type `sh` first if your shell is zsh, so the commands
below run as written. Each command cleans up after itself. Checks 2 and 3 need clang-format
and clang-tidy (Homebrew `llvm`), the same as `make lint`.

1. **A space in a file name no longer hides a violation.** Three forbidden things, each in a
   file with a space in its name:

   ```
   printf "void f( ) { throw 1; }\n"        > "src/core/zz probe.cpp"; a=$( sh tests/test_repo_shape.sh | grep -c "^  FAIL: R-ERR-03$" );   rm -f "src/core/zz probe.cpp"
   printf "virtual void poll( );\n"         > "src/core/zz probe.hpp"; b=$( sh tests/test_repo_shape.sh | grep -c "^  FAIL: R-CLEAN-09$" ); rm -f "src/core/zz probe.hpp"
   printf "LinkState step( Link& link );\n" > "src/core/zz probe.h";   c=$( sh tests/test_repo_shape.sh | grep -c "^  FAIL: R-ERR-02$" );   rm -f "src/core/zz probe.h"
   echo "$a $b $c"
   ```

   **Expected:** `1 1 1` — each check reported its violation. Before this phase: `0 0 0`.

2. **The layout check shows the real problem.** A badly spaced line in a spaced file name:

   ```
   printf "int  x = 1;\n" > "src/core/zz probe.cpp"; out=$( sh tests/test_style.sh 2>&1 ); rm -f "src/core/zz probe.cpp"
   printf "%s\n" "$out" | grep -c "zz probe.cpp:.*clang-format-violations"
   printf "%s\n" "$out" | grep -c "No such file"
   ```

   **Expected:** `1` then `0` — one formatting error named on that file, no missing-file error.
   Before this phase: `0` then `2`.

3. **The naming check shows the real problem.** A function named in the wrong style:

   ```
   printf "void BadName( ) {}\n" > "src/core/zz probe.cpp"; out=$( sh tests/test_style.sh 2>&1 ); rm -f "src/core/zz probe.cpp"
   printf "%s\n" "$out" | grep -c "zz probe.cpp:.*readability-identifier-naming"
   printf "%s\n" "$out" | grep -c "no such file or directory"
   ```

   **Expected:** `1` then `0`. Before this phase: `0` then `2`.

Finally run `sh tests/test_style.sh; echo "exit $?"` on its own. **Expected:** no line starting
`FAIL:`, two lines ending `rejection case: file name with a space`, and `exit 0` last. Other
lines (for example a `note:` about the macOS SDK) may appear; they are information, not a
verdict.
