# Phase 27-live-mutant-cache — how to check this yourself

No hardware is needed. Nothing on the Picos changes: no firmware is rebuilt or flashed.

## What was built

`make test` runs a file called `tests/test_checks_are_live.py`, the "liveness harness". Its job
is to prove that the other checks really check something. It does that by breaking each check on
purpose, one small piece at a time: a copy with one function emptied, or with one word removed
from a search pattern. Each broken copy is called a *mutant*. It then runs the copy and requires
it to fail. A mutant that fails is "killed", which is good. One that still passes is a
"survivor", and that means part of a check is not being tested.

There are about 160 mutants, and running all of them took about 6 of the 7 minutes `make test`
needed. Most of the time nothing they depend on has changed since the last run, so the answer
cannot have changed either.

Now the harness remembers. When a mutant is killed by one of its check's own test cases, the
harness writes a small marker file in `build/live-mutant-cache/`. The marker's name is a
fingerprint (a SHA-256 hash) of everything that mutant depends on: the harness itself, the full
text of the broken copy, and every repository file the check mentions. Next time, if the
fingerprint is the same, the mutant is skipped. If anything in it changed, even one character,
the fingerprint is different and the mutant runs again.

Four things are never remembered:

- a survivor (it must be reported every time);
- a mutant killed only by the check looking at the real repository, because that depends on
  files the fingerprint does not cover;
- a mutant that failed without printing any case `FAIL:` line (for example, one that crashed),
  because nothing says a case caught it;
- the harness's own self-test (`bootstrap`) and its first property (`accounting`). These are
  cheap and always run.

`make test FULL=1` ignores the cache and runs every mutant. Use it when you want certainty.

## Check it yourself

1. Open Terminal in the repository folder and run `make test` twice. The first run can take
   about 6–7 minutes; the second one should finish in roughly a minute and a half. Both must end
   with the line `OK`.

2. In the output of the second run, find the lines that start with `ok:   cache:`. There is one
   per check file, for example:

   ```
     ok:   cache: test_repo_shape.sh 145/145 served from cache
   ```

   The two numbers should be equal on every line. That means every mutant of that file was
   answered from the cache. The exact totals change as checks are added.

3. Optional, to see it re-run only what changed: add a blank comment line at the end of
   `tests/test_phase_docs.sh` (for example `# probe`), save, and run
   `OPTIONAL_TOOLS=1 python3 tests/test_checks_are_live.py`. The `test_phase_docs.sh` line now
   shows `0/4` (or `0/<its total>`), and every other line still shows equal numbers. Remove the line you added
   and save.

4. Run `make test FULL=1`. It takes the full 6–7 minutes and ends with `OK`. Instead of the
   per-file cache lines it prints `ok:   cache: bypassed (FULL=1)`.

## If something looks wrong

- **Deleting the cache is always safe.** `rm -rf build/live-mutant-cache` (or `make clean`)
  only makes the next run slow again.
- **`make test FULL=1` fails with a line ending `(cached as killed: a key misses a
  dependency)`.** The cache had trusted a result that is no longer true. That means the
  fingerprint misses something the check depends on. Delete the cache, and report it, because
  the harness needs to be fixed.
- **When to run `FULL=1` on purpose:** before closing a phase that upgraded a tool `make test`
  uses (`gitleaks`, `bash`, the compilers). Tool versions are not part of the fingerprint.
- **`FULL` set to anything other than `1`** (for example `FULL=yes`) makes the harness stop
  with a `FAIL:` line naming the value. Use `FULL=1`, or leave it out.
