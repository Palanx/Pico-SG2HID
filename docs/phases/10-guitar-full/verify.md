# Phase 10-guitar-full — how to check this yourself

This is the phase where the guitar plays for real: the gamepad firmware (`sg2hid.uf2`) talks to
the SG, the computer sees a gamepad, and pulling the guitar's plug out and back in does not break
anything. You check three things: the guitar plays, it survives being unplugged, and the firmware
is fast enough.

## What was built

### The guitar now meets the gamepad firmware, not only the observe instrument

Phase 09 connected the guitar only to `sg2hid_observe.uf2`, a read-only instrument. Everything it
learned (the 20 µs waits, the 8 ms poll period, the corrected button bits, the whammy mapping) is
already in `sg2hid.uf2`. This phase connects the guitar to it and checks the result end to end.
Little firmware changed: two numbers are now printed, and one time limit is shorter.

### A shorter limit on negotiation

When the master starts, or after the guitar comes back, it has to switch the guitar into analog
mode: three frames that go through "config mode". If the guitar stayed stuck in config mode, the
master would give up and start over after a time limit, `kNegotiationTimeoutUs`. That limit used
to be 100 ms. The real SG gets through config mode in about 24 ms (two frames), so the limit is
now six polls, 48 ms: twice what the guitar needs. A stuck guitar is noticed sooner, and a normal
one is never cut short.

### Latency: what it is, and the two numbers that measure it

**Latency** is the time from moving something on the guitar to the computer being able to see it.
Three delays add up:

1. **Waiting for the next poll.** The master asks the guitar for its buttons every 8 ms (ADR-0021:
   faster and the SG stops updating them). A press that lands just after a poll waits up to 8 ms.
2. **The handoff.** Once the master starts a poll, it exchanges the frame with the guitar, works
   out the gamepad report, and hands that report to the USB chip. The master measures this time.
3. **The USB interval.** The computer collects the report from the USB chip every 1 ms.

The worst case is therefore 8 ms + handoff + 1 ms. You and the agent agreed the handoff must stay
within 1 ms, so the worst case is 10 ms at most and about 5–6 ms on average. A game's lag
calibration absorbs the average.

The master's `hil:` line, printed about once a second, now ends in two new fields:

- **`hand=`** — the longest handoff in microseconds over that second's 125 polls. It must be
  1000 or less.
- **`busy=`** — how many of those polls found the USB side not ready to take a report. A report
  that finds it busy is not sent, and the computer sees the change one poll (8 ms) later. It must
  be 0.

**What is not measured.** The time inside the guitar, from your finger to the guitar's own chip
noticing the button, cannot be seen from the bus. Neither can what the computer does after it
collects the report (the browser, the game). Both come before or after the 10 ms above.

### The guitar harness

`make guitar MASTER=<port>` runs `tools/hil_digital.py --guitar`, which reads the master's serial
port and asks you to do things at the guitar. It needs no second Pico. It runs three parts:

| Part | What it asks | What it checks |
|---|---|---|
| `streaming` | nothing: leave the guitar alone for about a minute | every `hil:` line says `state=analog`, `busy=0`, `hand=` 1000 or less, and no poll is refused |
| `controls` | `hold green`, `hold red`, … for the ten controls, then `push the whammy to the stop and hold` | each control shows up in the gamepad report while you hold it; the whammy reads `C0` or more when held at the stop |
| `replug` | `unplug the guitar`, then `plug the guitar back in`, five times | the master notices the guitar is gone, sends the "nothing pressed" report while it is gone, and is back in analog mode after the replug, all without the USB connection dropping |

It prints `ok: <part>` after each part and ends with `guitar: PASS`. On a problem it prints
`FAIL: <part>: <reason>` and `guitar: FAIL`, and stops.

## Check it yourself

Read the whole section first. **Never flash a Pico while the guitar is plugged in** (R-SAFETY-08).

### 1. On the Mac

```
make test
make lint
make firmware
```

Each must finish with no `FAIL`. `make firmware` leaves `build/pico/sg2hid.uf2` and
`build/pico/sg2hid_emu.uf2`.

### 2. The emulator first: two Picos, no guitar

The firmware changed, so before the guitar sees it, it runs against the emulator (R-SAFETY-08:
emulator before guitar).

1. **The guitar is unplugged and off the bench.**
2. Follow `docs/phases/08-usb-hid/verify.md` §2 steps 1 to 5 with this phase's UF2 files: flash
   both Picos, find the ports, wire the bench, then `make hil MASTER=$MASTER EMU=$EMU`.
3. Expected: the same ten `ok:` lines as phase 08, then `hil: PASS`. The harness now also checks
   `hand=` on every line, so a `FAIL: …: hand=… over 1000` here means the handoff is too slow
   even without the guitar. Stop and report it.
4. Optional, to see the numbers: after `hil: PASS`, `cat $MASTER` in a second window and copy one
   `hil:` line; Ctrl-C to close it.
5. Signal wires out, then unplug both USB cables.

### 3. Connect the guitar

Same procedure as phase 09, with the gamepad firmware already on the master:

1. Take the emulator off the bench: `docs/phases/09-guitar-observe/verify.md` §4.2.
2. Rebuild the socket wiring and re-run the multimeter checklist: §4.3, **by PS2 pin number, not
   the breakout's printed numbers** (`docs/wiring.md` §This bench's breakout). Pin 3 (7.6 V)
   stays empty and insulated; nothing goes to VSYS or VBUS.
3. Plug in the guitar, then the master's USB into the Mac: §4.4 steps 1 and 2. The master starts
   polling as soon as it powers up.
4. Find the port: `ls /dev/cu.usbmodem*` (only the master is plugged in) and set
   `MASTER=/dev/cu.usbmodem…`.

### 4. The guitar harness on the Mac

**How to unplug and replug safely.** When the harness asks, pull the guitar's own cable plug out
of the socket, and push it back in. Leave the master powered and plugged into the computer. Do
not touch the breadboard or its wires. A real PS2 does the same thing when you pull a controller
out while it is running. Push the plug straight in, not at an angle, so no pin brushes against
the wrong contact on the way in.

1. Run:

   ```
   make guitar MASTER=$MASTER
   ```

2. `streaming` takes about a minute. Do not touch the guitar.
3. `controls`: when it prints `hold <control>`, press and hold that control until the next line
   appears (one or two seconds). For `tilt`, hold the neck straight up. For `strum up` and
   `strum down`, hold the strum bar in one direction. If a strum prompt does not move on, try the
   other direction: the names follow the gamepad's labels, not the bar's physical direction
   (phase 09, G3). Then push the whammy all the way down and hold it until `let go`.
4. `replug`: unplug when asked, wait for the next prompt, plug back in. Five times.
5. Expected:

   ```
   ok: streaming
   ok: controls
   ok: replug
   guitar: PASS
   ```

### 5. The gamepad tester on the Mac

1. Open a browser gamepad tester (as in phase 08: any page that shows the browser's gamepads).
   Press any button on the guitar: browsers list a gamepad only after its first press.
2. Press each fret, both strum directions, start and select, and tilt the neck. Each lights
   exactly one button. Write down which number lights for each.
3. Push the whammy slowly from rest to the stop. **Axis 0** goes smoothly from one end to the
   other and comes back when you let go. Write down the number it shows at rest and at the stop.
4. Unplug and replug the guitar three times, as in step 4. While it is out, nothing in the tester
   is lit and the whammy is at rest. After each replug, press green: it lights within a second or
   two. The gamepad must not disappear from the tester at any point.

### 6. The Steam Deck

1. Move the master to the Deck: unplug the master's USB from the Mac (the guitar stays in its
   socket), switch the Deck to Desktop Mode (Steam button → Power → Switch to Desktop), and plug
   the master into the Deck.
2. Copy `tools/hil_digital.py` to the Deck (a USB stick or `scp`). It is one file and needs only
   Python, which SteamOS has.
3. In **Konsole**: `ls /dev/ttyACM*` shows the serial ports. Unplug the master and run it again: the name that
   disappeared is the master's (`/dev/ttyACM0`, or `/dev/ttyACM1` when another device already holds
   `ttyACM0`, as on this bench). Plug it back in, then, with that name:

   ```
   python3 hil_digital.py --guitar --master /dev/ttyACM1
   ```

   Same prompts and same expected end, `guitar: PASS`. If it says `Permission denied` on the
   port, your user may not be allowed to open serial ports on SteamOS. Running it with `sudo`
   works if you have set a password for the `deck` user. This was not tried on this Deck, so
   report it if it happens.
4. Repeat step 5 (the gamepad tester) on the Deck. **The Steam client may hold the device:** if
   the tester shows nothing, quit Steam (its tray icon → Exit).
5. **What the Deck shows that the Mac does not.** Linux's joystick interface makes a small region
   in the middle of an axis read exactly 0 (bytes 112–142 of 0–255, phase 08's readings). The
   whammy now rests at one end, so that flat spot sits in the middle of its travel: around
   halfway down, for about a tenth of the bar's travel, the tester's number stops moving, then
   moves on. That is the
   browser's view of the axis, not the device. Write down whether you see it.

### 7. Record

Give the agent, or write in `docs/phases/10-guitar-full/notes.md`:

- the `make hil` output (step 2) and the `hil:` line if you copied one;
- the `make guitar` output on the Mac and the harness output on the Deck;
- for each host, the tester's button number for each control, the whammy at rest and at the stop,
  and whether the gamepad ever disappeared during the replug cycles;
- whether you saw the flat spot on the Deck (step 6.5).

### If something looks wrong

| What you see | What it points to |
|---|---|
| `FAIL: streaming: state=absent …` or `state=negotiating` | the guitar is not answering. Check the wiring by PS2 number (`docs/wiring.md`) and the socket contacts, as in phase 09 |
| `FAIL: streaming: Δrefused=…` | the link dropped while you were not touching the guitar: a loose contact, usually. Re-seat the guitar's plug and the bus wires, re-run |
| `FAIL: streaming: … busy=…` | the computer did not collect a report in time. Report it with the line: the fix (re-sending the report a moment later) is a firmware change the agent proposes |
| `FAIL: …: hand=… over 1000` | the master takes too long between starting a poll and handing over the report. Report it with the line |
| `FAIL: controls: no report with <control> …` | that control never reached the gamepad report while you held it. Try once more holding it longer; if it fails again, report which control |
| `FAIL: controls: no whammy at or above C0 …` | the whammy at its stop reads less than expected. Report the `whammy=` value from a `hil:` line (`cat $MASTER` with the bar held) |
| `FAIL: replug: no 'link: analog -> absent' …` | the master did not notice the guitar was gone within 30 seconds |
| `FAIL: replug: no 'link: <any> -> analog' …` | the guitar did not come back into analog mode within 30 seconds of the replug. Unplug and replug once more and watch `cat $MASTER` for `link:` lines |
| `FAIL: …: [Errno …]` (any part) | the serial port went away: the master reset or the USB dropped. During `replug` that means unplugging the guitar disturbed the master's power. Report it |
