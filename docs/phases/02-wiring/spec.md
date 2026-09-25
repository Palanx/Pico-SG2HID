# Phase 02-wiring — GPIO assignment, the pin table, the wiring document and the multimeter checklist

## Goal

After this phase, the bus's five signals have GPIO numbers, and those numbers live in exactly
one place: `src/core/pins.h`, a `constexpr` table of plain data that the host tests read
directly. `tests/test_pin_table.py` compiles `tests/pin_table_cases.cpp` against that table and
fails `make test` when `DATA` or `ACK` is anything but an input that is not push-pull
(R-SAFETY-01), when a bus signal is missing from the table or declared twice, or when
`docs/wiring.md` names a different GPIO for any signal than the table does (R-SAFETY-02). It also
fails when an entry uses a board-reserved GPIO (23, 24, 25, 29) or two signals share one
(R-SAFETY-03). `tests/test_repo_shape.sh` fails when any file under `src/` outside `src/hal/`
calls a Pico SDK function that configures a pin (R-SAFETY-09, split out of R-SAFETY-02 and
widened by operator decision on 2026-09-24). The fourth clause, that `src/hal/` configures pins
only by iterating the table, becomes R-SAFETY-10, `planned: 03-pio-bus`. `src/hal/` does not
exist before that phase.

`docs/wiring.md` tells the operator what connects to what: Pico pin, PS2 socket pin, the
resistors in between, and the 7.6 V pin left unconnected. `docs/phases/02-wiring/verify.md`
teaches the operator how to use a multimeter and then walks through the checklist. The checklist
has three parts. Continuity is checked with everything unpowered. Voltage is checked with the
Pico on USB and the guitar unplugged. Current is measured with the guitar connected by 3V3 and
GND only.

The operator runs the checklist. Their answers close requirements.md's OPEN item about the
connector and the current-draw half of the OPEN item about power. The other half of that item,
whether the SG *works* on 3V3 alone, cannot be observed without bus traffic, so it stays OPEN
and moves to `09-guitar-observe`.

Decided by the operator on 2026-09-24, and not to be re-litigated during implementation:

| Decision | Outcome |
|---|---|
| Connector on the bench | A PS2 female socket with all nine pins broken out. Pins are identified from the socket's numbering, then confirmed by continuity. |
| Current-draw measurement vs R-SAFETY-08 | Allowed in this phase with **power-only** wiring. Only 3V3 and GND reach the guitar, no signal line is connected, and the Pico is held in BOOTSEL. R-SAFETY-08's ordering governs the signal lines. ADR-0013 records this and amends ADR-0006's ordering sentence. |
| R-SAFETY-02's "src/hal/ iterates the table" clause | Split out as R-SAFETY-10, `planned: 03-pio-bus`. |
| R-SAFETY-02's function list | Widened. See the R-SAFETY-09 list below. |

The ADR-0005 grammar allows one `test:` path per rule, which forces a second split. R-SAFETY-02
keeps the table clauses, which are checked by `tests/test_pin_table.py`. The grep clause becomes
R-SAFETY-09, checked by `tests/test_repo_shape.sh`.

**The pin assignment and the wiring are this spec's decisions.** They are defaults, open to
operator veto before step 2 lands:

| Signal | Pico GPIO | Pico physical pin | PS2 socket pin | Direction | Drive mode | On the breadboard |
|---|---|---|---|---|---|---|
| DATA | GP2 | 4 | 1 | Input | OpenDrainInputOnly | 330 Ω in series; 10 kΩ pull-up to 3V3 on the Pico side |
| CMD | GP3 | 5 | 2 | Output | PushPull | 330 Ω in series |
| ATT | GP4 | 6 | 6 | Output | PushPull | 330 Ω in series |
| CLK | GP5 | 7 | 7 | Output | PushPull | 330 Ω in series |
| ACK | GP6 | 9 | 9 | Input | OpenDrainInputOnly | 330 Ω in series; 10 kΩ pull-up to 3V3 on the Pico side |
| VCC 3.3 V | 3V3(OUT) | 36 | 5 | — | — | direct |
| GND | GND | 38 | 4 | — | — | direct |
| 7.6 V | — | — | 3 | — | — | **not connected** (R-SAFETY-04) |
| unused | — | — | 8 | — | — | not connected |

The GPIOs are consecutive so that `03-pio-bus` can map them to PIO pin groups. The series
resistors limit the current if a wire is swapped or a pin is misconfigured: 3.3 V / 330 Ω is
10 mA, inside an RP2040 pin's rating. `OpenDrainInputOnly` means that the line is open-drain on
the bus, that the Pico only reads it, and that its internal pull-up is enabled. The external
10 kΩ sets the real edge rate.

R-SAFETY-09's forbidden set, outside `src/hal/`, is matched as the name followed by optional
whitespace and `(`. A trailing `*` means any `[a-z0-9_]` suffix:

| Family | Members |
|---|---|
| GPIO init | `gpio_init*` (covers `gpio_init_mask`) |
| GPIO direction | `gpio_set_dir*` (covers the `_masked`, `_in_masked`, `_out_masked`, `_all_bits` forms) |
| GPIO function | `gpio_set_function*` |
| GPIO pulls | `gpio_set_pulls`, `gpio_pull_up`, `gpio_pull_down`, `gpio_disable_pulls` |
| GPIO override | `gpio_set_oeover` |
| PIO | `pio_gpio_init`, `pio_sm_set_pindirs_with_mask*`, `pio_sm_set_consecutive_pindirs` |

## Context pointers

- `CLAUDE.md` §Hardware safety. The R-SAFETY lines it summarises change in step 1.
- `docs/constraints.md` §Invariants §Hardware safety holds R-SAFETY-01..10 and is where the
  bindings change. See §Process R-PROC-01 for the binding grammar and §Observed conventions for
  plain C arrays and `AlignArrayOfStructures`.
- `docs/adr/0005-rule-test-traceability.md` defines the binding grammar: one `test:` path per
  rule.
- `docs/adr/0006-fail-safe-hardware-policy.md` gives the pin-table decision and the "nothing
  powered before measured" ordering that ADR-0013 amends.
- `docs/adr/0008-cpp23.md` covers designated initializers for `pins.h`.
- `docs/templates/adr.md` is the template for ADR-0013.
- `docs/product/requirements.md` §Hard constraints and §Open questions. Step 7 rewrites the two
  OPEN items there.
- `docs/phases/00-scaffold/notes.md` §For later phases (the first section, lines 280–361). It
  covers the plain-array constraint on `pins.h`, the three requirements for any new check (RULE
  header, `LIVE` labels, cases routed through the reporting function plus a wiring case), and
  why scratch copies use `cp -a`.
- `docs/phases/01-ps2-codec/notes.md` §For later phases. It explains why `.py` checks are held
  to the accounting property only.
- `tests/test_ps2_codec.py` + `tests/ps2_codec_cases.cpp` are the pattern `test_pin_table.py` +
  `pin_table_cases.cpp` copy: the driver compiles the cases file with `make test`'s flags, lines
  are forwarded verbatim, rejection cases mutate a copied tree and must flip their own rule's
  line, and every mutation asserts that its anchor matched.
- `tests/test_repo_shape.sh`. `src_files( )`, `hits( )`, `report( )`, `run_all( )`, `reject`,
  `accept` and `wiring` are where R-SAFETY-09 lands.
- `tests/test_checks_are_live.py`. `DECL`, `CASE_LINE` and `property_accounting( )` define what a
  real-run line and a `LIVE` label must look like, and `alternation_jobs( )` demands one
  rejection case per regex alternative.
- `tests/test_rule_traceability.py`, `check( )`, is what fails if a binding and its marker
  disagree.
- `Makefile` shows `CXXFLAGS` and that only `tests/test_*.cpp` are built directly, which is why
  the cases file is not named `test_*`.
- `docs/phases/01-ps2-codec/verify.md` is the house register for a non-specialist `verify.md`.
- The Raspberry Pi Pico datasheet (pinout figure; §"Powering Pico", 3V3 load recommendation of
  under 300 mA). `docs/wiring.md` cites it by URL.
- A PS2 controller pinout reference, cited by URL in `docs/wiring.md` together with the side
  it is drawn from. The implementer finds and cites it; this spec does not supply one.

## Plan

1. **ADR and catalogue.**
   - Write `docs/adr/0013-power-only-current-draw-measurement.md`. It is accepted and records
     the power-only measurement: which wires, BOOTSEL, when. It supersedes only ADR-0006's
     ordering sentence for the power pins. Add "amended by ADR-0013" to ADR-0006's Status
     line, the way ADR-0001 records ADR-0008.
   - In `docs/constraints.md`, rewrite R-SAFETY-02 to its table clauses: every GPIO this
     project uses is declared exactly once in `src/core/pins.h` with signal, direction and
     drive mode, and `docs/wiring.md`'s pin table names the same GPIO per signal.
   - Add R-SAFETY-09 (the §Goal function list, outside `src/hal/`, under `src/`), with a
     split-and-widened note dated 2026-09-24. Leave it `planned: 02-wiring` for now.
   - Add R-SAFETY-10 (`src/hal/` configures pins only by iterating the table),
     `planned: 03-pio-bus`.
   - Append to R-SAFETY-08 one sentence stating that its ordering governs the signal lines,
     and that the phase-02 current draw is taken power-only per ADR-0013.
   - Update the R-SAFETY lines in `CLAUDE.md` to name 09 and 10.
   - Check: `make test` shows traceability green, and
     `grep -c 'planned: 03-pio-bus' docs/constraints.md` is 5.
2. **`src/core/pins.h`.**
   - Namespace `ps2`.
   - `enum class Signal : std::uint8_t { Data, Cmd, Att, Clk, Ack }`.
   - `enum class Direction : std::uint8_t { Input, Output }`.
   - `enum class DriveMode : std::uint8_t { PushPull, OpenDrainInputOnly }`.
   - `struct PinAssignment { std::uint8_t gpio; Signal signal; Direction direction; DriveMode drive; }`.
   - `inline constexpr PinAssignment kMasterPins[]` is a **plain C array** with designated
     initializers, holding the §Goal table.
   - The header comment points to `docs/wiring.md` and says the table is authoritative.
   - Include only `<cstdint>`.
   - Check: `c++ -std=c++23 -fsyntax-only -Isrc -xc++ src/core/pins.h` exits 0, and
     `make test` and `make lint` are green.
3. **`docs/wiring.md`.** It contains:
   - The §Goal table as a Markdown table whose rows start `| DATA | GP2 |`, `| CMD | GP3 |`, …
     (the signal name in upper case, then `GP<n>`). The check in step 4 parses this shape.
   - A parts list: the socket, 5 × 330 Ω, 2 × 10 kΩ, jumper wires.
   - A breadboard layout in words.
   - One paragraph per resistor on why it is there.
   - The 7.6 V pin 3 left unconnected, and how to insulate it.
   - The Pico datasheet and the PS2 pinout source by URL, with the viewing side of the socket
     stated.
   - Check: `grep -cE '^\| (DATA|CMD|ATT|CLK|ACK) \| GP[0-9]+ \|' docs/wiring.md` is 5.
4. **Pin-table check.**
   - `tests/pin_table_cases.cpp` defines one check function per rule, each taking
     `std::span<const ps2::PinAssignment>`, and one aggregate function.
   - The real run reports `kMasterPins` with one line per rule: `R-SAFETY-01`,
     `R-SAFETY-02 (table)` and `R-SAFETY-03`.
   - It prints one `pin: <SIGNAL> GP<n>` line per entry.
   - In-process rejection cases on fixture tables go through the same reporting function:
     DATA push-pull, ACK push-pull, DATA as output, ACK as output, a missing signal, a
     duplicated signal, one fixture for each of GPIO 23, 24, 25 and 29, and two signals on one
     GPIO. They print `rejection cases: n/n`.
   - Wiring cases, one per rule, run the aggregate on a bad table and require it to fail and
     name the rule. They print `wiring cases: n/n`.
   - The reserved GPIOs are a named `constexpr` in the test, not in `core`.
   - `tests/test_pin_table.py` has a docstring header with `RULE R-SAFETY-01`,
     `RULE R-SAFETY-02`, `RULE R-SAFETY-03`, `LIVE R-SAFETY-02 (table)` and
     `LIVE R-SAFETY-02 (wiring doc)`.
   - The driver compiles only the cases file with `make test`'s flags, forwards its lines, and
     compares its `pin:` lines to `docs/wiring.md`'s rows as the `R-SAFETY-02 (wiring doc)`
     line.
   - It runs four copied-tree mutations, reported as one `copied-tree rejection cases: n/4`
     line, each asserting that its anchor matched and requiring
     its own line to say FAIL:
     - DATA's drive set to `PushPull` flips R-SAFETY-01.
     - The ACK entry deleted flips `R-SAFETY-02 (table)`.
     - DATA's `GP2` changed in `docs/wiring.md` flips `R-SAFETY-02 (wiring doc)`.
     - CLK moved to GPIO 25 flips R-SAFETY-03.
   - Flip R-SAFETY-01..03 to ``test: `tests/test_pin_table.py` ``, each with a `**Scope**` clause
     naming what the check reads: `kMasterPins` only, the fixed reserved list, and the
     `docs/wiring.md` row shape.
   - Check: `python3 tests/test_pin_table.py` exits 0, and `make test` is green, including the
     `accounting: test_pin_table.py` line.
5. **R-SAFETY-09 grep.**
   - In `tests/test_repo_shape.sh`, add the `RULE R-SAFETY-09` header line and `find_safety09( )`
     over `src_files` minus paths under `$1/src/hal/`, through `hits( )`. Add its `run_all`
     line.
   - Add one `reject` per regex alternative. `tests/test_checks_are_live.py` names any that are
     missing. Also add rejects placed in `src/app/`, `src/emu/`, `src/usb/` and `src/core/`.
   - Add `accept` cases: the same call in `src/hal/x.cpp`; `gpio_put( 2, true );` and
     `gpio_get( 6 );` outside hal. Raise the false-positive floor to the new count.
   - Add one `wiring` case, and raise `wiring cases` to 11/11.
   - Flip R-SAFETY-09 to ``test: `tests/test_repo_shape.sh` `` with a `**Scope, recorded**`
     clause. Measure one fixture per form and record each as reported or not:
     - a call split across lines;
     - a call through a macro or a function pointer;
     - a direct register write (`sio_hw->gpio_oe_set`, `iobank0_hw`, `padsbank0_hw`);
     - a `set pindirs` instruction in a `.pio` file;
     - a `.c` file;
     - a call inside `/* … */` or a string.
   - Check: `sh tests/test_repo_shape.sh` shows `ok:   R-SAFETY-09` and `wiring cases: 11/11`,
     and `make test` is green.
6. **`docs/phases/02-wiring/verify.md`.** It has `## What was built` and `## Check it yourself`.
   It contains:
   - A multimeter primer:
     - the three modes used (resistance/continuity, DC volts, DC mA) and which jack the red
       probe goes in for each;
     - why a meter left in mA mode across a voltage is a short;
     - why 3V3–GND resistance on a Pico reads a climbing value instead of a fixed one.
   - The checklist, every item with its expected reading:
     - **Unpowered**, USB unplugged, guitar unplugged:
       - each signal line, Pico pin to socket pin, reads ≈ 330 Ω;
       - Pico 36 to socket 5, and Pico 38 to socket 4, beep;
       - 3V3 to GND is not near 0 Ω;
       - socket 3 has no continuity to Pico 36, 39 or 40, or to any signal line;
       - socket 5 has no continuity to Pico 39 or 40;
       - no two signal lines are bridged.
     - **Powered**, Pico on USB held in BOOTSEL, guitar unplugged:
       - socket 5 to socket 4 reads 3.2–3.4 V;
       - socket 3 to 4 reads ≈ 0 V;
       - DATA and ACK socket pins to 4 read ≈ 3.3 V.
     - **Current draw**: the five signal resistors removed from the breadboard, the meter in
       series in the 3V3 line on its mA range, the Pico in BOOTSEL, the guitar plugged in.
       Record the reading, then unplug and restore.
       - **Stop condition:** a reading ≥ 250 mA means the onboard regulator is not enough. Stop
         and report; that is a re-plan.
   - A place to write each reading.
   - Check: `grep -cE '^#+ What was built|^#+ .*[Cc]heck it' docs/phases/02-wiring/verify.md`
     is 2.
7. **Operator gate (manual).** The operator runs step 6's checklist and reports the readings.
   Then:
   - In `docs/product/requirements.md` §Open questions, replace the two OPEN lines with:
     - `- ANSWERED (02-wiring, <date>): <connector>`;
     - `- ANSWERED (02-wiring, <date>): the SG draws <N> mA from 3V3 …`;
     - `- OPEN (09-guitar-observe): does the SG answer the bus correctly on 3V3 alone?`.
   - Copy the full readings into `notes.md`.
   - Check: the requirements.md criteria below.

## Acceptance criteria

```
make test                                                        # expect: exit 0, last line OK
make lint                                                        # expect: exit 0
c++ -std=c++23 -fsyntax-only -Isrc -xc++ src/core/pins.h         # expect: exit 0
python3 tests/test_pin_table.py | grep -cE '^\s*ok:\s+(R-SAFETY-01|R-SAFETY-02 \(table\)|R-SAFETY-02 \(wiring doc\)|R-SAFETY-03)'   # expect: 4
python3 tests/test_pin_table.py | grep -cE '^\s*ok:\s+(copied-tree rejection|rejection|wiring) cases'   # expect: 3 — C++ in-process rejection, C++ wiring, driver copied-tree rejection
sh tests/test_repo_shape.sh | grep -cE '^\s*ok:\s+R-SAFETY-09|wiring cases: 11/11'   # expect: 2
python3 tests/test_checks_are_live.py | grep -c 'accounting: test_pin_table.py'     # expect: 1
grep -c 'planned: 02-wiring' docs/constraints.md                 # expect: 0
grep -cE '^- \*\*R-SAFETY-0[123]\*\* — .* — test: `tests/test_pin_table.py`$' docs/constraints.md   # expect: 3
grep -cE '^- \*\*R-SAFETY-09\*\* — .* — test: `tests/test_repo_shape.sh`$' docs/constraints.md       # expect: 1
grep -cE '^- \*\*R-SAFETY-10\*\* — .* — planned: 03-pio-bus$' docs/constraints.md                  # expect: 1
grep -c 'planned: 03-pio-bus' docs/constraints.md                # expect: 5 — R-SAFETY-07, R-SAFETY-10, R-PROTO-01, R-PROTO-06, R-ERR-05
grep -cE '^\| (DATA|CMD|ATT|CLK|ACK) \| GP[0-9]+ \|' docs/wiring.md   # expect: 5
ls docs/adr/0013-*.md                                            # expect: one file, Status: accepted
grep -c 'ADR-0013' docs/adr/0006-fail-safe-hardware-policy.md    # expect: 1
grep -cE '^#+ What was built|^#+ .*[Cc]heck it' docs/phases/02-wiring/verify.md   # expect: 2
grep -c '^- ANSWERED (02-wiring' docs/product/requirements.md     # expect: 2
grep -cE '^- ANSWERED \(02-wiring.* [0-9]+(\.[0-9]+)? mA' docs/product/requirements.md   # expect: 1
grep -c '^- OPEN' docs/product/requirements.md                   # expect: 1, and it names 09-guitar-observe
```

## Out of scope

- The emulator's pin table and R-SAFETY-06 belong to `05-emulator`.
- `src/hal/`, any SDK call, CMake, a firmware build and the bus clock rate belong to
  `03-pio-bus`, as does R-SAFETY-10's check. That phase also verifies R-SAFETY-09's names
  against the installed SDK headers. The SDK is not installed here.
- Whether the SG works on 3V3 alone belongs to `09-guitar-observe`, through the OPEN line step 7
  writes.
- Wiring any signal line to the guitar, and flashing anything while the guitar is connected, are
  forbidden (R-SAFETY-08). This phase connects power only.
- Mutation-testing `tests/test_pin_table.py`'s own internals is released `.py` harness debt (see
  `01-ps2-codec` notes). The copied-tree rejection cases stand in for it.
- A `CLAUDE.md` pointer-table row for `docs/wiring.md` is not wanted. `pins.h`'s header comment
  points to it.
