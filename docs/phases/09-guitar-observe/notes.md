# Phase 09-guitar-observe — notes

## Outcome

- base: `6a3d060` (the phase's own work is the commit after it; the two commits between
  `f9913a6`, the expansion, and the base are the tech-debt work below, committed apart so this
  phase's diff does not carry it)
- **Where implementation stopped (2026-10-09, round 1):** Plan steps 1, 2, 3 and 6 are done. Steps 4,
  5 and 7 are bench steps owed by the operator (`verify.md` Parts 2, 3 and 4). Steps 8–11 need the
  captures step 7 produces and have not started. The G table is therefore not yet in this file;
  §Outcome gains it in step 11.
- Step 1: `src/app/console.{h,cpp}` (new, namespace `console`) holds `state_name( )`, `fault_name( )`,
  `print_trace( )`, moved unchanged from `src/app/main.cpp`, and the new `print_link( from, to,
  fault )`. `main.cpp` calls them; its serial output is unchanged. `console.cpp` joined
  `SG2HID_MASTER_SOURCES`, so `sg2hid_loopback` links it too (unused there).
- Step 2: `src/core/trace_filter.{h,cpp}` (new): `kTraceGapUs` (10 000), `kMaxTracedLen`,
  `TracedFrame { wire, completed, state }`, `TraceFilter` (the last printed frame),
  `is_trace_due( filter, frame, now_us )` (pure, the Goal's four rules) and
  `note_printed( filter, frame, now_us )` (state only). `tests/test_trace_filter.cpp` (new) runs
  cases (a)–(f) and passes. Hand mutation, as the Plan asks: removing rule 4's
  `&& now_us - filter.printed_at_us >= kTraceGapUs` makes it print `FAIL: (c) …` and `FAIL: (f) …`
  and exit 1 (2026-10-09).
- Step 3: `src/app/observe.cpp` (new) → `build/pico/sg2hid_observe.uf2` through
  `add_master_firmware( sg2hid_observe src/app/observe.cpp )`. Digital mode sends `kReadPoll`
  (`kFrameStart, kCmdPoll, kPadByte ×3`) and judges with `step( decode_poll( ) )`; the line `analog`
  answers `ok: analog` and restarts a fresh `Master` under `advance( )`; any other non-empty line
  answers `error: unknown command`. Prints `link:` on a state change and `T1` when `is_trace_due( )`.
  The `Makefile` header and the `CMakeLists.txt` comments name the fourth firmware.
- Step 6: `docs/phases/09-guitar-observe/verify.md` (new): the R-SAFETY-08 order, the emulator
  proof (Part 2, Plan step 4), the `make hil` regression (Part 3, Plan step 5), and first contact
  (Part 4: emulator off, the `02-wiring` C1/C2 checklist re-run with pin 3 insulated and nothing on
  VBUS/VSYS, guitar in with the Pico unpowered, the two capture scripts, reading the decoder, the four
  stop conditions on screen).
- Host acceptance criteria run 2026-10-09: `make test` OK, `make lint` exit 0, `make typecheck` exit
  0, `make firmware` lists `sg2hid_observe.uf2`, `build/host/test_trace_filter` exit 0 with no
  `FAIL:`, the `verify.md` heading grep → 2, `git diff --quiet 51f1291 HEAD -- src/hal/ps2_master.pio
  src/hal/pio_port.cpp` exit 0. The vector, codec-case, marker, requirements, constraints and notes
  criteria wait for steps 8–11.
- **Tech debt paid in this session (operator's decision, 2026-10-09: "fix them in this session even
  though they don't interfere")**, both entries of `.claude/rules/tech-debt.md` that touch files this
  phase touches:
  - "`make test` runs its 16 test files one after another": the `Makefile`'s `test` recipe now runs
    every test file but `tests/test_checks_are_live.py` in the background into
    `build/host/test-logs/<name>.{log,rc}`, waits, runs the harness last and alone, then prints the
    logs in the old order and folds the exit codes. Output identical in content and order (diffed);
    measured warm: 54.2 s → 26.7 s before the R-ERR change, 32.2 s after. A planted failing file
    still ends the run in `FAIL` and `make: *** [test] Error 1`.
  - "Grep checks have no type information", the R-ERR-01/02 option: `find_err01( )` and
    `find_err02( )` in `tests/test_repo_shape.sh` are `clang-query` matchers through a new
    `cq_hits( )` over every `src/core/` file; `core_headers( )` and its `belay-debt:` are gone. A
    parse error or a matcher with no `N matches.` line is a finding. Cases rewritten as compilable
    fixtures over the real `core/link.h`: 21 rejections (including every spelling the old scope
    clauses listed as unseen, an anonymous-namespace `.cpp` definition and a lambda), 10 accepts
    (including an inherited `[[nodiscard]]` on an out-of-line definition and a marked captureless
    lambda, which forced `unless(isImplicit())`), the unparsable-matcher case and the two wiring
    cases. Without `clang-query`, `make test` prints `skip: R-ERR-01, R-ERR-02` and the case counts
    drop to match (measured with the tool hidden). R-ERR-01 and R-ERR-02's texts are rewritten
    around the new scope; R-ERR-02 now names `std::optional`. R-TOOL-01 gains a `clang-query >= 23`
    probe (`tests/test_tool_versions.sh`, six probes). Liveness harness: 42/42 neutered, 116/116
    alternations caught.
  - The tech-debt log entries were removed after the operator confirmed (see §For later phases).

## Deviations

- Spec said nothing about when the observe firmware starts polling; it now waits for
  `stdio_usb_connected( )` before its first frame, because USB stdio drops output until the host
  opens the port, and the first frame's `link: absent -> digital` line is G1's evidence and an
  acceptance criterion. `verify.md` says so; `src/app/observe.cpp` includes `pico/stdio_usb.h`.
- Spec's Plan names no file for the tech-debt work; the operator ordered both entries paid in this
  session (see §Outcome). Files touched that the spec's pointers and Plan never named: `Makefile`
  (the `test` recipe, beyond its header), `tests/test_repo_shape.sh`, `tests/test_tool_versions.sh`,
  and `docs/constraints.md` R-ERR-01, R-ERR-02 and R-TOOL-01. They are not this phase's Goal; they
  were committed apart by that decision: `1765505` (`make test` in parallel) and `6a3d060`
  (R-ERR-01/02 via `clang-query`). The phase's base is `6a3d060`, so its diff carries neither.
- Spec Plan step 4's check "for each of the five whammy payloads, at least one `T1` line carries its
  byte" is written in `verify.md` as one `grep` per value over nine-byte frames' last `in` byte; the
  spec gives no command.

## Debt

- None new in this phase's code. No `belay-debt:` comment was added; `tests/test_repo_shape.sh`
  lost one (`core_headers( )`'s anonymous-namespace gap, closed by the `clang-query` matcher).

## For later phases

- **This phase, steps 4–11**: resume from `verify.md` Part 2. The observe firmware has never run on
  hardware. If Part 2 fails, the instrument is wrong, not the guitar: fix `observe.cpp` before any
  guitar contact.
- **Tech-debt log**: done 2026-10-09 on the operator's confirmation. The entries "`make test` runs
  its 16 test files one after another" and "Grep checks have no type information" are removed from
  `.claude/rules/tech-debt.md`, and its `paths:` narrowed to the remaining entry's two files.
  Porting the other `test_repo_shape.sh` finders to `clang-query` was not taken and is not logged.
- **10-guitar-full**: `make test` now runs in parallel and writes `build/host/test-logs/`; a phase that
  adds a test file pays nothing extra in wall clock unless the file is the new slowest one.
