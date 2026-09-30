#!/usr/bin/env python3
"""Compile and run the frame-loop assertions against the real src/hal/bus_frame.cpp, then prove
they bite.

# RULE R-SAFETY-07 — docs/constraints.md §Invariants — every error and timeout path releases ATT
# RULE R-PROTO-06 — docs/constraints.md §Invariants — the master waits for ACK after every byte
#                   but the last

The driver for tests/bus_frame_cases.cpp, which is not named test_*.cpp for the reason
tests/test_ps2_codec.py gives. It is compiled with src/hal/bus_frame.cpp and a fake port, with
the flags `make test` uses, run, and its ok:/FAIL: lines forwarded verbatim.

The rejection cases copy the tree, mutate the frame loop into one realistic bug each, and
require that bug's rule line to turn to FAIL. Every mutation asserts its anchor matched: a
moved anchor mutates nothing, and the case would report the untouched tree's pass.
"""

import collections
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES = os.path.join("tests", "bus_frame_cases.cpp")
FRAME = os.path.join("src", "hal", "bus_frame.cpp")
# The flags make test uses (the Makefile's CXXFLAGS).
CXXFLAGS = ["-std=c++23", "-Wall", "-Wextra", "-Werror", "-Og", "-g", "-UNDEBUG", "-Isrc"]
# A mutant that loops forever must fail, not hang `make test`.
RUN_TIMEOUT_S = 30

failures = []


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def build_and_run(tree):
    """Compile the cases file in `tree` against its frame loop and run it. Returns (rc, output)."""
    cxx = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="busframe-build-") as out:
        binary = os.path.join(out, "cases")
        build = subprocess.run(
            [cxx, *CXXFLAGS, "-o", binary, CASES, FRAME],
            cwd=tree, capture_output=True, text=True, timeout=300,
        )
        if build.returncode != 0:
            return build.returncode, build.stdout + build.stderr
        try:
            run = subprocess.run([binary], cwd=tree, capture_output=True, text=True,
                                 timeout=RUN_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            return 1, "the cases binary did not finish within %ds" % RUN_TIMEOUT_S
        return run.returncode, run.stdout + run.stderr


def rule_verdicts(output):
    """Map rule id -> True when its line says ok, False when it says FAIL."""
    verdicts = {}
    for line in output.splitlines():
        stripped = line.strip()
        for prefix, verdict in (("ok:", True), ("FAIL:", False)):
            if stripped.startswith(prefix) and "R-" in stripped:
                rule = stripped.split("R-", 1)[1].split()[0].split("(")[0].rstrip(":,")
                verdicts["R-" + rule] = verdict
    return verdicts


def real_run():
    rc, out = build_and_run(ROOT)
    for line in out.splitlines():
        if line.strip():
            print(line if line.startswith("  ") else "        " + line)
    if rc != 0:
        fail("the frame-loop cases do not pass against the real src/hal/ (rc=%d)" % rc)


Mutation = collections.namedtuple("Mutation", "rule label anchor replacement")

MUTATIONS = [
    Mutation("R-SAFETY-07", "ATT is never released",
             "    att_release();\n", "\n"),
    Mutation("R-SAFETY-07", "the failure path returns before releasing ATT",
             "        if ( !in ) {\n            break;",
             "        if ( !in ) {\n            return done;"),
    Mutation("R-PROTO-06", "the last byte also waits for ACK",
             "should_wait_ack = done + 1 < frame.size();", "should_wait_ack = true;"),
    Mutation("R-PROTO-06", "no byte waits for ACK",
             "should_wait_ack = done + 1 < frame.size();", "should_wait_ack = false;"),
    Mutation("R-PROTO-06", "the loop goes on after a failed byte",
             "        if ( !in ) {\n            break;",
             "        if ( !in ) {\n            ++done;\n            continue;"),
]


def reject(mutation):
    rule, label, anchor, replacement = mutation
    tree = tempfile.mkdtemp(prefix="busframe-")
    try:
        shutil.copytree(ROOT, os.path.join(tree, "t"),
                        ignore=shutil.ignore_patterns(".git", "build", "mut_*"))
        work = os.path.join(tree, "t")
        path = os.path.join(work, FRAME)
        with open(path) as handle:
            before = handle.read()
        after = before.replace(anchor, replacement)
        if after == before:
            fail("%s rejection case (%s): the anchor %r is gone from %s, so nothing was mutated"
                 % (rule, label, anchor.strip(), FRAME))
            return False
        with open(path, "w") as handle:
            handle.write(after)

        rc, out = build_and_run(work)
        verdicts = rule_verdicts(out)
        if rc == 0:
            fail("%s rejection case (%s): the suite still passes" % (rule, label))
            return False
        if verdicts.get(rule) is not False:
            fail("%s rejection case (%s): the run failed but %s's own line did not say FAIL "
                 "(got %r)" % (rule, label, rule, verdicts.get(rule)))
            return False
        return True
    finally:
        shutil.rmtree(tree, ignore_errors=True)


def run_rejection_cases():
    passed = sum(1 for m in MUTATIONS if reject(m))
    total = len(MUTATIONS)
    if passed == total and total > 0:
        print("  ok:   rejection cases: %d/%d (each mutation flips its own rule to FAIL)"
              % (passed, total))
    else:
        print("  FAIL: rejection cases: %d/%d" % (passed, total))
        failures.append("rejection cases")


def main():
    real_run()
    run_rejection_cases()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
