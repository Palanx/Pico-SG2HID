"""Shared by the five MUTATIONS drivers: tests/test_ps2_codec.py, tests/test_bus_frame.py,
tests/test_bus_trace.py, tests/test_pin_table.py and tests/test_emulator.py.

- cxxflags( ) asks the Makefile for its CXXFLAGS, so a driver's mutants build with the same
  flags as `make test` and the two cannot drift.
- run_rejection_cases( ) skips a mutant already recorded as killed under an unchanged key, the
  shape tests/test_checks_are_live.py uses (27-live-mutant-cache): an empty marker file per kill
  under the gitignored build/, never a survivor, and FULL=1 bypasses the lookup.

Not a test file: the Makefile only runs tests/test_*.py. A driver sets sys.dont_write_bytecode
before importing this, so the import leaves no __pycache__ behind.
"""

import functools
import hashlib
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_ROOT = os.path.join(ROOT, "build", "mutant-cache")
# ponytail: one key over every file under these trees, so any edit there invalidates every
# suite. A per-suite dependency list is the upgrade if the cost of that shows.
KEYED_TREES = ("src", "tests", "tools")
KEYED_FILES = (os.path.join("docs", "wiring.md"),)
SKIPPED_DIRS = ("__pycache__",)
SKIPPED_PREFIX = "mut_"


def cxxflags():
    """The Makefile's CXXFLAGS as a list of words. Exits the driver when make fails."""
    run = subprocess.run(["make", "-s", "--no-print-directory", "print-CXXFLAGS"],
                         cwd=ROOT, capture_output=True, text=True)
    if run.returncode != 0:
        sys.exit("driver_support: `make print-CXXFLAGS` exited %d: %s"
                 % (run.returncode, (run.stdout + run.stderr).strip()))
    return run.stdout.split()


def is_full():
    """True when FULL=1. Exits the driver on any value but unset, empty or 1."""
    full = os.environ.get("FULL", "")
    if full not in ("", "1"):
        sys.exit("FULL must be unset, empty or 1, not %r" % full)
    return full == "1"


def keyed_paths():
    paths = list(KEYED_FILES)
    for tree in KEYED_TREES:
        for d, subdirs, files in os.walk(os.path.join(ROOT, tree)):
            subdirs[:] = [s for s in subdirs if s not in SKIPPED_DIRS]
            paths.extend(os.path.relpath(os.path.join(d, f), ROOT)
                         for f in files if not f.startswith(SKIPPED_PREFIX))
    return sorted(paths)


@functools.lru_cache(maxsize=None)
def tree_digest():
    """Hash of the compiler, the flags and every keyed file; computed once per run."""
    h = hashlib.sha256()
    h.update(os.environ.get("CXX", "c++").encode() + b"\0")
    h.update(" ".join(cxxflags()).encode() + b"\0")
    for rel in keyed_paths():
        with open(os.path.join(ROOT, rel), "rb") as handle:
            h.update(rel.encode() + b"\0" + handle.read() + b"\0")
    return h.digest()


def mutant_key(mutation):
    return hashlib.sha256(tree_digest() + repr(tuple(mutation)).encode()).hexdigest()


def run_rejection_cases(driver, mutations, reject):
    """Runs `reject` on each mutation not already recorded as killed. Returns (passed, served).

    `driver` is the calling file's __file__; its basename names the cache directory.
    """
    is_bypassed = is_full()
    cache = os.path.join(CACHE_ROOT, os.path.basename(driver))
    passed = served = 0
    for mutation in mutations:
        marker = os.path.join(cache, mutant_key(mutation))
        if not is_bypassed and os.path.exists(marker):
            passed += 1
            served += 1
        elif reject(mutation):
            passed += 1
            os.makedirs(cache, exist_ok=True)
            open(marker, "w").close()
    return passed, served
