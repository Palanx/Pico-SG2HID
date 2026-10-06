# Phase 05-emulator — how to check this yourself

This page is for someone who does not write firmware. Part 1 is commands you run on the Mac. Part 2 is a bench with **two Picos and no guitar anywhere** (R-SAFETY-08).

One Pico is the master from the earlier phases. The second Pico pretends to be the guitar, and that is the point of this phase: every later phase can be tested against something that cannot be damaged.

## What was built

**The emulator is a second Pico that answers the master the way a wired Guitar Hero SG would. It can also be told to misbehave in specific ways.**

### Why it exists

The real guitar is irreplaceable (ADR-0006). Before any code meets it, that code has to have worked against something that behaves like the guitar but costs nothing to break (ADR-0004). The emulator is that something. It is also the only way to make a "guitar" fail on purpose: drop an answer, answer late, or claim to be a different controller. Phase `06-hil-digital` needs those failures to prove the master recovers from them.

### What it does on the bus

Recall the bus from phase 24: the master sends a byte on `CMD` while ticking `CLK`, and at the same time the controller sends a byte back on `DATA`. After each byte except the last, the controller pulls `ACK` low for a moment to say "got it, send the next one".

A digital poll is five bytes. Here is what each side sends:

| byte | master sends | emulator sends | emulator ACKs it? |
|---|---|---|---|
| 0 | `01` ("are you there?") | `FF` (nothing yet) | yes, because the master sent `01` |
| 1 | `42` ("send buttons") | `41` (digital controller), or `73` in analog mode | yes, because the master sent `42` |
| 2 | `00` | `5A` ("ready") | yes |
| 3 | `00` | buttons, first byte (`FF` = nothing pressed) | yes |
| 4 | `00` | buttons, second byte | **no**: the last byte never gets an ACK |

In analog mode the frame is nine bytes, with four more payload bytes for the sticks and the whammy bar.

If byte 0 is not `01`, the emulator stays silent, the same way a real controller ignores a frame that is not addressed to it. If byte 1 is not `42`, it has already sent its id on that byte (each answer goes out while the master's byte is still coming in), and it is silent from byte 2 on. In both cases the master gives up at that byte.

### Open-drain: why the emulator can never fight the master

The emulator *drives* two wires, `DATA` and `ACK`. A wire can be driven in two ways:

- **Push-pull**: the pin actively pushes the wire to 3.3 V or pulls it to 0 V.
- **Open-drain**: the pin can only pull the wire to 0 V or let go. When it lets go, a pull-up resistor (on the master breadboard) brings the wire back to 3.3 V.

The emulator uses open-drain only (ADR-0016), the same way a real controller does. Its pins are forced so that their output level is always 0 V; the program only switches each pin between "connected" (pulls low) and "disconnected" (lets go). So even if a wire were in the wrong place, the emulator could never push 3.3 V against a pin that is pulling to 0 V.

### Telling it what to do

The emulator has its own USB serial port. You type one command per line, and it answers every line with `ok: <your line>` or `error: <why>`.

| Command | What it changes | At power-on |
|---|---|---|
| `mode digital` / `mode analog` | the id byte (`41` / `73`) and the frame length (5 / 9 bytes) | digital |
| `payload FF FF 80 80 80 80` | the six payload bytes, in hex; digital mode sends the first two | `FF FF 80 80 80 80` |
| `fault none` | no fault | this one |
| `fault ack 2` | no ACK after byte 2; everything else as normal | — |
| `fault late 50` | every ACK comes 50 µs after its byte instead of 10 µs | — |
| `fault id 79` | byte 1 is `79` instead of the real id | — |

Only one fault is active at a time; a new `fault` line replaces the old one. A command takes effect from the next frame. It is only read between frames, while `ATT` is high.

Two numbers are **guesses, not measurements**: the ACK comes 10 µs after a byte and lasts 2 µs. Phase `09-guitar-observe` measures the real SG and replaces them.

### The files

| File | What it is |
|---|---|
| `src/core/pins.h` | Now has a second pin table, `kEmulatorPins`: same GPIO numbers, with DATA and ACK as open-drain outputs and the rest as inputs. |
| `src/emu/sg_model.cpp` | What to answer, byte by byte, and the command parser. Pure logic, so the Mac tests it. |
| `src/hal/ps2_device.pio` | The tiny program in the Pico's PIO block that moves bits on the wire. |
| `src/hal/pio_device.cpp` | Sets up the emulator's pins (only from `kEmulatorPins`) and runs that program. |
| `src/emu/main.cpp` | The emulator firmware: serves frames, and reads commands between them. |
| `docs/wiring-emulator.md` | How to wire the second Pico to the master's breadboard. |
| `docs/adr/0016-emulator-open-drain-outputs.md` | Why the outputs are open-drain. |

## Check it yourself

### 1. The Mac-side checks

From the repository root:

| Command | Expected |
|---|---|
| `python3 tests/test_emulator.py; echo rc=$?` | three `ok:` lines, then `rc=0` |
| `python3 tests/test_pin_table.py \| grep R-SAFETY-06` | one line starting `ok:   R-SAFETY-06` |
| `make firmware && ls build/pico/*.uf2` | `build/pico/sg2hid.uf2` **and** `build/pico/sg2hid_emu.uf2` |
| `make test \| tail -1` | `OK` |

What the lines of `test_emulator.py` mean:

- `ok:   R-EMU-01 …`: the emulator's logic was fed a poll and answered with exactly the bytes in the hand-written files under `tests/vectors/`. It did this in digital mode, analog mode, and with buttons "pressed". It also stayed silent for a frame that starts wrong.
- `ok:   R-EMU-02 …`: each fault changed only its own part of the answer. Nine bad command lines were each refused and changed nothing, including `fault ack 8`, `fault late 0` and a line that is too long.
- `ok:   rejection cases: 2/2`: the test broke the logic on purpose twice (an ACK on the last byte; the id fault ignored). Each break turned its line to `FAIL`, so the checks bite.

### 2. The bench: two Picos, no guitar

**Before anything: the guitar is unplugged and not on the bench.** Nothing in this phase touches it.

1. **Flash the master.** Hold its **BOOTSEL** button, plug its USB in, release, then run `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`. The master runs the same loopback firmware as phase 04. Remove both jumpers from phase 24/04 (CMD→DATA and ATT→ACK): the emulator replaces them.
2. **Flash the emulator.** Do the same with the second Pico: `cp build/pico/sg2hid_emu.uf2 /Volumes/RPI-RP2/`.
3. **Find the two serial ports.** With only the master plugged in, run `ls /dev/cu.usbmodem*`: that name is the master. Plug the emulator in and run it again: the new name is the emulator. Write them down:

   ```
   MASTER=/dev/cu.usbmodem…   # the first one
   EMU=/dev/cu.usbmodem…      # the new one
   ```

4. **Wire them**, following `docs/wiring-emulator.md`. Both Picos are already on USB, which is the order that page asks for. That means five signal wires plus GND, and **no power wire between the Picos**.
5. **Open the emulator's port** in a second Terminal window, and leave it running. Replies to your commands appear here:

   ```
   cat $EMU
   ```

   In the first window, send a command and check that the second window shows `ok: fault none`:

   ```
   printf 'fault none\n' > $EMU
   ```

6. **Read the master.** For each row of the table below, send the command, then capture about two seconds of the master and decode it:

   ```
   printf 'mode digital\n' > $EMU      # the row's command
   head -n 15 $MASTER > build/emu.log
   python3 tools/trace_decode.py build/emu.log
   ```

   Every second, the master runs three frames (seq 0, 1, 2) and prints a `loopback:` line, followed by its `T1` trace line, for each one. The decoder turns the `T1` lines into one block per frame.

   | Command (send `fault none` before each fault row) | seq 0: a poll, `01 42 00 00 00` | seq 1: starts `FF` | seq 2: one byte, `80` |
   |---|---|---|---|
   | `mode digital` (the defaults) | `bytes=5/5`; decoded `in` `FF 41 5A FF FF` | `bytes=0/9 att=high`: stops at byte 0 | `bytes=1/1`; `in` `FF` |
   | `mode analog` | `bytes=5/5`; `in` `FF 73 5A FF FF` | as above | as above |
   | `fault ack 2` | stops at byte 2, `att=high` | as above | as above |
   | `fault late 200` | stops at byte 0, `att=high` | as above | as above |
   | `fault late 50` | `bytes=5/5`; decoded `ack` delays near 50 µs | as above | as above |
   | `fault id 79` | `bytes=5/5`; second `in` byte is `79` | as above | as above |
   | `fault none`, then `mode digital` | the first row again | as above | as above |

   How to read the rows:
   - **Defaults.** The master asked "are you there? send buttons" and got "digital controller, ready, nothing pressed". Each `ack` should read around 10 µs: the emulator's chosen delay, plus a µs or two of measurement overhead.
   - **seq 1** always stops at byte 0. It starts with `FF`, not `01`, so the emulator is not being addressed and stays silent, exactly like a real controller would. That is correct, not a fault.
   - **seq 2** is a single byte, and a single byte is also the last byte, so no ACK is needed. `in` is `FF` because the emulator sends nothing on byte 0.
   - **`fault late 200`.** The master waits at most 100 µs for an ACK, and 200 µs is past that, so it gives up at byte 0. Compare **`fault late 50`**, which is still inside the budget.
   - **`mode analog`, seq 0.** The master only sends five bytes, so it reads the first five of the nine the emulator would send. That is expected: the master does not ask for analog until phase `07-analog-mode`.

7. **Record each row's actual reading** in `docs/phases/05-emulator/notes.md` under `## Bench readings`: what you saw for seq 0, 1 and 2, or paste the decoded block. If a row differs from the table, write down what you saw instead. That is what the next session needs.

8. **To finish:** pull the signal wires first, then unplug both USB cables.

### If something looks wrong

- **Every frame aborts at byte 0, whatever the command.** The emulator is not seeing the master, or the master is not seeing the emulator's ACK. Check that GND is connected between the two boards, and that each wire is in the socket-side row of the right resistor.
- **`in` bytes look shifted or garbled** (for example `FE` or `7F` where `FF` was expected). A CLK or CMD wire is probably on the wrong row. Unplug the signal wires and recheck them against `docs/wiring-emulator.md`.
- **No `ok:` replies in the emulator window.** The `cat` is reading the wrong port. Re-run `ls /dev/cu.usbmodem*` and check which name is which.
