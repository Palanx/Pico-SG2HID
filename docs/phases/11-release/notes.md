# Phase 11-release — notes

## Outcome

- base: `3974a48` (`main` when `feat/11-release` was branched; the expansion is uncommitted
  in the same working tree)
- **Where implementation stopped (2026-10-13):** Plan steps 1–4 done. Step 3's bench run by the
  operator (`## Bench readings`); step 4 applied its README notes to `## Clone Hero`.
- **After the re-expansion (2026-10-13, round 3's escape):** the spec's Plan was reordered and
  re-numbered; every step's work already existed and none was redone. Plan 1 = `README.md`
  (unchanged since round 3); Plan 2 = the PS2-number correction in
  `docs/phases/02-wiring/verify.md` and `docs/wiring.md` (done earlier, now a Plan step);
  Plan 3 = the clean-clone build, re-run 2026-10-13 → `UF2`; Plan 4 = `verify.md` (unchanged);
  Plan 5 = the bench notes applied to `## Clone Hero` (unchanged). Acceptance criteria re-run
  after the re-expansion: 1 `1`; 2 the eleven headings in order; 3 and 4 no output, exit 1;
  5 `1`; 6 no output, exit 1; 7 `UF2`; 8 `2`; `make test` `OK`, `make lint` 0, `make typecheck` 0.
  The operator bench criterion stands from `## Bench readings` (same firmware sources).
- **Plan step 5, round after the re-expansion's first validation (2026-10-13):**
  `docs/phases/11-release/verify.md` brought up to date with the bench run. Property: no
  `verify.md` statement describes the README or the whammy as they were before the bench.
  Checked every section: intro (one broken line rejoined), §A README for using the device and
  §Building from nothing (still true), §Clone Hero (rewritten: mapper via space bar, steps tried
  on the bench), "One open question" → "The whammy needs calibrating" (answered by the bench, no
  firmware change), (d) step 3 (the before-first-move experiment) removed and (d) now says
  calibrate, (e)'s "steps 1 to 5" → "1 to 4", (g)'s whammy-before-first-move item → mapping and
  calibration, troubleshooting table + one row (wobbling sustains → calibrate). Criteria 1–6 and
  8 re-run: pass.
- Step 1 (first implementation): `README.md` rewritten with the eleven `##` sections of the spec's Goal table, in order.
  Clone Hero steps (control mapper, Replace, Calibrate, 5-fret instrument, `.dmg` / Linux
  `.tar.xz` from clonehero.net) come from the Clone Hero wiki
  (wiki.clonehero.net, "Controls" and "Installation" pages, read 2026-10-13), not from a bench
  run; step (d)/(e) of `verify.md` is where they are tried. `## If it does not work` names the
  four `state=` values `src/app/console.cpp` `state_name( )` prints (`absent`, `negotiating`,
  `digital`, `analog`).
- Step 2: criterion 6 run 2026-10-13 on the committed `HEAD` (`3974a48`, same build inputs: this
  phase changes no source): fresh pico-sdk 2.3.1 clone made with the README's exact command, a
  `git clone .`, `make firmware` → `UF2`, exit 0.
- Step 3: `docs/phases/11-release/verify.md`.
- Acceptance criteria run 2026-10-13 after steps 1–3: criteria 1–5 pass (`1`; the eleven headings
  in order; no stale string, exit 1; no literal port, exit 1; `1`); criterion 6 `UF2`; criterion 7
  `2`; `make test` 0 (`OK`), `make lint` 0, `make typecheck` 0. The operator bench criterion is
  not run.

## Deviations

- spec Goal `## Build` row said `brew install cmake`; `brew install cmake picotool` instead —
  criterion 6's log showed the build using `/opt/homebrew/bin/picotool`, and pico-sdk 2.3.1's
  `tools/Findpicotool.cmake` fetches and builds picotool from GitHub with `FetchContent` when it
  is absent, so without it a fresh Mac's `make firmware` needs a network fetch the README did
  not mention. Reconciled in the same edit: the Goal table's `## Build` row, the only spec
  sentence naming the brew installs.
- operator decision (2026-10-13): the README's Homebrew install steps are taken on faith, not
  run on a clean Mac; criterion 6 proves the SDK clone and the build only. Not debt.
- operator amended criterion: `the Deck flashed the clean-clone UF2 from Dolphin` → `the Mac
  flashed the clean-clone UF2 from Finder` — operator: flashing is Mac-only, both hosts play;
  flashing from the Deck is a contribution, not this phase. Reconciled in the same edit, every
  spec sentence naming where the UF2 is flashed: Goal `## Flash` row, Goal bench paragraph, Plan
  step 3 (b)–(e) (Mac now (d), Deck (e)), the bench criterion, Out of scope (new line). Also
  `PHASES.md`'s row acceptance ("a second machine flashes it" → the Mac flashes it), `README.md`
  `## Flash` (Dolphin step removed) and `verify.md` (b)–(e), (g). Checked: `grep -n Dolphin` over
  the spec, `README.md` and `verify.md` finds nothing.
- Files outside the spec's Plan, operator-approved (2026-10-13): `docs/phases/02-wiring/verify.md`
  Part C defined "Socket pin N" as the number printed on the breakout, and `docs/wiring.md`
  §Sources said pins are identified by that printed numbering; this bench's breakout prints them
  mirrored (wiring.md §This bench's breakout, phase 09), so the README's pointer to the checklist
  led to wiring the 7.6 V pin (printed 7) to CLK. Both now say PS2 number; wiring.md §Socket pin 3
  names the printed 7. README `## Wire` and `verify.md` (c) say PS2 number next to the pointer.
  Property: every instruction reachable from the README that names a socket pin by number.
  Checked: `grep -n 'printed\|numbering\|breakout'` over `docs/wiring.md` and
  `docs/phases/02-wiring/verify.md` — lines 68, 108–110 of wiring.md and line 108 of 02's verify
  fixed; wiring.md 73–94 already PS2-number; 02 verify line 218 is the recorded 2026-09-28 answer
  and left as history. Spec reconciled: Goal `## Wire` row, Context pointers (wiring.md
  section, 02 verify added — a missing pointer).
- The README's `## What it is` names the non-goals (the wireless guitar, other PS2 controllers,
  rumble, Windows, Pico 2) without the words "Les Paul", which criterion 3 rejects; requirements
  call it "the wireless Les Paul".
- spec amended (validation round 1, 2026-10-13): Goal `## Wire` row — deleted "the README
  repeats no pin numbers", and the parts are named as `docs/wiring.md` §Parts lists them;
  `## Build` row — the `PATH` remedy names the cask's
  `/Applications/ArmGNUToolchain/<version>/arm-none-eabi/bin`; `## If it does not work` row — the
  summary line about once a second (phase 10 `verify.md`) and every value `state_name( )` prints
  (`absent`, `negotiating`, `digital`, `analog`), with `src/app/console.cpp` added to Context
  pointers; `## Development` row — the seven targets as the `Makefile` header describes them;
  Plan 3(c) — reconnect via `## Flash`'s last paragraph, not phase 10 §3. Reconciled: the Context
  pointer for phase 10 `verify.md` §3–§6 (still true, the sequence is reused by reference), the
  acceptance criteria (none states pin numbers, cadence, states, targets or the `PATH`), Out of
  scope (none); no other spec sentence states these facts.
- spec amended (validation round 2, 2026-10-13): Goal `## Development` row "and what each needs"
  → "and the tools `make test`, `make lint` and `make firmware` need" — the needs the README
  states, a closed set. Reconciled: no other spec sentence states what a target needs.
- re-expansion (2026-10-13): the spec was rewritten from the row's Goal and these notes; its Goal
  table now names each README section's sources, not its content. The earlier `spec amended`
  entries above name spec text that no longer exists and stand as history. The operator
  decisions recorded above still hold: Mac-only flashing, both hosts play; nothing published;
  Homebrew installs taken on faith; the PS2-number correction in `docs/phases/02-wiring/verify.md`
  and `docs/wiring.md` (now Plan step 2).
- spec amended (validation round 1 after the re-expansion, 2026-10-13): Plan step 5 also brings
  `verify.md` §What was built up to date with the bench run; its check adds criterion 8. Owed to
  `/implement-phase`: `verify.md` §What was built still says the Clone Hero steps come from the
  wiki, not a bench run, and calls the whammy-before-first-move reading an open question with a
  firmware fix. Reconciled: Plan step 4 (writes `verify.md` before the bench, unchanged), Out of
  scope's firmware line (consistent); no other spec sentence states when `verify.md` changes.
- spec amended (validation round 2 after the re-expansion, 2026-10-13): Plan step 2 states that
  `docs/wiring.md` §Sources keeps its stop for a breakout with no numbering. Reconciled: Goal's
  wiring paragraph (about printed numbers, consistent); no other spec sentence names §Sources.

## Debt

- None.

## For later phases

- **Taste, from validation round 2 (not blocking):** `docs/wiring.md` §Sources still ends "If the
  breakout carries no numbering, stop and ask before wiring", which no longer follows from a text
  that no longer relies on printed numbering; README `## Flash` step 2 says "the computer" in a
  Mac-only section.
- **Taste, from validation round 3 (not blocking):** README `## Build`'s
  `export PICO_SDK_PATH="$PWD/pico-sdk"` is right only in the directory the clone was made in;
  `## Clone Hero` step 2's "Bind each action…" runs on instead of being its own step.
- **Taste, from validation after the re-expansion (not blocking):** `docs/wiring.md` §Socket pin 3
  has one reflowed line far longer than its neighbours; README `## If it does not work`'s
  `state=negotiating` bullet runs two sentences onto one long line; `verify.md`'s intro sentence
  breaks mid-line.
- **Taste, from validation round 2 after the re-expansion (not blocking):** `verify.md` (b) says
  "the master", a word the README never uses for the gamepad Pico.
- **Taste, from validation round 3 after the re-expansion (not blocking):** README `## Safety`
  says "pin 3" before `## Wire` explains that pin numbers are PS2 numbers; `verify.md` (a) step 2
  "clone a new copy here" reads as inside the repository clone, where the README says outside it.
- Whammy in a PS2 emulator (PCSX2): not a target (requirements, ADR-0019). If it becomes one,
  the PS2 game reads the whammy as left-stick Y resting at centre; with this firmware's rest at
  the axis minimum a half-axis binding leaves the first half of travel dead (inference, not
  tried). Owner: a new `/plan-feature` row, operator's call.

## Bench readings

Tree: `3974a48` + the uncommitted phase diff (docs only; firmware sources unchanged).

- **verify (d) Clone Hero on the Mac (operator, 2026-10-13):** "clone hero probado y jugado en
  mac, todo funciona perfecto"; on the whammy: "hasta el whammy funciona espectacular".
- **verify (d), whammy in Clone Hero (operator, 2026-10-13):** before the bar is first moved,
  sustains do not detect whammy (the host shows the axis at 0); after the first move it rests at
  -1, and with the first calibration sustains kept the whammy on at rest — the operator had
  calibrated it wrong. Recalibrated with the **center at -1**: "funciona como debería, es un calco
  de la lógica que he visto en guitar hero". Mapping the whammy needs the bar pushed to the stop
  for Clone Hero to detect the axis. Both now in README `## Clone Hero` steps 2 and 4 (Plan step
  4). Firmware unchanged: phase 10's "firmware nudge" fallback and a rest-at-center remap
  (discussed 2026-10-13, Santroller's HID mode also rests the whammy at the axis minimum) are not
  needed for Clone Hero.
- **Clone Hero control mapper (operator, 2026-10-13):** opened with the space bar on the keyboard
  from Clone Hero's main menu, or by clicking its "press space to open the control remapper"
  message (no keyboard needed); mapping and the whammy's Calibrate are both there. README
  `## Clone Hero` step 2 says so.
- **verify (e) Clone Hero on the Steam Deck (operator, 2026-10-13), same firmware as the Mac:**
  first reported "the zero was already -1, worked without calibrating", then corrected by the
  operator: the whammy did need calibrating with the center at -1, as on the Mac; Clone Hero then
  maps the axis's -1..1 to the game's 0..1. README `## Clone Hero` step 4 says to calibrate on
  every computer.
- **verify (a) and (b) (operator, 2026-10-13):** the firmware on the master, used on both the Mac
  and the Deck, was built from a fresh clone following the README and flashed from the Mac.
- **verify (e) result (operator, 2026-10-13):** "igual que en mac" — mapped, calibrated with the
  center at -1, and played as on the Mac.
- **verify (f), README gaps found at the bench and fixed in Plan step 4:** `## Clone Hero` did not
  say how to open the mapper (space bar, or clicking the "press space to open the control
  remapper" message), that the whammy binds only when pushed to the stop, nor that it must be
  calibrated with the center at -1 after moving the bar once. No other section was reported.
- **Bench criterion passes (2026-10-13):** the Mac flashed the clean-clone UF2; Clone Hero mapped
  and played with frets, strums and whammy on both hosts; the whammy before its first move is
  recorded above (no whammy at 0; after calibration, rest -1 is no whammy).

## Validation — 2026-10-13
- criteria: 11 passed / 0 failed (criteria 1–7 and the three project gates run this round; the
  operator bench criterion run by the operator on 2026-10-13 on the same firmware sources — this
  phase changes no source — see `## Bench readings`)
- project gates: test pass, lint pass, typecheck pass
- gates tree: f8688159f45e0b718392b5550a1ced89b8167dcc
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: contradicts (spec-side): Goal `## Wire` row "the README repeats no pin numbers" vs README `## Safety` "(pin 3)" — the clause sits in the `## Wire` row of a per-section table and was written about that section; undecidable: verify.md (c) reconnects via README `## Flash` while Plan 3(c) says phase 10 §3; README `## Build` cask `PATH` directory; `## If it does not work` `state=negotiating`/`digital` and the summary cadence; `## Development` `make typecheck` wording and the `make hil`/`make guitar` lines vs the `Makefile`; `## Wire` resistor counts vs `docs/wiring.md` (settled: 2 — `operator bench: verify.md §Check it yourself (a)–(e)` for the Clone Hero steps being bench-verified and for the whammy-before-first-move reading) (unstated: 5 — README intro, identity paragraph, browser whammy note, USB-C adapter, Clone Hero download; dropped "Why this exists"/"Hardware"; 02 verify and wiring.md edits per Deviations; PHASES.md row; verify.md troubleshooting table, `~/.zshrc` note, star power/pause checks)
- closure test: fail: five review undecidables are missing pointers
- findings: 6
- finding keys: §Goal README.md; §Plan docs/phases/11-release/verify.md; §Goal README.md; §Goal README.md; §Goal README.md; §Goal README.md
- spec size: 9975 (first)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-13 (round 2)
- criteria: 11 passed / 0 failed (criteria 1–7 and the three project gates re-run; the operator
  bench criterion unchanged, same firmware sources)
- project gates: test pass, lint pass, typecheck pass (re-run: the carry-over guard hit
  `.gitattributes` and `tests/vectors/*.h`)
- gates tree: f8688159f45e0b718392b5550a1ced89b8167dcc
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: undecidable: Goal `## Development` row "what each needs" over seven targets, the README states needs for three (settled: 2 — `operator bench: verify.md §Check it yourself (a)–(e)` for the whammy-before-first-move reading being taken and for the Clone Hero steps being bench-verified) (unstated: 10 — README intro, non-goals wording and identity, picotool and `~/.zshrc` notes, macOS whammy note, clonehero.net and 5-fret steps, `payload=`/`report=` and issue advice, Layout/Documentation under `## Development`, 02 verify and wiring.md hunks, PHASES.md row, verify.md (c) re-check and troubleshooting table)
- closure test: fail: the review's undecidable is a missing enumeration
- findings: 1
- finding keys: §Goal README.md
- spec size: 10395 (+420 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-13 (round 3)
- criteria: 11 passed / 0 failed (criteria 1–7 and the three project gates re-run; the operator
  bench criterion unchanged, same firmware sources)
- project gates: test pass, lint pass, typecheck pass (re-run: the carry-over guard hit
  `.gitattributes` and `tests/vectors/*.h`)
- gates tree: f8688159f45e0b718392b5550a1ced89b8167dcc
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: contradicts (spec-side): Goal `## What it is` row "one paragraph" vs README `## What it is`'s three paragraphs (what it does; supported and not; own USB identity and the Clone Hero mapping) — the Deviations entry on the non-goals already places them in that section (settled: 1 — `operator bench: verify.md §Check it yourself (a)–(e)` for the Clone Hero steps, calibration included, being bench-verified) (unstated: 12 — README tagline, `## Safety` rewording and pin 3, parts, picotool/`export`/`~/.zshrc`, eject note, macOS fallback and whammy note, Deck adapter and Desktop Mode, Clone Hero download and 5-fret, `## If it does not work` method and `state=` meanings, `## Development` targets/layout/table, 02 verify and wiring.md, PHASES.md row, verify.md extras)
- closure test: pass
- findings: 1
- finding keys: §Goal README.md
- spec size: 10395 (+0 since the previous validation)
- upstream: none
- not-ours: none
- verdict: escaped to /expand-phase: §Goal README.md

## Validation — 2026-10-13 (after re-expansion, round 1)
- criteria: 12 passed / 0 failed (criteria 1–8 and the three project gates run this round; the
  operator bench criterion from `## Bench readings`, same firmware sources)
- project gates: test pass, lint pass, typecheck pass (run: the carry-over guard hit
  `.gitattributes` and `tests/vectors/*.h`)
- gates tree: f8688159f45e0b718392b5550a1ced89b8167dcc
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: undecidable: `verify.md` §What was built ("The README's steps come from Clone Hero's own wiki, not from a bench run"; the whammy as an open question with a firmware fix) and (d)3 vs the Goal's "the bench" source and Out of scope's calibration line — Plan step 5 did not say whether `verify.md` follows the bench (settled: none) (unstated: 8 — README intro paragraph, "open an issue", PHASES.md pointer-table wording, dropped Why this exists/Hardware, verify.md troubleshooting strings, (c) re-check condition, (d)4 star power and pause, PHASES.md status)
- closure test: fail: the review's undecidable is a missing pointer
- findings: 1
- finding keys: §Plan docs/phases/11-release/verify.md
- spec size: 8541 (first after the re-expansion)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-13 (after re-expansion, round 2)
- criteria: 12 passed / 0 failed (criteria 1–8 and the three project gates run this round; the
  operator bench criterion from `## Bench readings`, same firmware sources)
- project gates: test pass, lint pass, typecheck pass (run: the carry-over guard hit
  `.gitattributes` and `tests/vectors/*.h`)
- gates tree: 4b004fee43b2188cc1c2667d351406c5ca83cd88
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: undecidable: `docs/wiring.md` §Sources keeps "If the breakout carries no numbering, stop and ask before wiring"; Plan step 2 did not say whether that stop survives naming by PS2 number (settled: none) (unstated: 14 — README blurb, `## What it is`, `## Safety` additions, `## Wire` parts and PS2 note, `## Build` installs/PATH/UF2 names, `## Flash` Mac-only and reconnect, both host sections, `## Clone Hero` steps, `## If it does not work` placeholders/states/"open an issue", `## Development` targets and layout, 02 verify definition, wiring.md §Socket pin 3, verify.md extras and error strings, PHASES.md row)
- closure test: fail: the review's undecidable is a missing pointer
- findings: 1
- finding keys: §Plan docs/wiring.md
- spec size: 8595 (+54 since the previous validation)
- upstream: none
- not-ours: none
- verdict: returned to spec

## Validation — 2026-10-13 (after re-expansion, round 3)
- criteria: 12 passed / 0 failed (criteria 1–8 and the three project gates run this round; the
  operator bench criterion from `## Bench readings`, same firmware sources)
- project gates: test pass, lint pass, typecheck pass (run: the carry-over guard hit
  `.gitattributes` and `tests/vectors/*.h`)
- gates tree: 4b004fee43b2188cc1c2667d351406c5ca83cd88
- boundary sweep: not swept: no file in the set is under a declared layer
- independent review: clean (settled: 1 — `operator bench: verify.md §Check it yourself (a)–(e)` for `verify.md` (a)'s "clone a new copy here" vs the README's "anywhere outside this repository") (unstated: 10 — README sections by pointer, picotool, Mac-only flash, Linux `.tar.xz` game download, "open an issue", `## Development` wording, 02 verify definition, wiring.md §Socket pin 3 and §Sources, verify.md extras and error strings, PHASES.md row)
- closure test: pass
- findings: 0
- finding keys: none
- spec size: 8595 (+0 since the previous validation)
- upstream: none
- not-ours: none
- verdict: done
