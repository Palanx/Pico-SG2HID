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
same cheerful pass.

That means "the tests pass" tells you nothing on its own. You could hand this repo to
someone who wrote fourteen scripts that report success and do nothing, and you could not
tell the difference by running them.

So every check here has to prove it can **fail**. Each one builds a small fake project in
a temporary folder, puts a deliberate violation in it, and demands that the check catches
it. If a check cannot be made to complain, the build fails — a check that has never
rejected anything is not a check.

Several checks also prove they **do not** complain about legitimate code, because a rule
that cries wolf gets switched off by whoever is annoyed by it, and then it protects
nothing.

This is not paranoia for its own sake. Broken checks here have only ever been caught by
deliberately breaking something and watching what the output does — reading the code looked
fine every time (`docs/phases/00-scaffold/notes.md` keeps the running list).

### The part that took nine rounds

A rejection case proves a check *can* fire. It does not prove the check is still **pointed at
this repository** — and that turned out to be four separate holes, each of which looked
perfectly fine in the source:

1. The rejection case ran its own private copy while the real scan had been replaced by "do
   nothing". `tests/test_secrets.sh` reported a clean scan of this repo and a clean scan of
   its history when neither was happening any more.
2. A pattern listing thirty forbidden things had rejection cases for two of them. The other
   twenty-eight could be deleted one at a time and every test still passed.
3. The line deciding *whether anything counts as a violation* could be gutted, because the
   cases each re-decided it for themselves instead of asking it.
4. A rule could be found and reported as broken, and still not make the suite exit non-zero.

None of these was found by reading. Every one was found by breaking something on purpose and
noticing that nothing complained. So that is now a check of its own —
`tests/test_checks_are_live.py`, which is why `make test` takes about a minute. It reads the
other checks' source, generates a broken version for every forbidden thing each one looks
for, runs each broken version, and **requires the suite to notice**. Concretely: it deletes
one item from a list of thirty and demands that some test fails. If none does, that item was
being watched by nobody, and the build stops with its name.

That is the difference between this phase and its first eight attempts. Before, a human
noticed a hole and added a case for it. Now the machine finds the holes and refuses to build
until each one has a case. Add a new forbidden thing to any check tomorrow and it will tell
you, on the next run, that nothing tests it yet.

Two things it cannot do, so you know the edges: it does not check the Python file's internal
logic (only that its rules get reported), and it cannot verify itself — for that it keeps a
deliberately incomplete example under `tests/fixtures/` whose known hole it must report every
run. If it ever goes blind, that example stops being reported and it fails on its own floor.

## Check it yourself — 5 minutes, no hardware

### 1. Everything passes

```
make test
```

Expect it to pass, saying `OK`. If it does not, stop and say so.

### 2. Break a rule on purpose and watch it get caught

This is the part worth doing. It is the only way to know the checks are real.

```
mkdir -p src/core
printf 'auto* p = new int;\n' > src/core/scratch.cpp
make test
```

Expect it to fail, and expect a result line naming `R-ARCH-03` and the file it found the
violation in.

Do not match the rest of that line against anything written here. **This document tells you
what a run does — whether it passed, which rule it named, which file — and never what a
result line looks like, including how many of them there are**, on purpose: a check's exact wording changes whenever it gains a case, and
a document that quoted one would be wrong more often than right. The rule id is the part that
lasts, and it is the part you look for.

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
failures scroll past together.

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

**If clang-tidy resolves on your machine**, the first two — the ones whose scratch file has an
`#include` — also fail an unrelated check, naming `R-STYLE-02` and `R-CLEAN-02` together and
reporting a header it could not find. (Without clang-tidy the suite skips that check and you
will not see it at all; `make test` is required to run with nothing but a C++23 compiler and
`python3`.) That is not your scratch file breaking a naming rule: clang-tidy cannot resolve
*any* `#include` until the project has a real build to read, which does not exist until phase
`03-pio-bus`. Ignore it here; what matters is that the rule you broke is named.

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

Expect it to pass. It decides several separate things and reports `R-PROC-01` for what it
decided: that the catalogue is consistent in both directions, that its rejection cases all
fired, and that its verdict reaches the exit code (§5 explains that last one).

Its rejection cases are the different ways the tie between a rule and its test can come
loose — a rule pointing at a file that does not exist, a test whose marker names a rule the
catalogue never declared, a rule marked "cannot be automated" that quietly has a test anyway
— each one built as a broken
example and each one required to be caught. This one check is the exception to the machinery
described above: the machine does not generate its coverage, so its nine examples were written
by hand, and a new way of coming loose would need a new one written by hand too. That is known,
deliberate, and owed to phase `01-ps2-codec`.

### 5. Watch a rule get caught and *still* let the build pass — then watch that get caught

This is the last of the holes listed further up — the one where a rule is found, reported,
and still lets the build pass. It is the least obvious of the four, so it is the one worth
doing by hand.

A check does two separate things when it finds a violation. It **says** so — it names the
rule — and it **votes** so, by setting the flag that decides whether the whole run failed.
Those are two different lines of code, and deleting the second used to leave a suite that
named the broken rule and then passed anyway. That is the trap: the complaint is still there
to read, so nothing looks wrong.
What closes it is a test per rule that refuses to accept the naming without the failure. You
are about to delete one of those votes and watch the suite catch you.

This is not hypothetical: it is the last of the four holes to be closed, and the reason every
one of the fourteen rules now carries a wiring case. You can prove it on any of them; the one
below is convenient because a single line carries the whole wire. First take a copy, because
you are about to edit a test:

```
cp tests/test_repo_shape.sh /tmp/rs.bak
python3 - <<'EOF'
import pathlib
p = pathlib.Path('tests/test_repo_shape.sh'); s = p.read_text()
p.write_text(s.replace('"$( find_err04   "$1" )" || fail=1', '"$( find_err04   "$1" )"', 1))
EOF
make test
```

You have just cut the wire for `R-ERR-04` — the rule against `.value()`, which crashes the
firmware outright on a chip with no exceptions. On any tree carrying that violation the check
would still find it and still say so; its vote would no longer count.

Expect `make test` to **fail anyway**, naming `R-ERR-04`. That is a test written for exactly
this: it plants an `R-ERR-04` violation in a throwaway folder, runs the real check over it,
and demands **both** that the output names the rule **and** that the run came back failed.
Either one alone is satisfied by the broken version you just created — which is why it has
to be both.

Put it back:

```
cp /tmp/rs.bak tests/test_repo_shape.sh ; rm -f /tmp/rs.bak ; make test
```

Expect `OK`. All fourteen rules are covered this way, and `R-SEC-01` twice over, because its
check scans two different things — the files as they are now, and the project's history — and
so has two wires to prove. Cut any one of them and `make test` fails. That is one of the four
ways this suite refuses to pass while something is broken, and it is the last of them to have
been closed.

## What the toolchain checks decide, and why one of them compiles a file

Two of the fourteen are about the tools rather than the code.

`R-TOOL-01` compares the tools it covers against a **floor** — the oldest version the rule
will accept.
It compares the whole version and not just the leading number, so a floor can care about the
minor number too. If one of these tools is not installed at all, `make test` skips it instead
of failing: the host tests are required to run with nothing but a C++23 compiler and
`python3`.

`R-TOOL-02` is the one check here that exists purely because of a mistake this project
already made. There were briefly two ARM compilers installed. The one that was easy to
install reported a *higher* version number and could not compile anything, because it
shipped without the standard C library. Any check that compared version numbers would have
approved it. The only thing that catches it is actually compiling a file, which is what
`R-TOOL-02` does every time: two lines of C++ that include `<cstdint>`, built for the
Pico's exact processor by whichever `arm-none-eabi-g++` comes first on your `PATH` — which
is the point, since the trap it guards against is the wrong one being first. If it ever fails, the compiler on your `PATH` is the wrong one —
not your code.
