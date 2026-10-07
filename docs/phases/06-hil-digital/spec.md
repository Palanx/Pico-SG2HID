# Phase 06-hil-digital — the master polls the emulator in digital mode

<!-- Written by /expand-phase, immediately before implementation, never earlier
     (P4). Amendable during implementation ONLY together with a Deviations
     entry in notes.md.
     Closure test (P5): a session — or a person — reading CLAUDE.md + this
     directory + the files pointed to below must be able to complete the
     phase. If either would need anything else, add the pointer or re-cut the
     phase. -->

## Goal

The master firmware, `build/pico/sg2hid.uf2`, stops being the loopback program and becomes a
link poller. Every `kPollPeriodUs` (1000 µs, measured start to start) it exchanges one digital
poll frame, the five master bytes `01 42 00 00 00`, through `exchange_frame( )`. It decodes the
answer with a new pure function, `ps2::decode_poll( )`, and advances a `ps2::Link` with
`ps2::step( )`, passing the microseconds since the previous poll started. It counts every poll
in a new pure `ps2::PollTally`. It never stops: a refused frame makes the link `Absent`, and the
next poll is attempted on schedule anyway.

The master prints these lines over USB serial:

- **Summary line.** After every `kPollsPerSummary` (1000) polls, one line in this form:
  `hil: polls=<n> refused=<n> changes=<n> us=<n> state=<state> fault=<fault> att=<high|low> payload=<XX XX|-->`.
  - `polls`, `refused` and `changes` are the tally's counters since boot, in decimal.
  - `us` is the time those 1000 polls took.
  - `state` is the link state, one of `absent`, `negotiating`, `digital` or `analog`.
  - `fault` is `Link::last_fault`, one of `none`, `ack-timeout`, `unknown-id`, `not-ready` or
    `negotiating`.
  - `att` is the `ATT` level read right after the last poll.
  - `payload` holds the first two payload bytes of the last good frame, in uppercase hex, or
    `--` before the first good frame.
- **Link change.** On every poll that changes the link state, one line
  `link: <was> -> <now> fault=<fault>`, followed by that poll's `T1` trace line (ADR-0015), or
  by `trace: line too long`.

Definitions used below:

- A **refused** poll is one whose `decode_poll( )` outcome has no value.
- A **payload change** is a good poll whose payload differs from the previous good poll's. The
  first good poll since boot is not a change.
- A **desync** is a refused poll or a payload change while the emulator answers with no fault.

The previous loopback program moves unchanged in behaviour to its own target,
`build/pico/sg2hid_loopback.uf2`, from `src/app/loopback.cpp`. R-PROTO-01's manual procedure
(the bit-order probe) therefore stays runnable.

A new host harness, `tools/hil_digital.py` (Python standard library only), drives both Picos
through their USB serial ports. It sends command lines to the emulator, using the 05-emulator
grammar, and reads the master's summary lines. It prints one line per scenario,
`ok: <scenario>` or `FAIL: <scenario>: <reason>`. It stops at the first `FAIL`, and it always
sends `fault none` to the emulator before it exits. Once its arguments are accepted, its last
line is `hil: PASS` (exit 0) or `hil: FAIL` (exit 1). `make hil MASTER=<port> EMU=<port>` runs it. That target is not part of
`make test`.

Rules that hold in every scenario:

- Every summary line the harness reads has `att=high` and `us` ≤ 1100000. That includes the
  lines it reads while it waits for an emulator `ok:`, which belong to no window.
- If the harness waits 3 s for a summary line and none arrives, the scenario fails with the reason
  `master silent`.
- A **window** of k summaries is the k summaries that follow **at least two** discarded summaries,
  all read after the emulator's `ok:`. **Δ** is a counter's value on the window's last summary
  minus its value on the last discarded summary.
  - Why two: the master and the emulator are separate serial ports, so the harness can order
    lines only as it reads them, not as they were printed. The first summary read after `ok:`
    may have been printed before the command took effect. The second was printed about
    `kPollsPerSummary` × `kPollPeriodUs` (1 s) after the first, so it was printed after the
    command took effect. Every poll a window counts therefore ran under the new command.

The scenarios, in order:

| scenario | emulator lines sent | passes when |
|---|---|---|
| `setup` | `fault none`, `mode digital`, `payload 7f fe 80 80 80 80` | each line is answered `ok: <line>` within 2 s, and a master `hil:` line arrives |
| `sustained` | none | over a window lasting `--seconds` (default 60) summaries: every summary has `state=digital` and `payload=7F FE`, Δrefused = 0 and Δchanges = 0 |
| `fault ack 0` | `fault ack 0` | over a 2-summary window: `state=absent`, `fault=ack-timeout`, Δrefused = Δpolls |
| `fault ack 3` | `fault ack 3` | as `fault ack 0` |
| `fault late 200` | `fault late 200` | as `fault ack 0` |
| `fault id 79` | `fault id 79` | over a 2-summary window: `state=absent`, `fault=unknown-id`, Δrefused = Δpolls |
| `fault late 50` | `fault late 50` | over a 2-summary window: `state=digital`, Δrefused = 0 |

Each fault scenario ends with a **recovery check**:

- The harness sends `fault none`.
- Over a 2-summary window, every summary has `state=digital` and `payload=7F FE`, Δrefused = 0
  and Δchanges = 0.

The scenario prints `ok:` only if both its own check and the recovery check pass.

## Context pointers

Read:

- `CLAUDE.md` — layering, the error model, hardware-safety rules and conventions.
- `docs/constraints.md`
  - §Invariants: R-SAFETY-07, R-SAFETY-08, R-SAFETY-09, R-SAFETY-10, R-PROTO-01, R-PROTO-02,
    R-PROTO-03, R-PROTO-05, R-PROTO-06, R-PROTO-07, R-EMU-01, R-EMU-02, R-ERR-01..04, R-CLEAN-02,
    R-CLEAN-04 and R-PROC-04.
  - §Error handling ("the firmware never stops").
  - §Testing (HIL suites are separate targets).
- `docs/adr/0011-pure-link-step.md` — `step( )` takes elapsed microseconds, and `app` does the
  unsigned subtraction.
- `docs/adr/0015-bus-trace-text-lines-cpu-timing.md` — the `T1` line printed on a link change.
- `src/core/link.h`, `src/core/link.cpp` — `Link`, `LinkState`, `FaultCause` and `step( )`,
  used unchanged. `step( )` keeps `last_fault` when the link recovers. A good digital frame
  moves the link from `Absent` straight to `DigitalStreaming`, with no `negotiating` in
  between: only a `Config` header leads to `Negotiating` (`state_for( )`). §Bench readings
  records that line, `link: absent -> digital fault=ack-timeout`.
- `src/core/ps2_frame.h` — `decode( )`, `DecodeOutcome`, `Ps2Frame` and `kMaxPayloadLen`.
  `decode( )` takes the response without its first byte, and a short span yields `AckTimeout`.
  `Ps2Frame` has exactly two fields, `id` and `payload`. Two frames are equal when both fields
  are equal.
- `src/core/ps2_protocol.h` — `kFrameStart`, `kCmdPoll`, `kPadByte`, `frame_len( )` and
  `payload_len( )`. The comment at the top explains why wire byte 0's answer is dropped.
  `kAckTimeoutUs` (100) is the longest the master waits for an `ACK`.
- `src/core/bus_trace.h` — `WireByte` and `format_trace_line( )`.
- `src/hal/bus_frame.h`, `src/hal/bus_port.h` — `exchange_frame( )` and `bus_init( )`, used
  unchanged.
- `src/app/main.cpp` — before this phase, the loopback program; it moves to
  `src/app/loopback.cpp`, whose `loopback: seq=… bytes=<done>/<n> …` line is what `verify.md`'s
  wiring check reads.
- `tools/hil_digital.py` — `Harness`'s check methods. In `sustained` and every recovery
  check, a failing summary gives the reason `state=<state> payload=<payload>`, and a failing
  window gives `Δrefused=<n> Δchanges=<n>` (`tools/hil_digital.py:151,153`).
- `tools/trace_decode.py` — the module docstring gives its invocation
  (`trace_decode.py [FILE ...]`; a serial port works as a FILE) and says it echoes every
  non-`T1` line unchanged. The `SHIFT_US` comment gives the 37 µs a byte spends shifting
  before the `ACK` wait starts.
- `src/emu/main.cpp`, `src/emu/sg_model.h` — the emulator reads command lines only while
  `ATT` is high and answers each one with `ok: <line>` or `error: <reason>`.
- `docs/phases/05-emulator/spec.md` §Goal — the command grammar: `mode`, `payload` (six hex
  bytes), and `fault none|ack <n>|late <us>|id <hh>`.
- `docs/phases/05-emulator/verify.md`:
  - §2 steps 1–2: flashing a Pico (hold BOOTSEL, plug in, `cp <uf2> /Volumes/RPI-RP2/`).
  - §2 step 3: finding the two ports with `ls /dev/cu.usbmodem*`.
  - §2 step 5: sending the emulator a command by hand. Run `cat $EMU` in one window and
    `printf 'fault none\n' > $EMU` in another; the emulator answers `ok: fault none`.
  - Its troubleshooting entry "No `ok:` replies in the emulator window": the emulator port is
    the wrong one.
- `docs/phases/05-emulator/notes.md` §For later phases (06 entries) and §Bench readings — the
  ports, and the ACK delays measured per fault.
- `docs/phases/24-pio-bus/notes.md` §For later phases — the real ACK slack is about 95 µs.
- `docs/phases/06-hil-digital/notes.md` — a re-expansion's ground truth: §Outcome (what
  landed), §Deviations (rounds 2 and 3), §For later phases (the bench hazard and its
  loopback check), §Bench readings (the measured `us` values and the 133 µs aborted byte), and
  every §Validation record.
- `docs/phases/04-trace-mode/notes.md` §For later phases — link polling can trace with
  `format_trace_line( )` unchanged.
- `docs/wiring-emulator.md` — the two-Pico bench, and the power order.
- `tests/vectors/README.md`, `tests/vectors/poll_exchange.h`,
  `tests/vectors/digital_idle.h`, `tests/vectors/digital_pressed.h`,
  `tests/vectors/unknown_id.h` — the literals the host test uses, which are `kAddressReply`,
  `kDigitalIdle`, `kDigitalPressed` and `kUnknownId`.
- `tests/ps2_codec_cases.cpp` — the house style for a host-test `main( )`: returns 0 only when
  every case passed.
- `Makefile` — `make test` builds and runs every `tests/test_*.cpp` against `src/core/*.cpp`.
- `CMakeLists.txt` — the two firmware targets and their explicit source lists.

Written by this phase:

- `src/core/poll.h`, `src/core/poll.cpp` (new)
- `tests/test_poll.cpp` (new)
- `src/app/loopback.cpp` (new, a copy of today's `src/app/main.cpp`)
- `src/app/main.cpp` (rewritten as the poller)
- `CMakeLists.txt`
- `Makefile`
- `tools/hil_digital.py` (new)
- `docs/constraints.md` — the texts of R-PROTO-01, R-PROTO-06 and R-PROTO-07 only.
- The flash lines in these files: `docs/phases/24-pio-bus/verify.md:128`,
  `docs/phases/04-trace-mode/verify.md:155`, `docs/phases/05-emulator/verify.md:94`.
- `docs/phases/06-hil-digital/verify.md` (new)

## Plan

**State at the third re-expansion (2026-10-07, after validation round 3 escaped on
`§Plan … verify.md` for the third time).** Steps 1–7 have landed and match this spec (notes
§Outcome). Every code gate and the bench have passed in every round since the first
re-expansion; every open finding is in `verify.md`. This re-expansion changes no behaviour.
It narrows step 8's closed list to the claims a validator can match by text (operator's
decision, 2026-10-07: "cut + narrow"), adds a pointer to the harness's `sustained` failure
reasons, and names the `verify.md` edits that are owed. Only step 8 owes an edit. The bench
does not run again, because nothing it judges changes. Re-run every other step's Check
unchanged.

1. **Pure poll logic in `core`.** Touches `src/core/poll.h` and `src/core/poll.cpp`.
   - `kDigitalPollLen = frame_len( ControllerId::Digital ) + 1`.
   - `kDigitalPoll`, a `std::array<std::uint8_t, kDigitalPollLen>` holding `kFrameStart`,
     `kCmdPoll` and three `kPadByte`.
   - `[[nodiscard]] DecodeOutcome decode_poll( std::span<const WireByte> wire,
     std::size_t completed )`.
     - It hands `decode( )` the `in` bytes of wire bytes `1 .. n-1`, where `n` is the smallest
       of `completed`, `wire.size( )` and `frame_len( ControllerId::Analog ) + 1`.
     - With `n` ≤ 1 it hands `decode( )` an empty span.
   - `struct PollTally` with `polls`, `refused` and `payload_changes` (all `std::uint32_t`,
     zero-initialised), plus the last good frame's payload and whether one exists yet.
   - `void count_poll( PollTally&, const DecodeOutcome& )`.
     - It adds 1 to `polls` on every call, and to `refused` when the outcome has no value.
     - On a good outcome it adds 1 to `payload_changes` when an earlier good payload exists and
       differs, then stores the new payload.
   - No SDK header, no clock and no allocation.

   Check: `make typecheck` → exit 0.

2. **Host test.** Touches `tests/test_poll.cpp`, which is picked up by `make test`'s
   `tests/test_*.cpp` wildcard. Its wire bytes are built only from the vectors named in Context
   pointers. Wire byte 0's `in` is always `kAddressReply`.
   - **Filler.** The test's wire can be longer than the answer, up to the analog poll length.
     Entries past the answer are filler: they are not protocol bytes, and R-PROTO-05 does not
     cover them. Their value is arbitrary (zero is fine). Only the `completed = 9` case reaches
     them, and that case exists to prove the decode ignores them.
   - One `FAIL: <case>` line is printed per failed case. On success the last line is
     `test_poll: ok`. The exit is 0 only if every case passes.

   The cases:
   - `kAddressReply` + `kDigitalIdle`, `completed = 5` → has a value, with id `Digital` and
     payload bytes 0–1 equal to `kDigitalIdle[ 2 ]` and `kDigitalIdle[ 3 ]`.
   - The same wire with `completed` = 0, 1, 3 and 4 → `AckTimeout` each time.
   - The same wire with `completed = 9` → it equals the `completed = 5` result: it has a value,
     and its `id` and `payload` (the only two `Ps2Frame` fields) are equal to that result's.
   - `kAddressReply` + the first four bytes of `kUnknownId`, `completed = 5` → `UnknownId`.
   - Tally, starting from a default `PollTally`:
     - idle, then idle again → `payload_changes` 0;
     - then a refused outcome → `refused` 1;
     - then idle → `payload_changes` still 0, because the previous good payload is kept across
       a refusal;
     - then `kDigitalPressed` → `payload_changes` 1;
     - `polls` ends at 5.

   Check: `make test` → last line `OK`. Second check: with `decode_poll( )` changed to read
   wire byte 0 as well, `make build/host/test_poll && build/host/test_poll` exits non-zero.
   Restore the file afterwards.

3. **Split the firmware targets.** Touches `src/app/loopback.cpp`, `CMakeLists.txt` and
   `Makefile`.
   - `cp src/app/main.cpp src/app/loopback.cpp`. In the copy only the first comment line
     changes, to name `sg2hid_loopback.uf2`. `main.cpp` stays the loopback until step 4.
   - In `CMakeLists.txt`, the `src/app/*.cpp` glob is replaced by explicit files.
     `sg2hid` builds from core + `src/hal/bus_frame.cpp` + `src/hal/pio_port.cpp` +
     `src/app/main.cpp`. A new target, `sg2hid_loopback`, builds from the same list with
     `src/app/loopback.cpp` in place of `main.cpp`.
   - `sg2hid_loopback` gets everything `sg2hid` gets: the `-Wall;-Wextra;-Werror` source
     property, `ps2_master.pio`, `pico_stdlib hardware_pio`, USB stdio on, UART stdio off,
     and extra outputs.
   - The `Makefile` header names the three UF2s.

   Check: `make firmware && ls build/pico/sg2hid.uf2 build/pico/sg2hid_loopback.uf2
   build/pico/sg2hid_emu.uf2` → exit 0. `make lint` → exit 0.

4. **The poller.** Touches `src/app/main.cpp`.
   - The poller behaves as the Goal describes, with named `constexpr`s `kPollPeriodUs = 1000`
     and `kPollsPerSummary = 1000`.
   - The state and fault names are file-local `std::string_view` lookups.
   - `elapsed_us` is the unsigned difference of two `time_us_32( )` readings, taken at
     consecutive poll starts.
   - It configures no pin: `bus_init( )` does.

   Check: `make firmware`, `make lint` and `make typecheck` → exit 0.

5. **The harness.** Touches `tools/hil_digital.py` and `Makefile`.
   - `tools/hil_digital.py` is executable, uses only the standard library, and does what the
     Goal describes.
     - Arguments: `--master PORT --emu PORT [--seconds N]`, with N ≥ 1.
     - It opens each port with `os.open` and puts it in raw mode with `tty.setraw( )`, whose
       default `TCSAFLUSH` discards input already queued.
     - The number of discarded summaries is one module constant, `DISCARDED_SUMMARIES = 2`,
       used by every window (Goal §window). **Landed 2026-10-06** (was one).
     - `payload=7F FE` is the harness's own input, sent to the emulator in `setup`. It is not
       a protocol-fixed byte (R-PROTO-05's 2026-09-17 ruling).
   - `make hil` runs it with `MASTER` and `EMU`. If either is empty, it exits 1 with a usage
     line. `hil` joins `.PHONY` and the header comment.

   Check: `python3 tools/hil_digital.py --help` → exit 0. `make hil` with no variables → exit 1
   and a usage line.

6. **Bench run.** No file is changed in this step. The setup:
   - Both Picos are wired per `docs/wiring-emulator.md`, with no guitar anywhere (R-SAFETY-08).
   - The master is flashed with `sg2hid.uf2` and the emulator with `sg2hid_emu.uf2`. Before
     inserting the signal wires, confirm the emulator answers `ok: fault none`.

   Check: `make hil MASTER=<master port> EMU=<emulator port>` → seven `ok:` lines in the
   Goal's scenario order, then `hil: PASS`, exit 0. Record the output in `notes.md`
   §Bench readings.

7. **Rule texts and earlier docs.** Touches `docs/constraints.md` and the three `verify.md`
   flash lines.
   - R-PROTO-01's `manual:` reason names the loopback firmware as
     `build/pico/sg2hid_loopback.uf2`.
   - R-PROTO-06's Scope clause gains one dated sentence, `**Observed <date> (`06-hil-digital`):**`.
     It says that against the emulator, `fault late 50` keeps the link streaming and
     `fault late 200` drops it with `ack-timeout` (`tools/hil_digital.py`).
   - R-PROTO-07's "(inferred, not yet observed)" becomes an observed statement, dated in the
     same form: the `sustained` scenario reads `7F FE` with zero payload changes.
   - No binding changes.
   - The three flash lines copy `sg2hid_loopback.uf2` instead of `sg2hid.uf2`.

   This step runs only after step 6 passes.

   Check: `python3 tests/test_rule_traceability.py` → exit 0.

8. **Operator document.** Touches `docs/phases/06-hil-digital/verify.md`, which carries
   `## What was built` and `## Check it yourself` (R-PROC-02). It covers:
   - what the poller and the harness do, and what a desync is;
   - which UF2 goes on which Pico, and how to find each port;
   - the power order from `docs/wiring-emulator.md`;
   - the `make hil` command and the expected `ok:` lines;
   - what each fault scenario shows, in plain words;
   - how to read a `link:` line with `tools/trace_decode.py`.

   **Closed list, three kinds of claim.** The list binds only what a validator can match by
   text:
   - a **number**: a decimal value, with or without a unit;
   - a **command**: anything shown for the operator to type, including a variable
     assignment;
   - an **expected output line**: a line shown as what a command, the master, the emulator
     or the harness prints.

   Every number, command and expected output line in `verify.md` appears in a row's text
   below: a command or output line as the same string, with `<…>` and `…` matching any
   text, and a number as the same value. Anything else of these three kinds is deleted.
   Extending the table is a spec amendment with a Deviations entry.

   Everything else is outside the list. That covers framing, causes, remedies, paraphrases of a
   row, design rationale, and the order of procedure steps. The operator judges it on the
   bench (R-PROC-02). A reviewer finding about it is recorded as taste, and is neither a
   `contradicts` nor an `undecidable`.
   The list was narrowed on 2026-10-07: under the earlier rule (every claim stated by a
   row's *text*), three validation rounds in a row each found new instances of the same
   kind.

   | # | claim `verify.md` may state | source |
   |---|---|---|
   | V1 | one poll `01 42 00 00 00` every 1 ms; a summary every 1000 polls, about once a second | Goal |
   | V2 | the meaning of each summary field, `us` being about 1 000 000 | Goal, summary bullet |
   | V3 | the example summary line `hil: polls=108000 refused=14212 changes=0 us=999953 state=digital fault=ack-timeout att=high payload=7F FE`, copied verbatim | notes §Bench readings |
   | V4 | `att=high` means `ATT` was released at the end of the poll | R-SAFETY-07 |
   | V5 | link states `digital` and `absent`; faults `ack-timeout` and `unknown-id` and what each means | Goal; `src/core/link.h` |
   | V6 | "A hang is a worse failure than a wrong report.", quoted verbatim | `docs/constraints.md` §Error handling |
   | V7 | the definition of a desync, and that both counts stay at zero against an emulator with no fault | Goal, definitions |
   | V8 | the window: skips two summaries or more (about 2 s), then judges (about 2 s more for a 2-summary window); the "why two" reasoning, and "1000 polls of 1 ms" | Goal, window bullet; V1 |
   | V9 | the seven scenarios, in order, with each one's emulator lines and pass condition; `sustained` lasting 60 s (60 000 polls) | Goal, scenario table; V1 |
   | V10 | `fault ack 3` stops at byte 3, the first button byte | notes §Bench readings (in the recovery block, byte 3 reads `7F`) |
   | V11 | a frame cut short is refused whole (R-PROTO-02); an unsupported id is refused (R-PROTO-03); id `79` is a DualShock 2 | R-PROTO-02, R-PROTO-03, `tests/vectors/unknown_id.h` |
   | V12 | the master waits at most 100 µs for an `ACK` (`kAckTimeoutUs`), so 200 µs late counts as no answer and 50 µs late does not | `src/core/ps2_protocol.h` |
   | V13 | the recovery check: `fault none`, then the window's conditions | Goal, recovery check |
   | V14 | run time: about 2 s for `setup`, 62 s for `sustained` and 8 s for each fault scenario, so under two minutes | this row (2 + 62 + 5 × 8 = 104 s; §Bench readings measured 1 min 43 s) |
   | V15 | why `7F FE` and not the emulator's default `FF FF`: `01111111` and `11111110` reversed bit for bit turn into each other (`FE 7F`), while `FF FF` reads the same either way | the binary in the same sentence; R-PROTO-05's ruling that `7F FE` is harness input; the default from `docs/phases/05-emulator/spec.md` §Goal |
   | V16 | the loopback firmware `build/pico/sg2hid_loopback.uf2` still exists because its probe is how R-PROTO-01 is checked by hand; phases 24, 04 and 05 now flash it | Goal; R-PROTO-01's `manual:` reason; step 7 |
   | V17 | Part 1 commands and outputs: `make test 2>&1 \| tail -n 1` → `OK`; `make firmware && ls build/pico/*.uf2` → the three UF2s; `make build/host/test_poll && build/host/test_poll` → `test_poll: ok`; `make hil` → `usage: make hil MASTER=<master port> EMU=<emulator port>` and an error | Acceptance criteria; step 2; `Makefile`'s `hil` recipe |
   | V18 | what `test_poll` checks: step 2's cases in plain words | step 2 |
   | V19 | no guitar on the bench | R-SAFETY-08 |
   | V20 | flashing each Pico (BOOTSEL, `cp <uf2> /Volumes/RPI-RP2/`), and which UF2 goes on which Pico | `docs/phases/05-emulator/verify.md` §2 steps 1–2; step 6 |
   | V21 | finding the ports one board at a time with `ls /dev/cu.usbmodem*`; the bench's ports as the assignments `MASTER=/dev/cu.usbmodem101` and `EMU=/dev/cu.usbmodem2101`, set in every Terminal window used | `docs/phases/05-emulator/verify.md` §2 step 3; notes §Bench readings |
   | V22 | the emulator check before wiring: `cat $EMU`, `printf 'fault none\n' > $EMU`, expect `ok: fault none`, close the `cat` with Ctrl-C | `docs/phases/05-emulator/verify.md` §2 step 5 |
   | V23 | power order (both on USB, then signal wires), and teardown (signal wires out first, then USB) | `docs/wiring-emulator.md` |
   | V24 | `make hil MASTER=$MASTER EMU=$EMU` and its eight expected lines | Goal; step 6; notes §Bench readings |
   | V25 | record the output in `notes.md` §Bench readings | step 6 |
   | V26 | reading a link change: `python3 tools/trace_decode.py $MASTER`; `printf 'fault ack 3\n' > $EMU`, then, after a pause of about two seconds, `printf 'fault none\n' > $EMU` (any pause works: the next poll, 1 ms later, already shows the change) | `tools/trace_decode.py` docstring; 05-emulator command grammar |
   | V27 | the two `link:` lines, the two `frame:` lines (`frame: 3/5 bytes, aborted at byte 3: no ACK after 133 us`, `frame: 5/5 bytes, complete`) and the per-byte lines of the two decoded blocks, each copied verbatim | notes §Bench readings |
   | V28 | the aborted byte's 133 µs = 37 µs shifting (`SHIFT_US`) + 100 µs (`kAckTimeoutUs`) | notes §Bench readings; `tools/trace_decode.py`; `src/core/ps2_protocol.h` |
   | V29 | `fault=` is kept when the link recovers, so it names why the link last dropped | `src/core/link.cpp` (`step( )`) |
   | V30 | troubleshooting, these five entries with only these causes: `FAIL: setup: emulator did not answer 'fault none'` → `EMU` is the wrong port; `FAIL: setup: master silent` → `MASTER` is the wrong port, or the master still runs the loopback firmware (it prints `loopback:` lines, not `hil:`); `FAIL: sustained: state=absent payload=--` → a wiring mistake looks exactly like this, isolated by the loopback check (`seq=0` reading `bytes=0/5` vs `bytes=5/5`), and it happened once; `FAIL: sustained: state=<state> payload=<payload>` other than `state=absent payload=--`, or `FAIL: sustained: Δrefused=<n> Δchanges=<n>` → recheck the wiring, run the loopback check, record the output; `FAIL: fault late 50: …` → record the output and do not change the timeout | 05-emulator verify.md troubleshooting; Goal (`master silent`); `src/app/loopback.cpp`; notes §For later phases (bench hazard); Out of scope (`kAckTimeoutUs` is measured, not tuned) |

   Owed at the third re-expansion (the only `verify.md` edits). **Landed 2026-10-07 (round 10).**
   - **Procedure order (§2 steps 2–4).** After each flash, the board is unplugged ("then
     unplug it"). Step 4 plugs in the master alone, runs `ls /dev/cu.usbmodem*`, then plugs in
     the emulator and runs it again. Under the assignment lines it says "Set both in every
     Terminal window you use", because §2 step 5 and §3 read `$MASTER` and `$EMU`.
   - **Troubleshooting `sustained` entry.** It shows the two harness lines of V30, with
     `<…>` placeholders, in place of "`FAIL: sustained: state=digital payload=…` with
     anything other than `7F FE`".
   - **§3, cut.** Step 3's narrative becomes the expected lines of V27, copied from notes
     §Bench readings: at least the two `link:` lines and the two `frame:` lines. It is followed
     by one sentence giving V28's sum and one giving V29. The clause "it is kept on purpose so
     the reason survives recovery" is deleted.
   - **Scenario table, cut.** Its columns are the Goal table's three, copied verbatim (V9).
     Each one-sentence reason (R-PROTO-02, R-PROTO-03, `kAckTimeoutUs`) goes in a list under
     the table.
   - Delete the aside "you met in phases 24 and 05" (validation round 3, taste).
   - Then check every number, command and expected output line left in `verify.md` against
     the table, and delete any that no row's text gives.

   Check: `sh tests/test_phase_docs.sh` → exit 0;
   `grep -c 'skips two summaries' docs/phases/06-hil-digital/verify.md` → ≥ 1;
   `grep -c 'T1 n=\|left running\|on purpose\|you met in phases' docs/phases/06-hil-digital/verify.md` → 0;
   `for p in 'then unplug it' 'Set both in every Terminal window you use' 'Δchanges=<n>'; do grep -qF "$p" docs/phases/06-hil-digital/verify.md || echo "missing: $p"; done` → no output.

## Acceptance criteria

Run in order. The last criterion needs the two-Pico bench.

```
make test 2>&1 | tail -n 1                                       # expect: OK
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
make firmware && test -f build/pico/sg2hid.uf2 && test -f build/pico/sg2hid_loopback.uf2 && test -f build/pico/sg2hid_emu.uf2   # expect: exit 0
make build/host/test_poll && build/host/test_poll                # expect: exit 0
grep -c 'decode_poll\|count_poll' src/app/main.cpp               # expect: >= 2
grep -c 'probe_wire_bits' src/app/loopback.cpp                   # expect: >= 1
python3 tools/hil_digital.py --help >/dev/null                   # expect: exit 0
grep -c '^DISCARDED_SUMMARIES = 2$' tools/hil_digital.py         # expect: 1
grep -c 'skips two summaries' docs/phases/06-hil-digital/verify.md   # expect: >= 1
grep -c 'T1 n=\|left running\|on purpose\|you met in phases' docs/phases/06-hil-digital/verify.md   # expect: 0
for p in 'then unplug it' 'Set both in every Terminal window you use' 'Δchanges=<n>'; do grep -qF "$p" docs/phases/06-hil-digital/verify.md || echo "missing: $p"; done   # expect: no output
make hil >/dev/null 2>&1; test $? -ne 0                          # expect: exit 0 (no ports → refused)
git diff --quiet main -- src/hal src/emu src/core/link.h src/core/link.cpp src/core/ps2_frame.h src/core/ps2_frame.cpp src/core/ps2_protocol.h tests/vectors tools/trace_decode.py   # expect: exit 0
grep -c 'sg2hid_loopback.uf2' docs/constraints.md                # expect: >= 1
grep -c 'Observed .*06-hil-digital' docs/constraints.md          # expect: 2
grep -c 'inferred, not yet observed' docs/constraints.md         # expect: 0
grep -l 'sg2hid_loopback.uf2' docs/phases/24-pio-bus/verify.md docs/phases/04-trace-mode/verify.md docs/phases/05-emulator/verify.md | wc -l   # expect: 3
python3 tests/test_rule_traceability.py                          # expect: exit 0
sh tests/test_phase_docs.sh                                      # expect: exit 0
make hil MASTER=<master port> EMU=<emulator port> | tail -n 1     # expect: hil: PASS (bench; exit 0)
```

## Out of scope

- Analog mode, the config-mode sequence, and a 9-byte poll. Polling an emulator in
  `mode analog` with the 5-byte digital poll is refused as `AckTimeout`, and that is not a
  scenario here. This belongs to 07-analog-mode.
- USB HID reports — 08-usb-hid.
- Any change to `src/hal/`, `src/emu/`, `kAckTimeoutUs` or `ps2_master.pio`. The bench measures
  them; it does not tune them.
- A runtime on/off switch for tracing (04-trace-mode's open note). No phase owns it.
- The `SHIFT_US` tech-debt entry. The operator chose to leave it (2026-10-06).
- Hotplug, meaning pulling a signal wire mid-run. It cannot be automated from a serial port.
  It belongs to 10-guitar-full (R4).
- A new rule id. The phase observes existing rules (R-PROTO-06, R-PROTO-07) and binds none.
- Counter wrap. `PollTally`'s `std::uint32_t` counters wrap after 2^32 polls, about 49.7 days
  at 1 ms per poll. The counts and the summary cadence at the wrap are not specified.
