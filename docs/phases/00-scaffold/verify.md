# Phase 00-scaffold — how to check this yourself

Written for someone who does not write C++ or firmware. Nothing here needs the Pico, the
guitar, or any hardware at all.

## What was built

Nothing that makes the guitar work. This phase built the **machinery that stops the rest
of the project from lying to you**.

The project has a list of rules in `docs/constraints.md` — things like "never configure
the DATA pin as an output" and "no code in `src/core/` may include a Pico SDK header".
Before this phase, those were sentences in a document. A sentence in a document does not
stop anyone from doing the thing it forbids.

Now fourteen of them are programs that run every time anyone types `make test`, and the
build fails if the rule is broken. The rule text and the program that enforces it are tied
together by an id (like `R-ARCH-01`). One of those fourteen — `R-PROC-01`, enforced by
`tests/test_rule_traceability.py` — is the rule that the tie itself has not come loose:
every rule still points at a real test, and every test still points back at a real rule.

## The problem this phase had to solve, and why it matters to you

Here is the awkward part, and it is the whole reason the phase looks the way it does.

There is **no firmware code in this repository yet**. So a check that says "no file under
`src/` may call `new`" passes right now — not because the rule is enforced, but because
there are no files to look at. A check that does nothing at all would print exactly the
same cheerful `ok:` line.

That means "the tests pass" tells you nothing on its own. You could hand this repo to
someone who wrote fourteen scripts that print `ok:` and do nothing, and you could not tell
the difference by running them.

So every check here has to prove it can **fail**. Each one builds a small fake project in
a temporary folder, puts a deliberate violation in it, and demands that the check catches
it. If a check cannot be made to complain, the build fails — a check that has never
rejected anything is not a check.

Several checks also prove they **do not** complain about legitimate code, because a rule
that cries wolf gets switched off by whoever is annoyed by it, and then it protects
nothing.

This is not paranoia for its own sake. Broken checks here have only ever been caught by
their own rejection cases — reading the code looked fine every time
(`docs/phases/00-scaffold/notes.md` keeps the running list).

## Check it yourself — 5 minutes, no hardware

### 1. Everything passes

```
make test
```

Expect a list of `ok:` lines and `OK` at the end. If it does not say `OK`, stop and say so.

### 2. Break a rule on purpose and watch it get caught

This is the part worth doing. It is the only way to know the checks are real.

```
mkdir -p src/core
printf 'auto* p = new int;\n' > src/core/scratch.cpp
make test
```

Expect it to fail, and expect a line naming the rule. The path is printed in full, and
there is no space after the line number — that is what the check really prints:

```
  FAIL: R-ARCH-03
        /path/to/pico-sg2hid/src/core/scratch.cpp:1:auto* p = new int;
```

`R-ARCH-03` is the rule that says firmware never allocates memory dynamically — on a chip
with 264 KB of RAM and no operating system, running out of memory mid-song is not
recoverable. Look it up in `docs/constraints.md` and you will find that exact id.

Now clean up:

```
rm -f src/core/scratch.cpp
make test
```

Expect `OK` again.

### 3. Try a few more, if you want

Same pattern as §2 — break it, run it, put it back — one rule at a time. Each line
creates its scratch file, runs the suite and removes the file again, so they are
self-contained; run them one at a time anyway and read the output before the next, or the
four `FAIL:` lines scroll past together.

```
printf '#include "pico/stdlib.h"\n' > src/core/scratch.h  ; make test ; rm -f src/core/scratch.h
```
`R-ARCH-01` — `src/core/` stays hardware-free.

```
printf '#include "hal/bus.h"\n'     > src/core/scratch.h  ; make test ; rm -f src/core/scratch.h
```
`R-ARCH-02` — `core` may not depend on `hal`.

```
printf 'bool flag = true;\n'        > src/core/scratch.cpp; make test ; rm -f src/core/scratch.cpp
```
`R-CLEAN-03` — booleans are named `is_`, `has_`, `can_`, `should_`.

```
printf '// TODO: fix\n'             > src/core/scratch.cpp; make test ; rm -f src/core/scratch.cpp
```
`R-CLEAN-05` — a `TODO` must say what closes it, e.g. `// TODO(09-guitar-observe): …`.

The first two also print a second, unrelated failure —
`FAIL: R-STYLE-02 / R-CLEAN-02` with `file not found`. That is not your scratch file
breaking a naming rule: clang-tidy cannot resolve *any* `#include` until the project has a
real build to read, which does not exist until phase `03-pio-bus`. Ignore it here; the
line that matters is the one naming the rule you broke.

Each one should fail naming that rule, and `make test` should say `OK` again as soon as
the file is removed. When you are finished:

```
rmdir src/core src 2>/dev/null ; make test
```

Expect `OK`. (`src/` is empty in this phase and git does not track empty folders, so
removing it puts the repository back exactly as it was.)

### 4. Confirm the rule/test tie is checked, not assumed

```
python3 tests/test_rule_traceability.py
```

Expect two lines: the catalogue is consistent, and `rejection cases: 9/9`. Those nine are
nine different ways the tie between a rule and its test can come loose — a rule pointing
at a file that does not exist, a test that no rule claims, a rule marked "cannot be
automated" that quietly has a test anyway — each one built as a broken example and each
one required to be caught.

## What the toolchain checks print, and why one of them compiles a file

Two of the fourteen are about the tools rather than the code. On a machine with everything
installed they look like this:

```
  ok:   R-TOOL-01: arm-none-eabi-g++ 12+ (15.3.1)
  ok:   R-TOOL-02: /path/to/arm-none-eabi-g++ compiles <cstdint> for cortex-m0plus
```

`R-TOOL-01` reads the floor first and the version actually found in brackets after it, so
the line above says "the rule wants 12 or better, this machine has 15.3.1". If a tool is
not installed at all, `make test` prints `skip:` for it instead of failing — the host tests
are required to run with nothing but a C++23 compiler and `python3`.

`R-TOOL-02` is the one check here that exists purely because of a mistake this project
already made. There were briefly two ARM compilers installed. The one that was easy to
install reported a *higher* version number and could not compile anything, because it
shipped without the standard C library. Any check that compared version numbers would have
approved it. The only thing that catches it is actually compiling a file, which is what
`R-TOOL-02` does every time: two lines of C++ that include `<cstdint>`, built for the
Pico's exact processor. If it ever fails, the compiler on your `PATH` is the wrong one —
not your code.
