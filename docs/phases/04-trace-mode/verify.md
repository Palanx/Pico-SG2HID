# Phase 04-trace-mode — how to check this yourself

Written for someone who does not write firmware. Part 1 is commands on the Mac. Part 2 is the
same one-Pico bench as phase 24-pio-bus: two jumper wires, **the guitar unplugged from the
socket for the whole phase** (R-SAFETY-08). The trace is the tool you will later point at the
real guitar (`09-guitar-observe`); this phase only proves the tool works, on the Pico talking
to itself.

## What was built

**A logic analyzer in software: every frame the Pico runs on the bus is printed as one line,
and a Mac script turns that line into something you can read.**

### Why

In phase 24 the Pico printed one summary line per frame: how many bytes completed, whether
they matched. That hides the two things you need when a real controller misbehaves: *which
byte* went wrong, and *how long* the controller took to answer. A trace line keeps both.

### A trace line, field by field

After every `loopback:` line, the Pico now prints a line like this:

```
T1 n=5 k=5 out=01,42,00,00,00 in=01,42,00,00,00 us=38,39,39,39,38
```

| Field | Meaning |
|---|---|
| `T1` | The format's name and version. If the format ever changes, the new one is `T2`, so old captures stay readable. |
| `n=5` | The frame is 5 bytes long. |
| `k=5` | 5 of them completed. `k` smaller than `n` means the frame stopped early, at byte number `k` (counting from 0). |
| `out=…` | Every byte the Pico sent, in hexadecimal (two digits, `00` to `FF`). |
| `in=…` | Every byte that came back. `--` for a byte that did not complete or was never sent. |
| `us=…` | For every byte the Pico *tried*, how many microseconds (µs, millionths of a second) passed between handing the byte to the PIO and the byte finishing or being given up on. `-` for a byte never tried. |

The timing is taken by the main CPU reading its microsecond clock, before and after each byte
(ADR-0015). That adds a few µs of slack to every number: good enough to see a controller that
answers in 10 µs versus one that never answers, not good enough for single-µs precision.

### What an `ACK` delay is, and why it is "elapsed minus the shift time"

Recall from phase 24: after each byte except the last, the controller pulls the `ACK` wire low
to say "got it, send the next one". How quickly it does that is the **`ACK` delay**. It is
the number most worth watching on the real guitar: a delay creeping toward the timeout
(100 µs) is a guitar about to drop off the bus.

The Pico does not time the `ACK` on its own. It times the whole byte. A byte's time is two
parts:

1. **The shift**: eight clock ticks to move the bits, plus a few instructions of PIO
   bookkeeping. This is fixed: 37 µs (37 PIO instructions at one per µs, counted in the
   comment at the top of `src/hal/ps2_master.pio`).
2. **The wait for `ACK`**: whatever the controller takes.

So `ACK delay = elapsed − 37 µs`. The decoder does that subtraction. It shows no `ACK` delay
for the last byte of a frame, because the last byte never waits for one.

### `SHIFT_US`, the 37

The 37 is the constant `SHIFT_US` at the top of `tools/trace_decode.py`. It is computed from
the PIO program, not measured, and the CPU's timing adds a few µs, so it is a **calibration
knob**. How to tell if it is off, from a loopback capture with the `ATT`→`ACK` jumper in
place (part 2, step 4): there, `ACK` is already low when each byte ends, so the true delay is
zero.

- Every `ack` should read between 0 and about 5 µs. That means 37 is right.
- If every `ack` reads, say, 8 to 10 µs, the real overhead is bigger: raise `SHIFT_US` by
  about that much.
- The last byte of each frame, which has no `ACK` wait, shows the shift plus overhead
  directly: its `us` value is what `SHIFT_US` should be close to.

A delay below zero cannot happen, so the decoder shows 0 for those.

### What the decoder prints

`python3 tools/trace_decode.py` turns the line above into:

```
frame: 5/5 bytes, complete
  byte 0: out 01 in 01 38 us ack 1 us
  byte 1: out 42 in 42 39 us ack 2 us
  byte 2: out 00 in 00 39 us ack 2 us
  byte 3: out 00 in 00 39 us ack 2 us
  byte 4: out 00 in 00 38 us
```

The headline is one of three:

| Headline | Meaning |
|---|---|
| `frame: 5/5 bytes, complete` | Every byte went through. |
| `frame: 0/5 bytes, aborted at byte 0: no ACK after 134 us` | Byte 0 was shifted, then the controller never pulled `ACK` low, and the Pico gave up after 134 µs. |
| `frame: 1/2 bytes, aborted at byte 1: did not complete after 135 us` | The *last* byte failed. It waits for no `ACK`, so it is reported as "did not complete" rather than "no ACK". |

Each row is one byte: its position, what was sent, what came back (`--` if nothing), how long
it took (or `not sent`), and, for a completed byte that is not the last, its `ack` delay.
Lines that are not trace lines (`probe:`, `loopback:`) are printed unchanged. A line that
starts with `T1 ` but is garbled prints `unreadable trace line: …`, and the script ends with
exit code 1 so you notice.

### The files

| File | What it is |
|---|---|
| `src/core/bus_trace.cpp` | Writes a trace line. Pure logic, no Pico library, so the Mac tests it. |
| `src/hal/pio_port.cpp` | Now also times each byte. |
| `src/hal/bus_frame.cpp` | The frame loop; now keeps each byte's time beside its received value. |
| `src/app/main.cpp` | The loopback program; now prints a trace line after every `loopback:` line. |
| `tools/trace_decode.py` | The Mac-side decoder. Plain Python, nothing to install. |
| `tests/vectors/trace_session.txt`, `.rendered` | A sample session and the decoder's expected output, both written by hand. |
| `tests/test_bus_trace.py` | Checks the firmware's writer and the decoder against those two files. |

## Check it yourself

### 1. The Mac-side checks

From the repository root:

| Command | Expected |
|---|---|
| `python3 tests/test_bus_trace.py; echo rc=$?` | three `ok:` lines, then `rc=0` |
| `python3 tests/test_bus_frame.py; echo rc=$?` | four `ok:` lines (two naming R-SAFETY-07 and R-PROTO-06, one about elapsed time, one `rejection cases: 5/5`), then `rc=0` |
| `printf 'T1 n=1 k=1 out=80 in=80 us=36\n' \| python3 tools/trace_decode.py` | `frame: 1/1 bytes, complete` and one byte row |
| `make firmware && test -f build/pico/sg2hid.uf2; echo rc=$?` | many `Building …` lines, then `rc=0` |
| `make test \| tail -1` | `OK` |

What the three lines of `test_bus_trace.py` mean:

- `ok:   format_trace_line returns 0 …` — the firmware's writer refuses impossible input
  (more bytes completed than the frame has) and a buffer too small for the line, rather than
  printing half a line.
- `ok:   R-PROTO-08 (…)` — the firmware's writer was given five hand-built frames and wrote
  exactly the five `T1` lines in `tests/vectors/trace_session.txt`; and the decoder, run on
  that file, printed exactly `tests/vectors/trace_session.rendered`. Both files were written
  by hand, so neither side checked itself.
- `ok:   rejection cases: 4/4 (…)` — the test broke the writer twice (wrong mark for the failed
  byte's time, wrong separator) and the decoder twice (forgot to subtract the 37 µs, gave the
  last byte an `ACK`), and each break turned the R-PROTO-08 line to `FAIL`.

These prove the *format* is right on both sides. They cannot prove the *timings* are right:
only the bench shows that.

### 2. The bench: one Pico, guitar unplugged

Same breadboard, jumpers and flashing as `docs/phases/24-pio-bus/verify.md` part 2. Read its
steps 1–3 if you have not done them before.

1. **Unplug the guitar from the PS2 socket.** USB unplugged too.
2. Jumpers in place: **socket pin 2 (CMD) → socket pin 1 (DATA)** and **socket pin 6 (ATT)
   → socket pin 9 (ACK)**, between the socket-side rows.
3. Flash: `make firmware`, hold **BOOTSEL**, plug USB in, release, then
   `cp build/pico/sg2hid.uf2 /Volumes/RPI-RP2/`.
4. Capture 50 lines (about seven seconds: seven lines per second) and decode them:

   ```
   PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 51 "$PORT" | tail -n +2 > build/trace.log
   python3 tools/trace_decode.py build/trace.log > build/trace.txt; echo $?
   less build/trace.txt
   ```

   **Expected:** `0` from the decoder. Every frame headline ends in `complete`. Every `ack`
   is between 0 and about 5 µs (the jumper holds `ACK` low, so there is nothing to wait for).
   Every byte's `us` is roughly 37–45.

   **Read one frame aloud**, for example the 5-byte one: "five bytes, all completed; the
   Pico sent 01 42 00 00 00 and got the same back, because CMD is wired to DATA; each byte
   took about 38 µs, and the ACK came about 1 µs after each shift ended." If you can say
   that from the decoded text, the trace is doing its job.

   As commands (each prints the expected number when correct):

   ```
   grep -c '^T1 ' build/trace.log                                  # 15 or more
   grep '^frame:' build/trace.txt | grep -vc 'complete$'           # 0
   awk '/ ack [0-9]+ us/ { for (i = 1; i <= NF; i++) if ($i == "ack" && $(i+1) > 5) bad++ } END { print bad + 0 }' build/trace.txt   # 0
   ```

5. **Now the timeout.** Unplug USB, remove **only the `ATT`→`ACK` jumper**, plug USB back in
   (no BOOTSEL). Capture and decode again:

   ```
   PORT=$(ls /dev/cu.usbmodem* | head -1); head -n 51 "$PORT" | tail -n +2 > build/trace-ackopen.log
   python3 tools/trace_decode.py build/trace-ackopen.log > build/trace-ackopen.txt; echo $?
   less build/trace-ackopen.txt
   ```

   **Expected:** `0` from the decoder, and these headlines repeating:

   ```
   frame: 0/5 bytes, aborted at byte 0: no ACK after … us
   frame: 0/9 bytes, aborted at byte 0: no ACK after … us
   frame: 1/1 bytes, complete
   ```

   **Say from the trace which byte aborted and after how long**: "byte 0, after about
   135 µs". The number should be close to the Pico's budget for one byte: 32 µs of shifting
   plus the 100 µs `ACK` timeout, plus a few µs of overhead. The rows below it show every
   other byte as `not sent`: the Pico stopped at the first failure. The 1-byte frame still
   completes, because its only byte is the last and waits for no `ACK`.

   As commands:

   ```
   grep '^frame:' build/trace-ackopen.txt | grep -vcE 'frame: 0/(5|9) bytes, aborted at byte 0: no ACK after [0-9]+ us|frame: 1/1 bytes, complete'   # 0
   grep -c 'frame: 1/1 bytes, complete' build/trace-ackopen.txt    # 5 or more
   ```

6. Unplug USB and remove the `CMD`→`DATA` jumper. The breadboard is back to its phase-02
   state.

Send `build/trace.log` and `build/trace-ackopen.log` to the validation session: it cannot run
the bench part itself.

### Decoding live

Instead of capturing to a file, you can watch the bus as it happens:

```
python3 tools/trace_decode.py /dev/cu.usbmodem*
```

Frames scroll by as the Pico prints them. Stop with **Ctrl-C**. Only one program can read the
port at a time, so close any other serial monitor first.
