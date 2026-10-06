#!/usr/bin/env python3
"""Compile and run the emulator-model assertions against the real src/emu/sg_model.cpp, then
prove they bite.

# RULE R-EMU-01 — docs/constraints.md §Invariants — the emulator answers a poll as the vectors say
# RULE R-EMU-02 — docs/constraints.md §Invariants — each fault and command changes only its part

The driver for tests/emulator_cases.cpp, which is not named test_*.cpp for the reason
tests/test_bus_frame.py gives. It is compiled with src/emu/sg_model.cpp, run, and its ok:/FAIL:
lines forwarded verbatim. The expected bytes are the hand-written vectors under tests/vectors/,
never anything from src/core/ (ADR-0004, R-PROTO-05).

The rejection cases copy the tree, mutate the model into one realistic bug each, and require
that bug's rule line to turn to FAIL. Every mutation asserts its anchor matched: a moved anchor
mutates nothing, and the case would report the untouched tree's pass.
"""

import collections
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES = os.path.join("tests", "emulator_cases.cpp")
MODEL = os.path.join("src", "emu", "sg_model.cpp")
CXXFLAGS = ["-std=c++23", "-Wall", "-Wextra", "-Werror", "-Og", "-g", "-UNDEBUG", "-Isrc"]
# A mutant that loops forever must fail, not hang `make test`.
RUN_TIMEOUT_S = 30

failures = []


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def build_and_run(tree):
    """Compile the cases file in `tree` against its model and run it. Returns (rc, output)."""
    cxx = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="emulator-build-") as out:
        binary = os.path.join(out, "cases")
        build = subprocess.run(
            [cxx, *CXXFLAGS, "-o", binary, CASES, MODEL],
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
        fail("the emulator cases do not pass against the real src/emu/ (rc=%d)" % rc)


Mutation = collections.namedtuple("Mutation", "rule label anchor replacement")

MUTATIONS = [
    Mutation("R-EMU-01", "the frame's last byte is ACKed too",
             "!m_is_broken && index < last_index()", "!m_is_broken && index <= last_index()"),
    Mutation("R-EMU-02", "the id fault is ignored",
             "if ( m_fault.kind == FaultKind::Id ) {", "if ( false ) {"),
]


def reject(mutation):
    rule, label, anchor, replacement = mutation
    tree = tempfile.mkdtemp(prefix="emulator-")
    try:
        work = os.path.join(tree, "t")
        for rel in ("src", "tests"):
            shutil.copytree(os.path.join(ROOT, rel), os.path.join(work, rel),
                            ignore=shutil.ignore_patterns("mut_*"))
        path = os.path.join(work, MODEL)
        with open(path) as handle:
            before = handle.read()
        after = before.replace(anchor, replacement)
        if after == before:
            fail("%s rejection case (%s): the anchor %r is gone from %s, so nothing was mutated"
                 % (rule, label, anchor.strip(), MODEL))
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
