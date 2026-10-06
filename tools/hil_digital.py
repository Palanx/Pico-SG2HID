#!/usr/bin/env python3
"""Two-Pico hardware-in-the-loop run for phase 06-hil-digital. Standard library only.

The master (build/pico/sg2hid.uf2) polls the emulator (build/pico/sg2hid_emu.uf2) once a
millisecond and prints a `hil:` summary every 1000 polls. This harness tells the emulator what to
answer, through the 05-emulator command grammar, and judges the master's summaries. It prints
`ok: <scenario>` or `FAIL: <scenario>: <reason>` per scenario, stops at the first FAIL, sends
`fault none` to the emulator before it exits, and ends with `hil: PASS` (exit 0) or `hil: FAIL`
(exit 1). The scenarios and what each must show are in docs/phases/06-hil-digital/spec.md §Goal.

    tools/hil_digital.py --master /dev/cu.usbmodem101 --emu /dev/cu.usbmodem2101
"""

import argparse
import os
import re
import select
import sys
import termios
import time
import tty

SUMMARY = re.compile(
    r"hil: polls=(?P<polls>\d+) refused=(?P<refused>\d+) changes=(?P<changes>\d+)"
    r" us=(?P<us>\d+) state=(?P<state>\S+) fault=(?P<fault>\S+) att=(?P<att>\S+)"
    r" payload=(?P<payload>.+)$"
)
COUNTERS = ("polls", "refused", "changes", "us")

MAX_SUMMARY_US = 1_100_000  # 1000 polls at 1 ms, plus 10 %
MASTER_SILENT_S = 3.0
EMU_ANSWER_S = 2.0
FAULT_WINDOW = 2

# The harness's own input, sent in `setup`; not a protocol-fixed byte (R-PROTO-05, 2026-09-17).
PAYLOAD_LINE = "payload 7f fe 80 80 80 80"
EXPECTED_PAYLOAD = "7F FE"


class Failure(Exception):
    pass


class Port:
    """A USB serial port in raw mode, read a line at a time."""

    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.fd)
        self.buf = b""

    def write_line(self, text):
        os.write(self.fd, (text + "\n").encode("ascii"))
        termios.tcdrain(self.fd)

    def _fill(self, timeout):
        if select.select([self.fd], [], [], timeout)[0]:
            try:
                self.buf += os.read(self.fd, 4096)
            except BlockingIOError:
                pass

    def read_line(self, deadline):
        """The next line without its CR/LF, or None at `deadline`."""
        while b"\n" not in self.buf:
            left = deadline - time.monotonic()
            if left <= 0:
                return None
            self._fill(left)
        line, _, self.buf = self.buf.partition(b"\n")
        return line.decode("ascii", "replace").rstrip("\r")

    def drop_complete_lines(self):
        """Discards every complete line received so far; keeps a partial one."""
        self._fill(0)
        _, sep, rest = self.buf.rpartition(b"\n")
        if sep:
            self.buf = rest


class Harness:
    def __init__(self, master, emu):
        self.master = master
        self.emu = emu

    def summary(self):
        """The master's next `hil:` line, parsed, after the checks that hold in every scenario."""
        deadline = time.monotonic() + MASTER_SILENT_S
        while True:
            line = self.master.read_line(deadline)
            if line is None:
                raise Failure("master silent")
            if not line.startswith("hil:"):
                continue
            match = SUMMARY.match(line)
            if not match:
                raise Failure(f"unreadable summary: {line}")
            got = match.groupdict()
            for key in COUNTERS:
                got[key] = int(got[key])
            if got["att"] != "high":
                raise Failure(f"att={got['att']}: {line}")
            if got["us"] > MAX_SUMMARY_US:
                raise Failure(f"us={got['us']} over {MAX_SUMMARY_US}: {line}")
            return got

    def send(self, text):
        """Sends one emulator line and waits for its `ok:`; the master's older lines are dropped."""
        self.emu.write_line(text)
        deadline = time.monotonic() + EMU_ANSWER_S
        while True:
            line = self.emu.read_line(deadline)
            if line is None:
                raise Failure(f"emulator did not answer '{text}'")
            if line == f"ok: {text}":
                break
            if line.startswith("error:"):
                raise Failure(f"emulator refused '{text}': {line}")
        self.master.drop_complete_lines()

    def window(self, k):
        """k summaries after one discarded one, and a Δ function over them."""
        before = self.summary()
        got = [self.summary() for _ in range(k)]
        return got, lambda key: got[-1][key] - before[key]


def expect_streaming(h, k):
    got, delta = h.window(k)
    for s in got:
        if s["state"] != "digital" or s["payload"] != EXPECTED_PAYLOAD:
            raise Failure(f"state={s['state']} payload={s['payload']}")
    if delta("refused") or delta("changes"):
        raise Failure(f"Δrefused={delta('refused')} Δchanges={delta('changes')}")


def expect_dropped(h, fault):
    got, delta = h.window(FAULT_WINDOW)
    for s in got:
        if s["state"] != "absent" or s["fault"] != fault:
            raise Failure(f"state={s['state']} fault={s['fault']}, expected absent/{fault}")
    if delta("refused") != delta("polls"):
        raise Failure(f"Δrefused={delta('refused')} of Δpolls={delta('polls')}")


def expect_late_kept(h):
    got, delta = h.window(FAULT_WINDOW)
    for s in got:
        if s["state"] != "digital":
            raise Failure(f"state={s['state']} fault={s['fault']}, expected digital")
    if delta("refused"):
        raise Failure(f"Δrefused={delta('refused')}")


def setup(h):
    for line in ("fault none", "mode digital", PAYLOAD_LINE):
        h.send(line)
    h.summary()


def fault_scenario(line, check):
    def run(h):
        h.send(line)
        check(h)
        h.send("fault none")
        try:
            expect_streaming(h, FAULT_WINDOW)
        except Failure as failure:
            raise Failure(f"recovery: {failure}") from None

    return line, run


def scenarios(seconds):
    return [
        ("setup", setup),
        ("sustained", lambda h: expect_streaming(h, seconds)),
        fault_scenario("fault ack 0", lambda h: expect_dropped(h, "ack-timeout")),
        fault_scenario("fault ack 3", lambda h: expect_dropped(h, "ack-timeout")),
        fault_scenario("fault late 200", lambda h: expect_dropped(h, "ack-timeout")),
        fault_scenario("fault id 79", lambda h: expect_dropped(h, "unknown-id")),
        fault_scenario("fault late 50", expect_late_kept),
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--master", required=True, help="the master Pico's serial port")
    parser.add_argument("--emu", required=True, help="the emulator Pico's serial port")
    parser.add_argument("--seconds", type=int, default=60,
                        help="summaries in the sustained window (default 60)")
    args = parser.parse_args()

    name = "setup"
    emu = None
    try:
        emu = Port(args.emu)
        h = Harness(Port(args.master), emu)
        for name, run in scenarios(args.seconds):
            run(h)
            print(f"ok: {name}", flush=True)
    except (Failure, OSError) as failure:
        print(f"FAIL: {name}: {failure}")
        print("hil: FAIL")
        return 1
    finally:
        if emu is not None:
            try:
                emu.write_line("fault none")
            except OSError:
                pass
    print("hil: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
