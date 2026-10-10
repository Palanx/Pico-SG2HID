# Phase 09-guitar-observe — notes

## Outcome

- base: `6a3d060` (the phase's own work is the commit after it; the two commits between
  `f9913a6`, the expansion, and the base are the tech-debt work below, committed apart so this
  phase's diff does not carry it)
- not-ours: .claude/rules/tech-debt.md — the operator: the two paid entries removed (3f83e24) and the belay reviewer-breach watch added, both tech-debt log upkeep this spec puts out of scope
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
- **Bench round 1, `verify.md` Part 2 (2026-10-09/10): failed, two defects.** `captures/emu.log`
  (kept as evidence): one good frame, then the emulator never answers again (`k=0`/`k=1`
  alternating), and `analog` answered `error: unknown command`.
  - Parser, fixed: `poll_command( )` built a `string_view` into `reader.text`, then reset the reader
    (`reader = LineReader{}` zeroes `text`) before comparing, so every line compared as NUL bytes
    and `analog` could never match. Reproduced 3/3 on the bench before the fix. The line reader
    moved to `src/core/command_line.{h,cpp}` (`take_char( )` judges before it clears), tested by
    `tests/test_command_line.cpp` (7 cases); hand mutation reintroducing the clear-before-judge
    order prints 4 `FAIL:` lines and exits 1 (2026-10-10). `make test`, `make lint`, `make
    typecheck`, `make firmware` green.
  - Bus desync, not reproduced, cause unproven: persisted across a `picotool reboot -f` of each
    board. Part 3 (`sg2hid.uf2`) then gave `hil: PASS`, but the bench was rewired for it, so that
    pass does not clear the first wiring. The emulator's `SgModel` answers repeated digital polls
    correctly on the host (scratch simulation, 4 frames). Most likely a bad contact in the first
    wiring: the failure outlived both reboots and disappeared after the rewire, while the only
    firmware change since is the parser, which never touches the bus.
- **Bench round 2, `verify.md` Part 2 (2026-10-10): pass**, run by the agent on the operator's
  wiring, firmware `c5abf41`. `captures/emu.log` (replaces round 1's, which stays in `c5abf41`):
  `-> digital` 1, `-> analog` 2, `ok: analog` 1, decoder exit 0, whammy `00 40 80 C0 FF` each
  present, green `BF` at byte 4. One frame aborted at byte 0 (`analog -> absent ack-timeout`, then
  renegotiated) as a `payload` line reached the emulator; the emulator's own USB activity is the
  likely cause, which the guitar does not have. Soak: 30 s of digital polls against the emulator
  in digital mode with six `payload` changes, every printed frame `k=5`, id `41`.
  Part 3 (`hil: PASS`) was run by the operator before this round, on `sg2hid.uf2`, which this
  round's fix does not touch.
  - `cat > file` captures are faithful: a silent port is the trace filter holding back a steady
    frame, not a stalled read.
- **First contact, `verify.md` Part 4 (2026-10-10): stop condition G1, "the guitar does not
  answer".** Operator's captures, untracked until the operator decides their fate. Attempt 1 is
  lost: `sg-digital.log` was started twice and the second `cat >` truncated the first. Attempt 2,
  from a fresh power-up: `sg-digital.log` is one line, the first frame, `01 42 00 00 00` aborted at
  byte 0 with no ACK after 134 µs; nothing else printed through the ten controls, so every frame
  failed identically. `sg-analog.log`: `ok: analog`, then `01 43 00 01 00` aborted at byte 0 the
  same way, and nothing after. No `link:` line in either. Decoder exit 0 on both. Same master
  firmware passed Part 2 on the emulator an hour earlier, and the operator never took the socket
  apart: the emulator was wired through the same socket, so the five signal paths through it are
  the ones Part 2 proved. What the emulator never needed is power: the emulator bench left socket
  pin 5 unwired (`verify.md` 4.3 step 1 adds it). The operator confirms pins
  5 and 4 are wired; Part 2 does not prove that path, because `bus_init( )` also enables the
  Pico's internal pull-ups on DATA and ACK, so the bus works against the self-powered emulator
  with the 3V3 rail loose. The operator knows one breadboard wire is unreliable, possibly the 3V3.
- **Power checked (2026-10-10): good, guitar still silent.** Operator: Pico pin 36, the 3V3 rail
  and socket pin 5 each 3.3 V (BOOTSEL, guitar unplugged); socket pin 5 at 3.25 V with the guitar
  plugged in and powered. Agent, read-only digital polls after a fresh power-up: no line in 20 s
  while the operator pressed and released green repeatedly, so no frame ever completed; the
  firmware was alive (`ping` answered `error: unknown command`). The untracked `sg-*.log` of the
  no-answer attempts are to be overwritten by the next capture, operator's decision; this entry
  is their record. Power is ruled out; the remaining suspects are in step 4 below and the
  socket-contact question in §Resume step 5.
- **Resume here (stopped 2026-10-10, operator away):**
  1. Everything unplugged; reseat the power path: Pico physical pin 36 (3V3 OUT) → 3V3 rail →
     socket pin 5, Pico physical pin 38 (GND) → GND rail → socket pin 4. Replace the bad wire.
  2. Master in BOOTSEL, guitar **unplugged**, multimeter DC 20 V, black probe on the GND rail.
     Red probe on Pico pin 36, then the 3V3 rail, then socket pin 5: each 3.2–3.35 V. A good pin
     36 with ~0 V further along names the loose wire. This is C2 of
     `docs/phases/02-wiring/verify.md` Part C; run C1 too if any wire was moved.
  3. All three good: master unplugged, guitar in, then USB (4.4); recapture 4.5 and 4.6, each
     `cat` started once. The current untracked `sg-digital.log` and `sg-analog.log` (the
     no-answer attempt) are overwritten by that recapture unless renamed first: operator's call,
     still open.
  4. If power measures good and the guitar still does not answer: second hypothesis, lower
     confidence. The master clocks 1–2 µs after ATT falls and adds no gap between bytes; the
     emulator tolerates that, a real controller may not. Arduino's PS2X uses 20 µs
     (`CTRL_BYTE_DELAY`, forum.arduino.cc/t/playstation-controller/22625). A code and protocol
     change: operator's decision, an ADR before it lands.
  5. Closed: the operator wired the emulator by pushing wire ends into the socket's own
     contacts, so Part 2 proved each signal through the connector, contacts included.
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
