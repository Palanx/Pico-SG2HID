#!/bin/sh
# No credentials in the tree.
#
# RULE R-SEC-01 — docs/constraints.md §Invariants — zero secrets in the repository
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

tree_out=$( mktemp ) || exit 1
hist_out=$( mktemp ) || exit 1

if gitleaks dir "$ROOT" --no-banner --redact >"$tree_out" 2>&1; then
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
if gitleaks git "$ROOT" --no-banner --redact >"$hist_out" 2>&1; then
    echo "  ok:   R-SEC-01 (history)"
else
    echo "  FAIL: R-SEC-01: gitleaks found secrets in the commit history"
    grep -iE 'finding|secret|rule|file' "$hist_out" | head -8 | sed 's|^|        |'
    fail=1
fi
rm -f "$tree_out" "$hist_out"

# --- rejection case --------------------------------------------------------------------
# The fake token below is deliberately NOT the AWS documentation example key
# (AKIAIOSFODNN7EXAMPLE): gitleaks allowlists that one, so using it would make a broken
# check look like a passing one. Learned the hard way while wiring the pre-commit gate.
tmp=$( mktemp -d ) || exit 1
# Assembled from pieces on purpose: written as one literal, this line would be a real
# detectable token sitting in the repository, and the check above would fail on its own
# test data. Found by running it.
prefix='gh'
printf 'GITHUB_TOKEN=%sp_9fK2mQ7xVzB4nR8tYwL3sD6hJ0aC5eU1gP2i\n' "$prefix" > "$tmp/leak.env"
if gitleaks dir "$tmp" --no-banner --redact >/dev/null 2>&1; then
    echo "  FAIL: R-SEC-01 rejection case did not fire — a planted token was not reported"
    fail=1
else
    echo "  ok:   R-SEC-01 rejection case (a planted token is reported)"
fi

# ...and an ordinary file must not be flagged.
rm -f "$tmp/leak.env"
printf 'nothing secret here\n' > "$tmp/plain.txt"
if gitleaks dir "$tmp" --no-banner --redact >/dev/null 2>&1; then
    echo "  ok:   R-SEC-01 false-positive case"
else
    echo "  FAIL: false positive — a plain file was reported as a secret"
    fail=1
fi
rm -rf "$tmp"

exit $fail
