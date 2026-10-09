# ADR-0017: The master negotiates analog mode through a pure sequencer and never streams digital

- Status: accepted
- Date: 2026-10-08

## Context

Phase `07-analog-mode` makes the master ask the controller for analog mode, because the whammy
axis exists only in an analog frame (R-PROTO-04). Until now the master sent one digital poll,
`01 42 00 00 00`, forever. Analog mode takes a sequence of frames: enter config mode, select
analog, leave config mode, then poll. Each step has to be judged on what came back, and the
judgement depends on which step is in progress. That is history, so it cannot live in `decode( )`
(ADR-0012).

Several shapes were open:

- where the sequence lives (`app` or `core`);
- how long each frame is, given that `exchange_frame( )` sends a fixed length and does not read
  the header to stop early;
- what happens when a controller answers the sequence but stays digital;
- when the emulator applies a command it received.

## Decision

- **A pure `core` sequencer.** `src/core/negotiation.h` holds the stage (`EnterConfig`,
  `SetAnalog`, `LeaveConfig`, `Polling`), the bytes each stage sends, and `advance( )`, which
  judges one exchanged frame and steps the `Link`. `app` only exchanges bytes and reads the
  clock (ADR-0002, ADR-0011).
- **The enter frame is 5 bytes and is judged on its prefix.** A controller answers
  `01 43 00 01 00` in whatever mode it is in, digital or analog, so the answer's length is not
  known in advance. Only the prefix is read: three completed wire bytes, a declared id, then
  `kReadyByte`. The rest is ignored and the `Link` is not stepped on success.
- **Every other frame is 9 bytes**, the length of an analog or config frame plus the address
  byte. A digital controller ACKs only 4 of them; `decode_poll( )` then decodes a digital frame
  from what completed.
- **A declined request is a fault.** When a frame after the enter answers with a valid prefix
  whose id is not the one the stage expects, the link drops to `Absent` with
  `FaultCause::Declined`, and the sequence restarts from `EnterConfig`. The master never streams
  digital: a guitar without its whammy is not the device this project builds.
- **The emulator applies a command at the start of the next frame**, and only when the master's
  wire byte 3 arrived. A real controller answers a frame before it has seen the whole command,
  so the answer to a command frame cannot already reflect it.

## Consequences

- **Easier.** The whole sequence runs on the laptop in `make test`, against hand-written
  vectors. The judging rules are a function of bytes, a stage and elapsed microseconds.
- **Harder.** Every frame after the enter is 9 bytes even to a digital controller. That costs
  one ACK timeout per frame on a declined link, which only matters while the link is failing.
- **Debt taken knowingly.** `kNegotiationTimeoutUs` is still a budget. The emulator negotiates in
  one frame of `Negotiating`, so it measures nothing about the guitar; `09-guitar-observe` owns
  tightening it.
- **Rejected: falling back to digital streaming.** It would give frets without a whammy and
  hide a guitar that refuses analog mode behind a link that looks healthy.
- **Rejected: reading the frame length from the header in `exchange_frame( )`.** It would move
  protocol knowledge into `hal` for a saving that only exists on a failing link.
- **Rejected: a retry backoff after `Declined`.** Nothing measured asks for one yet.
