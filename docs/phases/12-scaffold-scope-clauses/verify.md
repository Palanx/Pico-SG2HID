# Phase 12-scaffold-scope-clauses — how to check this yourself

Written for someone who does not write C++ or firmware. **This phase touches no hardware**:
no Pico, no guitar, no wiring, no multimeter. There are no physical steps.

## What was built

Nothing that runs. Four rules in `docs/constraints.md` each gained one paragraph of text, and
no program, test or source file changed. The four rules are R-STYLE-02 (naming), R-CLEAN-02
(function size), R-PROC-02 (every finished phase has a `verify.md`) and R-ERR-03 (no C++
exceptions under `src/`).

## Why a rule that says more than its check matters

Every rule in `docs/constraints.md` is tied to a program that enforces it, and `make test`
fails when the rule is broken. The project's convention is that **a rule with no scope text
promises full coverage**: if it says "no `throw` anywhere under `src/`", you are entitled to
believe that every `throw` under `src/` would be caught.

For these four rules that belief was false, and nothing said so:

- **R-STYLE-02 and R-CLEAN-02** say "naming matches" and "a function is at most 60 lines, 3
  parameters". The checker only looks at `src/core/` and the C++ files in `tests/`. Code in
  the other firmware folders (`hal`, `usb`, `app`, `emu`) is not looked at, and neither is any
  shell or Python script. There is already a real case: two shell functions in
  `tests/test_repo_shape.sh` take four parameters, and nothing complains.
- **R-PROC-02** says each finished phase has a `verify.md` "written for a non-specialist" with
  "exact physical steps". The checker only proves the file exists and is not empty. A file
  containing the single word "ok" would pass.
- **R-ERR-03** says no `throw`, `try` or `catch`. The checker ignores anything after `//` on a
  line (a comment) but understands nothing else about C++. So the word `throw` inside a
  `/* … */` comment or inside a quoted string is reported even though it is harmless (loud and
  annoying, but not dangerous). Worse, if a line contains a web address in quotes like
  `"http://x"`, the checker thinks everything after the `//` is a comment — so a real `throw`
  later on that same line is **missed silently**. This was measured with small test files, not
  assumed.

A gap you know about is something you can plan around. A gap hidden behind a green test run
gives you false confidence. This phase does not close any of the gaps; it makes each rule say
honestly how far its check actually reaches, so no one relies on protection that isn't there.

## How to check it by hand

1. Open `docs/constraints.md` in any text editor.
2. Search for each of these four ids in turn: `R-STYLE-02`, `R-CLEAN-02`, `R-PROC-02`,
   `R-ERR-03`. Each is one long line starting with `- **R-…**`.
3. On each of those lines, find the text **`Scope, recorded 2026-09-23`**. Read the sentences
   after it and confirm they say what the check does **not** look at:
   - R-STYLE-02: names `tidy_sources()` and says `hal`, `usb`, `app`, `emu` and non-C++ files
     are unchecked.
   - R-CLEAN-02: says the same, adds that shell and Python functions are never measured, and
     names `reject( )` and `accept( )`.
   - R-PROC-02: says the check proves the file is non-empty and reads none of it.
   - R-ERR-03: says `/* … */` comments and strings are reported, and that code after
     `"http://x"` on the same line is not seen.
4. Confirm each line still ends with ` — test: ` followed by the same test file as before
   (`tests/test_style.sh`, `tests/test_phase_docs.sh` or `tests/test_repo_shape.sh`).
5. In a terminal at the repository root, run `make test`.
   **Expected:** it finishes without any `FAIL:` line and the last command exits with status 0
   (`echo $?` right after prints `0`).

If step 3 finds a rule without the `Scope, recorded` text, or step 5 prints a `FAIL:` line,
the phase is not correct.
