# Phase 09-guitar-observe — first contact with the real SG, read-only

## Goal

The real SG meets the bus for the first time through a new instrument firmware,
`build/pico/sg2hid_observe.uf2`. The operator captures what the guitar answers, and the eleven questions of the G table below
are settled against those captures: each is confirmed, or a hand-transcribed vector plus a
code fix corrects it.

**The instrument (operator's decision, 2026-10-09).** `sg2hid_observe` is a third master
firmware, built from `src/app/observe.cpp`. It has no HID interface; USB stdio is its only USB
function. It configures pins only through `ps2::bus_init( )` (R-SAFETY-09, R-SAFETY-10). It
exchanges one frame every 1000 µs, start to start, in one of two modes:

| Mode | Entered | Frame sent | Link judged by |
|---|---|---|---|
| digital | at power-up | `01 42 00 00 00`, the read poll, built from `ps2_protocol.h` constants | `ps2::step( )` on `ps2::decode_poll( )` |
| analog | the line `analog` on its serial port, in either mode | `ps2::command_for( master.stage )`, from a fresh `ps2::Master` | `ps2::advance( )`, the analog-mode sequence `sg2hid` runs (ADR-0017) |

Digital mode sends only the read command, so the guitar stays in its power-up mode. That is
what "read-only" means in this phase.

- **Serial input.** The firmware accepts exactly one input line: `analog`, optionally followed
  by `\r`. It answers `ok: analog` and starts the sequence again from `EnterConfig`. It answers
  every other non-empty line with `error: unknown command`.
- **Serial output.** It prints a `link: <from> -> <to> fault=<fault>` line when the link state
  changes, using `sg2hid`'s names. It prints a frame's `T1` line (ADR-0015) when the
  **trace filter** says the frame is due.

**The trace filter** is pure, in `src/core/trace_filter.{h,cpp}`. It compares a frame with the
last *printed* frame. A frame is due when any of these holds:

1. No frame has been printed yet.
2. Its `out` bytes or their count differ from the last printed frame's.
3. Its link state differs from the last printed frame's.
4. Its `in` bytes or its completed count differ from the last printed frame's, **and** at least
   `kTraceGapUs` (10 000 µs) have passed since the last print.

Rule 4's gap holds back analog noise. A frame that persists is still printed once the gap has
passed, so no steady state goes unprinted. Rules 2 and 3 print every negotiation frame.

**The questions this phase settles: the G table.** It is the closed set. `notes.md` §Outcome
reproduces it with one disposition per row: `confirmed`, `corrected`, `re-owned` or
`unmeasurable`.

| id | Question | What `core` assumes today | Settled by |
|---|---|---|---|
| G1 | Does the SG answer the bus on 3V3 alone? | `requirements.md` OPEN line | `sg-digital.log` shows `link: absent -> digital` |
| G2 | Digital id byte | `kIdDigital` 0x41 | `sg_digital_idle.h` |
| G3 | Digital byte and bit of each of the ten `Button` controls | `kButtonsLowIndex`, `kButtonsHighIndex`, the ten `kMask*` in `guitar_state.h` | `sg_digital_controls.h` |
| G4 | The guitar's answer to each of the three negotiation frames, and the config id | `kIdConfig` 0xF3 | `sg_negotiation_answers.h` |
| G5 | Does the sequence, as sent, reach analog streaming, and with which id? | `kIdAnalog` 0x73; no `5A` padding on leave-config | `sg_negotiation_answers.h` + `sg_analog_idle.h` |
| G6 | Analog-mode byte and bit of each control: the same as G3? | the same two bytes and masks as digital | `sg_analog_controls.h` |
| G7 | Whammy payload index, its byte at rest, its byte at full deflection | `kWhammyIndex` 5; `kWhammyRest` 0x80 | `sg_analog_idle.h`, `sg_analog_whammy_full.h` |
| G8 | ACK delay: minimum and maximum decoded `ack` over the guitar captures | `kAckTimeoutUs` 100 (budget); emulator `kAckDelayUs` 10 (budget) | `trace_decode.py` over `sg-*.log` |
| G9 | ACK pulse width | emulator `kAckPulseUs` 2 (budget) | not observable: the master waits only for ACK to fall |
| G10 | Negotiation length, in frames | `kNegotiationTimeoutUs` 100 000 (budget) | `T1` lines from `ok: analog` to `-> analog` in `sg-analog.log` |
| G11 | Shift calibration | `SHIFT_CALIBRATION_US` 0 | `04-trace-mode` validation: loopback `ack` 0–1 µs on firmware 51f1291. Valid while `ps2_master.pio` and `pio_port.cpp` are unchanged since 51f1291 |

**Stop conditions.** Each one is an operator decision, recorded in `notes.md`. The phase stops
and asks; it does not pick.

- **G1 fails.** No frame of `sg-digital.log` completes. The question becomes the power supply.
- **G7's rest byte is not 0x80** (`kWhammyRest`). The neutral HID report (R-HID-02, ADR-0018)
  would then differ from the bar at rest.
- **G8 finds an `ack` above 50 µs**, less than 2× margin to `kAckTimeoutUs`.
- **G5 still fails after one corrected sequence has been re-captured.**

## Context pointers

- `CLAUDE.md` §Hardware safety: R-SAFETY-01/04/05/08. The first-contact order this phase is a
  step of.
- `docs/constraints.md`: R-SAFETY-08, R-PROTO-05 (amended here), R-PROTO-06 (its Scope names
  this phase), R-HID-02, R-CLEAN-02 (three parameters at most), R-CLEAN-05, and §Observed
  conventions, where the finding goes.
- `docs/adr/0004-second-pico-as-guitar-emulator.md`: the guitar is the only authority over
  both `core` and the emulator. `docs/adr/0015-bus-trace-text-lines-cpu-timing.md`: the `T1`
  format. `docs/adr/0017-analog-negotiation.md`: the sequence analog mode runs.
- `docs/product/requirements.md` §Open questions: the OPEN line G1 answers, and the ANSWERED
  form to copy.
- `src/app/main.cpp`: the poll loop, `state_name( )`, `fault_name( )` and `print_trace( )`,
  which move to `src/app/console.{h,cpp}`.
- `src/emu/main.cpp`: `poll_command( )`, the non-blocking `getchar_timeout_us( 0 )` line reader
  to copy.
- `CMakeLists.txt`: `add_master_firmware( )`, `SG2HID_MASTER_SOURCES`. `Makefile`: its header
  lists `sg2hid`, `sg2hid_loopback` and `sg2hid_emu`, and gains `sg2hid_observe`.
- `src/core/ps2_protocol.h`, `src/core/guitar_state.h`, `src/core/link.h`, `src/emu/sg_model.h`:
  the constants of the G table, the markers that name this phase, and the
  `ACK_PULSE_CYCLES` / `kDeviceAckPulseUs` static asserts to keep in step.
- `src/core/negotiation.h`, `src/core/poll.h`, `src/core/bus_trace.h`, `src/core/hid_report.h`
  (`Button`): the APIs the observe firmware and the cases call.
- `tests/ps2_codec_cases.cpp`: the `report( )` line format (`  ok:   <label>`), `state_with( )`,
  and `reach_polling( )`. `tests/test_ps2_codec.py`: their driver. `tests/test_poll.cpp`: the
  shape of a standalone `tests/test_*.cpp`, which `make test` builds by wildcard.
- `tests/vectors/README.md` and `tests/vectors/digital_pressed.h`: the vector house rules,
  one comment per byte.
- `tools/trace_decode.py`: usage, the `ack` row, and `SHIFT_CALIBRATION_US`.
- `docs/wiring.md`: the guitar socket wiring. `docs/wiring-emulator.md`: the two-Pico wiring.
  `docs/phases/02-wiring/verify.md`: the continuity and voltage checklist re-run before
  first contact.
- `docs/phases/04-trace-mode/notes.md` §For later phases and §Validation 2026-10-02: the capture
  recipe and G11's evidence.
- `docs/phases/05-emulator/notes.md` §Debt and `docs/phases/07-analog-mode/notes.md` §For later
  phases: the emulator budgets, and the `5A` padding question.
- `docs/phases/08-usb-hid/verify.md`: the port-finding steps and the `make hil` bench to
  re-run.

## Plan

1. **Shared console printing.** Touches `src/app/console.{h,cpp}` (new), `src/app/main.cpp` and
   `CMakeLists.txt`.
   - `state_name( )`, `fault_name( )` and `print_trace( )` move unchanged into `console`.
   - A `print_link( from, to, fault )` beside them prints the `link:` line. `main.cpp` calls it.
   - `console.cpp` joins `SG2HID_MASTER_SOURCES`.
   - `sg2hid`'s serial output does not change.
   - Check: `make firmware && make lint` → exit 0.
2. **Trace filter.** Touches `src/core/trace_filter.{h,cpp}` (new) and
   `tests/test_trace_filter.cpp` (new).
   - The Goal's four rules, with `kTraceGapUs = 10000`.
   - Each function `trace_filter.h` declares takes at most three parameters (R-CLEAN-02), and each one either returns a
     value or changes state, never both (R-CLEAN-07).
   - The test prints one `FAIL: <case>` line per failed case and exits non-zero on any. Its
     cases:
     - (a) the first frame is due;
     - (b) an identical frame is not due, even after the gap;
     - (c) an `in` change is not due before the gap, and is due once the gap has passed;
     - (d) an `out` change is due before the gap;
     - (e) a link-state change is due before the gap;
     - (f) a completed-count change behaves as (c).
   - Check: `make test` → `OK`, with `build/host/test_trace_filter` run and passing. Removing
     rule 4's gap condition makes it print `FAIL:`. Check that by hand and record it in notes.
3. **Observe firmware.** Touches `src/app/observe.cpp` (new), `CMakeLists.txt`
   (`add_master_firmware( sg2hid_observe src/app/observe.cpp )`) and the `Makefile` header.
   - It behaves as the Goal's table and its two bullets describe.
   - Check: `make firmware && ls build/pico/sg2hid_observe.uf2 && make lint` → exit 0.
4. **Instrument proven on the emulator** (R-SAFETY-08: the emulator before the guitar).
   Touches `docs/phases/09-guitar-observe/captures/emu.log` (new, raw and unedited).
   - Wire the bench per `docs/wiring-emulator.md`. Flash `sg2hid_observe` on the master. The
     emulator keeps `sg2hid_emu`.
   - Capture with `cat $MASTER > emu.log`. Drive the emulator through this script:
     1. `mode digital`;
     2. one `payload` change;
     3. `analog` sent to the master;
     4. `mode analog`, then the whammy sweep `00 40 80 C0 FF` sent as `payload` lines.
   - Check, on the log: `grep -c -- '-> digital'` ≥ 1, `grep -c -- '-> analog'` ≥ 1,
     `grep -c '^ok: analog'` = 1. `tools/trace_decode.py` exits 0. For each of the five
     whammy payloads, at least one `T1` line carries its byte.
5. **`sg2hid` regression.** Step 1 moved its printing, so re-run the bench of
   `docs/phases/08-usb-hid/verify.md` §2 step 5.
   - Check: `make hil MASTER=$MASTER EMU=$EMU | tail -n 1` → `hil: PASS`.
6. **Operator procedure.** Touches `docs/phases/09-guitar-observe/verify.md` (new; R-PROC-02
   headings). For a non-specialist, in this order:
   1. the R-SAFETY-08 order;
   2. re-running the `02-wiring` continuity and voltage checklist on the socket wiring, with
      socket pin 3 insulated and nothing on VBUS or VSYS;
   3. moving the master's bus wires from the emulator to the socket;
   4. plugging the guitar in **with the Pico unpowered**, then plugging in USB;
   5. the two capture scripts below;
   6. reading the decoder's rows and `ack` values;
   7. what each stop condition looks like on screen.

   Never flash while the guitar is connected (R-SAFETY-08).

   The **digital script** (`sg-digital.log`):
   1. 2 s untouched;
   2. each `Button` in `hid_report.h` order, held alone for about 1 s and released for about
      1 s — the five frets, strum up, strum down, start, select, then tilt (neck raised).

   The **analog script** (`sg-analog.log`), on the same power-up, so the guitar is not
   reflashed or replugged:
   1. send `analog`;
   2. 2 s untouched;
   3. the ten controls as above;
   4. the whammy pushed slowly to its stop, held 1 s, then released.

   Check: `grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' verify.md` → 2.
7. **First contact.** The operator runs `verify.md` and saves `sg-digital.log` and
   `sg-analog.log` raw under `captures/`.
   - Check: both files decode with exit 0. `sg-digital.log` contains `link: absent -> digital`.
     `sg-analog.log` contains `-> analog`.
   - A G1 stop condition halts the phase here.
8. **Vectors transcribed from the captures.** Touches `tests/vectors/sg_*.h` (six new files),
   `tests/vectors/README.md` and `docs/constraints.md` (R-PROTO-05).
   - The six files below hold complete frames as the bus delivered them. One comment per byte names
     the capture file and its `T1` line. The six files:
     - `sg_digital_idle.h`;
     - `sg_digital_controls.h`: ten frames in `Button` order;
     - `sg_negotiation_answers.h`: the `in` bytes of the three negotiation frames, wire byte 0
       included;
     - `sg_analog_idle.h`, with the constant `kSgWhammyAtRest`;
     - `sg_analog_controls.h`: ten frames;
     - `sg_analog_whammy_full.h`, with the constant `kSgWhammyFull`.
   - R-PROTO-05's text gains the second legitimate source: transcribed by hand from a
     real-guitar capture under `docs/phases/09-guitar-observe/captures/`, the authority
     (ADR-0004). Its binding is unchanged. The README gains one paragraph saying the same.
   - Check: `make test` still builds; no case reads the new vectors yet.
9. **Codec cases.** Touches `tests/ps2_codec_cases.cpp`. Six cases, each with a label that
   starts `sg_`:
   - `sg_digital_idle`: decodes as Digital, and nothing is pressed.
   - `sg_digital_controls`: each frame maps to exactly its control (`state_with( )`, whammy
     aside).
   - `sg_negotiation`: a fresh `Master` fed the three answers reaches `Polling`, then
     `sg_analog_idle` leaves the link `AnalogStreaming`.
   - `sg_analog_idle`: decodes as Analog, nothing is pressed, and the whammy is
     `kSgWhammyAtRest`.
   - `sg_analog_controls`: each frame maps to exactly its control.
   - `sg_analog_whammy_full`: the whammy is `kSgWhammyFull`, and none of the frame's other
     three axis bytes equals `kSgWhammyFull`.

   Check: `python3 tests/test_ps2_codec.py | grep -c '^  ok: *sg_'` → 6. A case that fails here
   is a `corrected` row and goes to step 10.
10. **Reconcile.** Touches only the `core` files the G rows name (`ps2_protocol.h`,
    `guitar_state.{h,cpp}`, `negotiation.cpp`, `link.h`), `src/emu/sg_model.h`, and the
    existing cases `make test` reports failing after a correction, with their vectors. Per row:
    - **G2–G7, `corrected`:** change the constant or sequence until the step-9 cases pass. If
      G2, G4 or G5 shows the SG answering `0x79`, repoint `unknown_id.h` to another undeclared
      byte. A G4/G5 correction changes the bytes sent, so recapture `sg-analog.log` with the
      corrected firmware.
    - **G8:** `kAckTimeoutUs` stays. Its `belay-debt:` line becomes a pointer to the finding.
      `kAckDelayUs` becomes the median decoded `ack`, rounded to whole µs. Then re-run step 5.
    - **G9:** `kAckPulseUs`'s marker says that trace mode cannot observe it, and names no
      phase. Recorded under notes §Debt.
    - **G10:** `kNegotiationTimeoutUs`'s marker cites the measured length and names
      `10-guitar-full` as the owner of tightening it.
    - **G11:** `SHIFT_CALIBRATION_US` stays 0. It changes only if a guitar `ack` reads
      negative, that is a byte shorter than `SHIFT_US`.
    - **Markers:** for each line `grep -rn '09-guitar-observe' src/` lists, replace its
      `TODO(09-guitar-observe)` or `belay-debt` text with a pointer to the finding.
    - Check: `make test` → `OK`, and `grep -rn '09-guitar-observe' src/` → no output.
11. **Record.** Touches `docs/constraints.md` §Observed conventions, `docs/product/requirements.md`,
    the R-PROTO-06 Scope clause, and `docs/phases/09-guitar-observe/notes.md`.
    - Write one finding, "The SG on the bus", dated, citing `captures/`, with one sub-bullet per
      G row.
    - The OPEN line becomes `- ANSWERED (09-guitar-observe, <date>): …`.
    - R-PROTO-06 gains an **Observed** sentence with the G8 range.
    - notes §Outcome holds the G table with dispositions.
    - Check: the acceptance criteria below.

## Acceptance criteria

`C=docs/phases/09-guitar-observe/captures`. Lines marked *bench* need the operator's hardware or
the captures it produced.

```
make test                                                        # expect: exit 0, last line OK
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
make firmware && ls build/pico/sg2hid_observe.uf2                # expect: exit 0, the file listed
build/host/test_trace_filter                                     # expect: exit 0, no FAIL: line
python3 tests/test_ps2_codec.py | grep -c 'FAIL:'                # expect: 0
python3 tests/test_ps2_codec.py | grep -c '^  ok: *sg_'          # expect: 6
ls tests/vectors/sg_*.h | wc -l                                  # expect: 6
grep -rn '09-guitar-observe' src/                                # expect: no output (exit 1)
grep -c '^- OPEN' docs/product/requirements.md                   # expect: 0
grep -c '^- ANSWERED (09-guitar-observe' docs/product/requirements.md   # expect: 1
grep -c 'docs/phases/09-guitar-observe/captures' docs/constraints.md    # expect: >= 2 (the finding, R-PROTO-05)
grep -c '^| G[0-9]' docs/phases/09-guitar-observe/notes.md       # expect: 11
grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' docs/phases/09-guitar-observe/verify.md   # expect: 2
git diff --quiet 51f1291 HEAD -- src/hal/ps2_master.pio src/hal/pio_port.cpp   # expect: exit 0 (G11's evidence holds)
ls $C                                                            # bench, expect: emu.log sg-analog.log sg-digital.log
for f in $C/*.log; do python3 tools/trace_decode.py "$f" >/dev/null || echo "BAD $f"; done   # bench, expect: no output
grep -c '^ok: analog' $C/emu.log                                 # bench, expect: 1
grep -c 'link: absent -> digital' $C/sg-digital.log              # bench, expect: >= 1 (G1)
grep -c -- '-> analog' $C/sg-analog.log                          # bench, expect: >= 1 (G5)
python3 tools/trace_decode.py $C/sg-*.log | awk '/ ack /{ for(i=1;i<=NF;i++) if($i=="ack" && $(i+1)>50) n++ } END{print n+0}'   # bench, expect: 0 (G8)
make hil MASTER=<master port> EMU=<emulator port> | tail -n 1    # bench, expect: hil: PASS
```

## Out of scope

- Any change to `sg2hid`'s behaviour, its HID output, hotplug, or latency belongs to
  `10-guitar-full`. Step 1 moves printing code without changing output.
- Tightening `kNegotiationTimeoutUs` is re-owned to `10-guitar-full` (G10).
- Measuring the ACK pulse width needs a logic analyzer. No phase owns it (G9).
- A `digital` serial command, or any other observe command: a power cycle returns the guitar
  to digital.
- A `T2` trace format, and any edit to `T1`, `trace_decode.py`'s parsing, `ps2_master.pio` or
  `pio_port.cpp`.
- A new loopback capture: G11 rests on `04-trace-mode`'s, while the bus code is unchanged.
- An ADR for the observe firmware: it constrains no later phase.
- Editing earlier phases' `verify.md` or `notes.md` (for example `01-ps2-codec/verify.md`'s
  marker count), and editing `.claude/rules/tech-debt.md`.
- Mapping controls the SG has beyond the ten `Button`s, and the wireless Les Paul.
