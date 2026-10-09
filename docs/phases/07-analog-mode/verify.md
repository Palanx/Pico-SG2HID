# Phase 07-analog-mode — how to check this yourself

This page is for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 is the same two-Pico bench as phase 06, **with no guitar anywhere** (R-SAFETY-08).

## What was built

**The master Pico now asks the controller to switch to analog mode before it reads anything, and from then on reads the whammy bar as a number from `00` to `FF`.** In phase 06 the master only ever read digital frames, which carry the buttons but no whammy at all.

### Why analog mode has to be asked for

A PS2 controller starts in **digital mode**. It answers a poll with an id byte, `41` ("digital"), the ready byte `5A`, then two bytes of buttons. Nothing else is in that frame, so there is no whammy to read.

In **analog mode** the same poll is answered with id `73` and six bytes after `5A`: the same two button bytes, then four axis bytes. On a Guitar Hero SG the whammy is one of those four (the last one, as far as the documentation says; phase 09 checks this against the real guitar). The project's rule R-PROTO-04 says the whammy is read only from an analog frame, so the master has to get the controller into analog mode first.

### What config mode is

A controller does not switch modes because it is polled differently. It has to be put into **config mode** first, a special state where it accepts setting commands. While in config mode it answers every frame with id `F3` and six `00` bytes, which means "I am listening, I am not reporting buttons".

### The four frames

The master sends these, one per millisecond, in this order (all bytes in hex):

| step | the master sends | what it means | the answer it expects |
|---|---|---|---|
| 1. enter config | `01 43 00 01 00` | "go into config mode" | any good answer: `41` or `73` or `F3`, then `5A` |
| 2. set analog | `01 44 00 01 03 00 00 00 00` | "switch to analog, and lock it" | `F3`, `5A`: still in config mode |
| 3. leave config | `01 43 00 00 00 00 00 00 00` | "leave config mode" | `F3`, `5A` |
| 4. poll | `01 42 00 00 00 00 00 00 00` | "send me your buttons and axes" | `73`, `5A`, then six bytes |

After step 4 the master keeps sending step 4 every millisecond. "Lock it" (`03`) stops the ANALOG button on a controller from switching it back to digital.

The first frame is only five bytes. A controller answers it in whatever mode it is in, and a digital answer is five bytes long, so the master does not wait for more. It only checks the first three bytes it gets back: a known id, then `5A`.

If any answer is wrong, the master starts again from step 1 on the next frame. Two kinds of wrong are possible:

- **Refused**, as in phase 06: no answer (`ack-timeout`), or an id this project does not know (`unknown-id`).
- **Declined**, new here: a perfectly good answer, but with the wrong id for the step. For example, the controller still answers `41` ("digital") after it was told to switch to analog. The fault is then `declined`.

The master never settles for digital mode. A guitar without its whammy is not the device this project builds (ADR-0017).

### What the master prints

When the link changes, the master prints a `link:` line. On a normal start against the emulator you see two:

```
link: absent -> negotiating fault=none
link: negotiating -> analog fault=none
```

`negotiating` means "the controller answered in config mode". `analog` means "good analog frames are arriving".

Once a second the master prints a summary. The summary from phase 06 gains a last field, `whammy=`:

```
hil: polls=60000 refused=0 changes=0 us=1000012 state=analog fault=none att=high payload=7F FE whammy=80
```

- `whammy=80` is the whammy of the last good analog frame, in hex. `80` is the middle of the range, `00` and `FF` the two ends. Before any analog frame it shows `--`.
- `refused` now counts every frame after which the link is `absent`, declined ones included.
- `changes` counts analog frames whose six data bytes differ from the previous good analog frame. Moving the whammy is a change.
- `payload=` is still the first two data bytes (the buttons), now of the last good analog frame.

### The emulator

The emulator (`build/pico/sg2hid_emu.uf2`) now understands the same four frames. It starts digital, as a real controller does. It goes into config mode, switches to analog and leaves config mode when the master asks. Like a real controller, it applies each command at the start of the next frame, not mid-frame.

It has one new command, `fault decline`. With it, the emulator ignores "switch to analog" and stays digital. This is how the harness checks that the master really refuses a controller that will not give it a whammy.

### The harness

`make hil` runs `tools/hil_digital.py`. It keeps its name from phase 06, but it now expects `state=analog` and `whammy=80` wherever phase 06 expected `state=digital`. It also gains two scenarios: `sweep`, which moves the whammy through five values, and `fault decline`.

**Phase 06's `verify.md` no longer matches this firmware.** Its expected lines show `state=digital`. Once the master runs this phase's `sg2hid.uf2`, you see `state=analog` instead. That page is left as it was, as the record of phase 06.

### Test-suite housekeeping

This phase also paid three items from the project's debt log. None changes what the Picos do:

- Five test scripts now ask the `Makefile` for the compiler flags instead of keeping their own copy.
- Those five scripts remember which of their deliberate-bug checks already passed. When nothing changed, `make test` skips them and prints `, <n> served from cache`. `make test FULL=1` runs them all again.
- Two edge cases of the decoder and the link state machine that had no test now have one.

## Check it yourself

### 1. On the Mac

| Command | What you should see |
|---|---|
| `make test 2>&1 \| tail -n 1` | `OK` (the first run takes several minutes) |
| `make test 2>&1 \| grep 'served from cache'` | five `rejection cases` lines, each ending `, <n> served from cache`, where `<n>` equals the number after the slash. The test-suite cache adds more `cache:` lines; ignore those. |
| `make firmware && ls build/pico/*.uf2` | three files: `sg2hid.uf2`, `sg2hid_emu.uf2` and `sg2hid_loopback.uf2` |
| `python3 tests/test_ps2_codec.py \| grep R-PROTO-10` | `ok:   R-PROTO-10 (the master negotiates analog mode and judges each answer)` |
| `python3 tests/test_emulator.py \| grep rejection` | `ok:   rejection cases: 4/4 …` |

### 2. The bench: two Picos, no guitar

**Before anything: the guitar is unplugged and not on the bench.**

Follow phase 06's bench procedure (`docs/phases/06-hil-digital/verify.md`, §2) steps 1 to 6 unchanged. It covers taking the old wiring apart, flashing `sg2hid.uf2` to the master and `sg2hid_emu.uf2` to the emulator, finding the two ports, checking that the emulator answers, and wiring the bench. Flash the files built by **this** phase's `make firmware`.

7. **Run it:**

   ```
   make hil MASTER=$MASTER EMU=$EMU
   ```

   It takes about two and a half minutes: about 2 seconds for `setup`, 62 for `sustained`, 15 for `sweep`, and 8 for each fault scenario. Expected output:

   ```
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

   The scenarios:

   | scenario | emulator lines sent | passes when |
   |---|---|---|
   | `setup` | `fault none`, `mode digital`, `payload 7f fe 80 80 80 80` | each line is answered `ok: <line>` within 2 s, and a master `hil:` line arrives |
   | `sustained` | none | over `--seconds` (default 60) summaries: every one has `state=analog`, `payload=7F FE` and `whammy=80`; Δrefused = 0 and Δchanges = 0 |
   | `sweep` | `payload 7f fe 80 80 80 <v>` for `<v>` = `00`, `40`, `80`, `c0`, `ff`, one at a time | after each line, the next judged summary has `state=analog` and `whammy=<v>` in capitals; Δrefused = 0 |
   | `fault ack 0`, `fault ack 3`, `fault late 200` | the fault line | over 2 summaries: `state=absent`, `fault=ack-timeout`, Δrefused = Δpolls |
   | `fault id 79` | `fault id 79` | over 2 summaries: `state=absent`, `fault=unknown-id`, Δrefused = Δpolls |
   | `fault late 50` | `fault late 50` | over 2 summaries: `state=analog`, Δrefused = 0 |
   | `fault decline` | `fault decline`, then `mode digital` | over 2 summaries: none has `state=analog`, every one has `fault=declined`, and Δrefused is at least 1 |

   Δ means how much a counter grew over the judged summaries. In plain words:

   - `sweep` is the end-to-end whammy test. The harness changes the byte the emulator sends, and the master's `whammy=` must show the same value. If the master read the wrong byte, it would stay at `80`.
   - `fault decline` first puts the emulator back in digital mode (`mode digital`), because by then it is already analog and the fault only blocks the *next* "switch to analog". The master then goes round the four steps again, fails at step 4 each time, and never shows `analog`. Between failures the link briefly shows `negotiating`, which is expected.

   After every fault the harness sends `fault none` and checks **recovery**: two summaries or more are skipped, then for the next two the link must be `analog`, reading `7F FE` with `whammy=80`, with no refused frame and no data change. In `fault decline`'s case, recovery means the master's next round of the four steps succeeds.

8. **Record the output** in `docs/phases/07-analog-mode/notes.md` under `## Bench readings`.
9. **To finish:** signal wires out first, then unplug both USB cables.

### 3. Watching one negotiation (optional)

1. Window 1: `python3 tools/trace_decode.py $MASTER`.
2. Window 2: `printf 'mode digital\n' > $EMU`.
3. Window 1 should show the master losing analog mode and getting it back, a `link:` line each time with that frame's bytes under it. These are the expected lines, not a bench recording:

   ```
   link: analog -> absent fault=declined
   link: absent -> negotiating fault=declined
   link: negotiating -> analog fault=declined
   ```

   The first frame was answered `41` while the master expected `73`, so it was declined. The next round of the four steps put the emulator back in analog. `fault=` keeps naming the last reason the link dropped, even after it recovers.

### If something looks wrong

- **`FAIL: setup: emulator did not answer 'fault none'`.** `EMU` is the wrong port.
- **`FAIL: sustained: state=absent payload=-- whammy=--`.** Either a wiring mistake (phase 06's `verify.md`, §If something looks wrong, gives the loopback check) or an emulator flashed before this phase: an old emulator breaks every frame that starts `01 43`, so the master never gets past step 1.
- **`FAIL: sustained: state=digital …`.** The master still runs phase 06's `sg2hid.uf2`. Reflash it.
- **`FAIL: sweep: state=analog whammy=80, expected analog/00`.** The master reads the whammy from the wrong byte. Record the output in `notes.md`.
- **Anything else.** Record the whole output in `notes.md` and do not change the timeouts.
