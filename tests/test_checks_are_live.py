#!/usr/bin/env python3
"""Prove the other checks are still connected to the repository they claim to check.

This file checks no rule. It checks the checks, and it is the phase's answer to the defect
that survived rounds 1-8: a check reporting success while the thing it names never ran.
There is no product code under src/ yet, so almost every check passes vacuously — a check
that has been silently disconnected looks exactly like one that works, and eight rounds of
reading the code did not tell them apart. Only mutating the source ever did.

Three properties, all DERIVED FROM THE SOURCE rather than from a list somebody maintains.
That is the whole point: a hand-written list of cases is what rounds 7 and 8 produced, and
it covered one check function out of eight.

  1. accounting  — every rule id a check file declares in its header produces a result line
                   THAT ONLY THE REAL RUN PRODUCES, and every rule id in its output is
                   declared. "The id appears somewhere in the output" is the criterion that
                   let rounds 1-9 pass: every check ships rejection and accept cases and
                   their lines carry the rule id too, so a file whose real run has been
                   deleted still reports the id. A case line says it is a case; the real
                   run's lines are the ones that do not.
  2. neutering   — making any one check function find nothing must make its file fail.
  3. alternation — removing any one alternative from any check function's pattern must make
                   its file fail. Generated from the pattern text, so an alternative added
                   tomorrow is covered tomorrow.

Properties 2 and 3 catch a gutted shared helper for free: break report()'s failure branch
and EVERY generated mutant survives, so this file fails on all of them at once.

belay-debt: the .py check under tests/ is covered by property 1 only — its internals are not
mutated, and its thirteen failure modes rest on thirteen hand-written rejection cases, which
is the weaker form. Upgrade when a rule arrives whose check is not a grep; owner 01-ps2-codec.
"""

import concurrent.futures
import glob
import os
import re
import signal
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(ROOT, "tests")
SELF = os.path.basename(__file__)

# Mutating this one is declared out of scope (see spec §Out of scope); it is still held to
# the accounting property, which is what proves its rules are reported at all.
NO_MUTATE = {"test_style.sh"}

# There is no naming convention any more, and that is the fix rather than a shortcut. A
# convention needs a list, the list needs maintaining, and the round-9 list had grown a
# fifth name §Plan step 3 never declared while `arm_compiles` — the whole of R-TOOL-02 —
# `ver_num` and `resolve` fell outside it, never mutated and reported nowhere, because the
# harness printed only the set it *discovered*. Measured before removing it: neutering each
# of the 25 parsable functions in the five shell checks makes its file fail, helpers and
# case drivers included. So every function defined in a check file is mutated, and nothing
# has to be kept in step with a list.

# The shell checks write "# RULE …"; the .py check declares its marker in a docstring
# with no comment prefix. Both are the declaration — accept either rather than making
# one file rewrite its header to satisfy the reader of it.
# A LIVE line declares the literal prefix of the result line that proves one real-run
# call happened: "# LIVE R-SEC-01 (history)", "# LIVE R-TOOL-01: clang-tidy". Whatever
# follows the id on the line is that prefix, so the declaration is the output.
DECL = re.compile(r"^#?\s*(RULE|LIVE)\s+(R-[A-Z]+-\d{2})(.*)$", re.M)
RESULT = re.compile(r"^\s*(ok|FAIL):\s*(.*)$", re.M)
RULE_IN_LINE = re.compile(r"R-[A-Z]+-\d{2}")
SKIPPED = re.compile(r"^\s*skip:", re.M)
SKIP_LINE = re.compile(r"^\s*skip:\s*(.*)$", re.M)
# The one criterion this whole file turns on. Asking "does any line carry this rule id" is
# satisfied by every check that has a rejection case, which is all of them — so it is true
# whether or not the real run still happens, and it is why deleting the real sweep from
# test_boundaries.sh or the real run from test_phase_docs.sh went unnoticed. The question
# has to be asked of the real run's own lines, and the house convention is that a case line
# says which case it is. Stated over the output, so it holds for a check written tomorrow.
CASE_LINE = re.compile(r"(rejection|false-positive|accept|wiring)\s+cases?", re.I)

failures = []
notes = []


def sweep_orphans():
    """Remove mutants a killed run left behind.

    Each mutant is unlinked in a `finally`, which SIGTERM skips, so rounds 9, 10 and 11 each
    left `tests/mut_*.sh` sitting in the tree — where tests/test_rule_traceability.py walks
    for `RULE` markers and where the gitleaks tree scan reads. Swept at start-up, so a
    previous kill is cleaned up by the next run, and from a signal handler, so this run does
    not leave any. Not a .gitignore line: that hides them instead of removing them.
    """
    for path in (glob.glob(os.path.join(TESTS, "mut_*.sh"))
                 + glob.glob(os.path.join(TESTS, "fixtures", "mut_*.sh"))):
        try:
            os.unlink(path)
        except OSError:
            pass


def _on_signal(signum, _frame):
    sweep_orphans()
    sys.exit(128 + signum)


def fail(msg):
    print("  FAIL: " + msg)
    failures.append(msg)


def check_files():
    out = []
    for name in sorted(os.listdir(TESTS)):
        if name == SELF or not name.startswith("test_"):
            continue
        if name.endswith(".sh") or name.endswith(".py"):
            out.append(name)
    return out


def run(path, cwd=ROOT):
    # OPTIONAL_TOOLS=1 unconditionally, and it is not a convenience. A check whose external
    # tool is absent is a hard FAIL without it, so the harness — which re-runs every check as
    # a subprocess — turned a missing gitleaks into "test_secrets.sh does not pass on the real
    # tree" and made `make test` fail on the clean clone the PHASES.md row promises (C++23 and
    # python3, nothing else). It also made §Plan step 2's `unproven:` branch unreachable: the
    # file failed before it could be reported skipped. The harness asks whether a check is
    # wired to the tree, never whether this machine has the tool.
    env = dict(os.environ, OPTIONAL_TOOLS="1")
    cmd = ["sh", path] if path.endswith(".sh") else [sys.executable, path]
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=300, env=env)
    return p.returncode, p.stdout + p.stderr


# --- function extraction --------------------------------------------------------------

def find_functions(text, unparsable=None):
    """Return [(name, start, end)] spanning the whole definition, body braces included."""
    out = []
    if unparsable is None:
        unparsable = []
    for m in re.finditer(r"^([A-Za-z_][A-Za-z0-9_]*)\(\s*\)\s*\{", text, re.M):
        name = m.group(1)
        # Quote-aware AND comment-aware, and it has to be both: find_err03's regex contains
        # a literal \{ and the
        # naive counter read it as a nesting brace, walked off the end of the file, and
        # dropped the function from the set — reporting "52/52 caught" while R-ERR-03 was
        # never mutated at all. A harness with that hole is the defect it exists to catch.
        depth, i, quote = 0, m.end() - 1, None
        while i < len(text):
            c = text[i]
            if quote:
                if c == quote:
                    quote = None
            elif c == "#" and text[i - 1] in " \t\n":
                # A comment, skipped whole. `arm_compiles( )` carries the line "R-TOOL-01's
                # own floor" — one apostrophe, which opened a quote state that never closed,
                # so the walker ran to EOF and dropped the function. R-TOOL-02's entire check
                # was therefore never mutated and never reported: the defect this file exists
                # to catch, sitting in this file.
                nl = text.find("\n", i)
                i = len(text) if nl < 0 else nl
                continue
            elif c in "'\"":
                quote = c
            elif c == "\\":
                i += 2
                continue
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        if depth != 0:
            # Never skip silently: an unparsable definition is an unmutated check.
            unparsable.append(name)
            continue
        out.append((name, m.start(), i + 1))
    return out


def neutered(text, span):
    """Replace a function definition with one that finds nothing and reports success."""
    name, start, end = span
    return text[:start] + "%s( ) { return 0; }" % name + text[end:]


# --- pattern extraction and alternation enumeration -----------------------------------

def patterns(defn):
    """A finder's regexes: every single-quoted string in a ONE-LINE body, at most two.

    The house shape is `find_xxx( ) { hits '<pattern>' '<exclusions>' $( core_files "$1" ); }`
    — one line, and both strings are check patterns. Two things follow, and both were holes.

    *Both* strings, not the first: round 9 took only the first, so `find_clean03`'s exclusion
    list was never mutated and dropping `can|` from `(is|has|can|should)_` left the file and
    the harness green while `bool can_fire = true;` silently became a false positive. A hole
    in the exclusion half is a hole; what catches it is an accept case rather than a rejection
    case, which is why the property needed no other change to reach it.

    One-line bodies only, and that is what keeps this derived instead of listed. A
    multi-line body is a helper, and its quoted strings are `sed` and `printf` expressions,
    not check patterns: `hits`'s own `s|//.*||` and `s|^|${hits_f}:|` split on `|` like a
    regex and yield mutants that are "caught" because sed breaks, which inflates the count
    while proving nothing about coverage. Functions with no pattern are printed, so a finder
    that stops being a one-liner is visible rather than silently unmutated.
    """
    body = defn[defn.index("{") + 1:]
    if "\n" in body.rstrip().rstrip("}").rstrip():
        return []
    base = len(defn) - len(body)
    return [(m.group(1), m.start(1) + base)
            for m in list(re.finditer(r"'([^']*)'", body))[:2]]


def alternatives(pat):
    """Every |-separated alternative at every nesting depth.

    Returns [(start, end)] into pat. Depth matters: find_arch01 is one top-level branch
    wrapping two groups of 7 and 24 prefixes, and splitting only at depth 0 would generate
    one mutant for a pattern with thirty-odd forbidden forms — which is precisely the gap
    round 8 shipped.
    """
    spans = []
    # group_stack holds (group_content_start, [split positions]) for each open '('
    stack = [(0, [])]
    i, n = 0, len(pat)
    while i < n:
        c = pat[i]
        if c == "\\":
            i += 2
            continue
        if c == "[":
            j = i + 1
            if j < n and pat[j] == "^":
                j += 1
            if j < n and pat[j] == "]":
                j += 1
            while j < n and pat[j] != "]":
                j += 2 if pat[j] == "\\" else 1
            i = j + 1
            continue
        if c == "(":
            stack.append((i + 1, []))
        elif c == ")":
            if len(stack) > 1:
                start, splits = stack.pop()
                spans.extend(_segments(start, splits, i))
        elif c == "|":
            stack[-1][1].append(i)
        i += 1
    start, splits = stack[0]
    spans.extend(_segments(start, splits, n))
    return [s for s in spans if s[1] > s[0]]


def _segments(start, splits, end):
    """A group with k splits has k+1 alternatives; one alternative alone is not a choice."""
    if not splits:
        return []
    bounds = [start] + [s + 1 for s in splits]
    ends = [s for s in splits] + [end]
    return list(zip(bounds, ends))


def drop_alternative(pat, span):
    """Remove one alternative together with one adjacent '|'.

    Never leave an empty alternative: '(a||b)' matches everything and the file would fail
    for the wrong reason, which reads as a passing mutation test and is worse than no test.
    """
    s, e = span
    if e < len(pat) and pat[e] == "|":
        return pat[:s] + pat[e + 1:]
    if s > 0 and pat[s - 1] == "|":
        return pat[:s - 1] + pat[e:]
    return None  # the only alternative in its group: nothing to remove


# --- the three properties --------------------------------------------------------------

def property_accounting(names):
    """Returns the set of file names that could not be proven, for the mutation properties.

    A file that skipped anything is not mutated. Every mutant of it would skip too, exit 0
    and be counted a survivor — the build would fail on a clean clone for the opposite of the
    real reason. Unproven is stated, never silent (the `unproven:` notes at the end).
    """
    ok, unproven = 0, set()
    for name in names:
        path = os.path.join(TESTS, name)
        text = open(path).read()
        decls = DECL.findall(text)
        rules = {d[1] for d in decls if d[0] == "RULE"}
        labels = {(d[1] + d[2]).strip() for d in decls if d[0] == "LIVE" and d[2].strip()}
        if not rules:
            fail("%s declares no rule in its header" % name)
            continue
        rc, out = run(path)
        if rc != 0:
            fail("%s does not pass on the real tree (rc=%d)" % (name, rc))
            continue
        # ANY skip, not only a whole-file skip. `test_tool_versions.sh` probes four tools
        # and reports each on its own line, so with one absent it emits `skip:` AND `ok:`
        # lines: the old "skipped and produced no result line" test was false for it, and the
        # missing tool's `# LIVE R-TOOL-01: <tool>` label then prefixed zero lines and failed
        # the file. A partial skip is a partial proof, which is not a proof.
        skips = [s.strip() for s in SKIP_LINE.findall(out)]
        if skips:
            notes.append("unproven: %s (%d skip: %s)" % (name, len(skips), "; ".join(skips)))
            unproven.add(name)
            continue
        real_lines, case_lines = [], []
        for _kind, rest in RESULT.findall(out):
            (case_lines if CASE_LINE.search(rest) else real_lines).append(rest.strip())
        reported = set()
        for line in real_lines:
            reported.update(RULE_IN_LINE.findall(line))
        missing = sorted(rules - reported)
        if missing:
            fail("%s declares %s, reported by nothing but its own cases — the real run is "
                 "gone" % (name, ", ".join(missing)))
            continue
        undeclared = sorted({r for line in real_lines + case_lines
                             for r in RULE_IN_LINE.findall(line)} - rules)
        if undeclared:
            fail("%s reports %s, which its header does not declare"
                 % (name, ", ".join(undeclared)))
            continue
        # A label pins ONE real-run line. Exactly one, and never a substring: "(history)"
        # also occurs inside "R-SEC-01 false-positive case (history)", so a substring test
        # was satisfied by a case line while the real scan it named had been deleted.
        # Uniqueness is what stops a label from drifting onto a neighbour's line.
        missing_labels = sorted(
            l for l in labels
            if sum(1 for line in real_lines if line.startswith(l)) != 1
        )
        if missing_labels:
            fail("%s declares the live call(s) %s but the run produced no such line"
                 % (name, ", ".join(missing_labels)))
            continue
        ok += 1
        print("  ok:   accounting: %-28s %d rule(s), %d live label(s)"
              % (name, len(rules), len(labels)))
    return unproven


def mutate_and_run(path, mutant, label):
    """Write a mutant beside the original and require the file to reject it.

    Returns (label, survived). Nothing is printed here: these run in a pool, and a gate
    whose output order changes between runs is a gate nobody can diff.
    """
    with tempfile.NamedTemporaryFile("w", dir=os.path.dirname(path), prefix="mut_",
                                     suffix=".sh", delete=False) as fh:
        fh.write(mutant)
        tmp = fh.name
    try:
        rc, _out = run(tmp)
    finally:
        os.unlink(tmp)
    return label, rc == 0


def run_mutants(jobs):
    """Run every mutant, in parallel, and return the survivors in submission order.

    One mutant is one subprocess that mostly waits on other subprocesses, so this is I/O
    bound and threads are enough. Serially this step is the whole suite's wall clock, and it
    grows with every alternative any later phase adds to any pattern.

    It reports nothing itself: a survivor is a defect for the two properties below and the
    expected result for bootstrap(), which is what lets the fixture run through this exact
    code path instead of a copy of it.
    """
    if not jobs:
        return []
    workers = min(len(jobs), (os.cpu_count() or 2) * 2)
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(lambda j: mutate_and_run(j[0], j[1], j[2]), jobs))
    return [(label, meta) for (label, survived), (_p, _m, _l, meta)
            in zip(results, jobs) if survived]


def mutation_score(jobs):
    """Run the mutants; every survivor is a mutation nothing demonstrates."""
    survivors = run_mutants(jobs)
    for label, _meta in survivors:
        fail("%s — the mutant passes, so nothing demonstrates it" % label)
    return len(jobs) - len(survivors), len(jobs), [m for _l, m in survivors]


def property_neutering(names, unproven):
    jobs = []
    for name in names:
        if name in NO_MUTATE or name in unproven or not name.endswith(".sh"):
            continue
        text = open(os.path.join(TESTS, name)).read()
        unparsable = []
        fns = find_functions(text, unparsable)
        if unparsable:
            fail("%s: could not parse the definition of %s — an unparsed check is an "
                 "unmutated check" % (name, ", ".join(unparsable)))
        if not fns:
            fail("%s: no function found — a check file that defines none cannot be mutated"
                 % name)
            continue
        print("  ok:   functions in %-26s %s" % (name, ", ".join(f[0] for f in fns)))
        for fn in fns:
            jobs.append((os.path.join(TESTS, name), neutered(text, fn),
                         "%s: neutering %s is not caught" % (name, fn[0]), None))
    caught, total, _gaps = mutation_score(jobs)
    return caught, total


def alternation_jobs(path):
    """Every one-alternative-removed mutant of one check file, as run_mutants() jobs.

    bootstrap() calls this too. That is the point: the fixture goes through the same
    extraction, the same removal and the same runner as the real files, so a break in any
    of them shows up on the floor instead of hiding behind a second copy of the loop.
    """
    name = os.path.basename(path)
    text = open(path).read()
    jobs, patternless = [], []
    for fn_name, start, end in find_functions(text):
        pats = patterns(text[start:end])
        if not pats:
            patternless.append(fn_name)
            continue
        for pat, off in pats:
            for span in alternatives(pat):
                reduced = drop_alternative(pat, span)
                if reduced is None:
                    continue
                mutant = text[:start + off] + reduced + text[start + off + len(pat):]
                alt = pat[span[0]:span[1]]
                jobs.append((path,
                             mutant,
                             "%s: dropping '%s' from %s is not caught"
                             % (name, alt, fn_name),
                             (name, fn_name, alt)))
    return jobs, patternless


def property_alternation(names, unproven):
    jobs = []
    for name in names:
        if name in NO_MUTATE or name in unproven or not name.endswith(".sh"):
            continue
        file_jobs, patternless = alternation_jobs(os.path.join(TESTS, name))
        jobs.extend(file_jobs)
        if patternless:
            print("  ok:   no check pattern in %-19s %s"
                  % (name, ", ".join(patternless)))
    return mutation_score(jobs)


# --- the bootstrap floor ----------------------------------------------------------------

def bootstrap():
    """A harness that is silently broken reports no gaps — which is the failure it exists to
    catch, one level up. It cannot test itself without a regress, so it gets a fixture with a
    known-uncovered alternative and must report exactly that one.

    It runs through alternation_jobs() and run_mutants(), the same two functions property 3
    drives the real check files with. An inlined copy of that loop was the earlier shape and
    it could report "fixture gap as expected" while the real properties were blind — a floor
    that cannot fail is not a floor.
    """
    fixture = os.path.join(ROOT, "tests", "fixtures", "incomplete_check.sh")
    if not os.path.exists(fixture):
        fail("bootstrap fixture missing: tests/fixtures/incomplete_check.sh")
        return
    fixture_jobs, _patternless = alternation_jobs(fixture)
    survivors = run_mutants(fixture_jobs)
    if len(survivors) == 1:
        print("  ok:   bootstrap: fixture gap reported as expected (%s)"
              % survivors[0][1][2])
    else:
        fail("bootstrap: expected exactly 1 uncovered alternative in the fixture, got %d %s"
             % (len(survivors), [s[1][2] for s in survivors]))


def main():
    sweep_orphans()
    for sig in (signal.SIGTERM, signal.SIGINT):
        signal.signal(sig, _on_signal)

    names = check_files()
    if not names:
        fail("no check files found under tests/")
        return 1

    unproven = property_accounting(names)

    caught, total = property_neutering(names, unproven)
    if total and caught == total:
        print("  ok:   neutered: %d/%d caught" % (caught, total))
    elif total:
        fail("neutered: %d/%d caught" % (caught, total))

    caught, total, gaps = property_alternation(names, unproven)
    if total and caught == total:
        print("  ok:   alternations: %d/%d caught" % (caught, total))
    elif total:
        fail("alternations: %d/%d caught — no rejection case covers: %s"
             % (caught, total, "; ".join("%s/%s '%s'" % g for g in gaps)))

    bootstrap()

    for note in notes:
        print("  " + note)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
