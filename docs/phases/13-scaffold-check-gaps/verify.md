# Phase 13-scaffold-check-gaps — how to check this yourself

Written for someone who does not write C++ or firmware. **This phase touches no hardware**:
no Pico, no guitar, no wiring, no multimeter. There are no physical steps — every check below
is a command typed in a terminal at the repository root.

## What was built

Phase 12 wrote down, inside five rules, what their automatic checks could not see. This
phase fixed the checks so four of those blind spots are gone, and rewrote the rules so they
describe what the checks now read. Nothing that runs on the Pico changed; only the test
scripts under `tests/`, one comment in `.clang-tidy`, and rule text in `docs/constraints.md`.

The five gaps, in plain terms:

1. **G1 — the naming checker only looked in one folder.** The tool that enforces naming,
   function size and "no magic numbers" (clang-tidy) read `src/core/` and the C++ test files,
   and skipped the four folders the firmware itself will live in: `src/hal`, `src/usb`,
   `src/app`, `src/emu`. It now reads every `.cpp` and `.h` under `src/`. One consequence you
   will see later: when phase `03-pio-bus` adds files that include the Pico SDK, `make lint`
   will fail on them with `clang-diagnostic-error` until that phase gives the tool the SDK's
   include paths. That failure is intended — a loud "I cannot read this file" beats a silent
   skip.
2. **G2 — a web address could hide a forbidden word.** The "no exceptions" check (R-ERR-03)
   ignores comments, and it decided that everything after `//` on a line is a comment. So in
   `const char* u = "http://x"; throw E;` it thought `//x"; throw E;` was a comment and missed
   the `throw`. It now only treats `//` as a comment when the `//` is outside quotes.
3. **G3 — three test helpers broke the "at most three parameters" rule.** `reject( )` and
   `accept( )` in `tests/test_repo_shape.sh` and `check_version( )` in
   `tests/test_tool_versions.sh` each took four. Each now takes three; no test case was
   dropped.
4. **G4 — any non-empty `verify.md` counted as a manual.** The check behind R-PROC-02 (every
   finished phase has a file like this one) only asked "does the file exist and is it not
   empty?". It now also requires two headings: one starting `What was built` and one
   containing `Check it` or `check it` — this file has both. It still cannot judge whether the writing is clear;
   the rule says so.
5. **G5 — files ending in `.hpp`, `.cc` or `.inl` were invisible.** The list of source files
   most `tests/test_repo_shape.sh` checks read contained only `.cpp` and `.h`. It now contains
   all five extensions.

## Check it yourself — about 5 minutes, no hardware

Each step plants a deliberately bad file, runs a check, and deletes the file, all on one
line. Copy each line exactly. **Expected** is what the terminal should print.

### 1. Everything passes

```
make test; echo $?
make lint; echo $?
```

**Expected:** no line starting `  FAIL:`, and each `echo $?` prints `0`.

### 2. G1 — a badly named function in each firmware folder is caught

```
for d in hal usb app emu; do mkdir -p src/$d; printf 'int BadName( ) { return 0; }\n' > src/$d/zz_probe.cpp; done; make lint 2>&1 | grep -E 'src/(hal|usb|app|emu)/zz_probe\.cpp:.*readability-identifier-naming' | grep -oE 'src/[a-z]+/' | sort -u | wc -l; rm -f src/*/zz_probe.cpp
```

**Expected:** `4` — one per folder. Before this phase it printed `0`.

### 3. G2 — a `throw` after a quoted web address is caught

```
mkdir -p src/hal; printf 'const char* u = "http://x"; throw E;\n' > src/hal/zz_probe.cpp; sh tests/test_repo_shape.sh | grep -c '^  FAIL: R-ERR-03$'; rm -f src/hal/zz_probe.cpp
```

**Expected:** `1`. Before this phase it printed `0`.

### 4. G3 — no test helper reads a fourth parameter

```
grep -nE '\$\{?[4-9]' tests/*.sh tests/fixtures/*.sh | wc -l
```

**Expected:** `0`. (`$4` is how a shell function reads its fourth parameter.)

### 5. G4 — a one-line `verify.md` is now rejected

This temporarily replaces phase 12's `verify.md` with the words "see above", runs the check,
and puts the original back:

```
f=docs/phases/12-scaffold-scope-clauses/verify.md; cp "$f" "$f.bak"; printf 'see above\n' > "$f"; sh tests/test_phase_docs.sh | grep -c '^  FAIL: R-PROC-02$'; mv "$f.bak" "$f"; git diff --quiet "$f"; echo $?
```

**Expected:** `1` (the check fired), then `0` (the original file is back, byte for byte).

### 6. G5 — a `throw` in a `.hpp`, `.cc` or `.inl` file is caught

```
mkdir -p src/hal; for e in hpp cc inl; do printf 'throw E;\n' > src/hal/zz_probe.$e; sh tests/test_repo_shape.sh | grep -c '^  FAIL: R-ERR-03$'; rm -f src/hal/zz_probe.$e; done
```

**Expected:** `1` three times, one per line.

### 7. Nothing was left behind

```
rmdir src/hal 2>/dev/null; git status --porcelain | grep -c zz_probe
```

**Expected:** `0`.

### 8. The rules describe the new checks

Open `docs/constraints.md` and search for each id:

- **R-STYLE-02, R-CLEAN-02, R-CLEAN-04** — their scope text says `src/*.cpp`, `src/*.h` and
  `tests/*.cpp`, and no longer lists `hal`, `usb`, `app`, `emu` as unchecked. R-CLEAN-02 no
  longer mentions `reject( )` or `accept( )`, but still says shell and Python functions are
  never measured.
- **R-ERR-03** — no longer says code after `"http://x"` is not seen; still says a forbidden
  word inside `/* … */` or inside quotes is reported (a harmless false alarm).
- **R-PROC-02** — names the two required headings, and says the prose under them is judged by
  no check.

If any step prints something other than its **Expected** value, the phase is not correct.
