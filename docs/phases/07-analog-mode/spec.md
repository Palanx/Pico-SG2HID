# Phase 07-analog-mode — the master negotiates analog mode and reads the whammy

<!-- Written by /expand-phase, immediately before implementation, never earlier
     (P4). Amendable during implementation ONLY together with a Deviations
     entry in notes.md.
     Closure test (P5): a session — or a person — reading CLAUDE.md + this
     directory + the files pointed to below must be able to complete the
     phase. If either would need anything else, add the pointer or re-cut the
     phase. -->

## Goal

The master firmware (`build/pico/sg2hid.uf2`) stops sending digital polls. It runs the
config-mode sequence and streams analog frames only. The emulator (`build/pico/sg2hid_emu.uf2`)
answers that sequence on the bus. The harness reads the whammy value the emulator is set to
through the master's summary line.

Operator decisions, 2026-10-08:

- the sequencer is pure `core`;
- a declined analog request drops the link to `Absent` and retries;
- the harness sweeps the whammy through the emulator's `payload` command;
- the enter-config frame is five bytes and is judged on its prefix only;
- this phase also pays three tech-debt entries, listed under **Debt paid** below.

**The sequence.** The master's frames (wire bytes, master side):

| stage | frame | answer the stage expects |
|---|---|---|
| `EnterConfig` | `01 43 00 01 00` (5 bytes) | any known id followed by `5A`; nothing else is read |
| `SetAnalog` | `01 44 00 01 03 00 00 00 00` (9 bytes) | a `Config` frame (`F3`) |
| `LeaveConfig` | `01 43 00 00 00 00 00 00 00` (9 bytes) | a `Config` frame |
| `Polling` | `01 42 00 00 00 00 00 00 00` (9 bytes) | an `Analog` frame (`73`) |

At boot, and after any frame that leaves the link `Absent`, the stage is `EnterConfig`. The
**prefix** of an answer holds when at least three wire bytes completed, wire byte 1 is a
declared controller id, and wire byte 2 is `kReadyByte`. One frame is judged like this:

1. `EnterConfig`: the frame is accepted when all five bytes completed and the prefix holds.
   Then the stage becomes `SetAnalog` and the `Link` is not stepped. Otherwise the link is
   stepped with `decode_poll( )`'s outcome.
2. Any other stage, when the prefix holds and its id is not the one the stage expects (see the
   table): the link becomes `Absent` with `last_fault = FaultCause::Declined` and
   `us_in_state = 0`. This is **declined**.
3. Any other stage, otherwise: the link is stepped with `decode_poll( )`'s outcome. If the link
   is then `Absent`, the stage becomes `EnterConfig`. If it is not, `SetAnalog` becomes
   `LeaveConfig`, `LeaveConfig` becomes `Polling`, and `Polling` stays.

On the happy path the master prints `link: absent -> negotiating` and then
`link: negotiating -> analog`.

**The summary line** (06-hil-digital's) changes in three ways:

- `refused` counts frames after which the link is `Absent`.
- `changes` counts `AnalogStreaming` frames whose payload differs from the previous good analog
  frame's.
- The line gains a trailing field, `whammy=<XX|-->`. It is the `whammy` of `map_frame( )` applied
  to the last good analog frame, in uppercase hex, or `--` before the first one.

`payload=` still shows the first two payload bytes, now of the last good analog frame. `polls`
counts every frame exchanged. `fault=` gains the value `declined`.

**The emulator** boots digital and models config mode:

- In config mode it answers id `F3`, ready byte `5A` and six `00` payload bytes. Every byte but
  the frame's ninth gets an ACK.
- Wire byte 1 is accepted when the master sends `0x42` or `0x43`, and also `0x44` in config
  mode. Any other byte breaks the frame, as 05-emulator defined.
- A command takes effect at the start of the next frame, and only if the master's wire byte 3
  arrived:
  - `43` with byte 3 `01` enters config mode;
  - `43` with byte 3 `00` leaves it;
  - `44` with byte 3 `01` selects analog;
  - `44` with byte 3 `00` selects digital.
  - Any other byte-3 value changes nothing.
- A new command, `fault decline`, makes `44 … 01` select nothing.
- `mode digital|analog` also leaves config mode. A `43`/`44` command still pending from the
  previous frame takes effect at the next frame's start, after it.

**The harness** `tools/hil_digital.py` keeps its name, its rules, windows and recovery checks.
It changes these expectations:

| scenario | emulator lines sent | passes when |
|---|---|---|
| `setup` | `fault none`, `mode digital`, `payload 7f fe 80 80 80 80` | each answered `ok: <line>`, and a master `hil:` line arrives |
| `sustained` | none | over a `--seconds` window: every summary `state=analog payload=7F FE whammy=80`, Δrefused = 0, Δchanges = 0 |
| `sweep` | for each of `00`, `40`, `80`, `C0`, `FF` in that order: `payload 7f fe 80 80 80 <v>` | after each line, over a 1-summary window: `state=analog whammy=<V>` (uppercase), Δrefused = 0 |
| `fault ack 0`, `fault ack 3`, `fault late 200` | the fault line | as in 06-hil-digital: `state=absent fault=ack-timeout`, Δrefused = Δpolls |
| `fault id 79` | `fault id 79` | `state=absent fault=unknown-id`, Δrefused = Δpolls |
| `fault late 50` | `fault late 50` | over a 2-summary window: `state=analog`, Δrefused = 0 |
| `fault decline` | `fault decline`, then `mode digital` | over a 2-summary window: no summary has `state=analog`, every summary has `fault=declined`, Δrefused ≥ 1 |

The `sweep` scenario ends by sending the setup payload line again. Every fault scenario ends
with 06-hil-digital's recovery check, now `state=analog payload=7F FE whammy=80`.

**Debt paid** (entries in `.claude/rules/tech-debt.md`):

- "Two `decode` / `step` contracts are specified and asserted by nothing" — two cases.
- "Every `MUTATIONS`-list mutant runs on every `make test`":
  - `tests/test_ps2_codec.py`, `tests/test_bus_frame.py`, `tests/test_bus_trace.py`,
    `tests/test_pin_table.py` and `tests/test_emulator.py` (**the five drivers**) skip a mutant
    already recorded as killed under an unchanged key;
  - `FULL=1` bypasses the lookup.
- "Test drivers copy the `Makefile`'s `CXXFLAGS` by hand" — the five drivers take the flags
  from `make -s --no-print-directory print-CXXFLAGS`.

## Context pointers

- `CLAUDE.md` — layering, error model, hardware safety, conventions.
- `docs/constraints.md`:
  - §Invariants: R-PROTO-02..06, R-EMU-01, R-EMU-02, R-ERR-01..04, R-CLEAN-02, R-CLEAN-04,
    R-SAFETY-08, R-PROC-01.
  - §Error handling: the firmware never stops; every error path returns to "absent, retry".
- `docs/adr/0011-pure-link-step.md` — `step( )` is pure and takes elapsed µs.
- `docs/adr/0012-decode-status-carries-decode-outcomes-only.md` — a status that needs history
  is a `FaultCause`, never a `DecodeStatus`. `Declined` is one.
- `src/core/ps2_protocol.h` — the command bytes, already declared: `kCmdConfig`,
  `kCmdSetMode`, `kConfigEnter`, `kConfigLeave`, `kModeAnalog`, `kModeDigital`, `kModeLocked`,
  plus `kPadByte`, `id_from_byte( )` and `frame_len( )`.
- `src/core/link.h`, `src/core/link.cpp`:
  - `Link`, `FaultCause` (gains `Declined`) and `step( )`;
  - `add_saturating( )`, for the debt case;
  - the `belay-debt:` marker on `kNegotiationTimeoutUs`, which says this phase owns tightening
    it.
- `src/core/ps2_frame.h`, `src/core/ps2_frame.cpp` — `decode( )` refuses a short span as
  `AckTimeout` and ignores bytes past the frame (the debt case).
- `src/core/poll.h`, `src/core/poll.cpp`:
  - `decode_poll( )`, used unchanged;
  - `count_poll( )` and `PollTally`, which change;
  - `kDigitalPoll`, deleted;
  - `kDigitalPollLen`, which stays because `tests/test_poll.cpp` uses it.
- `src/core/guitar_state.h` — `map_frame( )` and `GuitarState::whammy`: the summary's whammy
  goes through it (R-PROTO-04).
- `src/core/bus_trace.h` — `WireByte`.
- `src/hal/bus_frame.h` — `exchange_frame( )`. A byte whose ACK never came is not counted as
  completed, and the last byte of a frame waits for no ACK. A 9-byte poll to a digital
  controller therefore completes 4 bytes.
- `src/app/main.cpp` — the poller: `poll_once( )`, `print_summary( )`, `fault_name( )`.
- `src/emu/sg_model.h`, `src/emu/sg_model.cpp` — `SgModel::step( )`, `reset( )`, `apply( )`,
  `parse_fault( )`, and the wire-index constants.
- `docs/phases/05-emulator/spec.md` §Goal — the emulator command grammar this phase extends.
- `tests/ps2_codec_cases.cpp`, `tests/test_ps2_codec.py` — where the codec, link and new
  negotiation cases go, the rule-line convention (`report( is_ok, "R-… (…)" )`), and
  `MUTATIONS`.
- `tests/emulator_cases.cpp`, `tests/test_emulator.py` — the same for the emulator.
- `tests/test_bus_frame.py`, `tests/test_bus_trace.py`, `tests/test_pin_table.py` — the other
  three drivers that gain the cache and the flags lookup. `test_bus_trace.py` also runs
  `tools/trace_decode.py`; `test_pin_table.py` also reads `docs/wiring.md`.
- `tests/test_poll.cpp` — `count_poll( )`'s cases, which change with its signature.
- `tests/test_checks_are_live.py` — `mutation_score( )` and `CACHE_DIR`: the 27-live-mutant-cache
  shape this phase copies (empty marker files under `build/`, kills only, `FULL=1` bypass,
  `FULL` must be unset, empty or `1`).
- `tests/vectors/README.md`, `tests/vectors/poll_exchange.h` (`kPollCommand`, `kAddressReply`),
  `tests/vectors/config_mode.h`, `tests/vectors/analog_idle.h`,
  `tests/vectors/analog_whammy_full.h`, `tests/vectors/digital_idle.h`
  — hand-written vectors (R-PROTO-05).
- `tools/hil_digital.py` — the harness.
- `docs/phases/06-hil-digital/spec.md` §Goal — the harness rules this phase keeps: the summary
  line's fields, a **window** (k summaries after two discarded ones), **Δ**, the `master silent`
  timeout, the `att=high` and `us` rules, and the **recovery check**.
- `Makefile` — `CXXFLAGS`, the `test` recipe (passes `FULL`), `hil`, the header comment.
- `.claude/workflow/carry-over-exempt` — lists `Makefile`, `src/emu/main.cpp`,
  `tests/ps2_codec_cases.cpp`, `tests/test_ps2_codec.py` and `tools/hil_digital.py`. Each line
  must be re-checked when its file changes (the file's header says so).
- `.claude/rules/tech-debt.md` — the three entries this phase pays.
- `docs/phases/06-hil-digital/notes.md` §For later phases — the 07 bullets, the bench hazard,
  and the macOS accessory approval.
- `docs/phases/06-hil-digital/verify.md` — the flashing and port-finding procedure that this
  phase's `verify.md` reuses.
- `docs/phases/05-emulator/notes.md` §For later phases — the emulator answers only `01 42 …`.
- `docs/adr/0016-emulator-open-drain-outputs.md` — the newest ADR, whose shape
  ADR-0017 copies (no `docs/templates/` file exists for `verify.md`; the shape of
  `docs/phases/06-hil-digital/verify.md` is the one to follow).

## Plan

1. **ADR.** Write `docs/adr/0017-analog-negotiation.md`. It records:
   - a pure `core` sequencer;
   - the 5-byte enter judged on its prefix;
   - every other frame 9 bytes;
   - `FaultCause::Declined` → `Absent` → retry, with the master never streaming digital;
   - the emulator applying commands at the next frame's start.

   Check: `test -f docs/adr/0017-analog-negotiation.md` → exit 0.

2. **Flags from `make`.**
   - `Makefile` gains a `print-CXXFLAGS` target (listed in `.PHONY`) that prints `$(CXXFLAGS)`.
   - New `tests/driver_support.py` has `cxxflags( )`. It runs
     `make -s --no-print-directory print-CXXFLAGS` in the repo root and returns the
     whitespace-split words. It fails loudly if `make` exits non-zero.
   - Each of the five drivers sets `sys.dont_write_bytecode = True` before importing it, and
     replaces its `CXXFLAGS` literal with `cxxflags( )`.

   Check:
   - `grep -l '"-std=c++23"' tests/test_*.py` → no output;
   - `make -s print-CXXFLAGS` → the `Makefile`'s line;
   - `make test` → `OK`;
   - `git status --porcelain tests | grep pycache` → no output.

3. **Mutant cache in the five drivers.** `tests/driver_support.py` gains the cache:
   - **The key** is a SHA-256 over:
     - `$CXX` (default `c++`);
     - the flags;
     - the content of every file under `src/`, `tests/` and `tools/`, excluding `mut_*` and
       `__pycache__`, plus `docs/wiring.md`, in sorted path order. This is computed once per
       run.
     - the mutation's own fields.

     Add a `# ponytail:` comment: any edit under those trees invalidates every suite. A
     per-suite dependency list is the upgrade if that cost shows.
   - **A hit** is an empty file at `build/mutant-cache/<driver basename>/<key>`.
   - **A kill is recorded** only when `reject( )` returns true. A survivor is never recorded.
   - **`FULL=1`** skips the lookup but still records. A `FULL` value other than unset, empty or
     `1` fails the driver.
   - **Output:** each driver's `rejection cases: n/n` line ends with
     `, <k> served from cache`.

   Check:
   - `make test` twice → the second run's five lines each read `, <n> served from cache`, with
     n equal to that line's total;
   - `FULL=1 python3 tests/test_emulator.py` → `0 served from cache`;
   - `git check-ignore build/mutant-cache` → exit 0.

4. **The two `decode` / `step` cases** in `tests/ps2_codec_cases.cpp`:
   - `kDigitalIdle` plus one trailing `0x00` byte decodes to the same frame as `kDigitalIdle`;
   - from a fresh `Link`, `step( analog, 0 )`, then `step( analog, UINT32_MAX )` twice, leaves
     `us_in_state == UINT32_MAX`. The analog frame comes from `kAnalogIdle`.

   Check:
   - `python3 tests/test_ps2_codec.py` → exit 0;
   - by hand: replace `add_saturating`'s body with a plain `return base + addend;` → the
     saturation case prints `FAIL`. Restore the body. Record the run in `notes.md`.

5. **Vector.** New `tests/vectors/negotiation.h`: `kEnterConfig` (5 bytes), `kSetAnalog`
   (9 bytes) and `kLeaveConfig` (9 bytes), each byte commented. These are the table in §Goal.
   `tests/vectors/README.md` gains one paragraph naming the file as master-side bytes, like
   `poll_exchange.h`.

   Check: `make test` → `OK`. R-PROTO-05's check still passes: nothing under `src/`
   references `tests/vectors`.

6. **`core`: `Declined`, the sequencer, the tally.**
   - `src/core/link.h`:
     - `FaultCause` gains `Declined` with a one-line comment;
     - the `belay-debt:` marker on `kNegotiationTimeoutUs` names `09-guitar-observe` instead of
       this phase (the emulator negotiates in one frame of `Negotiating`, so it measures
       nothing about the guitar).
   - New `src/core/negotiation.h` / `.cpp`:
     - `enum class NegotiationStage { EnterConfig, SetAnalog, LeaveConfig, Polling }`;
     - `struct Master { Link link; NegotiationStage stage = EnterConfig; }`;
     - `command_for( NegotiationStage ) -> std::span<const std::uint8_t>`, returning the
       §Goal table's bytes, built from `ps2_protocol.h` constants;
     - `struct Exchanged { std::span<const WireByte> wire; std::size_t completed; }`;
     - `[[nodiscard]] LinkState advance( Master&, const Exchanged&, std::uint32_t elapsed_us )`,
       implementing §Goal's judging rules 1–3 exactly.
   - `src/core/poll.{h,cpp}`:
     - `kDigitalPoll` is deleted;
     - `count_poll` becomes `count_poll( PollTally&, LinkState now, const DecodeOutcome& )`:
       `polls` +1 always; `refused` +1 when `now` is `Absent`; the payload comparison only when
       `now` is `AnalogStreaming` and the outcome has a value.
   - `tests/test_poll.cpp`: the tally case is rewritten to the new signature, with the same five
     steps, and gains one case: a good digital outcome with `now = Absent` counts as refused
     and changes nothing else.
   - `tests/ps2_codec_cases.cpp`:
     - `command_for( )` equals the three vectors and `kPollCommand`;
     - the happy path, from a fresh `Master`:
       - `kDigitalIdle` answering the enter: stage `SetAnalog`, link `Absent`;
       - `kConfigMode` twice: `Negotiating`, then stage `Polling`;
       - `kAnalogIdle`: `AnalogStreaming`;
     - the enter answered with the first 4 bytes of `kAnalogIdle`, 5 wire bytes completed:
       accepted;
     - the enter with 0 completed: `Absent`, `AckTimeout`, stage `EnterConfig`;
     - `Polling` answered by `kDigitalIdle` with 4 completed: `Absent`, `Declined`, stage
       `EnterConfig`;
     - `SetAnalog` answered by `kDigitalIdle` with 4 completed: `Declined`;
     - `Polling` answered by a refusal (`kUnknownId`): `Absent`, `UnknownId`, stage
       `EnterConfig`;
     - one rule line, `R-PROTO-10 (…)`, that aggregates these.
   - `tests/test_ps2_codec.py` `MUTATIONS` gains one R-PROTO-10 mutation: the declined branch
     is skipped, so a wrong id falls through to `step( )`.
   - `docs/constraints.md` gains **R-PROTO-10**, the master side of §Goal's sequence and
     judging rules, bound `test: tests/test_ps2_codec.py`. `tests/test_ps2_codec.py`'s
     docstring gains its `RULE R-PROTO-10` marker.

   Check:
   - `make test` → `OK`, with `R-PROTO-10` printing `ok:` and its rejection case flipping it;
   - `python3 tests/test_rule_traceability.py` → exit 0;
   - `make typecheck` → exit 0.

7. **Emulator.**
   - `src/emu/sg_model.{h,cpp}`:
     - config mode and the deferred commands, as in §Goal;
     - `FaultKind::Decline`, with the grammar `fault none|ack <n>|late <us>|id <hh>|decline`;
     - the `mode` command clears config mode.
   - `tests/emulator_cases.cpp`, driven with `tests/vectors/negotiation.h` and
     `kPollCommand`:
     - from boot, the enter is answered `kAddressReply` + `kDigitalIdle`;
     - set-analog and leave-config are each answered `kAddressReply` + `kConfigMode`, with the
       ninth byte not ACKed;
     - the next poll is answered `kAddressReply` + `kAnalogIdle`;
     - after `mode analog`, the enter is answered `kAddressReply` + the first 4 bytes of
       `kAnalogIdle`;
     - `01 44` outside config mode breaks the frame: no ACK from byte 1, `kReleasedByte` after;
     - with `fault decline`, the poll after the sequence is answered as `kDigitalIdle`;
     - `fault decline x` is refused.
   - `tests/test_emulator.py` `MUTATIONS` gains two mutations:
     - R-EMU-01: set-analog is ignored;
     - R-EMU-02: the decline fault is ignored.
   - `docs/constraints.md`: R-EMU-01's text and Scope clause name the config-mode answers and
     the accepted wire-byte-1 commands; R-EMU-02's name `fault decline` and the new refused
     line.

   Check:
   - `python3 tests/test_emulator.py` → exit 0, rejection cases 4/4;
   - `make test` → `OK`.

8. **Master firmware.** `src/app/main.cpp`:
   - `Poller` holds a `ps2::Master` instead of a bare `Link`, plus the last whammy;
   - `poll_once( )`:
     - fills `wire` (9 entries) from `command_for( stage )`;
     - exchanges `wire.first( command.size( ) )`;
     - calls `decode_poll`, then `advance`, then `count_poll( tally, now, outcome )`;
     - stores `map_frame( *outcome ).whammy` when `now` is `AnalogStreaming` and the outcome
       has a value;
   - `print_summary( )` appends `whammy=`;
   - `fault_name( )` gains `declined`;
   - the header comment describes the analog poller.

   `Makefile`'s header comment for `make firmware` and `make hil` names this phase.

   Check:
   - `make firmware` → exit 0;
   - `make lint` → exit 0.

9. **Harness.** `tools/hil_digital.py`:
   - the `SUMMARY` regex gains `whammy`;
   - `expect_streaming` checks `state=analog` and `whammy=80`;
   - the `sweep` and `fault decline` scenarios are added as in §Goal;
   - the docstring points at this spec.

   Check: `python3 tools/hil_digital.py --help` → exit 0.

10. **Carry-over list.** Re-read each `.claude/workflow/carry-over-exempt` line whose file this
    phase changed, and confirm it still reads no phase `spec.md` or `notes.md`. Otherwise
    delete the line.

    Check: `grep -n 'spec.md\|notes.md' $(grep -v '^#' .claude/workflow/carry-over-exempt)` →
    every hit is a comment or docstring that names a path, and none opens one (read by hand,
    recorded in `notes.md`).

11. **Bench.** Flash `sg2hid.uf2` to the master and `sg2hid_emu.uf2` to the emulator. Never
    with the guitar connected (R-SAFETY-08). Run `make hil MASTER=… EMU=…`.

    Check: `hil: PASS`. Record in `notes.md` §Bench readings:
    - the `link:` lines of one negotiation;
    - one summary line from `sustained`;
    - one from `fault decline`.

12. **`docs/phases/07-analog-mode/verify.md`**, for a non-specialist, with the two R-PROC-02
    headings. It explains:
    - what config mode is;
    - why the guitar must be asked for analog mode before the whammy exists;
    - what the four frames do.

    It gives the flash and `make hil` procedure, reusing 06's, and the expected `hil: PASS`. It
    says what `whammy=` shows during `sweep`. It notes that 06's `verify.md` bench expectations
    (`state=digital`) no longer hold for this firmware.

    Check: `sh tests/test_phase_docs.sh` → exit 0 once the phase is `done`. Before that,
    `grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' docs/phases/07-analog-mode/verify.md` →
    2.

## Acceptance criteria

Run in order. The last criterion needs the two-Pico bench.

```
make test 2>&1 | tail -n 1                                       # expect: OK
make test 2>&1 | grep -c 'rejection cases: .*served from cache'  # expect: 5
make test 2>&1 | sed -nE 's/.*rejection cases: [0-9]+\/([0-9]+).*, ([0-9]+) served from cache.*/\1 \2/p' | awk '$1 != $2 { bad=1 } END { exit bad }'   # expect: exit 0 (a warm run serves every mutant from the cache)
FULL=1 python3 tests/test_emulator.py | grep -c ', 0 served from cache'   # expect: 1
git check-ignore -q build/mutant-cache                           # expect: exit 0
grep -l '"-std=c++23"' tests/test_*.py | wc -l                   # expect: 0
make -s --no-print-directory print-CXXFLAGS                      # expect: -std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc
git status --porcelain | grep -c pycache                         # expect: 0
make lint                                                        # expect: exit 0
make typecheck                                                   # expect: exit 0
make firmware && test -f build/pico/sg2hid.uf2 && test -f build/pico/sg2hid_emu.uf2 && test -f build/pico/sg2hid_loopback.uf2   # expect: exit 0
make build/host/test_poll && build/host/test_poll                # expect: exit 0
python3 tests/test_ps2_codec.py | grep -c 'ok: .*R-PROTO-10'     # expect: 1
python3 tests/test_emulator.py | grep -c 'rejection cases: 4/4'  # expect: 1
grep -c '^- \*\*R-PROTO-10\*\*' docs/constraints.md              # expect: 1
grep -c 'Declined' src/core/link.h                               # expect: >= 1
grep -cw 'kDigitalPoll' src/core/poll.h                         # expect: 0
grep -c 'belay-debt.*09-guitar-observe\|09-guitar-observe.*tighten' src/core/link.h   # expect: >= 1
grep -c 'decline' src/emu/sg_model.cpp                           # expect: >= 1
grep -c 'whammy=' src/app/main.cpp tools/hil_digital.py | grep -c ':0$'   # expect: 0
test -f docs/adr/0017-analog-negotiation.md                      # expect: exit 0
test -f tests/vectors/negotiation.h                              # expect: exit 0
grep -rl 'tests/vectors' src | wc -l                             # expect: 0
python3 tools/hil_digital.py --help >/dev/null                   # expect: exit 0
make hil >/dev/null 2>&1; test $? -ne 0                          # expect: exit 0 (no ports → refused)
grep -n 'spec.md\|notes.md' $(grep -v '^#' .claude/workflow/carry-over-exempt)   # expect: every hit a comment or docstring naming a path; none opens one (read by hand)
git diff --quiet main -- src/hal tools/trace_decode.py tests/vectors/poll_exchange.h tests/vectors/config_mode.h tests/vectors/analog_idle.h tests/vectors/digital_idle.h   # expect: exit 0
python3 tests/test_rule_traceability.py                          # expect: exit 0
grep -c '^#\+ What was built\|^#\+ .*[Cc]heck it' docs/phases/07-analog-mode/verify.md   # expect: 2
make hil MASTER=<master port> EMU=<emulator port> | tail -n 1     # expect: hil: PASS (bench; exit 0)
```

## Out of scope

- Reading the frame length from the header in `exchange_frame( )` — rejected by the operator
  2026-10-08. Any edit under `src/hal/`.
- Tightening `kNegotiationTimeoutUs` — re-owned to `09-guitar-observe` (Plan step 6).
- Streaming digital as a fallback, or a retry backoff after `Declined` — not wanted.
- The `0x45` status query, pressure modes, rumble (`0x4D`), and the `5A` padding some libraries
  send on leave-config.
- An emulator `sweep` command that ramps the axis by itself — the harness drives `payload`.
- Renaming `tools/hil_digital.py`. R-PROTO-06 and R-PROTO-07 cite the name.
- Editing `docs/phases/06-hil-digital/verify.md`.
- The USB HID descriptor and report — `08-usb-hid`.
- The real guitar — `09-guitar-observe`.
- Caching `tests/test_trace_shift.py` (it has no build) or `tests/test_checks_are_live.py`
  (already cached).
- A per-suite dependency key for the mutant cache.
- Editing `.claude/rules/tech-debt.md`. The three entries' removal is proposed to the operator
  after validation, never written by this phase.
- The other debt entries ("`test_rule_traceability.py`'s own logic…", "Grep checks have no
  type information", "`make test` runs its 16 test files one after another").
