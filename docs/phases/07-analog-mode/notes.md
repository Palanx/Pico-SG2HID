# Phase 07-analog-mode — notes

## Outcome

- base: 082b1db (the expansion commit; the phase's work is uncommitted on top of it)

- `docs/adr/0017-analog-negotiation.md`: a pure `core` sequencer; a 5-byte enter judged on its
  prefix; every other frame 9 bytes; `Declined` → `Absent` → retry, never digital streaming; the
  emulator applies a command at the next frame's start.
- `src/core/negotiation.{h,cpp}` (new): `NegotiationStage`, `Master`, `command_for( )`,
  `Exchanged`, `advance( )`, implementing §Goal's judging rules 1–3.
- `src/core/link.h`: `FaultCause::Declined`; the `kNegotiationTimeoutUs` `belay-debt:` marker now
  names `09-guitar-observe`.
- `src/core/poll.{h,cpp}`: `kDigitalPoll` deleted; `count_poll( PollTally&, LinkState now,
  const DecodeOutcome& )` counts `refused` when `now` is `Absent` and payload changes only for
  `AnalogStreaming` frames.
- `src/emu/sg_model.{h,cpp}`: config mode (id `F3`, six `00`), byte 1 accepts `42`/`43` and `44`
  in config mode, `43`/`44` commands applied in `reset( )` when wire byte 3 arrived,
  `FaultKind::Decline` / `fault decline`, `mode` leaves config mode.
- `src/app/main.cpp`: the poller sends `command_for( stage )`, calls `decode_poll`, `advance`,
  `count_poll`, keeps the whammy of the last good analog frame; the summary gains `whammy=`;
  `fault=declined`.
- `tools/hil_digital.py`: `whammy` in `SUMMARY`; streaming expects `state=analog whammy=80`;
  `sweep` and `fault decline` scenarios.
- `tests/vectors/negotiation.h` (new) and a README paragraph.
- `tests/ps2_codec_cases.cpp`: the two debt cases (trailing byte ignored; `us_in_state`
  saturates), seven negotiation cases and the `R-PROTO-10` rule line. `tests/test_ps2_codec.py`
  gains the `RULE R-PROTO-10` marker and one mutation.
- `tests/emulator_cases.cpp`: config-mode sequence cases, `01 44` outside config mode (replacing
  the `01 43` case), `fault decline` and `fault decline x`. `tests/test_emulator.py`: two more
  mutations (4/4).
- `tests/test_poll.cpp`: tally case on the new signature (analog frames), plus "good digital
  frame left `Absent` is refused".
- `tests/driver_support.py` (new): `cxxflags( )` from `make print-CXXFLAGS`, and the mutant cache
  (`build/mutant-cache/<driver>/<key>`, kills only, `FULL=1` bypasses, any other `FULL` fails).
  The five drivers import it with `sys.dont_write_bytecode = True` and print
  `, <k> served from cache`.
- `Makefile`: `print-CXXFLAGS` target; header comment names this phase.
- `docs/constraints.md`: R-PROTO-10 added (`test: tests/test_ps2_codec.py`); R-EMU-01 and
  R-EMU-02 amended for config mode and `fault decline`.
- `docs/phases/07-analog-mode/verify.md`.

Acceptance (2026-10-08, this Mac): every criterion but the bench one passes. `make test` → `OK`
twice; the warm run's five `rejection cases` lines are all served from cache; `make lint`,
`make typecheck`, `make firmware` exit 0.

By hand (Plan step 4): `add_saturating`'s body replaced with `return base + addend;` (plus
`(void)kMax;` so `-Werror` still compiles it) → `FAIL: link: us_in_state saturates at UINT32_MAX
instead of wrapping`. Body restored; `git diff src/core/link.cpp` showed only the step-6 edits.

Carry-over list (Plan step 10): of the listed files this phase changed, `Makefile` names no phase
path; `tests/ps2_codec_cases.cpp` ("notes.md, round 2"), `tests/test_ps2_codec.py`
("see spec.md §Out of scope") and `tools/hil_digital.py` (docstring naming 06's and 07's
`spec.md`) name one in a comment or docstring and open none. Every line stays.
`src/emu/main.cpp` is unchanged.

## Deviations

- Spec §Goal harness table: `fault decline` sends only `fault decline`. It now sends `fault
  decline`, then `mode digital`, because after `sustained` the emulator is already analog and the
  fault only blocks the next `44 … 01`. With `fault decline` alone the master keeps streaming
  analog and the scenario cannot pass. The spec's §Goal table is amended. Checked for the same
  claim: Plan step 9 says "as in §Goal", and the acceptance list has no per-scenario line, so
  neither needed changing.
- Spec Plan step 10 and its acceptance line: `grep -l 'spec.md\|notes.md' <exempt files> | wc -l`
  → `0` cannot hold. The exempt list exists for files that *name* a phase path without reading
  it, and `src/emu/main.cpp`, which this phase does not touch, already matches. Both are amended
  to "every hit is a comment or docstring naming a path, none opens one", checked by hand
  (Outcome above).
- R-PROTO-10's mutation is `if ( false && id.has_value() && … )`, not `if ( false )`. The plain
  form leaves `expected_id( )` unused, `-Werror` fails the build, and the rule line never prints.
- `Poller` holds `has_whammy` next to the whammy, so the summary can print `--` before the first
  analog frame as §Goal asks.
- `tests/test_poll.cpp`'s tally case now runs on `kAnalogIdle` / `kAnalogWhammyFull` with
  `now = AnalogStreaming`, so it includes `tests/vectors/analog_whammy_full.h`, which the
  Context pointers did not list. Resolved in validation round 1: the pointer is now in the
  spec's Context pointers.
- Validation round 1, spec-side, two amendments to spec.md:
  - §Goal emulator bullet "`mode digital|analog` also leaves config mode." now also says a `43`/`44` command still pending from the previous frame takes effect at the next frame's start, after the `mode` line. That is what the code does, and it follows from "a command takes effect at the start of the next frame". Checked for the same fact: Plan step 7 ("the `mode` command clears config mode") still holds, and so does R-EMU-01's text in docs/constraints.md ("`mode digital|analog` also leaves config mode"), which says nothing about pending commands. No other statement asserts it.
  - Context pointers: the vectors line gains `tests/vectors/analog_whammy_full.h` (the missing pointer recorded above). Checked: no other spec line lists the vectors.

## Debt

- `tests/driver_support.py` keys every mutant on every file under `src/`, `tests/`, `tools/` plus
  `docs/wiring.md` (`# ponytail:` comment). Ceiling: any edit there re-runs all five suites
  (about 20 s). Upgrade: a per-suite dependency list.
- The cache trusts a recorded kill until the key changes; a dependency outside the key (none
  known) would serve a stale kill until `make test FULL=1`.
- `kNegotiationTimeoutUs` is still a budget, now owned by `09-guitar-observe` (`belay-debt:` in
  `src/core/link.h`).
- No new `belay-debt:` markers.

## For later phases

- **Operator, after validation:** the three paid tech-debt entries in `.claude/rules/tech-debt.md`
  ("Two `decode` / `step` contracts…", "Every `MUTATIONS`-list mutant…" for the five drivers,
  "Test drivers copy the `Makefile`'s `CXXFLAGS` by hand") are ready to be removed or narrowed.
  The `MUTATIONS` entry still covers `tests/test_trace_shift.py`, which has no build and was out
  of scope. This phase did not edit the log.
- **08-usb-hid**: `Poller` already keeps the last good analog frame's whammy; the HID report
  can take `map_frame( *outcome )` at the same point in `poll_once( )`.
- **09-guitar-observe**: the real SG may answer the sequence differently from the emulator, and
  may need the `5A` padding on leave-config that some libraries send (out of scope here). Both
  are unverified recollections of the protocol documentation, not measurements. The `declined` fault and the `T1`
  line on every link change are how to see it.
- `src/emu/main.cpp`'s header still says the command grammar is in 05-emulator's spec; `fault
  decline` is in 07's. Not edited (the spec did not name the file). Whoever next touches it
  should fix the pointer.
- **Validation 2026-10-09 (round 1), reviewer taste (not blocking):** R-EMU-01's scope clause names "A second copied-tree mutation" before "One copied-tree mutation"; `is_same_bytes( )` wraps `std::ranges::equal` in one line; `reach_polling( )` repeats the happy path's first three steps; `kWireArgumentIndex` and `kWirePayloadStart` are both 3; the five drivers' duplicated `served from cache` print logic could live in `driver_support`; the unreachable fallbacks of `command_for( )` (`kAnalogPoll`) and `next_stage( )` (`EnterConfig`) differ.
- **Validation 2026-10-09 (round 2), reviewer taste (not blocking):** R-EMU-01's scope clause still lists the second mutation before the first; `mutant_key` hashes in two levels (`tree_digest( )` then the mutation) where the spec describes one SHA-256; `kSetAnalog`/`kLeaveConfig` in `negotiation.cpp` are one element per line while `kEnterConfig`/`kAnalogPoll` are inline; the R-PROTO-10 mutation's rationale lives in a comment, not in its label; plus round 1's `kWireArgumentIndex`/`kWirePayloadStart` and `is_same_bytes( )`.

## Bench readings

2026-10-09. Master on `/dev/cu.usbmodem101` (`sg2hid.uf2` from this phase), emulator on
`/dev/cu.usbmodem2101` (`sg2hid_emu.uf2` from this phase), both flashed by the operator, no guitar.
Firmware checked before the run: the master's summary carries `whammy=`, and the emulator answers
`ok: fault decline` (a line the 05/06 emulator refuses).

```
$ make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101
ok: setup
ok: sustained
ok: sweep
ok: fault ack 0
ok: fault ack 3
ok: fault late 200
ok: fault id 79
ok: fault late 50
ok: fault decline
hil: PASS
```

After the run, one renegotiation (`mode digital` sent to the emulator while streaming). The
accepted enter steps no link, so it prints no `link:` line; the trace under each line is the
frame that changed the link:

```
link: analog -> absent fault=declined
T1 n=9 k=4 out=01,42,00,00,00,00,00,00,00 in=FF,41,5A,7F,--,--,--,--,-- us=48,46,46,47,133,-,-,-,-
link: absent -> negotiating fault=declined
T1 n=9 k=9 out=01,44,00,01,03,00,00,00,00 in=FF,F3,5A,00,00,00,00,00,00 us=47,47,47,47,47,47,47,46,36
link: negotiating -> analog fault=declined
T1 n=9 k=9 out=01,42,00,00,00,00,00,00,00 in=FF,73,5A,7F,FE,80,80,80,80 us=47,46,46,47,46,46,46,46,36
```

A streaming summary:

```
hil: polls=245000 refused=69657 changes=7 us=1000008 state=analog fault=declined att=high payload=7F FE whammy=80
```

A summary under `fault decline` then `mode digital`. 500 refused per 1000 frames is the predicted
cycle: of each four frames, the accepted enter (link still `Absent`) and the declined poll end
`Absent`:

```
hil: polls=248000 refused=70473 changes=7 us=1000007 state=negotiating fault=declined att=high payload=7F FE whammy=80
```

- `us` per 1000 frames: 999 995 to 1 000 008, inside the 1 100 000 limit.
- One unexplained frame: right after `fault decline` and `mode digital` were sent back to back, the
  first poll got no ACK at byte 0 (`link: analog -> absent fault=ack-timeout`, `k=0`). After it,
  every frame behaved as predicted. It fell outside every judged window. A plausible cause, not
  verified, is the emulator still handling the second serial line when `ATT` fell. It did not
  happen after a single `mode digital`.

## Validation — 2026-10-09
- criteria: 30 passed / 0 failed (c26 read by hand: every hit a comment or docstring, none opens a phase file; bench line run with MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101 → `hil: PASS`)
- project gates: test pass, lint pass, typecheck pass
- gates tree: ef46ef7ad1b802c6aa1ab79861fc9c0f3b2508d5
- boundary sweep: clean
- independent review: undecidable: §Goal emulator bullets do not say whether a serial `mode` line cancels a `43`/`44` command still pending from the previous frame (src/emu/sg_model.cpp `apply( )` leaves it pending, so `reset( )` applies it after the `mode` line) (settled: 2 — `make test 2>&1 | sed -nE … | awk '$1 != $2 …'` for a `mut_*` directory under the keyed trees; `make hil MASTER=<master port> EMU=<emulator port> | tail -n 1` for `link:`/`T1` volume during a declined loop) (unstated: 14 — `count_poll` counts the accepted enter as refused; `advance( )` re-decodes; main.cpp text-size constants; rewritten header comments; `same_prefix( )` and the `01 43`→`01 44` case swap; config-mode and decline cases folded into R-EMU-01/02 lines; trailing-byte case compares via `payload_matches`; `tree_digest( )` re-runs `cxxflags( )`; FAIL branch prints served count; `sweep` sends lowercase; `fault_scenario( then= )`; verify.md §3 and troubleshooting list; R-EMU-01/02 "Amended" wording; emulator id from `static_cast( mode_id( ) )`)
- closure test: fail: Deviations reports a missing pointer (`tests/vectors/analog_whammy_full.h`, used by tests/test_poll.cpp); the unsettled undecidable above
- findings: 2
- finding keys: §Goal src/emu/sg_model.cpp; §Context pointers tests/test_poll.cpp
- spec size: 25149 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-09 (round 2)
- criteria: 30 passed / 0 failed (c26 read by hand again: every hit a comment or docstring; bench line run with MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101 → `hil: PASS`, exit 0)
- project gates: test pass, lint pass, typecheck pass (carried over)
- gates tree: ef46ef7ad1b802c6aa1ab79861fc9c0f3b2508d5
- boundary sweep: clean
- independent review: clean (settled: 1 — `make test 2>&1 | sed -nE 's/.*rejection cases: [0-9]+\/([0-9]+).*, ([0-9]+) served from cache.*/\1 \2/p' | awk '$1 != $2 { bad=1 } END { exit bad }'` for a `mut_*` directory under the keyed trees) (unstated: 13 — main.cpp constants and `print_trace( frame )`; double decode in `poll_once`/`advance`; reworded `FaultCause` and poll.h comments, poll.h includes link.h; negotiation.cpp helpers and fallbacks; emulator pending-command state, decline read at apply time; emulator_cases helpers and the `01 43`→`01 44` swap; ps2_codec_cases helpers and constants; `fault_scenario( then= )`, lowercase sweep lines; R-PROTO-10/R-EMU wording; verify.md §3, housekeeping, troubleshooting, run times; driver_support internals; `refused` counting every accepted enter). The reviewer also ran one `diff -q` of its CLAUDE.md copy against the repo's (no difference) outside its four inputs; recorded, not used.
- closure test: pass
- findings: 0
- finding keys: none
- spec size: 25299 (+150 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
