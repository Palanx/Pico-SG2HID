# Phase 22-scaffold-typecheck-transitive — how to check this yourself

Written for someone who does not write C++, shell or firmware. **This phase touches no
hardware**: no Pico, no guitar, no wiring, no multimeter. There are no physical steps — only
commands to paste into a terminal.

## What was built

Two automatic checks that, before this phase, reported "pass" without checking what their
names promise.

**1. A header compiles on its own.** C++ code is split into `.cpp` files (the code) and `.h`
files, called *headers* (declarations other files borrow with `#include`). A header that uses
something — say the type `std::uint8_t` — must itself include the header that declares it
(`<cstdint>`). If it forgets, it can still work by luck: every file that uses it happened to
include `<cstdint>` first. The day a new file includes it first, the build breaks somewhere
far from the real mistake.

`make typecheck` now takes each header in `src/core/` alone and asks the compiler to check it
with nothing included before it. A header that relies on luck fails, and the check names it.
`scripts/check.sh` (the gate the workflow runs) now runs this instead of printing
`workflow gap: no 'typecheck' tool configured`. It is **not** part of `make test`, so
`make test` still needs nothing but a C++ compiler and `python3`.

It only covers `src/core/`. The other folders (`src/hal/` etc.) need the Pico SDK to compile
and will be checked by `make firmware`, from phase `03-pio-bus`.

**2. Forbidden dependencies through a middleman.** The project's layers may only depend in
one direction: `core` must never depend on `hal` (the hardware layer). The old sweep read each
file's own `#include` lines, so this chain passed:

```
src/core/x.cpp    includes  common/y.h      (common is in no layer: nothing judges it)
src/common/y.h    includes  hal/bus.h       (core now depends on hal, via y.h)
```

That is a *transitive include*: `x.cpp` never names `hal`, but it gets it through `y.h`. A
second checker, `include-check.sh`, follows such chains; it existed but `make test` never ran
it. `tests/test_boundaries.sh` now runs both checkers on every file, and two new test cases
prove it: one plants exactly that chain and requires it to be refused; one replaces the new
checker with a broken stub and requires "the checker did not run" rather than a pass.

One caveat: `include-check.sh` needs bash version 4 or newer. macOS ships bash 3.2 at
`/bin/bash`; if that is the one found first, the chain case fails `make test` loudly instead
of passing silently. `bash --version` tells you which one you have (Homebrew installs 5.x).

## Check it yourself

Open a terminal in the repository root. Each command cleans up after itself.

1. **Real headers pass.** `make typecheck; echo "rc=$?"`
   **Expected:** no line containing `does not compile`, and `rc=0`.

2. **A lucky header is caught.** A header using `std::uint8_t` without including `<cstdint>`:

   ```
   d=$( mktemp -d ) && printf '#pragma once\ninline std::uint8_t f( ) { return 0; }\n' > "$d/bad.h" && make typecheck CORE_HDRS="$d/bad.h"; echo "rc=$?"; rm -rf "$d"
   ```

   **Expected:** an error containing `undeclared identifier 'std'`, a line
   `typecheck: …/bad.h does not compile on its own`, and `rc=` followed by a non-zero number.

3. **The same header, fixed, passes.**

   ```
   d=$( mktemp -d ) && printf '#pragma once\n#include <cstdint>\ninline std::uint8_t f( ) { return 0; }\n' > "$d/good.h" && make typecheck CORE_HDRS="$d/good.h"; echo "rc=$?"; rm -rf "$d"
   ```

   **Expected:** `rc=0`.

4. **The workflow gate runs it.** `scripts/check.sh typecheck; echo "rc=$?"`
   **Expected:** `== typecheck: make typecheck`, then `PASS`, no `workflow gap` line, `rc=0`.

5. **The chain is refused.** `sh tests/test_boundaries.sh; echo "rc=$?"`
   **Expected:** no `FAIL:` line; a line
   `ok:   R-ARCH-02 rejection case (core -> unlayered header -> hal is refused)`; the line
   `ok:   rejection cases: 5/5`; `rc=0`.

6. **The new case really depends on the new checker.** Swap the checker for a program that
   always says "fine":

   ```
   INCLUDE_HOOK=/usr/bin/true sh tests/test_boundaries.sh; echo "rc=$?"
   ```

   **Expected:** a `FAIL:` line for the chain case (`… returned 0, expected 1 …`) and `rc=1`.
   That is what the sweep reported for this chain before the phase.

7. **The rule's text matches the test.** Rule R-ARCH-02 in `docs/constraints.md` lists what
   the check does *not* catch; the chain above must no longer be on that list, and the new
   checker must be named.
   - `grep -c 'common/y.h' docs/constraints.md` → `0`.
   - `grep -n 'include-check.sh' docs/constraints.md` → at least one line, and it is the
     `R-ARCH-02` line.
   - `python3 tests/test_rule_traceability.py; echo "rc=$?"` → `rc=0`: every rule still points
     at a test that points back at it.

8. **`make test` did not grow a new requirement.** `make -n test | grep -c 'fsyntax-only'` →
   `0`. `make -n` prints what `make test` would run without running it; `0` means the
   typecheck is not among those steps.

9. **Nothing else broke.** `make test` → last line `OK`; `make lint` → exit 0.
