# Phase 16-scaffold-spelling-clauses — how to check this yourself

Written for someone who does not write C++ or firmware. **This phase touches no hardware**:
no Pico, no guitar, no wiring, no multimeter. There are no physical steps.

## What was built

Nothing that runs. Seven rules in `docs/constraints.md` gained text, and no program, test or
source file changed. The seven are R-ARCH-01 (no hardware headers in `src/core/`), R-ARCH-03
(no dynamic allocation), R-CLEAN-03 (boolean names), R-CLEAN-05 (every `TODO` names its owner),
R-CLEAN-09 (no inheritance in `src/core/`), R-ERR-03 (no C++ exceptions) and R-ERR-04 (no
`.value()` on a `std::expected`).

## Why a rule that says more than its check matters

Each of these rules is enforced by one line of `tests/test_repo_shape.sh` that searches the
source text for a pattern. A pattern search does not understand C++: it only finds the exact
shapes of text it was written for. The project's convention is that **a rule with no scope text
promises full coverage** — and these seven had none (R-ERR-03 had some, but only about comments).

In fact every one of them lets real violations through. Examples, each measured, not assumed:

- R-ERR-03: `try` on one line and `{` on the next is not reported; `try {` on one line is.
- R-ARCH-03: `calloc`, `realloc`, `delete[]` and `std::make_unique` are not reported.
- R-CLEAN-03: `bool flag{ true };` is not reported; `bool flag = true;` is.
- R-CLEAN-03 and R-CLEAN-05 are written for the whole repository but only read files under
  `src/`, so a bare `TODO` in a test script is never seen.

Some patterns also err the other way — they report harmless code (R-ERR-04 reports `.value( )`
on a `std::optional`, where it is allowed). That is annoying but safe: it fails loudly.

This phase closes none of the gaps. It makes each rule say how far its check reaches, so no one
relies on protection that isn't there.

## How to check it by hand

1. Open `docs/constraints.md` and search for each of the seven ids. Each is one long line
   starting with `- **R-…**`.
2. On each line find **`Scope, recorded`**. For six rules it is dated `2026-09-24`; for R-ERR-03
   the existing 2026-09-23 clause continues with **`Its regex, measured 2026-09-24`**. Confirm the
   text lists forms that are "outside the binding and none is reported".
3. Confirm each line still ends with ` — test: `tests/test_repo_shape.sh``.
4. Re-run one measurement yourself, the known `try` / `{` case. In a terminal at the repository
   root:

   ```
   S=$(mktemp -d)
   { sed -n '/^src_files( )/,/^# report /p' tests/test_repo_shape.sh | sed '$d'
     /usr/bin/grep -E '^find_[a-z0-9]+\( *\)' tests/test_repo_shape.sh; } > "$S/finders.sh"
   mkdir -p "$S/t/src/core"
   printf '%s\n' 'try' '{' > "$S/t/src/core/x.cpp"
   sh -c ". '$S/finders.sh'; find_err03 '$S/t'"
   ```

   **Expected:** no output at all — the check does not see it. Then, to see what "reported"
   looks like, write the same thing on one line and re-run the last command:

   ```
   printf '%s\n' 'try {' > "$S/t/src/core/x.cpp"
   sh -c ". '$S/finders.sh'; find_err03 '$S/t'"
   ```

   **Expected:** one line ending in `x.cpp:1:try {`. Delete `$S` afterwards (`rm -rf "$S"`);
   it is outside the repository.
5. Run `make test`. **Expected:** no `FAIL:` line, and `echo $?` right after prints `0`.

If step 2 finds a rule without the `Scope, recorded` text, step 4 prints something different,
or step 5 prints a `FAIL:` line, the phase is not correct.
