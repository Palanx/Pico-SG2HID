# Phase 06-hil-digital — notes

## Outcome

- base: c3f7c66
- `src/core/poll.{h,cpp}` (new, pure): `kDigitalPollLen`, `kDigitalPoll` (`01 42 00 00 00`), `decode_poll( wire, completed )`, `PollTally` and `count_poll( )`. `decode_poll( )` drops wire byte 0's answer and hands `decode( )` at most an analog frame, never reading past `completed`.
- `tests/test_poll.cpp` (new): the spec's step-2 cases, built from `tests/vectors/` only. `make test` runs it through its `tests/test_*.cpp` wildcard. The step-2 mutation was run: with `decode_poll( )` reading wire byte 0 as well, 5 of the 9 checks fail and the test exits 1. The file was then restored byte for byte, and the test is green again.
- `src/app/loopback.cpp` (new): today's loopback program. Only its first comment line changed, to name `sg2hid_loopback.uf2`.
- `src/app/main.cpp` (rewritten): the link poller.
  - It polls every 1000 µs, start to start: `decode_poll( )` → `count_poll( )` → `step( )`. `elapsed_us` is the unsigned difference of two `time_us_32( )` readings.
  - Every 1000 polls it prints a `hil:` summary. On a link change it prints a `link:` line, then that poll's `T1` line.
  - It configures no pin.
- `CMakeLists.txt`: the `src/app/*.cpp` glob is gone. `add_master_firmware( target main_source )` builds `sg2hid` from `main.cpp` and `sg2hid_loopback` from `loopback.cpp`. Both get the same settings: per-source `-Wall;-Wextra;-Werror`, `ps2_master.pio`, `pico_stdlib hardware_pio`, USB stdio on, UART stdio off, and extra outputs.
- `Makefile`: the header names the three UF2s and `make hil`. New `hil` target, also added to `.PHONY`.
- `tools/hil_digital.py` (new, executable, standard library only): the seven scenarios and their recovery checks, the `master silent` timeout, the `att=high` and `us` rules, and `fault none` on exit.
- `docs/constraints.md`: R-PROTO-01's `manual:` reason names `build/pico/sg2hid_loopback.uf2`. R-PROTO-06 and R-PROTO-07 each gain a dated `**Observed 2026-10-06 (`06-hil-digital`):**` sentence; R-PROTO-07's "(inferred, not yet observed)" is removed. No binding changed.
- The flash lines in `docs/phases/24-pio-bus/verify.md:128`, `docs/phases/04-trace-mode/verify.md:155` and `docs/phases/05-emulator/verify.md:94` now copy `sg2hid_loopback.uf2`.
- `docs/phases/06-hil-digital/verify.md` (new).
- Acceptance: all 18 criteria pass on 2026-10-06. `make test` → `OK`, and the bench → `hil: PASS` (see `## Bench readings`). The firmware was rebuilt after the bench run with no source change.

## Deviations

- Spec step 3 lists the settings the two master targets share. They are written once, in a CMake `function( add_master_firmware … )` called twice, not as two copied blocks, so the targets cannot drift apart.
- Spec step 5 Check says `make hil` with no variables exits 1. The recipe does exit 1 after printing the usage line, but `make` reports any failed recipe as exit 2, and the recipe cannot change that. The acceptance criterion (`test $? -ne 0`) holds.
- Spec step 6 has the operator flash both Picos with the signal wires out. On the operator's instruction, the agent flashed them instead: the master through `picotool load -f --ser` and a copy to `RPI-RP2`, the emulator by a copy to `RPI-RP2`. The emulator was in BOOTSEL and unwired when it was flashed. No guitar was on the bench at any point (R-SAFETY-08).
- Round 2 (validation finding §Goal `tools/hil_digital.py`): fixed as code-side, so the Goal's "last line is `hil: PASS` or `hil: FAIL`" holds without a scope limit. Property: every exception a `Port` call can raise ends in `FAIL: <scenario>: <reason>` + `hil: FAIL`. Enumerated every OS-level call in the harness: `os.open` (OSError), `tty.setraw` (termios.error), `os.write` (OSError), `termios.tcdrain` (termios.error), `select.select` (OSError; `select.error is OSError`), `os.read` (OSError, `BlockingIOError` already handled). `termios.error` was the only uncaught class, missed in two places: `main( )`'s handler and the `finally` that sends `fault none` (whose `tcdrain( )` could raise after `hil: FAIL` was printed). Both now catch one `PORT_ERRORS = (OSError, termios.error)` tuple. Not covered on purpose: `KeyboardInterrupt` (operator abort, not a harness outcome). Checked: `--master /dev/null --emu /dev/null` and `--master /nonexistent --emu /nonexistent` both end `hil: FAIL`, exit 1. `make test` → `OK`, `make lint` clean, and `make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101` → all seven `ok:` and `hil: PASS` on 2026-10-06 after the change.
- Round 2: `verify.md` §recovery said "within two seconds"; it now says the harness skips one summary and then judges two (about 3 s), matching `Harness.window( )`. Spec §Goal states the recovery check as a 2-summary window with no time, so it needed no change; `verify.md:122` ("wait two seconds") is a manual-procedure wait, a different fact, left as is.
- Round 2: repaired `notes.md`, where the validation round's "reviewer taste" block had been spliced into the middle of the Outcome's Acceptance line; it now sits at the end of `## Validation — 2026-10-06`. No content was changed.
- Round 3, finding §Goal `tools/hil_digital.py` (Goal: "Every summary line has `att=high` and `us` ≤ 1100000"): the operator chose to fix it in code, not to narrow the spec. `Harness.send( )` used to throw away the master lines it skipped while waiting for `ok:`. Now `Port.take_complete_lines( )` returns those lines, and `send( )` runs every `hil:` line among them through `checked_summary( )`, the same checks `summary( )` applies. Every summary line the harness reads is now checked. Lines left unread in the kernel buffer are read and checked by the next `summary( )`. The spec is not changed. Statements checked: the Goal's rule line and its window definition (a window still counts only summaries after the one it discards, and checking a line does not put it in a window); Plan step 5 (no change). None needed reconciling.
- Round 3, finding §Plan step 8 `verify.md`: spec Context pointers amended. The `link.h`/`link.cpp` bullet now says `step( )` keeps `last_fault` when the link recovers (`src/core/link.cpp:41-43`). The `ps2_protocol.h` bullet now names `kAckTimeoutUs` (100) as the longest wait for an `ACK` (`src/core/ps2_protocol.h:45-49`). `verify.md` is not changed. Statements checked for reconciliation: Goal "`fault` is `Link::last_fault`" (consistent); Out of scope "`kAckTimeoutUs` … the bench measures them" (consistent); "the real ACK slack is about 95 µs" (24-pio-bus pointer, a different fact). None made redundant. Not covered: `verify.md:123`'s "37 µs of shifting" rests on neither pointer.

## Debt

- None.

## For later phases

- **07-analog-mode**:
  - The poller's loop (`poll_once( )` in `src/app/main.cpp`) is where the config-mode sequence goes.
  - `decode_poll( )` already accepts up to a 9-byte analog poll. Only `kDigitalPoll` and its length are digital-specific.
  - `PollTally::last_payload` compares all six payload bytes, so a whammy sweep counts as payload changes. Any check meant to detect desyncs in analog mode must allow for that.
- **09-guitar-observe**: the master already prints a `T1` line for every poll that changes the link. Against the guitar, `python3 tools/trace_decode.py <master port>` shows every dropout with its per-byte ACK timing, and the firmware needs no change for that.
- **Bench hazard (any phase that rewires the two-Pico bench)**: after a replug, a wiring mistake looks exactly like a firmware fault. Every poll aborts at byte 0 with `ack-timeout`, while the emulator still answers commands. On 2026-10-06 the first `make hil` failed this way (`FAIL: sustained: state=absent payload=--`), and rewiring fixed it. Isolate it by flashing `sg2hid_loopback.uf2` and checking that seq 0 reads `5/5`. `verify.md` §If something looks wrong gives this procedure.
- **macOS accessory approval**: on this Mac, a Pico rebooted into BOOTSEL by `picotool reboot -u -f` enumerated as `RP2 Boot`, but was not reachable (no `RPI-RP2` volume, picotool saw nothing) until it was physically replugged after the "Allow accessory" approval. Once approved, `picotool load -x -f --ser <serial>` worked without a button press. In BOOTSEL the board reports a different serial (`E0C9125B0D9B`) from the one in application mode (`5303284730F33A1C`).
- **Validation round 2 (2026-10-06), reviewer taste (not blocking):** `print_summary( Poller& )` both prints and advances `batch_start`; `kLongestPoll` in `poll.cpp` repeats the length formula locally; `tests/test_poll.cpp` gets `std::size_t`/`std::uint32_t` only through `poll.h`.

## Bench readings

2026-10-06. Master on `/dev/cu.usbmodem101` (`sg2hid.uf2`, USB serial `5303284730F33A1C`). Emulator on `/dev/cu.usbmodem2101` (`sg2hid_emu.uf2`). Wired per `docs/wiring-emulator.md`, no guitar.

```
$ make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101
ok: setup
ok: sustained
ok: fault ack 0
ok: fault ack 3
ok: fault late 200
ok: fault id 79
ok: fault late 50
hil: PASS
```

After the run, a capture through `tools/trace_decode.py` of `fault ack 3`, then `fault none`:

```
hil: polls=105000 refused=12001 changes=0 us=999997 state=digital fault=unknown-id att=high payload=7F FE
link: digital -> absent fault=ack-timeout
frame: 3/5 bytes, aborted at byte 3: no ACK after 133 us
  byte 0: out 01 in FF 48 us ack 11 us
  byte 1: out 42 in 41 47 us ack 10 us
  byte 2: out 00 in 5A 47 us ack 10 us
  byte 3: out 00 in -- 133 us
  byte 4: out 00 in -- not sent
hil: polls=106000 refused=13000 changes=0 us=1000049 state=absent fault=ack-timeout att=high payload=7F FE
hil: polls=107000 refused=14000 changes=0 us=999998 state=absent fault=ack-timeout att=high payload=7F FE
link: absent -> digital fault=ack-timeout
frame: 5/5 bytes, complete
  byte 0: out 01 in FF 47 us ack 10 us
  byte 1: out 42 in 41 46 us ack 9 us
  byte 2: out 00 in 5A 46 us ack 9 us
  byte 3: out 00 in 7F 46 us ack 9 us
  byte 4: out 00 in FE 36 us
hil: polls=108000 refused=14212 changes=0 us=999953 state=digital fault=ack-timeout att=high payload=7F FE
```

- `us` per 1000 polls: 999 953 to 1 000 049, well inside the 1 100 000 limit.
- The run's total `refused` was 12 001. Each fault window ran for whole seconds, so those refusals come in thousands; the odd 1 fell outside every judged window. It may be the first poll after boot or a refusal inside a discarded summary. It was not traced, and no judged window saw it.

## Validation — 2026-10-06
- criteria: 18 passed / 0 failed (bench: `make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101` → `hil: PASS`, re-run this round)
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/app/`, `src/core/`)
- index: stale after commit 20f71b8; rebuilt.
- independent review: contradicts (code-side): spec §Goal lines 51–53, "Its last line is `hil: PASS` (exit 0) or `hil: FAIL` (exit 1)". `main( )` in `tools/hil_digital.py` catches only `(Failure, OSError)`. `termios.error`, raised by `tty.setraw( )` in `Port.__init__( )` and by `termios.tcdrain( )` in `write_line( )`, is not a subclass of `OSError` (its MRO is `termios.error`, `Exception`). Evidence for code-side: reproduced this round. `tools/hil_digital.py --master /dev/null --emu /dev/null` ends in a traceback, with no `FAIL:` line and no `hil: FAIL` line. The Goal's sentence has no scope limit, and no Deviations entry narrows it. The same path is reached when a Pico drops off USB mid-run. (settled: 1 — `make hil MASTER=<master port> EMU=<emulator port> | tail -n 1` for `drop_complete_lines( )` reading at most 4096 bytes in one pass) (unstated: 7 — the `?` fallbacks in `state_name`/`fault_name`; the whole-array payload compare in `count_poll( )`; the zero filler in `test_poll.cpp`; the harness's extra failure reasons and its `recovery:` prefix; `set_source_files_properties` applied twice to the shared sources; the bench port names and the troubleshooting section in `verify.md`; `att` read after the `link:` printing)
- closure test: pass (all four notes sections present; every file in the set outside the four workflow paths is named under the spec's "Written by this phase"; the scenarios are enumerated in the Goal's table)
- findings: 1
- finding keys: §Goal tools/hil_digital.py
- spec size: 17364 (first)
- upstream: none
- not-ours: none
- verdict: returned to implementation. In `tools/hil_digital.py` `main( )`, catch `termios.error` alongside `Failure` and `OSError`, so every exit prints `FAIL: <scenario>: <reason>` and then `hil: FAIL`. Enumerate the property ("every exception the harness can raise ends in `hil: FAIL`"): check each call into `os`, `termios`, `tty` and `select` in `Port`. Also fix `verify.md`'s "within two seconds" (about 3 s).
- **Validation 2026-10-06, reviewer taste (not blocking):**
  - `verify.md` says recovery happens "within two seconds". The harness discards one summary and then judges two, so it takes about 3 s.
  - `verify.md` says a wrong sampling edge "would be a bit off". That is reasoning, not an observation.
  - `PAYLOAD_LINE` and `EXPECTED_PAYLOAD` in `tools/hil_digital.py` are two separate constants; neither is derived from the other.
  - `kWireLen = 10` in `tests/test_poll.cpp` is written as a literal, not derived from the analog frame length.

## Validation — 2026-10-06 (round 2)
- criteria: 17 passed / 0 failed (bench re-run this round: `make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101` → seven `ok:`, `hil: PASS`, exit 0)
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/app/`, `src/core/`)
- index: stale (`tools/hil_digital.py`); rebuilt, now fresh.
- independent review: undecidable: (1) §Goal "Every summary line has `att=high` and `us` ≤ 1100000" — `Harness.send( )` discards master lines (`drop_complete_lines( )`) while it waits for `ok:`, so those summaries are never checked; the spec does not say whether "every summary line" means every line the master prints or only the summaries the harness judges. No settling criterion. (2) §Plan step 8 `verify.md` — it states `kAckTimeoutUs` = 100 µs, an aborted byte's `us` ≈ 135, and that `last_fault` survives recovery; the spec states neither the timeout value nor `step( )`'s `last_fault` retention, so the reviewer cannot tell whether the operator's expected readings are true. No settling criterion. (Checked by the validator: both claims are true in the tree — `src/core/ps2_protocol.h:49`, `src/core/link.cpp:41-43` — so this is a missing pointer, not wrong code.) (settled: none) (unstated: 7 — `test_poll`'s final line and `wire_of( )` zero fill; first `elapsed_us` measured from boot and link lines printed before the summary; `O_NONBLOCK | O_NOCTTY` and non-`ok:` emulator lines skipped; the extra failure reasons `unreadable summary`/`emulator refused`; `set_source_files_properties` applied per target; `make hil` exits 2 (Deviations); the `verify.md` troubleshooting section and example line)
- closure test: fail: two `undecidable` findings with no settling criterion = missing pointers
- findings: 2
- finding keys: §Goal tools/hil_digital.py; §Plan docs/phases/06-hil-digital/verify.md
- spec size: 17364 (+0)
- upstream: none
- not-ours: none
- verdict: returned to spec. Note: `§Goal tools/hil_digital.py` repeats round 1's key (a different finding on the same pair). This is round 2, so it does not escape. If it comes back in round 3, the phase goes back to `/expand-phase`. Fixes, deleting text first: (1) narrow Goal line 58 to the summaries the harness judges, or make it a code-side change so `send( )` checks the lines it drops; (2) add `kAckTimeoutUs` (`src/core/ps2_protocol.h`) and `step( )`'s `last_fault` retention (`src/core/link.cpp`) to Context pointers, or delete those claims from `verify.md`. Each amendment needs a Deviations entry that lists the statements it reconciled.

## Validation — 2026-10-06 (round 3)
- criteria: 17 passed / 0 failed (bench re-run with the round-3 harness: `make hil MASTER=/dev/cu.usbmodem101 EMU=/dev/cu.usbmodem2101` → seven `ok:`, `hil: PASS`, exit 0; `--master /dev/null --emu /dev/null` → `hil: FAIL`)
- project gates: test pass, lint pass, typecheck pass
- boundary sweep: clean (13 deny rules; files under `src/app/`, `src/core/`)
- index: stale (`tools/hil_digital.py`); rebuilt, now fresh.
- independent review: undecidable: (a) Plan step 2 "Its wire bytes are built only from the vectors named in Context pointers" — `wire_of( )` in `tests/test_poll.cpp` pads the 10-byte wire with zeros, and `completed = 9` hands `decode( )` four of them; the spec does not say whether padding past the answer counts as wire bytes. No settling criterion. (b) Plan step 8 "how to read a `link:` line with `tools/trace_decode.py`" — the spec does not state how `trace_decode.py` is invoked or what it does with lines that are not `T1`, so `verify.md` §3's `python3 tools/trace_decode.py $MASTER` cannot be checked. No settling criterion. (Checked by the validator: it takes paths and prints other lines unchanged, `tools/trace_decode.py:106,116`, so this is a missing pointer, not a wrong procedure.) (settled: 2 — `make hil MASTER=<master port> EMU=<emulator port> | tail -n 1` for a pre-`ok:` summary arriving after `take_complete_lines( )` and becoming the discarded one, which shifts Δ's baseline; the bench catches this only by chance, since the race lasts a few ms per command — `make firmware && test -f build/pico/sg2hid.uf2 && …` for `pico_generate_pio_header( )` being called once per master target) (unstated: 10 — `us` includes the previous summary's printing; `att` sampled after the `link:`/`T1` printing; busy-wait schedule with no catch-up; non-`hil:` lines skipped and `master silent` reset only by `hil:`; port-open failure reported as `FAIL: setup`; `KeyboardInterrupt` uncaught, `fault none` still sent; `test_poll`'s final line; "over 60 000 polls" in R-PROTO-07; the `?` fallbacks; the Makefile header text)
- closure test: fail: two `undecidable` findings with no settling criterion = missing pointers
- findings: 2
- finding keys: §Plan tests/test_poll.cpp; §Plan docs/phases/06-hil-digital/verify.md
- spec size: 17487 (+123)
- upstream: none
- not-ours: none
- verdict: escaped to /expand-phase: §Plan docs/phases/06-hil-digital/verify.md. The key recurs from round 2. The finding behind it differs (round 2: `kAckTimeoutUs`/`last_fault`; round 3: how `trace_decode.py` is invoked), which is the false recurrence named in the command's belay-debt note. The rule is applied as written: status set to `pending`. For the re-expansion: Plan step 8 lists the topics `verify.md` covers, but the spec does not ground the facts and procedures inside each topic (timings, tool invocation). Either point to their sources or narrow step 8. Plan step 2's "only from the vectors" needs a ruling on the padding. Also settle the window baseline: "after the emulator's `ok:`" as the host reads lines, or as the master prints them.

