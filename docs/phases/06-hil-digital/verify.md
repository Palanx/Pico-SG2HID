# Phase 06-hil-digital — how to check this yourself

This page is for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 is the same two-Pico bench as phase 05, **with no guitar anywhere** (R-SAFETY-08).

## What was built

**The master Pico stops being a loopback tester and becomes what the finished device will be at its core: a loop that asks the controller for its buttons a thousand times a second, and keeps going whatever the answer.** A script on the Mac then drives both Picos and checks, automatically, that the master reads the emulator correctly and recovers from each fault it injects.

### The poller (`build/pico/sg2hid.uf2`)

Every millisecond the master sends one digital poll, the five bytes `01 42 00 00 00`. What comes back is checked by the same decoder the host tests check (`src/core/`): either it is a proper frame ("digital controller, ready, these buttons"), or it is **refused**, with a reason.

Each answer moves the **link**, the master's opinion of what is on the other end of the cable:

- `digital`: a controller is answering with good digital frames.
- `absent`: the last poll was refused. The reason is kept as the **fault**: `ack-timeout` (the controller stopped acknowledging bytes, or acknowledged too late) or `unknown-id` (it answered as a kind of controller this project does not support).

A refused poll never stops the master. The next poll goes out one millisecond later anyway, and as soon as a good frame arrives the link is `digital` again. As `docs/constraints.md` §Error handling puts it: "A hang is a worse failure than a wrong report."

The master prints two kinds of line on its USB serial port:

- Once a second (every 1000 polls), a **summary**:

  ```
  hil: polls=108000 refused=14212 changes=0 us=999953 state=digital fault=ack-timeout att=high payload=7F FE
  ```

  (A real line from the bench run: see `## Bench readings` in `notes.md`.) `polls`, `refused` and `changes` count since the master booted. `us` is how long the last 1000 polls took, so about 1 000 000 (one second). `att=high` means the master let go of the controller at the end of the poll, as it must (R-SAFETY-07). `payload` is the two button bytes of the last good frame.
- Whenever the link changes, a **link line**, followed by that poll's `T1` trace line (ADR-0015):

  ```
  link: digital -> absent fault=ack-timeout
  ```

### What a desync is

A **desync** is the master misreading a controller that is answering correctly: either a refused poll, or the buttons changing when nobody pressed anything (`changes` counts every time the button bytes differ from the previous good frame). Against an emulator told to answer normally, both must stay at exactly zero.

### The harness (`tools/hil_digital.py`, run by `make hil`)

The script talks to both Picos at once. It tells the emulator what to do (the `mode`, `payload` and `fault` commands from phase 05), then reads the master's summaries and judges them. It runs seven **scenarios** in order, prints one line per scenario, and stops at the first one that fails.

After each command it skips two summaries, sometimes more, then judges the ones after them. Why two: the master and the emulator are two separate serial ports, so the script can only put their lines in the order it reads them, not the order they were printed. The first summary it reads after the emulator's `ok:` may have been printed before the command took effect. The second was printed about a second later (1000 polls of 1 ms), so it comes after the command. Every poll the script then judges ran under the new command.

### The old loopback program

It still exists, as `build/pico/sg2hid_loopback.uf2`, because its bit-order probe is how R-PROTO-01 is checked by hand. Phases 24, 04 and 05's instructions now flash that file instead of `sg2hid.uf2`.

## Check it yourself

### 1. On the Mac

| Command | What you should see |
|---|---|
| `make test 2>&1 \| tail -n 1` | `OK` |
| `make firmware && ls build/pico/*.uf2` | three files: `sg2hid.uf2`, `sg2hid_emu.uf2` and `sg2hid_loopback.uf2` |
| `make build/host/test_poll && build/host/test_poll` | `test_poll: ok` |
| `make hil` | `usage: make hil MASTER=<master port> EMU=<emulator port>` and an error: without the two ports it refuses to run |

`test_poll` feeds the new poll decoder hand-written answers from `tests/vectors/`: a good idle frame, the same frame cut short (each cut must be refused as `AckTimeout`), and a frame with an unsupported id. It also checks the counters: an idle frame twice is not a change, a refused poll does not reset the comparison, and a pressed button is one change.

### 2. The bench: two Picos, no guitar

**Before anything: the guitar is unplugged and not on the bench.**

1. **Signal wires out.** If the bench from phase 05 is still wired, take it apart first: signal wires out, then USB (`docs/wiring-emulator.md`, "To take the bench apart").
2. **Flash the master** with the poller. Hold its **BOOTSEL** button, plug its USB in, release, then `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`. Wait for the volume to disappear, then unplug it.
3. **Flash the emulator** the same way with `cp build/pico/sg2hid_emu.uf2 /Volumes/RPI-RP2/`, then unplug it.
4. **Find the ports, one board at a time.** Plug in only the master (no BOOTSEL this time) and run `ls /dev/cu.usbmodem*`: the name shown is the master's. Plug in the emulator and run it again; the new name is the emulator's. Write them down as two lines like these, which are the names on the bench this was written on:

   ```
   MASTER=/dev/cu.usbmodem101
   EMU=/dev/cu.usbmodem2101
   ```

   Set both in every Terminal window you use: paste the two lines, with your own names, into each window before anything else. The commands below read `$MASTER` and `$EMU`, and a window where they are not set passes an empty name.

5. **Check the emulator answers** before wiring, the same way as phase 05 (`docs/phases/05-emulator/verify.md`, §2 step 5). In a second Terminal window run `cat $EMU`, then in the first: `printf 'fault none\n' > $EMU`. The second window must show `ok: fault none`. Then close the `cat` with Ctrl-C.
6. **Wire the bench** per `docs/wiring-emulator.md`. Both Picos are already on USB, which is the order that page asks for: power first, signal wires second.
7. **Run it:**

   ```
   make hil MASTER=$MASTER EMU=$EMU
   ```

   It takes under two minutes: about 2 seconds for `setup`, 62 for `sustained` and 8 for each fault scenario. Expected output, one line per scenario:

   ```
   ok: setup
   ok: sustained
   ok: fault ack 0
   ok: fault ack 3
   ok: fault late 200
   ok: fault id 79
   ok: fault late 50
   hil: PASS
   ```

   The scenarios, as the spec states them:

   | scenario | emulator lines sent | passes when |
   |---|---|---|
   | `setup` | `fault none`, `mode digital`, `payload 7f fe 80 80 80 80` | each line is answered `ok: <line>` within 2 s, and a master `hil:` line arrives |
   | `sustained` | none | over a window lasting `--seconds` (default 60) summaries: every summary has `state=digital` and `payload=7F FE`, Δrefused = 0 and Δchanges = 0 |
   | `fault ack 0` | `fault ack 0` | over a 2-summary window: `state=absent`, `fault=ack-timeout`, Δrefused = Δpolls |
   | `fault ack 3` | `fault ack 3` | as `fault ack 0` |
   | `fault late 200` | `fault late 200` | as `fault ack 0` |
   | `fault id 79` | `fault id 79` | over a 2-summary window: `state=absent`, `fault=unknown-id`, Δrefused = Δpolls |
   | `fault late 50` | `fault late 50` | over a 2-summary window: `state=digital`, Δrefused = 0 |

   Δ means how much a counter grew over the judged summaries. In plain words:

   - `sustained` is the no-desync test: a minute of polling with zero refused polls and zero button changes.
   - `fault ack 3` stops acknowledging at byte 3, the first button byte. A frame cut short is refused whole, never half-read (R-PROTO-02).
   - The master waits at most 100 µs for an `ACK` (`kAckTimeoutUs`), so 200 µs late counts as no answer and 50 µs late does not.
   - Id `79` is a DualShock 2. An unsupported controller is refused, never guessed at (R-PROTO-03).

   After every fault the harness sends `fault none` and checks **recovery**: it skips two summaries or more (about two seconds, for the reason given above), then for the next two summaries (about two seconds more) the link must be `digital`, reading `7F FE`, with no refused poll and no button change. A fault scenario prints `ok:` only if both the fault and the recovery behaved.

   Why `7F FE` and not the default `FF FF`: `7F` is `01111111` and `FE` is `11111110`. A master reading bits in the wrong order would turn each into the other and show `FE 7F`. `FF FF` reads `FF FF` either way, so it would hide that mistake.

8. **Record the output** in `docs/phases/06-hil-digital/notes.md` under `## Bench readings`.
9. **To finish:** signal wires out first, then unplug both USB cables.

### 3. Reading a link line yourself (optional)

Of the master's lines, the harness reads only the summaries. To see what one failing poll looked like on the wire:

1. Window 1: `python3 tools/trace_decode.py $MASTER`. Summary lines scroll past once a second.
2. Window 2: `printf 'fault ack 3\n' > $EMU`, wait two seconds, then `printf 'fault none\n' > $EMU`.
3. Window 1 shows these lines, with summary lines in between (bench recording, `notes.md` §Bench readings):

   ```
   link: digital -> absent fault=ack-timeout
   frame: 3/5 bytes, aborted at byte 3: no ACK after 133 us
     byte 0: out 01 in FF 48 us ack 11 us
     byte 1: out 42 in 41 47 us ack 10 us
     byte 2: out 00 in 5A 47 us ack 10 us
     byte 3: out 00 in -- 133 us
     byte 4: out 00 in -- not sent
   link: absent -> digital fault=ack-timeout
   frame: 5/5 bytes, complete
     byte 0: out 01 in FF 47 us ack 10 us
     byte 1: out 42 in 41 46 us ack 9 us
     byte 2: out 00 in 5A 46 us ack 9 us
     byte 3: out 00 in 7F 46 us ack 9 us
     byte 4: out 00 in FE 36 us
   ```

   The 133 us of the aborted byte is 37 µs to shift the byte out (`SHIFT_US` in `tools/trace_decode.py`) plus the 100 µs the master waits for an ACK (`kAckTimeoutUs`). `fault=` is kept when the link recovers, so on the second `link:` line it names why the link last dropped.

### If something looks wrong

- **`FAIL: setup: emulator did not answer 'fault none'`.** `EMU` is the wrong port.
- **`FAIL: setup: master silent`.** `MASTER` is the wrong port, or the master still runs the loopback firmware, which prints `loopback:` lines, not `hil:`.
- **`FAIL: sustained: state=absent payload=--`.** A wiring mistake looks exactly like this. To isolate it, flash the master with `sg2hid_loopback.uf2` and read its `seq=0` line: `bytes=0/5` points to the wiring, `bytes=5/5` does not. This happened once while this phase was built.
- **`FAIL: sustained: state=<state> payload=<payload>`** (any other than `state=absent payload=--`) or **`FAIL: sustained: Δrefused=<n> Δchanges=<n>`**. Recheck every wire against `docs/wiring-emulator.md`, run the loopback check above, and record the output in `notes.md`.
- **`FAIL: fault late 50: …`.** Record the output, and do not change the timeout.
