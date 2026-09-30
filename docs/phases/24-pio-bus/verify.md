# Phase 24-pio-bus — how to check this yourself

Written for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 uses
one Pico on the breadboard from phase 02 and two jumper wires. **The guitar stays unplugged
from the socket for the whole of this phase** (R-SAFETY-08): nothing here has been tested
against a controller yet, and this is not the phase where that happens.

## What was built

**The bus master: the part of the firmware that talks the PS2 controller's language.**

### The five wires, in one frame

A PS2 controller and the thing polling it (a console, or here the Pico) exchange short
*frames* of bytes. Five signal wires carry a frame:

| Wire | Who drives it | What it does |
|---|---|---|
| `ATT` (attention) | Pico | Pulled low for the whole frame: "you, controller, are being talked to". High = bus idle. |
| `CLK` (clock) | Pico | Ticks eight times per byte. Every tick is one bit. Idle high. |
| `CMD` (command) | Pico | The bit the Pico sends, one per tick. |
| `DATA` | controller | The bit the controller sends back, one per tick, at the same time. |
| `ACK` (acknowledge) | controller | After each byte, the controller pulls it low briefly: "got it, send the next one". |

So both sides send a byte at the same time: while the Pico shifts one byte out on `CMD`, the
controller shifts one byte back on `DATA`. After every byte except the last, the Pico waits
for the `ACK` pulse before starting the next byte (rule R-PROTO-06). If `ACK` never comes, the
Pico gives up on that byte after a timeout (`kAckTimeoutUs`, 100 µs), ends the frame, and puts
`ATT` back high (rule R-SAFETY-07). A frame that ended early is reported as "k of n bytes
done", not as a crash; deciding what that means for the guitar's state is a later phase.

### LSB first, mode 3, in one picture

Sending `0x01` (binary `00000001`) looks like this on the wires:

```
ATT  ‾‾‾‾\_____________________________________________/‾‾‾‾
CLK  ‾‾‾‾‾‾‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
CMD         1   0   0   0   0   0   0   0
            ^ bit 0 goes first ("least significant bit first", LSB-first)
```

- **LSB-first** (R-PROTO-01): the lowest bit of the byte travels first. `0x01` is therefore
  `1` then seven `0`s. Most-significant-first would be seven `0`s then `1`.
- **SPI mode 3** is a name for the clock's shape: the clock rests high, each bit is *put on*
  the wire while the clock is low, and *read* at the moment the clock goes back up (the
  rising edge). The bit has had half a tick to settle by then.

The clock runs at 250 kHz (`kBusClockHz`): 4 µs per bit, 32 µs per byte.

### What PIO is

The RP2040 has two small co-processors called **PIO** (programmable I/O). Each runs a tiny
program of a few instructions, one instruction per tick of its own clock, and nothing else
can interrupt it. That is how the Pico produces a perfectly regular clock and reads each bit
at exactly the right moment while the main CPU does other things. The program is
`src/hal/ps2_master.pio`: nine instructions, with a comment on top counting how many ticks
each step takes.

### The files

| File | What it is |
|---|---|
| `src/hal/ps2_master.pio` | The PIO program: clock, shift out, shift in, wait for `ACK`. |
| `src/hal/pio_port.cpp` | Sets up the five pins from the pin table, loads the PIO program, sends one byte at a time, and restarts the PIO program when a byte times out. Also the *bit-order probe* (below). |
| `src/hal/bus_frame.cpp` | The frame loop: pull `ATT` low, send each byte, stop at the first failure, put `ATT` high. It uses no Pico library at all, so the Mac can test it. |
| `src/app/main.cpp` | The loopback program for this phase's bench test. |
| `tests/test_bus_frame.py` | The Mac-side test of the frame loop. |

Pin setup happens in exactly one loop over the pin table in `src/core/pins.h`. `DATA` and
`ACK` are only ever made inputs with a pull-up (R-SAFETY-01: making them outputs could short
two chips against each other). A new automatic check, R-SAFETY-10, fails the build if any
pin-setting call in `src/hal/` does not use the pin table's entry.

## Check it yourself

### 1. The Mac-side checks

From the repository root:

| Command | Expected |
|---|---|
| `python3 tests/test_bus_frame.py; echo rc=$?` | three `ok:` lines, then `rc=0` |
| `sh tests/test_repo_shape.sh \| grep SAFETY-10` | `ok:   R-SAFETY-10` |
| `make firmware && test -f build/pico/sg2hid.uf2; echo rc=$?` | many `Building …` lines, then `rc=0` |
| `make lint; echo rc=$?` | only `ok:` lines, then `rc=0` |
| `make test \| tail -1` | `OK` |

What the three lines of `test_bus_frame.py` mean:

- `ok:   R-SAFETY-07 (…)` — the frame loop was run against a *fake* bus, one that pretends a
  byte failed at every possible position in frames of 1, 2, 5 and 9 bytes. In every one,
  `ATT` went low once at the start and high once at the end, and nothing was sent after it
  went high.
- `ok:   R-PROTO-06 (…)` — in those same runs, every byte but the last asked for an `ACK`
  wait, the last did not, and nothing was sent after a failed byte.
- `ok:   rejection cases: 5/5 (…)` — the test then broke the frame loop on purpose in five
  realistic ways (forgot to release `ATT`, returned early on failure, waited for `ACK` on
  every byte, on no byte, kept going after a failure) and checked that each break turns the
  right line into `FAIL`. This is what shows the two `ok:` lines above are not decoration.

This proves the *logic*. It does not prove the timing: the fake bus has no clock.

### 2. The bench: one Pico talking to itself

With no controller, the Pico can still check its own clock and shifting by talking to
itself. Two jumper wires make the loop:

- **`CMD` → `DATA`**: whatever the Pico sends comes straight back as what it receives. Every
  byte should return unchanged.
- **`ATT` → `ACK`**: while `ATT` is low (a frame is running), `ACK` is low too, so every
  `ACK` wait is satisfied immediately.

Both jumpers go **between socket-side rows**, on the far side of the 330 Ω resistors. That way
each connection passes through two 330 Ω resistors, and a pin mistake cannot pass more than
a few milliamps.

**Steps.**

1. **Unplug the guitar from the PS2 socket.** Leave the Pico's USB cable unplugged too.
2. Put one jumper wire from the row of **socket pin 2 (CMD)** to the row of **socket pin 1
   (DATA)**, and one from the row of **socket pin 6 (ATT)** to the row of **socket pin 9
   (ACK)**. Pin numbers are in `docs/wiring.md`.
3. Build and flash as in phase 23: `make firmware`, then hold **BOOTSEL**, plug USB in,
   release, and `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`.
4. Record 40 lines from the Pico (the first line may be cut in half, so it is dropped):

   ```
   PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 41 "$PORT" | tail -n +2 > build/loopback.log
   cat build/loopback.log
   ```

   This takes about ten seconds: the Pico prints four lines per second.

**Expected**, repeating once a second:

```
probe: 0x01 on the wire: 1,0,0,0,0,0,0,0
loopback: seq=0 bytes=5/5 match=yes att=high us=…
loopback: seq=1 bytes=9/9 match=yes att=high us=…
loopback: seq=2 bytes=1/1 match=yes att=high us=…
```

What each field means:

| Field | Meaning | Correct value |
|---|---|---|
| `seq` | which test sequence: 0 is a digital poll (`01 42 00 00 00`), 1 is nine bytes chosen to catch a stuck or swapped wire, 2 is the single byte `80` | 0, 1, 2 in turn |
| `bytes=k/n` | how many of the `n` bytes completed | `k` equals `n` |
| `match` | every byte came back exactly as it was sent | `yes` |
| `att` | the `ATT` wire's level after the frame | `high` (released) |
| `us` | how long the frame took, in microseconds | between 32 and 64 µs per byte: roughly 160–320 for seq 0, 288–576 for seq 1, 32–64 for seq 2 |
| probe | the order the bits of `0x01` appeared on `CMD` (see below) | `1,0,0,0,0,0,0,0` |

The same judgement as commands. Each one prints the count on the right when it is correct:

```
grep -c '^probe: 0x01 on the wire: 1,0,0,0,0,0,0,0$' build/loopback.log                      # 5 or more
grep '^loopback:' build/loopback.log | grep -vc 'match=yes att=high'                          # 0
awk '/^loopback:/ { split($3, b, "[=/]"); split($6, u, "="); if (u[2] < 32 * b[3] || u[2] > 64 * b[3]) bad++ } END { print bad + 0 }' build/loopback.log   # 0
```

**Now the timeout path.** Take away `ACK` so the Pico waits for a pulse that never comes:

5. Unplug USB. Remove the **`ATT`–`ACK`** jumper only (leave `CMD`–`DATA` in place). Plug USB
   back in (no BOOTSEL: the firmware is already on the Pico).
6. Record again:

   ```
   PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 41 "$PORT" | tail -n +2 > build/ackopen.log
   cat build/ackopen.log
   ```

**Expected**, repeating once a second:

```
probe: 0x01 on the wire: 1,0,0,0,0,0,0,0
loopback: seq=0 bytes=0/5 match=no att=high us=…
loopback: seq=1 bytes=0/9 match=no att=high us=…
loopback: seq=2 bytes=1/1 match=yes att=high us=…
```

- Sequences 0 and 1 stop at their first byte, `0/5` and `0/9`: that byte waited for `ACK`,
  none came within the timeout, and the Pico gave up. **`att=high` is the line that matters
  most**: the frame ended badly and `ATT` was still released.
- Sequence 2 still completes: its only byte is the last byte of its frame, and the last byte
  never waits for `ACK`. It also shows that the Pico recovered from the two aborted frames
  just before it.

As commands:

```
grep '^loopback:' build/ackopen.log | grep -vcE 'bytes=0/(5|9) match=no att=high|bytes=1/1 match=yes att=high'   # 0
grep -c 'bytes=1/1 match=yes att=high' build/ackopen.log                                      # 5 or more
```

7. Unplug USB and **remove the `CMD`–`DATA` jumper** too. The breadboard is back to its
   phase-02 state.

Send `build/loopback.log` and `build/ackopen.log` to the validation session: it cannot run
the bench part itself.

### Why the probe line means LSB-first, and why the loopback alone cannot tell

The loopback compares what came back with what was sent. The same PIO program shifts both
directions in the same order, so if it were sending the *highest* bit first it would also be
receiving the highest bit first, and every byte would still come back unchanged. The
loopback proves the clock, the shifting and the sampling moment work together. It says
nothing about which bit goes first.

The probe is different. It slows the bus clock down about 500 times, so that each bit
lasts about 2 ms instead of 4 µs, which is slow enough for the main CPU to watch. It then sends `0x01` and
reads the `CMD` wire itself each time the clock rises, writing the bits down in the order
they appeared. `0x01` is the one byte whose only `1` is its lowest bit, so:

- `1,0,0,0,0,0,0,0` means the lowest bit went first: **LSB-first, correct**.
- `0,0,0,0,0,0,0,1` would mean the highest bit went first: wrong order.
- `probe: timeout` means the clock never ticked: the PIO program is not running.

The probe reads the wire, not the program, so a mistake in the program would show up here.
That is why R-PROTO-01 is now bound to this manual check.

### What this phase does not prove

With `ATT` jumpered to `ACK`, `ACK` is low for the whole frame, so the `ACK` wait is
satisfied at once. On this bench only the *timeout* path of the wait is really exercised
(steps 5–6). Whether the wait actually waits for a real pulse, and whether 100 µs is enough
for the real guitar, is first observable against the emulator in `06-hil-digital` and
measured on the guitar in `09-guitar-observe`.
