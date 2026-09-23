#!/usr/bin/env python3
"""Compile and run the PS2 codec assertions against the real src/core/, then prove they bite.

# RULE R-PROTO-02 — docs/constraints.md §Invariants — a cut-short frame yields no partial
#                   report; the link transitions to Absent
# RULE R-PROTO-03 — docs/constraints.md §Invariants — an unknown controller id is refused
# RULE R-PROTO-04 — docs/constraints.md §Invariants — the whammy comes only from an
#                   analog frame; any other frame yields the axis at rest

This is the driver for tests/ps2_codec_cases.cpp, which is deliberately not named test_*.cpp:
the Makefile would otherwise build and run it a second time with nothing around it. Here it
is compiled against src/core/*.cpp with the same flags `make test` uses, run, and its
ok:/FAIL: lines forwarded verbatim.

Forwarding matters for the accounting property in tests/test_checks_are_live.py: the rule
lines this file declares above must be produced by the REAL run, and a case line must be
distinguishable from a real-run line. The house convention does that by making case lines say
they are cases ("rejection cases: 3/3"), which the real run's lines never do.

The rejection cases below are what makes a rule line mean something. Each copies the tree to
a temp directory, mutates src/core/ so the codec stops refusing, recompiles, and requires that
rule's line to turn to FAIL. Without them a rule line proves only that the file printed a
string. Every mutation asserts that the replacement changed the text before it is used — a
str.replace whose anchor has moved mutates nothing, the recompile succeeds, and the case
reports the exit 0 it was handed. That is 00-scaffold's §Owed item, and it is why the assert
is not optional.
"""

import collections
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES = os.path.join("tests", "ps2_codec_cases.cpp")
# The flags make test uses (see the Makefile's CXXFLAGS), so a warning that would fail the
# build there fails here too rather than surfacing two steps later.
CXXFLAGS = ["-std=c++23", "-Wall", "-Wextra", "-Werror", "-Og", "-g", "-UNDEBUG", "-Isrc"]

failures = []


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def build_and_run(tree):
    """Compile the cases file in `tree` against its src/core/ and run it.

    Returns (returncode, output). A compile failure is returned as a non-zero code with the
    compiler's output, never raised: a rejection case's mutation is *expected* to break
    something, and distinguishing "refused to compile" from "compiled and failed" is the
    caller's business.

    The binary goes to the system temp directory, NOT under `tree`, and inside a context
    manager so it is removed on the exception paths too. The first version of this put it in
    `tempfile.mkdtemp(dir=tree)`, which for the real run is the repository itself — every
    `make test` left two `tmp*/` directories behind in the working tree, where
    `git ls-files --others` and the gitleaks tree scan both go looking. Same shape as the
    `mut_*.sh` orphans that tests/test_checks_are_live.py has to sweep; not worth repeating,
    so this one cannot create them in the first place.
    """
    cxx = os.environ.get("CXX", "c++")
    core = sorted(
        os.path.join("src", "core", f)
        for f in os.listdir(os.path.join(tree, "src", "core"))
        if f.endswith(".cpp")
    )
    with tempfile.TemporaryDirectory(prefix="ps2codec-build-") as out:
        binary = os.path.join(out, "cases")
        build = subprocess.run(
            [cxx, *CXXFLAGS, "-o", binary, CASES, *core],
            cwd=tree, capture_output=True, text=True, timeout=300,
        )
        if build.returncode != 0:
            return build.returncode, build.stdout + build.stderr
        run = subprocess.run([binary], cwd=tree, capture_output=True, text=True, timeout=300)
        return run.returncode, run.stdout + run.stderr


def rule_verdicts(output):
    """Map rule id -> True when its line says ok, False when it says FAIL.

    Read off the real run's own lines rather than the exit code, so a case can assert that
    ONE rule flipped instead of "something, somewhere, failed".
    """
    verdicts = {}
    for line in output.splitlines():
        stripped = line.strip()
        for prefix, verdict in (("ok:", True), ("FAIL:", False)):
            if stripped.startswith(prefix) and "R-" in stripped:
                rule = stripped.split("R-", 1)[1].split()[0].split("(")[0].rstrip(":,")
                verdicts["R-" + rule] = verdict
    return verdicts


# --- the real run -----------------------------------------------------------------------

def real_run():
    rc, out = build_and_run(ROOT)
    for line in out.splitlines():
        if line.strip():
            print(line if line.startswith("  ") else "        " + line)
    if rc != 0:
        fail("the codec cases do not pass against the real src/core/ (rc=%d)" % rc)
    return out


# --- rejection cases --------------------------------------------------------------------

# One mutation per rule. Each is the realistic bug — the codec stops refusing the thing its
# rule says it refuses — and each is chosen so the mutated tree still COMPILES and still has
# defined behaviour. A mutant that fails to build, or that reads past a buffer, exits non-zero
# for a reason that has nothing to do with the rule, and `reject` below rejects that too: it
# requires the rule's own line to say FAIL, not merely a non-zero exit.
# R-CLEAN-02 caps a function at three parameters and says the rest become a struct; a
# namedtuple is this language's struct. The cap binds here even though clang-tidy, which
# checks the rule, reads no Python — see spec.md §Out of scope.
Mutation = collections.namedtuple("Mutation", "rule label rel anchor replacement")

MUTATIONS = [
    Mutation(
        "R-PROTO-03",
        "accept an unknown controller id as a digital one",
        os.path.join("src", "core", "ps2_protocol.h"),
        "        return std::nullopt;",
        "        return ControllerId::Digital;",
    ),
    Mutation(
        "R-PROTO-04",
        "read the whammy out of a digital payload",
        os.path.join("src", "core", "guitar_state.cpp"),
        "    if ( frame.id == ControllerId::Analog ) {",
        "    if ( true ) {",
    ),
    Mutation(
        "R-PROTO-02",
        "trust the bytes that arrived instead of the length the header announced",
        os.path.join("src", "core", "ps2_frame.cpp"),
        "    const std::size_t expected_len = frame_len( *id );",
        "    const std::size_t expected_len = kPrefixLen;",
    ),
]


def reject(mutation):
    """Mutate one file so the codec stops refusing, and require its rule's line to say FAIL."""
    rule, label, rel, anchor, replacement = mutation
    tree = tempfile.mkdtemp(prefix="ps2codec-")
    try:
        shutil.copytree(ROOT, os.path.join(tree, "t"),
                        ignore=shutil.ignore_patterns(".git", "build", "mut_*"))
        work = os.path.join(tree, "t")
        path = os.path.join(work, rel)
        with open(path) as handle:
            before = handle.read()
        after = before.replace(anchor, replacement)
        # The assert 00-scaffold owed. A moved anchor mutates nothing and the case would
        # then report the untouched tree's exit 0 as a pass.
        if after == before:
            fail("%s rejection case: the anchor %r is gone from %s, so nothing was mutated"
                 % (rule, anchor.strip(), rel))
            return False
        with open(path, "w") as handle:
            handle.write(after)

        rc, out = build_and_run(work)
        verdicts = rule_verdicts(out)
        if rc == 0:
            fail("%s rejection case (%s): the suite still passes, so nothing demonstrates it"
                 % (rule, label))
            return False
        if verdicts.get(rule) is not False:
            fail("%s rejection case (%s): the run failed but %s's own line did not say FAIL "
                 "(got %r) — the mutation is caught by something else"
                 % (rule, label, rule, verdicts.get(rule)))
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
