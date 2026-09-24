# Phase 15-scaffold-file-lists — how to check this yourself

Written for someone who does not write C++ or firmware. **This phase touches no hardware**:
no Pico, no guitar, no wiring, no multimeter. There are no physical steps — every check below
is a command typed in a terminal at the repository root.

## What was built

**What a file extension is.** The part of a file name after the last dot — `.cpp` in
`link.cpp`. C++ has no single extension: `.cpp` and `.cc` both hold code, `.h` and `.hpp` both
hold declarations ("this function exists and takes these arguments"), and `.inl` holds code
that a header pulls in. The compiler accepts all five.

**Why a check that lists extensions can skip a file silently.** Every automatic check here
starts by building a list of files to read, usually "every file ending in X or Y". A file
ending in anything else is simply never opened. The check prints no warning about it — it
passes, because it found nothing wrong in the files it did read. So a rule that says "no
`virtual` in `src/core/`" was only true for `.cpp` and `.h`: the same code in a `.hpp` passed.

Phase 13 fixed one of those lists. This phase fixes the other four:

| list | what reads it | before | after |
|---|---|---|---|
| `core_files( )` | "no platform headers / no inheritance / no bare status returns in `src/core/`" (R-ARCH-01, R-CLEAN-09, R-ERR-01) | `.cpp`, `.h` | all five |
| `core_headers( )` | "results must be marked `[[nodiscard]]`" (R-ERR-02) | `.h` | `.h`, `.hpp` |
| `sources()` | formatting (R-STYLE-01) | `.cpp`, `.h` | all five |
| `tidy_sources()` | naming, function size, magic numbers (R-STYLE-02, R-CLEAN-02, R-CLEAN-04) | `.cpp`, `.h` under `src/` | all five under `src/` |

R-ERR-02 deliberately stays at declarations only: `[[nodiscard]]` is written where a function
is declared, not where its body is, so reading `.cpp`, `.cc` or `.inl` would flag correct
code. The rule text now says so, including that a declaration placed in a `.inl` is not
checked.

One tool quirk had to be handled: the naming checker (clang-tidy) refuses a `.inl` file
unless told "this is C++" with `-xc++`, failing with `unable to handle compilation, expected
exactly one compiler job`. It now gets that flag for `.inl`, as it already did for `.h`.

The rule texts in `docs/constraints.md` were rewritten to match. The one extension still not
checked under `src/` is `.c` (plain C), and the rules say so.

Every C++ file in the project today is `.cpp` or `.h`, so nothing that passed before fails now.

## Check it yourself — about 5 minutes, no hardware

Each step plants a deliberately bad file, runs a check, and deletes the file, all on one
line. Copy each line exactly. **Expected** is what the terminal should print. Before this
phase, steps 2–5 printed `0` where they now print `1` or `3`.

### 1. Everything passes

```
make test; echo $?
make lint; echo $?
```

**Expected:** no line starting `  FAIL:`, and each `echo $?` prints `0`.

### 2. A `virtual` in a `.hpp`, `.cc` or `.inl` under `src/core/` is caught

```
for e in hpp cc inl; do printf 'virtual void poll( );\n' > src/core/zz_probe.$e; sh tests/test_repo_shape.sh | grep -c '^  FAIL: R-CLEAN-09$'; rm -f src/core/zz_probe.$e; done
```

**Expected:** three lines, each `1`.

### 3. A missing `[[nodiscard]]` in a `.hpp` is caught

```
printf 'LinkState step( Link& link );\n' > src/core/zz_probe.hpp; sh tests/test_repo_shape.sh | grep -c '^  FAIL: R-ERR-02$'; rm -f src/core/zz_probe.hpp
```

**Expected:** `1`.

### 4. Bad formatting in each new extension is caught

```
for e in hpp cc inl; do printf 'int  x;\n' > src/core/zz_probe.$e; sh tests/test_style.sh | grep -c '^  FAIL: R-STYLE-01'; rm -f src/core/zz_probe.$e; done
```

**Expected:** three lines, each `1`. (`int  x;` has two spaces; the formatter wants one.)

### 5. A badly named function in each new extension is caught

```
mkdir -p src/hal; for e in hpp cc inl; do printf 'int BadName( ) { return 0; }\n' > src/hal/zz_probe.$e; done; make lint 2>&1 | grep -E 'src/hal/zz_probe\.(hpp|cc|inl):.*readability-identifier-naming' | grep -oE 'zz_probe\.[a-z]+' | sort -u | wc -l; rm -f src/hal/zz_probe.*; rmdir src/hal
```

**Expected:** `3` — one per extension. Functions must be `lower_case`; `BadName` is not.

### 6. Nothing was left behind

```
git status --porcelain | grep -c zz_probe
```

**Expected:** `0`. If it prints more, delete the `zz_probe` files it lists.
