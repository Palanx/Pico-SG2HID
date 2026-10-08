#!/usr/bin/env python3
"""Check that the trace decoder's nominal shift time is the PIO program's, then prove the
check bites.

# RULE R-PROTO-09 — docs/constraints.md §Invariants — the decoder's SHIFT_NOMINAL_US equals
#                   ps2_master.pio's no-ACK cycles at kCyclesPerBit and kBusClockHz

The cycles are counted by walking src/hal/ps2_master.pio from `.wrap_target` to `.wrap` with
the ACK flag (`out y, …`) clear. Only the instructions and `jmp` conditions that program uses
are understood; anything else on the path is a FAIL, never a guess. SHIFT_CALIBRATION_US is
ADR-0015's hand-set knob and is not checked.

The cases mutate the four texts in memory. Each rejection case must turn the R-PROTO-09 line
to FAIL, the accept case must leave it ok, and every mutation asserts its anchor occurs
exactly once.
"""

import collections
import fractions
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PIO = os.path.join("src", "hal", "ps2_master.pio")
PORT = os.path.join("src", "hal", "pio_port.cpp")
PROTOCOL = os.path.join("src", "core", "ps2_protocol.h")
DECODER = os.path.join("tools", "trace_decode.py")
FILES = (PIO, PORT, PROTOCOL, DECODER)
RULE_LABEL = ("R-PROTO-09 (SHIFT_NOMINAL_US equals ps2_master.pio's no-ACK cycles at "
              "kCyclesPerBit and kBusClockHz)")
US_PER_SECOND = 1000000
# A walk this long is a loop the conditions below cannot leave.
MAX_STEPS = 10000
LABEL = re.compile(r"^(\w+):\s*(.*)$")
DELAY = re.compile(r"\[(\d+)\]\s*$")
SIDE = re.compile(r"\s+side\s+\d+")
NOMINAL = re.compile(r"^SHIFT_NOMINAL_US = (\d+)$", re.M)

failures = []


class Unreadable(Exception):
    pass


def read(path):
    with open(os.path.join(ROOT, path)) as handle:
        return handle.read()


def pio_cycles(text):
    """Cycles from `.wrap_target` to `.wrap` for a byte that waits for no ACK."""
    program = []
    labels = {}
    start = end = None
    for raw in text.splitlines():
        line = raw.split(";", 1)[0].strip()
        if not line or line.startswith((".program", ".side_set")):
            continue
        if line == ".wrap_target":
            start = len(program)
        elif line == ".wrap":
            end = len(program)
        elif LABEL.match(line):
            label = LABEL.match(line)
            labels[label.group(1)] = len(program)
            if label.group(2):
                program.append(label.group(2))
        else:
            program.append(line)
    if start is None or end is None:
        raise Unreadable("no .wrap_target or .wrap in the program")
    x = y = 0
    cycles = 0
    pc = start
    # The pass after the last allowed instruction only checks for .wrap.
    for executed in range(MAX_STEPS + 1):
        if pc == end:
            return cycles
        if executed == MAX_STEPS:
            break
        if not 0 <= pc < len(program):
            raise Unreadable("the walk left the program before .wrap")
        line = program[pc]
        delay = DELAY.search(line)
        if "[" in line and not delay:
            raise Unreadable("a delay that is not the last token in %r" % line)
        cycles += 1 + (int(delay.group(1)) if delay else 0)
        words = SIDE.sub("", DELAY.sub("", line)).replace(",", " ").split()
        pc += 1
        try:
            if words[0] == "set" and words[1] == "x":
                x = int(words[2])
            elif words[0] == "out" and words[1] == "y":
                y = 0
            elif words[0] == "jmp":
                condition, target = ((words[1], words[2]) if len(words) == 3
                                     else (None, words[1]))
        except (IndexError, ValueError):
            raise Unreadable("an operand it cannot read in %r" % line)
        if words[0] == "jmp":
            if target not in labels:
                raise Unreadable("jmp to unknown label %r" % target)
            if condition is None:
                is_taken = True
            elif condition == "x--":
                is_taken = x != 0
                x -= 1
            elif condition == "!y":
                is_taken = y == 0
            else:
                raise Unreadable("jmp condition %r is not understood" % condition)
            if is_taken:
                pc = labels[target]
    raise Unreadable("the walk did not reach .wrap in %d instructions" % MAX_STEPS)


def constant(text, name):
    match = re.search(r"constexpr std::uint32_t %s\s*=\s*(\d+);" % name, text)
    if not match:
        raise Unreadable("no constexpr std::uint32_t %s" % name)
    return int(match.group(1))


def check(texts, is_verbose):
    """Prints the R-PROTO-09 line for `texts`, with the reason when `is_verbose`. Returns ok."""
    reason = None
    try:
        cycles = pio_cycles(texts[PIO])
        hertz = constant(texts[PORT], "kCyclesPerBit") * constant(texts[PROTOCOL], "kBusClockHz")
        nominal = fractions.Fraction(cycles * US_PER_SECOND, hertz)
        match = NOMINAL.search(texts[DECODER])
        if not match:
            raise Unreadable("no SHIFT_NOMINAL_US line in the decoder")
        if nominal != int(match.group(1)):
            reason = "%d cycles give %s us; the decoder says %s" % (cycles, nominal,
                                                                    match.group(1))
    except Unreadable as error:
        reason = str(error)
    is_ok = reason is None
    if is_verbose:
        print("  %s %s" % ("ok:  " if is_ok else "FAIL:", RULE_LABEL))
        if reason:
            print("        " + reason)
    return is_ok


Mutation = collections.namedtuple("Mutation", "path label anchor replacement")

REJECTIONS = [
    Mutation(PIO, "one instruction added", "set x, 7", "set x, 7\n    nop"),
    Mutation(PIO, "a delay changed", "side 0 [1]", "side 0 [2]"),
    Mutation(PROTOCOL, "the bus clock changed", "kBusClockHz = 250000", "kBusClockHz = 125000"),
    Mutation(PORT, "the divider changed", "kCyclesPerBit  = 4", "kCyclesPerBit  = 8"),
    Mutation(DECODER, "the copy drifted", "SHIFT_NOMINAL_US = 37", "SHIFT_NOMINAL_US = 38"),
]
ACCEPT = Mutation(DECODER, "the calibration knob moved", "SHIFT_CALIBRATION_US = 0",
                  "SHIFT_CALIBRATION_US = 2")


def mutated(texts, mutation):
    """`texts` with `mutation` applied, or None unless its anchor occurs exactly once."""
    before = texts[mutation.path]
    count = before.count(mutation.anchor)
    if count != 1:
        print("  FAIL: %s: the anchor %r occurs %d times in %s, not once"
              % (mutation.label, mutation.anchor, count, mutation.path))
        return None
    return dict(texts, **{mutation.path: before.replace(mutation.anchor, mutation.replacement)})


def run_cases(texts):
    passed = 0
    for mutation in REJECTIONS:
        work = mutated(texts, mutation)
        if work is None:
            continue
        if check(work, False):
            print("  FAIL: rejection case (%s): the check still passes" % mutation.label)
        else:
            passed += 1
    total = len(REJECTIONS)
    if passed == total:
        print("  ok:   rejection cases: %d/%d (each mutation turns the R-PROTO-09 line to FAIL)"
              % (passed, total))
    else:
        print("  FAIL: rejection cases: %d/%d" % (passed, total))
        failures.append("rejection cases")
    work = mutated(texts, ACCEPT)
    if work is not None and check(work, False):
        print("  ok:   accept case: %s, and the check still passes" % ACCEPT.label)
    else:
        print("  FAIL: accept case: %s" % ACCEPT.label)
        failures.append("accept case")


def main():
    texts = {path: read(path) for path in FILES}
    if not check(texts, True):
        failures.append("R-PROTO-09")
    run_cases(texts)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
