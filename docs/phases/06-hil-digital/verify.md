# Phase 06-hil-digital — how to check this yourself

This page is for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 is the same two-Pico bench as phase 05, **with no guitar anywhere** (R-SAFETY-08).

## What was built

**The master Pico stops being a loopback tester and becomes what the finished device will be at its core: a loop that asks the controller for its buttons a thousand times a second, and keeps going whatever the answer.** A script on the Mac then drives both Picos and checks, automatically, that the master reads the emulator correctly and survives every fault the emulator can fake.

### The poller (`build/pico/sg2hid.uf2`)

Every millisecond the master sends one digital poll, the five bytes `01 42 00 00 00` you met in phases 24 and 05. What comes back is checked by the same decoder the host tests check (`src/core/`): either it is a proper frame ("digital controller, ready, these buttons"), or it is **refused**, with a reason.

Each answer moves the **link**, the master's opinion of what is on the other end of the cable:

- `digital`: a controller is answering with good digital frames.
- `absent`: the last poll was refused. The reason is kept as the **fault**: `ack-timeout` (the controller stopped acknowledging bytes, or acknowledged too late) or `unknown-id` (it answered as a kind of controller this project does not support).

A refused poll never stops the master. The next poll goes out one millisecond later anyway, and as soon as a good frame arrives the link is `digital` again. That is the "the firmware never stops" rule (`docs/constraints.md` §Error handling): a USB gamepad that silently freezes is worse than one that briefly reports nothing pressed.

The master prints two kinds of line on its USB serial port:

- Once a second (every 1000 polls), a **summary**:

  ```
  hil: polls=60000 refused=0 changes=0 us=1000123 state=digital fault=none att=high payload=7F FE
  ```

  `polls`, `refused` and `changes` count since the master booted. `us` is how long the last 1000 polls took, so about 1 000 000 (one second). `att=high` means the master let go of the controller at the end of the poll, as it must (R-SAFETY-07). `payload` is the two button bytes of the last good frame.
- Whenever the link changes, a **link line**, followed by that poll's `T1` trace line from phase 04:

  ```
  link: digital -> absent fault=ack-timeout
  T1 n=5 k=3 out=01,42,00,00,00 in=FF,41,5A,--,-- us=47,47,47,133,-
  ```

### What a desync is

A **desync** is the master misreading a controller that is answering correctly: either a refused poll, or the buttons changing when nobody pressed anything (`changes` counts every time the button bytes differ from the previous good frame). Against an emulator told to answer normally, both must stay at exactly zero. One wrong bit in a whole minute of polling would show up here.

### The harness (`tools/hil_digital.py`, run by `make hil`)

The script talks to both Picos at once. It tells the emulator what to do (the `mode`, `payload` and `fault` commands from phase 05), then reads the master's summaries and judges them. It runs seven **scenarios** in order, prints `ok: <scenario>` for each that passes, and stops at the first `FAIL: <scenario>: <reason>`. Whatever happens, it tells the emulator `fault none` before it exits, so the bench is left answering normally.

It ignores the first summary after each command, because that summary's second was partly before the command and partly after. It judges the ones after it.

### The old loopback program

It still exists, as `build/pico/sg2hid_loopback.uf2`, because its bit-order probe is the only way to check R-PROTO-01 by hand. Phases 24, 04 and 05's instructions now flash that file instead of `sg2hid.uf2`.

## Check it yourself

### 1. On the Mac

| Command | What you should see |
|---|---|
| `make test 2>&1 \| tail -n 1` | `OK` |
| `make firmware && ls build/pico/*.uf2` | three files: `sg2hid.uf2`, `sg2hid_emu.uf2` and `sg2hid_loopback.uf2` |
| `make build/host/test_poll && build/host/test_poll` | `test_poll: ok` |
| `make hil` | `usage: make hil MASTER=<master port> EMU=<emulator port>` and an error: without the two ports it refuses to run |

`test_poll` feeds the new poll decoder hand-written answers from `tests/vectors/`: a good idle frame, the same frame cut short after 0, 1, 3 and 4 bytes (each must be refused as `AckTimeout`), and a frame with an unsupported id. It also checks the counters: an idle frame twice is not a change, a refused poll does not reset the comparison, and a pressed button is one change.

### 2. The bench: two Picos, no guitar

**Before anything: the guitar is unplugged and not on the bench.**

1. **Signal wires out.** If the bench from phase 05 is still wired, pull the five signal wires and the GND wire between the boards first (`docs/wiring-emulator.md`, "To take the bench apart").
2. **Flash the master** with the poller. Hold its **BOOTSEL** button, plug its USB in, release, then `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`.
3. **Flash the emulator** the same way with `cp build/pico/sg2hid_emu.uf2 /Volumes/RPI-RP2/`. If it already runs the phase-05 emulator, you can skip this: the file has not changed.
4. **Find the ports.** With only the master plugged in, `ls /dev/cu.usbmodem*` shows the master's name. Plug in the emulator and run it again; the new name is the emulator's. On the bench this was written on they were:

   ```
   MASTER=/dev/cu.usbmodem101
   EMU=/dev/cu.usbmodem2101
   ```

5. **Check the emulator answers** before wiring. In a second Terminal window run `cat $EMU`, then in the first: `printf 'fault none\n' > $EMU`. The second window must show `ok: fault none`. Close the `cat` with Ctrl-C: the harness needs the port to itself.
6. **Wire the bench** per `docs/wiring-emulator.md`. Both Picos are already on USB, which is the order that page asks for: power first, signal wires second.
7. **Run it:**

   ```
   make hil MASTER=$MASTER EMU=$EMU
   ```

   It takes about a minute and a half. Expected output, one line per scenario:

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

   What each scenario shows, in plain words:

   | scenario | what the emulator is told | what the master must do |
   |---|---|---|
   | `setup` | answer normally, digital mode, buttons `7F FE` | print a summary within 3 seconds |
   | `sustained` | nothing new | 60 seconds (60 000 polls) of `state=digital` and `payload=7F FE`, with **zero** refused polls and **zero** button changes: no desync |
   | `fault ack 0` | never acknowledge byte 0 | every poll refused, link `absent`, fault `ack-timeout` |
   | `fault ack 3` | stop acknowledging at byte 3, half way through the buttons | the same: a frame cut short is refused whole, never half-read (R-PROTO-02) |
   | `fault late 200` | acknowledge every byte, but 200 µs late | the same: the master waits at most 100 µs (`kAckTimeoutUs`), so a late answer counts as no answer |
   | `fault id 79` | answer as a DualShock 2 (id `79`) | every poll refused, link `absent`, fault `unknown-id`: an unsupported controller is refused, never guessed at (R-PROTO-03) |
   | `fault late 50` | acknowledge every byte 50 µs late | keep streaming with no refused poll: 50 µs is inside the 100 µs budget |

   After every fault the harness sends `fault none` and checks **recovery**: it skips one summary (about one second, time for the link to come back), then for the next two summaries (about two seconds more) the link must be `digital`, reading `7F FE`, with no refused poll and no button change. A fault scenario prints `ok:` only if both the fault and the recovery behaved.

   Why `7F FE` and not the default `FF FF`: `7F` is `01111111` and `FE` is `11111110`. A master reading bits in the wrong order would turn each into the other and show `FE 7F`; one sampling on the wrong clock edge (R-PROTO-07) would be a bit off and show something else again. `FF FF` reads `FF FF` either way, so it would hide both mistakes.

8. **Record the output** in `docs/phases/06-hil-digital/notes.md` under `## Bench readings`.
9. **To finish:** signal wires out first, then unplug both USB cables.

### 3. Reading a link line yourself (optional)

The harness only reads summaries. To see what one failing poll looked like on the wire:

1. Window 1: `python3 tools/trace_decode.py $MASTER`. Summary lines scroll past once a second.
2. Window 2: `printf 'fault ack 3\n' > $EMU`, wait two seconds, then `printf 'fault none\n' > $EMU`.
3. Window 1 shows `link: digital -> absent fault=ack-timeout`, then a decoded block for that poll: bytes 0–2 completed, byte 3 waited for an ACK that never came (its `us` is about 135: the 37 µs of shifting plus the 100 µs wait), byte 4 was never sent. Then, after `fault none`, `link: absent -> digital fault=ack-timeout` and a block where all five bytes completed. `fault=` on that line is why the link *last* dropped; it is kept on purpose so the reason survives recovery.

### If something looks wrong

- **`FAIL: setup: emulator did not answer 'fault none'`.** `EMU` is the wrong port, or another program (a `cat` left running) has it open.
- **`FAIL: setup: master silent`.** `MASTER` is the wrong port, or the master still runs the loopback firmware: flash `sg2hid.uf2` again.
- **`FAIL: sustained: state=absent payload=--`.** The master never got one good frame: every poll stops at byte 0, while the emulator still answers commands. This is almost always the wiring, usually GND or one signal wire in the wrong row. To be sure, flash the master with `sg2hid_loopback.uf2`. If its `seq=0` line also shows `bytes=0/5`, it is the wiring. Fix it until `seq=0` shows `bytes=5/5`, then flash `sg2hid.uf2` again. This happened once while this phase was built.
- **`FAIL: sustained: Δrefused=…`** with a small number. The master misread some polls. Check the GND wire first, then each signal wire against `docs/wiring-emulator.md`. Record the number in `notes.md` either way.
- **`FAIL: sustained: state=digital payload=FE FD`** or similar shifted bytes. A wire is on the wrong row, or the bit order is wrong. Unwire and recheck.
- **`FAIL: fault late 50: …`.** The ACK budget is tighter than the emulator's 50 µs delay. Record the output; do not change the timeout here (that is phase 09's measurement).
