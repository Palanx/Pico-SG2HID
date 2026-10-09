# Phase 08-usb-hid — how to check this yourself

This page is for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 is the same two-Pico bench as phases 06 and 07, **with no guitar anywhere** (R-SAFETY-08), first with the automated harness, then by eye in a gamepad tester on the Mac and on the Steam Deck.

## What was built

**The master Pico is now a USB gamepad.** Plug it into a Mac or a Linux machine and the computer sees a game controller called `SG2HID Guitar`, with ten buttons and one axis, without installing anything. Fed by the emulator, the five frets, strum up, strum down, start, select and tilt are the ten buttons, and the whammy is the axis.

### What a HID report descriptor is, and why no driver is needed

USB defines a family of devices called **HID**, "human interface devices": keyboards, mice, gamepads. Every operating system ships one generic HID driver that works for all of them. It can do that because a HID device describes itself: when it is plugged in, it sends the computer a short list of bytes, the **report descriptor**, that says "my messages are N bytes long; this bit is button 1, that bit is button 2, this byte is an axis from 0 to 255". The computer reads that description once, then decodes every message the device sends from it. No guitar-specific code is needed on the computer.

The master's descriptor is 44 bytes. In plain words it says:

- I am a **game pad**.
- **Ten buttons**, one bit each, starting at bit 0 of the first byte: green, red, yellow, blue, orange, strum up, strum down, start, select, tilt, in that order.
- **Six unused bits**, so the buttons fill exactly two bytes.
- **One axis, "X"**, one byte, from 0 to 255: the whammy.

Each message (a **report**) is therefore 3 bytes: two bytes of buttons, then the whammy. A button bit is `1` when pressed, the opposite of the PS2 bus, where `0` means pressed. The master turns one into the other.

A test on the laptop compares those 44 bytes, one by one, with a copy written by hand from the HID specification (rule R-HID-01). If someone changes the report layout without the descriptor, `make test` fails.

### What a composite device is, and why the serial port stays

One USB plug can carry several **interfaces** at once. A device built like that is called **composite**. The master now has two:

- the **serial port** it had before, where it prints `hil:`, `link:` and `T1` lines. `make hil` and `tools/trace_decode.py` keep working through it.
- the **gamepad**, new in this phase.

The computer shows both. On the Mac the serial port is still a `/dev/cu.usbmodem…` name, but **its name may differ from phase 07's**, because the device describes itself differently now. Find it again (Part 2, step 3).

One thing is lost: the master no longer accepts a "reboot into BOOTSEL" command over USB. To flash it, hold its **BOOTSEL** button while plugging it in, as in phase 06.

### What the neutral report is for

While the master is not reading good analog frames (the guitar is unplugged, or still negotiating, or a frame failed), it sends the computer the **neutral report**: no button pressed, whammy at `80`, the middle (rule R-HID-02). Without it, a button held at the moment the guitar dropped out would stay pressed on the computer.

The master sends a report every millisecond, after each poll. If the computer is not ready for it yet, that report is dropped, not kept for later: the next one is newer.

### What the master prints

The summary line gains a last field, `report=`: the last report sent to the computer, as three hex bytes.

```
hil: polls=60000 refused=0 changes=0 us=1000012 state=analog fault=none att=high payload=7F FE whammy=80 report=00 02 80
```

(This is the expected shape, not a bench recording.) Here `00 02 80` means: no button in the first byte, bit 1 of the second byte set (that is button 10, tilt), whammy `80`. The emulator's setup payload `7F FE` has tilt pressed, which is why tilt shows here.

### The harness

`make hil` checks the report on every summary it already judged:

| check | report required |
|---|---|
| `sustained`, `fault late 50`, and the recovery after each fault | `00 02 80` |
| `sweep`, after each whammy value `<V>` | `00 02 <V>` |
| `fault ack 0`, `fault ack 3`, `fault late 200`, `fault id 79`, `fault decline` | `00 00 80`, the neutral report |

A new scenario, `buttons`, runs after `sweep`. It presses each of the ten controls on its own, through the emulator, and checks that exactly that control's bit appears in the report:

| control | emulator button bytes | report |
|---|---|---|
| green | `ff bf` | `01 00 80` |
| red | `ff df` | `02 00 80` |
| yellow | `ff ef` | `04 00 80` |
| blue | `ff 7f` | `08 00 80` |
| orange | `ff fb` | `10 00 80` |
| strum up | `ef ff` | `20 00 80` |
| strum down | `bf ff` | `40 00 80` |
| start | `f7 ff` | `80 00 80` |
| select | `fe ff` | `00 01 80` |
| tilt | `ff fe` | `00 02 80` |

The harness also has a **demo mode**, `--demo`, for checking by eye. It needs only the emulator's port. It presses each control for 2 seconds, printing `pressed: <control>` first, then holds the whammy at `00`, `40`, `80`, `C0` and `FF` for 2 seconds each, printing `whammy: <V>`. It judges nothing: you watch the gamepad tester.

## Check it yourself

### 1. On the Mac

| Command | What you should see |
|---|---|
| `make test 2>&1 \| tail -n 1` | `OK` |
| `python3 tests/test_ps2_codec.py \| grep R-HID` | `ok:   R-HID-01 (the HID report descriptor equals the hand-written vector)` and `ok:   R-HID-02 (while the link is not AnalogStreaming, the report is neutral)` |
| `make firmware && ls build/pico/*.uf2` | three files: `sg2hid.uf2`, `sg2hid_emu.uf2` and `sg2hid_loopback.uf2` |
| `python3 tools/hil_digital.py --help` | a usage text that lists `--master`, `--emu`, `--seconds` and `--demo` |

### 2. The bench: two Picos, no guitar

**Before anything: the guitar is unplugged and not on the bench.**

1. **Flash both Picos** following phase 06's bench procedure (`docs/phases/06-hil-digital/verify.md`, §2) steps 1 to 3: signal wires out, then `sg2hid.uf2` to the master and `sg2hid_emu.uf2` to the emulator, each by holding **BOOTSEL** while plugging it in. Use the files built by **this** phase's `make firmware`.
2. **macOS may ask "Allow accessory to connect?"** the first time the new master is plugged in. Click **Allow**. If the master does not show up after that, unplug it and plug it back in.
3. **Find the ports again**, one board at a time, as in phase 06 step 4: plug in only the master and run `ls /dev/cu.usbmodem*`; plug in the emulator and run it again. The new name is the emulator's. Write both down:

   ```
   MASTER=/dev/cu.usbmodem…
   EMU=/dev/cu.usbmodem…
   ```

4. **Check the emulator answers**, then **wire the bench**: phase 06 steps 5 and 6, unchanged.
5. **Run the harness:**

   ```
   make hil MASTER=$MASTER EMU=$EMU
   ```

   Expected output, in this order:

   ```
   ok: setup
   ok: sustained
   ok: sweep
   ok: buttons
   ok: fault ack 0
   ok: fault ack 3
   ok: fault late 200
   ok: fault id 79
   ok: fault late 50
   ok: fault decline
   hil: PASS
   ```

6. **The Mac sees a gamepad.** With the master still plugged in, run:

   ```
   hidutil list --matching '{"VendorID":0x2E8A,"ProductID":0x000A,"PrimaryUsagePage":1,"PrimaryUsage":5}'
   ```

   At least one line must contain `SG2HID Guitar`. In that command, `0x2E8A` and `0x000A` are the master's USB identity (ADR-0003), and usage page 1, usage 5 is "Generic Desktop, Game Pad", the first line of the descriptor.

7. **The gamepad tester on the Mac.**
   1. Open a browser gamepad tester (any web page that shows the browser's gamepads: their buttons, their axes and the numbers they report).
   2. A browser lists a gamepad **only after its first button press**. The demo's first press does that, so start the demo first and look again after a few seconds.
   3. In a Terminal: `python3 tools/hil_digital.py --demo --emu $EMU`.
   4. For each `pressed: <control>` line, exactly one button lights up in the tester, a different one each time. Write down which button number lights for each control.
   5. For each `whammy: <V>` line, **axis 0** moves, and only axis 0. Write down the number the tester shows at `00`, `80` and `FF`.
   6. The demo ends with `demo: done`.

8. **The gamepad tester on the Steam Deck.**
   1. On the Deck, switch to **Desktop Mode** (Steam button → Power → Switch to Desktop).
   2. **Move the master's USB to the Deck, in the safe order** (`docs/wiring-emulator.md`, "To take the bench apart"): signal wires out, unplug the master from the Mac, plug it into the Deck (a USB-C adapter or hub), then wire the bench again. The emulator stays on the Mac.
   3. On the Deck, open the **Konsole** terminal and run `ls /dev/input/by-id/ | grep -i sg2hid`. One line must end in `-event-joystick`. That is Linux's generic HID driver handing the master to programs as a joystick.
   4. **The Steam client may hold the device.** If the tester below shows nothing, quit Steam (its tray icon → Exit) and try again.
   5. Open a browser gamepad tester on the Deck, then on the Mac run `python3 tools/hil_digital.py --demo --emu $EMU` again. Check and write down the same things as in step 7.

9. **Two summary lines by hand**, with the master back on the Mac (moved in the same safe order as step 8.2) and the bench wired. The harness prints only `ok:` lines, and two programs reading one port would each get only part of its lines, so these are taken after the harness, not during it.
   1. In a second Terminal window run `cat $MASTER`. A `hil:` line arrives every second. The demo (steps 7 and 8) left the emulator with nothing pressed (`payload=FF FF … report=00 00 80`), so first, in the first window, put the harness's setup payload back: `printf 'payload 7f fe 80 80 80 80\n' > $EMU`. Within two seconds the line is a `sustained` one: `state=analog … payload=7F FE whammy=80 report=00 02 80`. Copy one. Its `refused` and `fault` fields count from the master's power-up, so refusals from moving it between hosts show there; what matters is that `refused` stops growing.
   2. In the first window: `printf 'payload ff bf 80 80 80 80\n' > $EMU` (green alone, the `buttons` table's first row). Within two seconds the lines in the second window end `report=01 00 80`. Copy one.
   3. `printf 'payload 7f fe 80 80 80 80\n' > $EMU` to put the setup payload back, then close the `cat` with Ctrl-C.
10. **Record** in `docs/phases/08-usb-hid/notes.md` under `## Bench readings`: the `make hil` output; the two `hil:` lines from step 9; the `hidutil` line; the `/dev/input/by-id` line; for each host, the tester's button number per control and the axis readings at `00`, `80` and `FF`.
11. **To finish:** signal wires out first, then unplug both USB cables.

### If something looks wrong

- **`FAIL: setup: emulator did not answer 'fault none'`.** `EMU` is the wrong port. The names may have changed in step 3.
- **`FAIL: sustained: unreadable summary: … whammy=80`** (the line ends at `whammy=`, with no `report=`). The master still runs phase 07's `sg2hid.uf2`. Reflash it.
- **`hidutil` prints nothing, or the tester never lists a gamepad.** Check that the master runs this phase's firmware (its summary line ends in `report=`), and that the accessory was allowed (step 2).
- **A control lights the wrong button, or two buttons.** Record which, with the `make hil` output, in `notes.md`. `make hil`'s `buttons` scenario checks the same thing from the master's side.
- **Anything else.** Record the whole output in `notes.md`.
