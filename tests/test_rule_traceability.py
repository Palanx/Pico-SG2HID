#!/usr/bin/env python3
"""Rule/test traceability meta-test.

RULE R-PROC-01 — docs/constraints.md §Invariants — every rule has exactly one binding,
                 and the binding agrees with the tree in both directions.

The grammar it enforces is fixed by docs/adr/0005-rule-test-traceability.md:

    - **R-AREA-NN** — <rule text> — test: `path`
    - **R-AREA-NN** — <rule text> — manual: <why it cannot be machine-checked>
    - **R-AREA-NN** — <rule text> — planned: <phase-id>

Every path this reads is a parameter, never a constant. That is what lets the rejection
cases at the bottom feed it deliberately broken catalogues: a checker that can only look
at the real repo cannot be shown to reject anything, and on a tree with no product code
almost every check here passes vacuously. Standard library only.
"""

import os
import re
import shutil
import sys
import tempfile

RULE_LINE = re.compile(
    r"^- \*\*(R-[A-Z]+-\d{2})\*\* — (.+?) — (test|manual|planned): (.+)$", re.M
)
ANY_RULE_LINE = re.compile(r"^- \*\*R-.*$", re.M)
RULE_ID = re.compile(r"R-[A-Z]+-\d{2}")
MARKER = re.compile(r"RULE (R-[A-Z]+-\d{2})")
PHASE_ROW = re.compile(r"^\| ([0-9]{2}-[a-z0-9-]+) \|(.*)\|\s*$", re.M)

# The rejection fixtures below need marker-shaped text. Writing it literally would plant
# real markers in this very file, which the walk would then find and report as naming
# undeclared rules — a checker that fails on its own test data. Built at runtime instead.
_M = "RU" "LE"


# Markers are code, not prose. docs/ holds the catalogue itself, the ADR that defines the
# grammar (which quotes a marker as an example) and the phase specs; .claude/commands/ is
# the workflow package. Counting those as markers would make the catalogue cite itself.
SKIP_PREFIXES = ("docs/", ".claude/commands/", ".git/", "build/")
SKIP_DIRS = {".git", "build", "node_modules"}


def read(path):
    with open(path, encoding="utf-8", errors="ignore") as handle:
        return handle.read()


def collect_markers(root):
    """Map rule id -> set of files carrying a `RULE <id>` marker."""
    found = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, root)
            if rel.startswith(SKIP_PREFIXES):
                continue
            try:
                text = read(full)
            except OSError:
                continue
            for rule_id in MARKER.findall(text):
                found.setdefault(rule_id, set()).add(rel)
    return found


def phase_status(phases_text):
    """Map phase id -> status, from the PHASES.md table."""
    status = {}
    for phase_id, rest in PHASE_ROW.findall(phases_text):
        columns = [c.strip() for c in rest.split("|")]
        if columns:
            status[phase_id] = columns[-1]
    return status


def check(root, constraints, phases, claude):
    """Return a list of human-readable problems. Empty list means consistent."""
    problems = []
    catalogue = read(constraints)
    rules = RULE_LINE.findall(catalogue)
    ids = [r[0] for r in rules]

    for line in ANY_RULE_LINE.findall(catalogue):
        if not RULE_LINE.match(line):
            seen = RULE_ID.search(line)
            problems.append(
                f"{seen.group(0) if seen else '<no id found>'}: unparsable rule line in "
                f"{constraints} (see ADR-0005 for the grammar): " + line[:90]
            )

    for rule_id in sorted(set(ids)):
        if ids.count(rule_id) > 1:
            problems.append(
                f"{rule_id}: declared {ids.count(rule_id)} times in {constraints}, must be once"
            )

    statuses = phase_status(read(phases))
    markers = collect_markers(root)

    for rule_id, _text, binding, value in rules:
        value = value.strip().strip("`")
        if binding == "test":
            target = os.path.join(root, value)
            if not os.path.exists(target):
                problems.append(f"{rule_id}: bound to '{value}', which does not exist")
            elif f"RULE {rule_id}" not in read(target):
                problems.append(
                    f"{rule_id}: '{value}' carries no 'RULE {rule_id}' marker — "
                    "the test cannot be traced back to the rule"
                )
        elif binding == "planned":
            if value not in statuses:
                problems.append(
                    f"{rule_id}: planned in phase '{value}', which is not in {phases}"
                )
            elif statuses[value] == "done":
                problems.append(
                    f"{rule_id}: planned in phase '{value}', which {phases} marks done — "
                    "the phase closed without settling the rule"
                )
        elif binding == "manual":
            marked = markers.get(rule_id, set())
            if marked:
                problems.append(
                    f"{rule_id}: marked manual but has a marker in {sorted(marked)} — "
                    "if it is testable, bind it with test:"
                )

    for rule_id, files in markers.items():
        if rule_id not in ids:
            problems.append(
                f"marker {rule_id} in {sorted(files)} names no rule declared in the catalogue"
            )

    for rule_id in sorted(set(RULE_ID.findall(read(claude)))):
        if rule_id not in ids:
            problems.append(
                f"{rule_id}: mentioned in {claude} but not declared in {constraints}"
            )

    return problems


# --------------------------------------------------------------------------------------
# Rejection cases. Each builds a temp repo with exactly one defect and asserts that the
# checker names it. Without these, a checker that returned [] unconditionally would look
# identical to a passing one on this tree.
# --------------------------------------------------------------------------------------

MINIMAL_PHASES = """| id | goal | depends | acceptance | status |
|----|------|---------|------------|--------|
| 00-scaffold | x | - | y | in-progress |
| 09-closed | x | - | y | done |
"""


def make_repo(tmp, rules_body, claude_body="# c\n", files=None):
    os.makedirs(os.path.join(tmp, "docs"), exist_ok=True)
    os.makedirs(os.path.join(tmp, "tests"), exist_ok=True)
    constraints = os.path.join(tmp, "docs", "constraints.md")
    phases = os.path.join(tmp, "docs", "phases.md")
    claude = os.path.join(tmp, "CLAUDE.md")
    with open(constraints, "w") as handle:
        handle.write("## Invariants\n\n" + rules_body)
    with open(phases, "w") as handle:
        handle.write(MINIMAL_PHASES)
    with open(claude, "w") as handle:
        handle.write(claude_body)
    for rel, body in (files or {}).items():
        full = os.path.join(tmp, rel)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, "w") as handle:
            handle.write(body)
    return constraints, phases, claude


CASES = [
    (
        "test: path that does not exist",
        "- **R-X-01** — text — test: `tests/missing.sh`\n",
        {},
        "does not exist",
    ),
    (
        "test: file present but with no marker",
        "- **R-X-01** — text — test: `tests/t.sh`\n",
        {"tests/t.sh": "# no marker here\n"},
        f"carries no '{_M} R-X-01' marker",
    ),
    (
        "marker naming an undeclared rule",
        "- **R-X-01** — text — test: `tests/t.sh`\n",
        {"tests/t.sh": f"# {_M} R-X-01\n# {_M} R-GHOST-99\n"},
        "names no rule declared",
    ),
    (
        "manual: rule that carries a marker anyway",
        "- **R-X-01** — text — manual: because\n",
        {"tests/t.sh": f"# {_M} R-X-01\n"},
        "marked manual but has a marker",
    ),
    (
        "planned: phase that is not in the index",
        "- **R-X-01** — text — planned: 99-nonexistent\n",
        {},
        "which is not in",
    ),
    (
        "planned: phase that is already done",
        "- **R-X-01** — text — planned: 09-closed\n",
        {},
        "marks done",
    ),
    (
        "duplicate rule id",
        "- **R-X-01** — a — manual: because\n- **R-X-01** — b — manual: because\n",
        {},
        "declared 2 times",
    ),
    (
        "unparsable rule line",
        "- **R-X-01** — text with no binding at all\n",
        {},
        "unparsable rule line",
    ),
    (
        "CLAUDE.md cites a rule the catalogue does not declare",
        "- **R-X-01** — text — manual: because\n",
        {},
        "R-Y-02: mentioned in",
    ),
]


def run_rejection_cases():
    passed = 0
    for label, rules_body, files, expected in CASES:
        tmp = tempfile.mkdtemp(prefix="ruletrace-")
        try:
            claude_body = "R-Y-02\n" if "CLAUDE.md cites" in label else "# c\n"
            constraints, phases, claude = make_repo(tmp, rules_body, claude_body, files)
            problems = check(tmp, constraints, phases, claude)
            if any(expected in p for p in problems):
                passed += 1
            else:
                print(f"  REJECTION CASE FAILED: {label}")
                print(f"    expected a problem containing: {expected!r}")
                print(f"    got: {problems}")
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    return passed


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    problems = check(
        root,
        os.path.join(root, "docs", "constraints.md"),
        os.path.join(root, "docs", "phases", "PHASES.md"),
        os.path.join(root, "CLAUDE.md"),
    )
    failed = False
    if problems:
        print("  FAIL: R-PROC-01: rule/test traceability")
        for problem in problems:
            print("        " + problem)
        failed = True
    else:
        print("  ok:   R-PROC-01 (catalogue consistent in both directions)")

    passed = run_rejection_cases()
    if passed == len(CASES):
        print(f"  ok:   R-PROC-01 rejection cases: {passed}/{len(CASES)}")
    else:
        print(f"  FAIL: R-PROC-01 rejection cases: {passed}/{len(CASES)}")
        failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
