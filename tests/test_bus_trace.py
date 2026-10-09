#!/usr/bin/env python3
"""Check both ends of the bus trace against the same hand-written `T1` lines, then prove the
check bites.

# RULE R-PROTO-08 — docs/constraints.md §Invariants — the firmware's trace formatter and the
#                   host decoder are both checked against the same hand-written T1 lines

The literals are tests/vectors/trace_session.txt (a session as the serial port would show it)
and tests/vectors/trace_session.rendered (the decoder's exact output for it), both written by
hand. Neither end produced either file.

- Formatter: tests/bus_trace_cases.cpp, which is not named test_*.cpp for the reason
  tests/test_ps2_codec.py gives, is compiled with src/core/bus_trace.cpp and run. Its `T1`
  lines must equal the session's well-formed `T1` lines, in order; its other lines are
  forwarded.
- Decoder: tools/trace_decode.py runs on the session. It must exit 1, because the session's
  last line is malformed on purpose, and its standard output must equal the rendered file.

The rejection cases copy the tree, mutate the formatter or the decoder into one realistic bug
each, and require the R-PROTO-08 line to turn to FAIL. Every mutation asserts its anchor
matched: a moved anchor mutates nothing, and the case would report the untouched tree's pass.
"""

import collections
import os
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True  # importing driver_support leaves no __pycache__
import driver_support  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES = os.path.join("tests", "bus_trace_cases.cpp")
FORMATTER = os.path.join("src", "core", "bus_trace.cpp")
DECODER = os.path.join("tools", "trace_decode.py")
SESSION = os.path.join("tests", "vectors", "trace_session.txt")
RENDERED = os.path.join("tests", "vectors", "trace_session.rendered")
# The flags `make test` uses, read from the Makefile (tests/driver_support.py).
CXXFLAGS = driver_support.cxxflags()
TRACE_PREFIX = "T1 "
# A mutant that loops forever must fail, not hang `make test`.
RUN_TIMEOUT_S = 30
RULE = "R-PROTO-08"
RULE_LABEL = ("R-PROTO-08 (formatter and decoder agree with the hand-written T1 lines in "
              "tests/vectors/)")

failures = []


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def read(tree, path):
    with open(os.path.join(tree, path)) as handle:
        return handle.read()


def run_formatter(tree):
    """Compile and run the cases file in `tree`. Returns (rc, T1 lines, other lines)."""
    cxx = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="bustrace-build-") as out:
        binary = os.path.join(out, "cases")
        build = subprocess.run(
            [cxx, *CXXFLAGS, "-o", binary, CASES, FORMATTER],
            cwd=tree, capture_output=True, text=True, timeout=300,
        )
        if build.returncode != 0:
            return build.returncode, [], (build.stdout + build.stderr).splitlines()
        try:
            run = subprocess.run([binary], cwd=tree, capture_output=True, text=True,
                                 timeout=RUN_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            return 1, [], ["the cases binary did not finish within %ds" % RUN_TIMEOUT_S]
    lines = (run.stdout + run.stderr).splitlines()
    return (run.returncode, [l for l in lines if l.startswith(TRACE_PREFIX)],
            [l for l in lines if not l.startswith(TRACE_PREFIX)])


def run_decoder(tree):
    """Run the decoder in `tree` on the session. Returns (rc, stdout)."""
    try:
        run = subprocess.run([sys.executable, DECODER, SESSION], cwd=tree, capture_output=True,
                             text=True, timeout=RUN_TIMEOUT_S)
    except subprocess.TimeoutExpired:
        return None, ""
    return run.returncode, run.stdout


def check(tree, is_verbose):
    """Prints the R-PROTO-08 line for `tree`, with the reasons when `is_verbose`. Returns ok."""
    reasons = []
    expected = [l for l in read(tree, SESSION).splitlines() if l.startswith(TRACE_PREFIX)]
    # The session's last T1 line is the malformed one; the formatter never writes it.
    expected = expected[:-1]
    rc, got, other = run_formatter(tree)
    if is_verbose:
        for line in other:
            if line.strip():
                print(line if line.startswith("  ") else "        " + line)
    if rc != 0:
        reasons.append("the formatter cases did not pass (rc=%d)" % rc)
    if got != expected:
        reasons.append("formatter lines differ from the session's T1 lines:")
        reasons.extend("    want %s\n          got  %s" % pair
                       for pair in zip(expected + [""] * len(got), got + [""] * len(expected))
                       if pair[0] != pair[1])
    rc, out = run_decoder(tree)
    if rc != 1:
        reasons.append("the decoder exited %r on the session, not 1 (the malformed line)" % rc)
    if out != read(tree, RENDERED):
        reasons.append("the decoder's output differs from %s" % RENDERED)
    is_ok = not reasons
    print("  %s %s" % ("ok:  " if is_ok else "FAIL:", RULE_LABEL))
    if is_verbose:
        for reason in reasons:
            print("        " + reason)
    return is_ok


def real_run():
    if not check(ROOT, True):
        failures.append(RULE)


Mutation = collections.namedtuple("Mutation", "path label anchor replacement")

MUTATIONS = [
    Mutation(FORMATTER, "the formatter prints - for the failed byte's elapsed time",
             "const bool is_attempted = i <= completed;",
             "const bool is_attempted = i < completed;"),
    Mutation(FORMATTER, "the formatter separates list items with ;",
             "= ',';", "= ';';"),
    Mutation(DECODER, "the decoder does not subtract SHIFT_US from the ACK delay",
             "max(0, us[i] - SHIFT_US)", "max(0, us[i])"),
    Mutation(DECODER, "the decoder treats the last byte as waiting for ACK",
             "waits_ack = i + 1 < n", "waits_ack = True"),
]


def reject(mutation):
    path, label, anchor, replacement = mutation
    tree = tempfile.mkdtemp(prefix="bustrace-")
    try:
        shutil.copytree(ROOT, os.path.join(tree, "t"),
                        ignore=shutil.ignore_patterns(".git", "build", "mut_*"))
        work = os.path.join(tree, "t")
        before = read(work, path)
        after = before.replace(anchor, replacement)
        if after == before:
            fail("%s rejection case (%s): the anchor %r is gone from %s, so nothing was mutated"
                 % (RULE, label, anchor, path))
            return False
        with open(os.path.join(work, path), "w") as handle:
            handle.write(after)
        with open(os.devnull, "w") as sink:
            stdout, sys.stdout = sys.stdout, sink
            try:
                is_ok = check(work, False)
            finally:
                sys.stdout = stdout
        if is_ok:
            fail("%s rejection case (%s): the check still passes" % (RULE, label))
            return False
        return True
    finally:
        shutil.rmtree(tree, ignore_errors=True)


def run_rejection_cases():
    passed, served = driver_support.run_rejection_cases(__file__, MUTATIONS, reject)
    total = len(MUTATIONS)
    if passed == total and total > 0:
        print("  ok:   rejection cases: %d/%d (each mutation turns the R-PROTO-08 line to FAIL), "
              "%d served from cache" % (passed, total, served))
    else:
        print("  FAIL: rejection cases: %d/%d, %d served from cache"
              % (passed, total, served))
        failures.append("rejection cases")


def main():
    real_run()
    run_rejection_cases()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
