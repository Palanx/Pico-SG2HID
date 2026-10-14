# Phase 11-release — how to check this yourself

This phase changes no firmware. It rewrites `README.md` so that someone with only the README —
no phase documents, no agent — can build the firmware, flash it, wire it, and play Clone Hero on
a Mac and on the Steam Deck. Flashing happens on the Mac only; both computers play. You check it
by doing exactly that, reading only the README, and noting every place it let you down.

## What was built

### A README for using the device, not for developing it

The old README described the project before any firmware existed. The new one is ordered the way
you use it: what it is, safety, wiring (a pointer to `docs/wiring.md`), building, flashing,
using it on each computer, mapping it in Clone Hero, and what to look at when it does not work.
The development commands moved to the end.

### Building from nothing

The README now gives every install step on a Mac: `cmake` and `picotool` from Homebrew, the ARM
compiler as a Homebrew cask, and the Pico SDK cloned at version 2.3.1, the version
`CMakeLists.txt` insists on. The agent checked the build half of this: a fresh copy of this
repository, built against a freshly downloaded SDK made with the README's exact command, produced
`sg2hid.uf2`. The Homebrew installs are not tested on a clean Mac: they are already on this one, and you
decided to take them on faith.

`picotool` is a small program that turns the compiled firmware into the `.uf2` file. Without it,
the SDK quietly downloads and builds its own copy during `make firmware`. The README installs it
up front so the build does not depend on that download.

### Clone Hero

The device does not pretend to be an official guitar (ADR-0003), so Clone Hero does not know
which button is which. You tell it once per computer, in its control mapper, which opens with
the space bar (or by clicking the message that says so) from Clone Hero's main menu. These steps
were tried on the bench on the Mac and on the Deck, and the README says what that run found.

**The whammy needs calibrating.** The device reports the whammy at -1 at rest and +1 at the stop.
Until the bar first moves, the computer shows it at 0, half-way; that is phase 10's open question,
and the bench answered it: Clone Hero, calibrated with the whammy's center at -1 after the bar has
moved once, reads rest as no whammy and the stop as full whammy, on both computers. No firmware
change is needed.

## Check it yourself

Read the whole section first. Use **only `README.md`** for steps (a) to (e) — if you need
anything else, that is a gap in the README and goes in your notes (f). **Never flash a Pico
while the guitar is plugged in.**

### (a) Build on the Mac, from a fresh clone

1. In a new terminal, somewhere outside this repository:

   ```
   cd "$(mktemp -d)"
   git clone <this repository's URL or path> sg2hid
   cd sg2hid
   ```

2. Follow the README's **Build** section from step 1. The installs will say "already installed";
   that is fine. For step 2 (the SDK), clone a new copy here even though you have one, and use
   this new copy's `export PICO_SDK_PATH=…` **in this terminal only** — do not put it in
   `~/.zshrc`, so your normal setup is untouched.
3. Expected: `make firmware` ends with no error and `ls build/pico/sg2hid.uf2` shows the file.

### (b) Flash the master from the Mac

1. **Unplug the guitar from the socket.** Leave the breadboard wiring as it is.
2. Follow the README's **Flash** section with `build/pico/sg2hid.uf2` from the fresh clone.
3. Expected: the `RPI-RP2` drive disappears within a few seconds. A "disk not ejected properly"
   message is normal.

### (c) Connect the guitar

As in the README's **Flash** section's last paragraph: unplug the Pico's USB, plug the guitar into
its socket, plug the Pico back into the Mac. The wiring has not changed since phase 10, so there
is no need to repeat the multimeter checklist. If you have touched the breadboard since phase 10,
repeat it first (`docs/phases/02-wiring/verify.md`, Part C, sections C1 and C2), **by PS2 pin
number, found by socket position** (`docs/wiring.md` §Finding each pin on the socket).

### (d) Clone Hero on the Mac

1. Follow **Use it on macOS**: a browser gamepad tester shows `SG2HID Guitar`, ten buttons and
   the whammy.
2. Follow **Clone Hero**: install it, map the guitar, calibrate the whammy.
3. Play a song: hit notes with each fret and both strum directions, use the whammy on a sustained
   note (the note's tail wobbles), and tilt the neck to activate star power when it is available.
   Pause with start. With the bar at rest, a sustained note's tail stays still.
4. Write down every step where the README's Clone Hero instructions did not match what you saw.

### (e) Clone Hero on the Steam Deck

1. Move the Pico to the Deck (the guitar stays in its socket).
2. Follow **Use it on the Steam Deck**, then **Clone Hero**, with the same checks as (d) steps
   1 to 4.

### (f) Your notes on the README

For each README section you used in (a) to (e), write down where it was wrong, unclear, or
missing a step — or "fine".

### (g) Record

Give the agent, or write in `docs/phases/11-release/notes.md` under `## Bench readings`:

- (a): whether `make firmware` passed in the fresh clone;
- (b): whether the Mac flashed the Pico;
- (d) and (e), for each computer: whether mapping and calibration worked, and whether the song
  played with frets, strums, whammy and star power;
- (f): your notes.

### If something looks wrong

| What you see | What it points to |
|---|---|
| `make firmware: PICO_SDK_PATH is unset.` | the `export` line was not run in this terminal |
| `pico-sdk … found …, but this firmware is pinned to 2.3.1` | the SDK clone is not at 2.3.1: the `--branch 2.3.1` part of the command was changed |
| `RPI-RP2` never appears | BOOTSEL was not held while plugging in, or the cable is charge-only. Try again holding it until the drive shows up |
| Clone Hero sees no controller | Steam is still running on the Deck (quit it), or the macOS accessory prompt was dismissed (unplug, replug) |
| A control does nothing in Clone Hero but lights in the gamepad tester | the mapping missed it: map that action again |
| Sustained notes wobble with the whammy at rest | the whammy is not calibrated: README **Clone Hero** step 4, center at -1 |
| Nothing in the tester either | the README's **If it does not work** section: read the `state=` field |
