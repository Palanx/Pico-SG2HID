#!/usr/bin/env python3
"""Render the firmware's `T1` bus trace lines (ADR-0015) as readable frames.

Usage: trace_decode.py [FILE ...]   — reads standard input when no file is named. A serial
device works as a FILE (`trace_decode.py /dev/cu.usbmodem*`): lines are decoded as they arrive.

A `T1` line is

    T1 n=<n> k=<k> out=<b0>,...,<bn-1> in=<b0>,...,<bn-1> us=<u0>,...,<un-1>

(the format is fixed in src/core/bus_trace.h). Each one becomes a headline saying whether the
frame completed or where it aborted, then one row per byte. Any other line is echoed unchanged.
A line that starts like a trace line but does not parse is reported as unreadable, and the run
exits 1 at the end. Standard library only.
"""

import re
import sys

PREFIX = "T1 "
LINE = re.compile(r"T1 n=([0-9]+) k=([0-9]+) out=(\S+) in=(\S+) us=(\S+)")
HEX_BYTE = re.compile(r"[0-9A-F]{2}")
DECIMAL = re.compile(r"[0-9]+")
ABSENT_IN = "--"
ABSENT_US = "-"

# The microseconds a byte spends on the wire before the master starts waiting for ACK, in two
# parts.
#
# SHIFT_NOMINAL_US is src/hal/ps2_master.pio's cycle count for a byte that waits for no ACK,
# converted at kCyclesPerBit cycles per bus bit and kBusClockHz. tests/test_trace_shift.py
# computes it from those sources and fails when this copy differs (R-PROTO-09).
#
# SHIFT_CALIBRATION_US is the calibration knob (ADR-0015), set by hand: the firmware's clock
# runs on the CPU, so every elapsed time also carries a few us of polling overhead. To set it,
# capture a loopback trace with ATT jumpered to ACK (ACK already low, so the true delay is
# zero): the last byte of each frame, which waits for no ACK, shows the shift time plus
# overhead, and every `ack` delay should read 0 to a few us. Delays that all sit well above
# zero mean the sum is too small.
SHIFT_NOMINAL_US = 37
SHIFT_CALIBRATION_US = 0
SHIFT_US = SHIFT_NOMINAL_US + SHIFT_CALIBRATION_US


class Unreadable(Exception):
    pass


def items(field, count):
    values = field.split(",")
    if len(values) != count:
        raise Unreadable()
    return values


def parse(line):
    """Returns (n, k, out, inn, us) with absent entries as None, or raises Unreadable."""
    match = LINE.fullmatch(line)
    if not match:
        raise Unreadable()
    n, k = int(match.group(1)), int(match.group(2))
    if n == 0 or k > n:
        raise Unreadable()
    out = items(match.group(3), n)
    inn = items(match.group(4), n)
    us = items(match.group(5), n)
    for i in range(n):
        if not HEX_BYTE.fullmatch(out[i]):
            raise Unreadable()
        if i < k:
            if not HEX_BYTE.fullmatch(inn[i]):
                raise Unreadable()
        elif inn[i] != ABSENT_IN:
            raise Unreadable()
        if i <= k:
            if not DECIMAL.fullmatch(us[i]):
                raise Unreadable()
        elif us[i] != ABSENT_US:
            raise Unreadable()
    return (n, k, out, [v if i < k else None for i, v in enumerate(inn)],
            [int(v) if i <= k else None for i, v in enumerate(us)])


def render(n, k, out, inn, us):
    if k == n:
        lines = ["frame: %d/%d bytes, complete" % (k, n)]
    elif k < n - 1:
        lines = ["frame: %d/%d bytes, aborted at byte %d: no ACK after %d us" % (k, n, k, us[k])]
    else:
        # The last byte waits for no ACK, so only its shift can have failed.
        lines = ["frame: %d/%d bytes, aborted at byte %d: did not complete after %d us"
                 % (k, n, k, us[k])]
    for i in range(n):
        row = "  byte %d: out %s in %s" % (i, out[i], inn[i] if inn[i] is not None else ABSENT_IN)
        if us[i] is None:
            lines.append(row + " not sent")
            continue
        row += " %d us" % us[i]
        waits_ack = i + 1 < n
        if i < k and waits_ack:
            row += " ack %d us" % max(0, us[i] - SHIFT_US)
        lines.append(row)
    return lines


def decode(handle):
    """Decodes every line of `handle` to standard output. Returns False if any was unreadable."""
    is_clean = True
    for raw in handle:
        line = raw.rstrip("\r\n")
        if not line.startswith(PREFIX):
            print(line, flush=True)
            continue
        try:
            print("\n".join(render(*parse(line))), flush=True)
        except Unreadable:
            print("unreadable trace line: " + line, flush=True)
            is_clean = False
    return is_clean


def main(paths):
    if not paths:
        return 0 if decode(sys.stdin) else 1
    is_clean = True
    for path in paths:
        with open(path, errors="replace") as handle:
            is_clean = decode(handle) and is_clean
    return 0 if is_clean else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
