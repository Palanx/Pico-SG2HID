#!/bin/sh
# No credentials in the tree.
#
# RULE R-SEC-01 — docs/constraints.md §Invariants — zero secrets in the repository
#
# The rule says "source, config or history", which is two independent scans of the real
# repository, and one result line each. The LIVE lines below say so to
# tests/test_checks_are_live.py, which then requires both labels to appear verbatim when
# this file runs — so deleting either real scan fails the build instead of passing
# quietly. A rejection case cannot do that job: it builds its own fixture.
# LIVE R-SEC-01 (working tree)
# LIVE R-SEC-01 (history)
#
# gitleaks is outside the C++23 + python3 floor that R-PROC-04 guarantees, so a missing
# binary is a skip under OPTIONAL_TOOLS (how `make test` runs it) and a failure otherwise
# (running this file directly). `make lint` runs tests/test_style.sh only — it never reaches
# this check.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
fail=0

if ! command -v gitleaks >/dev/null 2>&1; then
    if [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
        echo "  skip: R-SEC-01: gitleaks not found (brew install gitleaks)"
        exit 0
    fi
    echo "  FAIL: R-SEC-01: gitleaks not found (brew install gitleaks)"
    exit 1
fi

# Capability, not version. Both scans below hard-code a subcommand; a gitleaks too old to
# know one exits non-zero and would be reported as "found secrets", which is the wrong
# diagnosis of a tooling problem. `--help` on an unknown subcommand exits 1, on a known one
# 0. gitleaks is given no R-TOOL-01 version floor on purpose: the floor would need a release
# number nobody here has measured, and this probe answers the question the floor was for.
for sub in dir git; do
    if ! gitleaks "$sub" --help >/dev/null 2>&1; then
        echo "  FAIL: R-SEC-01: this gitleaks has no '$sub' subcommand — too old to scan with"
        exit 1
    fi
done

# scan <subcommand> <root> <output-file> — the only place a scan is spelled out. Round 7's
# validation found the previous shape reporting `ok:` for two scans that had been stubbed
# out: the real runs and the rejection cases were separate inlined command lines, so nothing
# tied the pass to a scan that still works. Every scan below — real tree, real history, and
# all four cases — goes through this function, which is what makes the `ok:` lines evidence.
scan( ) {
    gitleaks "$1" "$2" --no-banner --redact >"$3" 2>&1
}

# `gitleaks git` on a directory that is not a repository exits 1 — the same code as "found a
# secret" — so an export with no .git would be reported as a leak. Same misdiagnosis class as
# the subcommand probe above, and it gets the same treatment: its own line.
if [ ! -d "$ROOT/.git" ]; then
    echo "  FAIL: R-SEC-01: $ROOT has no .git — the history clause cannot be scanned here"
    exit 1
fi

tree_out=$( mktemp ) || exit 1
hist_out=$( mktemp ) || exit 1

if scan dir "$ROOT" "$tree_out"; then
    echo "  ok:   R-SEC-01 (working tree)"
else
    echo "  FAIL: R-SEC-01: gitleaks found secrets in the tree"
    grep -iE 'finding|secret|rule|file' "$tree_out" | head -8 | sed 's|^|        |'
    fail=1
fi

# The rule says "source, config or history". A secret deleted in the next commit is still in
# the repository, so the working-tree scan alone does not bind the rule. Its own output file:
# one shared path would let the second scan erase the first scan's findings before they are
# read.
if scan git "$ROOT" "$hist_out"; then
    echo "  ok:   R-SEC-01 (history)"
else
    echo "  FAIL: R-SEC-01: gitleaks found secrets in the commit history"
    grep -iE 'finding|secret|rule|file' "$hist_out" | head -8 | sed 's|^|        |'
    fail=1
fi
rm -f "$tree_out" "$hist_out"

# --- rejection cases --------------------------------------------------------------------
# The fake token below is deliberately NOT the AWS documentation example key
# (AKIAIOSFODNN7EXAMPLE): gitleaks allowlists that one, so using it would make a broken
# check look like a passing one. Learned the hard way while wiring the pre-commit gate.
#
# Assembled from pieces on purpose: written as one literal, this line would be a real
# detectable token sitting in the repository, and the scan above would fail on its own test
# data. Found by running it.
prefix='gh'
token="${prefix}p_9fK2mQ7xVzB4nR8tYwL3sD6hJ0aC5eU1gP2i"
rejected=0
accepted=0
out=$( mktemp ) || exit 1

# (1) tree scan: a planted token in a plain directory must be reported.
tmp=$( mktemp -d ) || exit 1
printf 'GITHUB_TOKEN=%s\n' "$token" > "$tmp/leak.env"
if scan dir "$tmp" "$out"; then
    echo "  FAIL: R-SEC-01 rejection case did not fire — a planted token was not reported"
    fail=1
else
    echo "  ok:   R-SEC-01 rejection case (working tree: a planted token is reported)"
    rejected=$(( rejected + 1 ))
fi
rm -rf "$tmp"

# (2) history scan: the clause the tree scan cannot reach. A token committed and then
# deleted is gone from the working tree and still in the repository, so this case asserts
# BOTH halves — the tree scan comes back clean and the history scan still fires.
# It does NOT protect the real history scan above: deleting that call leaves this file green,
# because this case builds its own fixture. The LIVE label in the header is what protects it,
# and tests/test_checks_are_live.py is what enforces the label.
tmp=$( mktemp -d ) || exit 1
(
    cd "$tmp" || exit 1
    git init -q .
    printf 'GITHUB_TOKEN=%s\n' "$token" > leak.env
    git add leak.env
    git -c user.email=t@t -c user.name=t commit -qm 'add'
    git rm -q leak.env
    git -c user.email=t@t -c user.name=t commit -qm 'remove'
) || { echo "  FAIL: R-SEC-01 rejection case could not build its git fixture"; fail=1; }
if scan dir "$tmp" "$out" && ! scan git "$tmp" "$out"; then
    echo "  ok:   R-SEC-01 rejection case (history: a deleted token is still found)"
    rejected=$(( rejected + 1 ))
else
    echo "  FAIL: R-SEC-01 rejection case did not fire — a token deleted in a later commit"
    echo "        was not reported by the history scan (or the tree scan wrongly saw it)"
    fail=1
fi
rm -rf "$tmp"

if [ "$rejected" -ge 2 ]; then
    echo "  ok:   rejection cases: $rejected (floor 2)"
else
    echo "  FAIL: rejection cases: $rejected (floor 2)"
    fail=1
fi

# --- false-positive cases ---------------------------------------------------------------
# A scan that reports everything is as useless as one that reports nothing.
tmp=$( mktemp -d ) || exit 1
printf 'nothing secret here\n' > "$tmp/plain.txt"
if scan dir "$tmp" "$out"; then
    echo "  ok:   R-SEC-01 false-positive case (working tree)"
    accepted=$(( accepted + 1 ))
else
    echo "  FAIL: false positive — a plain file was reported as a secret"
    fail=1
fi
rm -rf "$tmp"

tmp=$( mktemp -d ) || exit 1
(
    cd "$tmp" || exit 1
    git init -q .
    printf 'nothing secret here\n' > plain.txt
    git add plain.txt
    git -c user.email=t@t -c user.name=t commit -qm 'add'
) || { echo "  FAIL: R-SEC-01 false-positive case could not build its git fixture"; fail=1; }
if scan git "$tmp" "$out"; then
    echo "  ok:   R-SEC-01 false-positive case (history)"
    accepted=$(( accepted + 1 ))
else
    echo "  FAIL: false positive — a clean history was reported as carrying a secret"
    fail=1
fi
rm -rf "$tmp"

if [ "$accepted" -ge 2 ]; then
    echo "  ok:   false-positive cases: $accepted (floor 2)"
else
    echo "  FAIL: false-positive cases: $accepted (floor 2)"
    fail=1
fi
rm -f "$out"

exit $fail
