#!/usr/bin/env python3
"""Compile and run the pin-table assertions against the real src/core/pins.h, check the table
against docs/wiring.md, then prove each rule line bites.

RULE R-SAFETY-01 — docs/constraints.md §Invariants — DATA and ACK are never push-pull outputs
RULE R-SAFETY-02 — docs/constraints.md §Invariants — every GPIO declared once in pins.h, and
                   docs/wiring.md names the same GPIO per signal
RULE R-SAFETY-03 — docs/constraints.md §Invariants — no reserved GPIO, no GPIO shared
LIVE R-SAFETY-02 (table)
LIVE R-SAFETY-02 (wiring doc)

This is the driver for tests/pin_table_cases.cpp, which is deliberately not named test_*.cpp:
the Makefile would otherwise build and run it a second time with nothing around it. Here it
is compiled with the flags `make test` uses, run, and its ok:/FAIL: lines forwarded verbatim.
pins.h is header-only, so only the cases file is compiled.

R-SAFETY-02 is checked by two independent real-run lines, hence the two LIVE labels: the
table half comes from the C++ run, the wiring-doc half from comparing that run's `pin:` lines
with docs/wiring.md's rows here.

The copied-tree rejection cases each copy the tree, break one file the way a real mistake
would, and require that rule's own line to turn to FAIL. Every mutation asserts that its
anchor matched first — a replace whose anchor has moved mutates nothing, and the case would
then report the untouched tree's pass as a caught mutation.
"""

import collections
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES = os.path.join("tests", "pin_table_cases.cpp")
WIRING = os.path.join("docs", "wiring.md")
PINS = os.path.join("src", "core", "pins.h")
# The flags make test uses (see the Makefile's CXXFLAGS).
CXXFLAGS = ["-std=c++23", "-Wall", "-Wextra", "-Werror", "-Og", "-g", "-UNDEBUG", "-Isrc"]

PIN_LINE = re.compile(r"^\s*pin:\s+([A-Z]+)\s+GP(\d+)\s*$", re.M)
# The row shape docs/wiring.md promises: `| DATA | GP2 | …`.
WIRING_ROW = re.compile(r"^\| ([A-Z]+) \| GP(\d+) \|", re.M)
WIRING_LABEL = "R-SAFETY-02 (wiring doc)"

failures = []


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def build_and_run(tree):
    """Compile the cases file in `tree` and run it. Returns (returncode, output).

    The binary goes to the system temp directory, never under `tree`, so the real run leaves
    nothing in the working tree (the 01-ps2-codec lesson).
    """
    cxx = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="pintable-build-") as out:
        binary = os.path.join(out, "cases")
        build = subprocess.run([cxx, *CXXFLAGS, "-o", binary, CASES],
                               cwd=tree, capture_output=True, text=True, timeout=300)
        if build.returncode != 0:
            return build.returncode, build.stdout + build.stderr
        run = subprocess.run([binary], cwd=tree, capture_output=True, text=True, timeout=300)
        return run.returncode, run.stdout + run.stderr


def wiring_line(tree, output):
    """The R-SAFETY-02 (wiring doc) verdict: the run's `pin:` lines equal wiring.md's rows.

    Returns (is_ok, detail). Both sides must list the same signals, each once, on the same
    GPIO. No `pin:` lines at all is a failure, not a vacuous match.
    """
    pins = PIN_LINE.findall(output)
    with open(os.path.join(tree, WIRING)) as handle:
        rows = WIRING_ROW.findall(handle.read())
    if not pins:
        return False, "the cases run printed no pin: lines"
    table = collections.Counter(pins)
    doc = collections.Counter(rows)
    if table != doc:
        only_table = sorted("%s GP%s" % p for p in (table - doc))
        only_doc = sorted("%s GP%s" % p for p in (doc - table))
        return False, "pins.h only: %s; wiring.md only: %s" % (only_table, only_doc)
    return True, "%d signals match" % len(pins)


def verdict_line(ok, label, detail):
    return "  %s %s: %s" % ("ok:  " if ok else "FAIL:", label, detail)


def run_tree(tree):
    """Everything one tree produces: the C++ lines plus the wiring-doc line."""
    rc, out = build_and_run(tree)
    ok, detail = wiring_line(tree, out)
    out = out.rstrip("\n") + "\n" + verdict_line(ok, WIRING_LABEL, detail) + "\n"
    return (rc == 0 and ok), out


def line_says_fail(output, label):
    """True when the real-run line starting with `label` says FAIL."""
    for line in output.splitlines():
        stripped = line.strip()
        if stripped.startswith("FAIL:") and stripped[len("FAIL:"):].strip().startswith(label):
            return True
    return False


# --- the real run -----------------------------------------------------------------------

def real_run():
    is_ok, out = run_tree(ROOT)
    for line in out.splitlines():
        if line.strip():
            print(line if line.startswith("  ") else "        " + line)
    if not is_ok:
        failures.append("the pin table does not pass against the real tree")


# --- copied-tree rejection cases --------------------------------------------------------

Mutation = collections.namedtuple("Mutation", "label rule rel anchor replacement")

MUTATIONS = [
    Mutation("DATA driven push-pull", "R-SAFETY-01", PINS,
             ".direction = Direction::Input,\n     .drive     = DriveMode::OpenDrainInputOnly},\n"
             "    {.gpio      = 3,",
             ".direction = Direction::Input,\n     .drive     = DriveMode::PushPull},\n"
             "    {.gpio      = 3,"),
    Mutation("ACK entry deleted", "R-SAFETY-02 (table)", PINS,
             "    {.gpio      = 6,\n     .signal    = Signal::Ack,\n"
             "     .direction = Direction::Input,\n"
             "     .drive     = DriveMode::OpenDrainInputOnly},\n",
             ""),
    Mutation("DATA's GPIO changed in wiring.md", WIRING_LABEL, WIRING,
             "| DATA | GP2 |", "| DATA | GP8 |"),
    Mutation("CLK moved to GPIO 25", "R-SAFETY-03", PINS,
             "    {.gpio      = 5,\n     .signal    = Signal::Clk,",
             "    {.gpio      = 25,\n     .signal    = Signal::Clk,"),
]


def reject(mutation):
    tmp = tempfile.mkdtemp(prefix="pintable-")
    try:
        work = os.path.join(tmp, "t")
        for rel in ("src", "tests", "docs"):
            shutil.copytree(os.path.join(ROOT, rel), os.path.join(work, rel),
                            ignore=shutil.ignore_patterns("mut_*"))
        path = os.path.join(work, mutation.rel)
        with open(path) as handle:
            before = handle.read()
        after = before.replace(mutation.anchor, mutation.replacement, 1)
        if after == before:
            fail("copied-tree rejection case %s: the anchor is gone from %s, so nothing was "
                 "mutated" % (mutation.label, mutation.rel))
            return False
        with open(path, "w") as handle:
            handle.write(after)
        is_ok, out = run_tree(work)
        if is_ok:
            fail("copied-tree rejection case %s: the tree still passes" % mutation.label)
            return False
        if not line_says_fail(out, mutation.rule):
            fail("copied-tree rejection case %s: the run failed but %s's own line did not say "
                 "FAIL" % (mutation.label, mutation.rule))
            return False
        return True
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def run_rejection_cases():
    passed = sum(1 for m in MUTATIONS if reject(m))
    total = len(MUTATIONS)
    if passed == total:
        print("  ok:   copied-tree rejection cases: %d/%d (each mutation flips its own line)"
              % (passed, total))
    else:
        print("  FAIL: copied-tree rejection cases: %d/%d" % (passed, total))
        failures.append("copied-tree rejection cases")


def main():
    real_run()
    run_rejection_cases()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
