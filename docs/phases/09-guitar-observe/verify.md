# Phase 09-guitar-observe — how to check this yourself

This page is for someone who does not write firmware. It is the first time the real guitar meets the bus, so it is longer than the earlier ones, and the order of its parts is a safety rule, not a suggestion. Part 1 is commands on the Mac. Part 2 proves the new firmware on the two-Pico bench, with **no guitar anywhere**. Part 3 re-checks the gamepad firmware on the same bench. Part 4 is first contact with the guitar.

## What was built

### A third firmware for the master Pico: the observe instrument

`build/pico/sg2hid_observe.uf2` is a measuring instrument, not the product. It does three things:

- It talks to whatever is plugged into the socket, one frame every millisecond, exactly as the gamepad firmware does.
- It prints what it sent and what came back on its USB serial port, as `T1` lines, the trace format from phase 04.
- It does **not** pretend to be a gamepad. The Mac sees only a serial port.

It has two modes.

| Mode | When | What it sends | Why |
|---|---|---|---|
| **digital** | from power-up | only `01 42 00 00 00`: "give me your buttons" | This is the **read-only** mode. The guitar is never asked to change anything, so it stays exactly as it wakes up. |
| **analog** | after you type `analog` into its serial port | the same three-step sequence the gamepad firmware uses to switch the guitar to analog mode, then analog reads | This is how we learn whether the switch works on the real guitar, and what the whammy really sends. |

There is no way back to digital mode except unplugging the USB, which powers the guitar off and on.

The firmware **waits until something opens its serial port** before it sends its first frame. A USB serial port throws away what is printed before the Mac is listening, so without the wait the very first line, the one that says "the guitar answered", would be lost before you could start recording.

### What it prints

Two kinds of line, plus answers to what you type.

```
link: absent -> digital fault=none
T1 n=5 k=5 out=01,42,00,00,00 in=FF,41,5A,FF,FF us=38,39,39,39,37
```

(These are the expected shape, not a recording.)

- A **`link:` line** appears when the master's opinion of the controller changes: `absent` (nothing answers), `negotiating` (in the middle of the switch to analog), `digital` or `analog` (answering in that mode). `fault=` says why it last dropped to `absent`.
- A **`T1` line** is one frame. `n=` is how many bytes were exchanged, `k=` how many completed. `out=` is what the master sent, byte by byte. `in=` is what came back. `us=` is how long each byte took, in microseconds.
- `ok: analog` answers `analog`; `error: unknown command` answers anything else.

**Not every frame is printed.** A thousand lines a second would bury the port. A frame is printed when it is the first one, when what the master **sent** changed, when the **link state** changed, or when what came **back** changed and at least 10 ms have passed since the last printed line. That 10 ms holds back the jitter of an analog stick or whammy, but a frame that stays the same is still printed once, so a steady state never goes unrecorded. This rule lives in `src/core/trace_filter.{h,cpp}` and is tested on the laptop by `tests/test_trace_filter.cpp`.

### Reading a frame: what each byte means

The decoder, `tools/trace_decode.py`, turns each `T1` line into a headline and one row per byte:

```
frame: 5/5 bytes, complete
  byte 0: out 01 in FF 38 us ack 2 us
  byte 1: out 42 in 41 39 us ack 3 us
  byte 2: out 00 in 5A 39 us ack 3 us
  byte 3: out 00 in FF 39 us ack 3 us
  byte 4: out 00 in FF 37 us
```

(Expected shape again.) In a frame the guitar answers:

| byte | what the master sends | what the guitar answers |
|---|---|---|
| 0 | `01`: "controller, wake up" | nothing meaningful, often `FF` |
| 1 | `42`: "give me your state" | its **id**: which mode it is in. The code expects `41` in digital mode, `73` in analog mode and `F3` in config mode, but nobody has seen what this guitar sends. That is one of the questions below. |
| 2 | `00` | `5A`, "here comes the data" |
| 3, 4 | `00` | the **buttons**, two bytes. A bit is `0` while its button is pressed, `1` when released. |
| 5 to 8 | `00` (analog frames only) | four more bytes, the **axes**. The code expects the whammy at byte 8. |

`ack N us` is how long the guitar took to say "got that byte" after the master finished sending it. The master gives up after 100 µs.

### The eleven questions: the G table

Everything the code knows about the guitar came from documentation and from the emulator, which was written from the same documentation. This phase checks it against the real thing. Each question is answered by a capture you make in Part 4:

| id | Question | Answered by |
|---|---|---|
| G1 | Does the guitar answer at all on 3.3 V? | a `link: absent -> digital` line in `sg-digital.log` |
| G2 | Which id does it send in digital mode? | byte 1 of the untouched digital frames |
| G3 | Which byte and bit is each button? | the digital frames while you hold each control |
| G4 | What does it answer to each step of the switch to analog? | the first frames after `ok: analog` |
| G5 | Does the switch reach analog mode, and with which id? | a `-> analog` line, and byte 1 afterwards |
| G6 | Are the buttons in the same place in analog mode? | the analog frames while you hold each control |
| G7 | Where is the whammy, and what does it read at rest and fully pushed? | the analog frames, untouched and with the whammy held down |
| G8 | How slow is the guitar's "got that byte"? | the largest and smallest `ack` in both captures |
| G9 | How long is that pulse? | **cannot be measured here**: the master only watches for the line to fall |
| G10 | How many frames does the switch to analog take? | the `T1` lines between `ok: analog` and `-> analog` |
| G11 | Is the decoder's timing calibrated? | already settled by phase 04's loopback capture, while the bus code is unchanged |

After Part 4, the captures go back to the agent (Claude), who turns each answer into a test and fixes the code where the guitar disagrees with it.

## Check it yourself

### 1. On the Mac

| Command | What you should see |
|---|---|
| `make test 2>&1 \| tail -n 1` | `OK` |
| `build/host/test_trace_filter; echo $?` | no `FAIL:` line, then `0` |
| `make firmware && ls build/pico/*.uf2` | four files: `sg2hid.uf2`, `sg2hid_emu.uf2`, `sg2hid_loopback.uf2` and `sg2hid_observe.uf2` |
| `make lint 2>&1 \| grep -c FAIL` | `0` |

### 2. The instrument on the emulator: two Picos, no guitar

**Why this comes first.** The project's rule R-SAFETY-08 fixes the order in which new bus code meets hardware: tests on the laptop, then one Pico talking to itself, then a Pico against the emulator, then the real guitar **only listening**, and only after that the guitar in full. The observe firmware is new code. It meets the emulator before it meets the guitar, so a bug shows up on a board that costs nothing.

**The guitar is unplugged and not on the bench.**

1. **Flash the master with `sg2hid_observe.uf2`**, following phase 06's bench procedure (`docs/phases/06-hil-digital/verify.md`, §2, steps 1 to 3): signal wires out first, then hold **BOOTSEL** while plugging the master in, and copy `build/pico/sg2hid_observe.uf2` to the `RPI-RP2` drive. The emulator keeps `sg2hid_emu.uf2`.
2. **Find the ports again**, one board at a time, as in phase 06 step 4. The master's serial port **has a new name**: the observe firmware describes itself differently from the gamepad firmware. Write both down:

   ```
   MASTER=/dev/cu.usbmodem…
   EMU=/dev/cu.usbmodem…
   ```

3. **Wire the bench**: phase 06 steps 5 and 6, unchanged (both Picos on USB first, then the five signal wires and GND).
4. **Start the capture.** In a Terminal window, from the repository folder:

   ```
   mkdir -p docs/phases/09-guitar-observe/captures
   cat $MASTER > docs/phases/09-guitar-observe/captures/emu.log
   ```

   It prints nothing on screen: everything goes into the file. Leave it running.
5. **Drive the script** from a second Terminal window (set `MASTER` and `EMU` there too). Wait about two seconds after each line:

   ```
   printf 'mode digital\n' > $EMU
   printf 'payload ff bf 80 80 80 80\n' > $EMU
   printf 'analog\n' > $MASTER
   printf 'mode analog\n' > $EMU
   printf 'payload ff ff 80 80 80 00\n' > $EMU
   printf 'payload ff ff 80 80 80 40\n' > $EMU
   printf 'payload ff ff 80 80 80 80\n' > $EMU
   printf 'payload ff ff 80 80 80 c0\n' > $EMU
   printf 'payload ff ff 80 80 80 ff\n' > $EMU
   ```

   The second line presses "green" on the emulator. The third switches the master to analog mode. The last five move the emulated whammy through five positions.
6. **Stop the capture** with **Ctrl-C** in the first window.
7. **Check the log**, from the repository folder:

   | Command | What you should see |
   |---|---|
   | `C=docs/phases/09-guitar-observe/captures` | (sets a shortcut, prints nothing) |
   | `grep -c -- '-> digital' $C/emu.log` | `1` or more |
   | `grep -c -- '-> analog' $C/emu.log` | `1` or more |
   | `grep -c '^ok: analog' $C/emu.log` | exactly `1` |
   | `python3 tools/trace_decode.py $C/emu.log > /dev/null; echo $?` | `0` |
   | `for v in 00 40 80 C0 FF; do printf "$v: "; grep -c "^T1 n=9 .*in=[^ ]*,$v us=" $C/emu.log; done` | five lines, each with a count of `1` or more |

   The last command looks for each whammy value as the last `in` byte of a nine-byte frame. If one shows `0`, wait longer between the `payload` lines and capture again.

8. **Read it by eye**: `python3 tools/trace_decode.py $C/emu.log | less`. You should see, in this order: a `link: absent -> digital` line, five-byte frames with `in` starting `FF 41 5A`, a frame whose byte 4 changes from `FF` to `BF` (green), `ok: analog`, then a few frames whose `out` bytes change (the switch to analog), a `-> analog` line, and nine-byte frames whose last `in` byte steps through `00 40 80 C0 FF`. Press **q** to leave.

Do not edit `emu.log`. It is evidence, kept exactly as the port gave it.

### 3. The gamepad firmware still works

Phase 09 moved the code that prints the `link:` and `T1` lines into a file both firmwares share, so the gamepad firmware is re-checked.

1. Signal wires out. Flash the master with `build/pico/sg2hid.uf2` (BOOTSEL, as above). Find its port again: it is the gamepad's name from phase 08, not the observe firmware's.
2. Wire the bench again, then run:

   ```
   make hil MASTER=$MASTER EMU=$EMU | tail -n 1
   ```

   Expected: `hil: PASS`.

### 4. First contact with the guitar

Read this whole part before starting. **Never flash a Pico while the guitar is plugged in** (R-SAFETY-08): flashing means a board restarting into unknown code with the guitar on its wires.

#### 4.1 Put the instrument back on the master

With the bench still wired as in Part 3: signal wires out, then flash the master with `sg2hid_observe.uf2` again (BOOTSEL). Unplug the master's USB once it has restarted. The master now carries the observe firmware and is unpowered.

#### 4.2 Take the emulator off the bench

1. Pull the emulator's five signal wires and its GND wire out of the master's breadboard.
2. Unplug the emulator's USB. Put the emulator away: it is not used again in this part.

#### 4.3 Rebuild the socket wiring and re-run the multimeter checklist

The socket now takes the guitar, so its wiring is checked again from scratch, exactly as the first time.

1. Wire the socket per `docs/wiring.md`: each signal's 330 Ω resistor from the Pico pin's row to the socket pin's row, the two 10 kΩ pull-ups, **socket pin 5 to the 3V3 rail** and **socket pin 4 to the GND rail**. In the emulator bench, socket pin 5 had no wire; it gets one now.
2. **Socket pin 3 stays empty and insulated** (tape or heat-shrink over it). It is the 7.6 V pin, and anything on it can destroy the Pico (R-SAFETY-04).
3. **Nothing goes to Pico pin 39 (VSYS) or pin 40 (VBUS).** Both carry 5 V from USB, and the guitar is powered from 3.3 V only (R-SAFETY-05).
4. Run `docs/phases/02-wiring/verify.md`, **Part C, sections C1 and C2** (read its Part B, the multimeter primer, first if you need to). C1 is with everything unplugged; C2 is with the master in BOOTSEL and the guitar **unplugged**. Section C3 (current draw) is not needed again.
5. **If any reading is outside its expected range, stop here.** Unplug everything and recheck the wiring before going on.

After C2, unplug the master's USB.

#### 4.4 Plug in the guitar, then the USB

The order matters:

1. With the master **unplugged**, plug the guitar's cable into the socket.
2. Then plug the master's USB into the Mac.

The guitar gets its power from the master, so it powers up together with it, and it is never connected to a running bus halfway through a frame. The master does nothing on the bus until the next step opens its serial port.

3. Find the master's port (`ls /dev/cu.usbmodem*`; only one Pico is plugged in now) and set `MASTER=…` in two Terminal windows.

#### 4.5 The digital capture: `sg-digital.log`

1. In the first window:

   ```
   C=docs/phases/09-guitar-observe/captures
   cat $MASTER > $C/sg-digital.log
   ```

   Opening the port starts the master polling.
2. **Leave the guitar untouched for 2 seconds.**
3. Then, one at a time, **hold each control alone for about 1 second, then release it for about 1 second**, in this order: green fret, red, yellow, blue, orange, strum up, strum down, start, select, then tilt (raise the neck to vertical and bring it back down).
4. Press **Ctrl-C** in the first window.

Do not unplug the USB: the analog capture runs on the same power-up.

#### 4.6 The analog capture: `sg-analog.log`

1. In the first window: `cat $MASTER > $C/sg-analog.log`
2. In the second window: `printf 'analog\n' > $MASTER`
3. **Leave the guitar untouched for 2 seconds.**
4. The ten controls again, in the same order, each held about 1 second and released about 1 second.
5. **The whammy:** push it slowly all the way to its stop, hold it there for 1 second, then let it go.
6. Press **Ctrl-C** in the first window.
7. Unplug the master's USB, then the guitar.

Both files stay exactly as recorded. Do not open them in an editor that might save them back.

#### 4.7 Read the captures

| Command | What you should see |
|---|---|
| `for f in $C/*.log; do python3 tools/trace_decode.py "$f" >/dev/null \|\| echo "BAD $f"; done` | nothing |
| `grep -c 'link: absent -> digital' $C/sg-digital.log` | `1` or more (G1) |
| `grep -c -- '-> analog' $C/sg-analog.log` | `1` or more (G5) |
| `python3 tools/trace_decode.py $C/sg-*.log \| grep -o 'ack [0-9]* us' \| sort -n -k2 \| sed -n '1p;$p'` | the smallest and the largest `ack` (G8) |

Then read each capture by eye with `python3 tools/trace_decode.py $C/sg-digital.log | less`:

- byte 1's `in` value in the untouched frames is the guitar's digital id (G2);
- for each control, the frames that change bytes 3 or 4 show which bit went to `0` (G3).

In `sg-analog.log`, the frames after `ok: analog` show the switch, the analog id, the buttons and the whammy (G4 to G7). The whammy is the `in` byte that moves only while you push the bar: write down its byte number, its value at rest and its value at the stop.

#### 4.8 What a stop looks like

Four results stop the phase. They are not failures of yours: each one means the guitar disagrees with an assumption that matters, and the operator decides what happens next. If you see one, stop and report it with the capture.

| Stop | What you see |
|---|---|
| **The guitar does not answer (G1).** | `sg-digital.log` has no `link: absent -> digital` line. The decoder's headlines all read `aborted at byte 0` or `aborted at byte 1: no ACK after … us`, and every `in` byte is `FF`. The next question is the power supply. |
| **The whammy does not rest at `80` (G7).** | In the untouched analog frames, the whammy byte reads something other than `80`. The gamepad's "nothing pressed" report puts the whammy at `80`, so the computer would see the bar pushed while it is not. |
| **The guitar is slow (G8).** | The largest `ack` from 4.7 is above `50`. The master gives up at 100 µs, so less than twice that margin is too thin. |
| **The switch to analog fails (G5).** | `sg-analog.log` has no `-> analog` line, typically a run of `negotiating` and `absent` lines after `ok: analog`. The first such failure leads to one corrected sequence and a new capture; a second failure stops the phase. |

## What comes after the captures

The agent transcribes frames from the captures into new test vectors (`tests/vectors/sg_*.h`), one comment per byte saying which capture line it came from, adds six `sg_` cases to the codec tests, and fixes the code wherever the guitar disagrees. The results are recorded as one finding, "The SG on the bus", in `docs/constraints.md`.
