#!/bin/sh
# Every closed phase must leave the operator a way to check it by hand.
#
# RULE R-PROC-02 — docs/constraints.md §Invariants — a phase whose status is `done` has a
#                  verify.md written for a non-specialist
#
# The phase table is the parameter, not a constant, so the rejection case can hand this a
# table with a closed phase and no verify.md. Today no phase is done, so the real run
# passes without examining anything — which is exactly the state where a broken check is
# invisible.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
fail=0

# missing_verify <phases-file> <phases-dir> — prints one line per closed phase with no
# usable verify.md.
missing_verify( ) {
    awk -F'|' '/^\|/ {
        id = $2; status = $(NF-1)
        gsub(/^[ \t]+|[ \t]+$/, "", id)
        gsub(/^[ \t]+|[ \t]+$/, "", status)
        if ( id ~ /^[0-9][0-9]-/ && status == "done" ) { print id }
    }' "$1" | while read -r phase_id; do
        if [ ! -s "$2/$phase_id/verify.md" ]; then
            echo "$phase_id is done but $2/$phase_id/verify.md is missing or empty"
        fi
    done
}

found=$( missing_verify "$ROOT/docs/phases/PHASES.md" "$ROOT/docs/phases" )
# grep -c prints 0 AND exits 1 when there are no matches, so `|| echo 0` would print two.
closed=$( grep -cE '^\|[[:space:]]*[0-9][0-9]-.*\|[[:space:]]*done[[:space:]]*\|' "$ROOT/docs/phases/PHASES.md" 2>/dev/null || true )
if [ -n "$found" ]; then
    echo "  FAIL: R-PROC-02"
    printf '%s\n' "$found" | sed 's|^|        |'
    fail=1
else
    echo "  ok:   R-PROC-02 ($closed phases done, all with a verify.md)"
fi

# --- rejection case --------------------------------------------------------------------
tmp=$( mktemp -d ) || exit 1
mkdir -p "$tmp/phases/03-something"
cat > "$tmp/PHASES.md" <<'TABLE'
| id | goal | depends | acceptance | status |
|----|------|---------|------------|--------|
| 03-something | x | - | y | done |
| 04-other | x | - | y | pending |
TABLE
if [ -n "$( missing_verify "$tmp/PHASES.md" "$tmp/phases" )" ]; then
    echo "  ok:   R-PROC-02 rejection case (a done phase with no verify.md is caught)"
else
    echo "  FAIL: R-PROC-02 rejection case did not fire"
    fail=1
fi

# ...and a closed phase that DOES have one must not be reported.
printf 'how to check this by hand\n' > "$tmp/phases/03-something/verify.md"
if [ -n "$( missing_verify "$tmp/PHASES.md" "$tmp/phases" )" ]; then
    echo "  FAIL: false positive — a done phase with a verify.md was reported"
    fail=1
else
    echo "  ok:   R-PROC-02 false-positive case"
fi
rm -rf "$tmp"

exit $fail
