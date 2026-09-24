#!/bin/sh
# Every closed phase must leave the operator a way to check it by hand.
#
# RULE R-PROC-02 — docs/constraints.md §Invariants — a phase whose status is `done` has a
#                  verify.md written for a non-specialist
#
# The phase table is the parameter, not a constant, so the cases can hand this a table with
# a closed phase and no verify.md. The real run examines every `done` phase; the rejection
# cases are what prove the check would fire, since a real tree that passes proves nothing.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
fail=0

# missing_verify <phases-file> <phases-dir> — prints one line per closed phase with no
# usable verify.md: missing, empty, or lacking either of the two headings R-PROC-02 names —
# a line matching `^#+ What was built` and one matching `^#+ .*[Cc]heck it`. The headings are
# all it reads; whether the prose under them serves a non-specialist is judged by no check.
missing_verify( ) {
    awk -F'|' '/^\|/ {
        id = $2; status = $(NF-1)
        gsub(/^[ \t]+|[ \t]+$/, "", id)
        gsub(/^[ \t]+|[ \t]+$/, "", status)
        if ( id ~ /^[0-9][0-9]-/ && status == "done" ) { print id }
    }' "$1" | while read -r phase_id; do
        mv_f="$2/$phase_id/verify.md"
        if [ ! -s "$mv_f" ] || ! grep -qE '^#+ What was built' "$mv_f" \
             || ! grep -qE '^#+ .*[Cc]heck it' "$mv_f"; then
            echo "$phase_id is done but $mv_f is missing, empty, or lacks a '# What was built' or '# … Check it' heading"
        fi
    done
}

# report <rule-id> <found-text> [ok-suffix] — one FAIL line naming the rule, or one ok line.
# Returns 1 when <found-text> is non-empty, i.e. when a violation was found.
#
# It does NOT set `fail` itself: the caller does. That is what lets the cases below assert
# this function's VERDICT instead of re-deciding "is the finder's output non-empty" for
# themselves. They used to do exactly that — `[ -n "$found" ]` written out three separate
# times — so gutting only the real run's copy left this file *and* the liveness harness
# green. §Plan step 6 requires the shared shape for exactly that reason.
report( ) {
    if [ -n "$2" ]; then
        echo "  FAIL: $1"
        printf '%s\n' "$2" | sed 's|^|        |'
        return 1
    fi
    echo "  ok:   $1${3:-}"
    return 0
}

# run_all <phases-file> <phases-dir> — the aggregate: finder, then verdict, then a return
# code. One function for the real tree and every fixture, so no case can pass while the
# path the real run takes is broken.
run_all( ) {
    # grep -c prints 0 AND exits 1 when there are no matches, so `|| echo 0` would print two.
    ra_closed=$( grep -cE '^\|[[:space:]]*[0-9][0-9]-.*\|[[:space:]]*done[[:space:]]*\|' "$1" 2>/dev/null || true )
    report R-PROC-02 "$( missing_verify "$1" "$2" )" " ($ra_closed phases done, all with a verify.md)"
}

run_all "$ROOT/docs/phases/PHASES.md" "$ROOT/docs/phases" || fail=1

# --- rejection case --------------------------------------------------------------------
tmp=$( mktemp -d ) || exit 1
mkdir -p "$tmp/phases/03-something"
cat > "$tmp/PHASES.md" <<'TABLE'
| id | goal | depends | acceptance | status |
|----|------|---------|------------|--------|
| 03-something | x | - | y | done |
| 04-other | x | - | y | pending |
TABLE
if run_all "$tmp/PHASES.md" "$tmp/phases" >/dev/null 2>&1; then
    echo "  FAIL: R-PROC-02 rejection case did not fire"
    fail=1
else
    echo "  ok:   R-PROC-02 rejection case (a done phase with no verify.md is caught)"
fi

# The wiring case: the same aggregate on the same bad tree, asserting what the rejection
# case cannot — that the verdict reaches the return code *and* names the rule. Without it,
# a `report` that printed FAIL and returned 0 would satisfy every case above.
wiring_out=$( run_all "$tmp/PHASES.md" "$tmp/phases" 2>&1 )
wiring_rc=$?
if [ "$wiring_rc" -ne 0 ] && printf '%s' "$wiring_out" | grep -q 'R-PROC-02'; then
    echo "  ok:   R-PROC-02 wiring case (the verdict reaches the exit code)"
else
    echo "  FAIL: R-PROC-02 wiring case: rc=$wiring_rc, output did not name the rule"
    fail=1
fi

# A verify.md that exists but lacks a required heading: one case per heading, plus the
# one-line file the check used to accept. Each must be caught on its own.
reject_verify( ) { # reject_verify <verify.md-content> <what-it-lacks>
    printf '%s\n' "$1" > "$tmp/phases/03-something/verify.md"
    if run_all "$tmp/PHASES.md" "$tmp/phases" >/dev/null 2>&1; then
        echo "  FAIL: R-PROC-02 rejection case did not fire on a verify.md with $2"
        fail=1
    else
        echo "  ok:   R-PROC-02 rejection case (a verify.md with $2 is caught)"
        verify_rejected=$(( verify_rejected + 1 ))
    fi
}
verify_rejected=0
reject_verify 'see above' 'one line and no heading'
reject_verify '## What was built' 'no check-it heading'
reject_verify '## Check it yourself' 'no what-was-built heading'
# The count is what makes a helper that silently runs nothing fail: its ok lines vanish, and
# nothing else would notice.
if [ "$verify_rejected" -ne 3 ]; then
    echo "  FAIL: R-PROC-02 heading rejection cases: $verify_rejected/3"
    fail=1
fi

# ...and a closed phase whose verify.md carries both headings must not be reported.
printf '## What was built\nx\n## Check it yourself\ny\n' > "$tmp/phases/03-something/verify.md"
if run_all "$tmp/PHASES.md" "$tmp/phases" >/dev/null 2>&1; then
    echo "  ok:   R-PROC-02 false-positive case"
else
    echo "  FAIL: false positive — a done phase with both verify.md headings was reported"
    fail=1
fi
rm -rf "$tmp"

exit $fail
