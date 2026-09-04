#!/bin/sh
# Layer boundary sweep.
#
# RULE R-ARCH-02 — docs/constraints.md §Invariants — every file under src/ respects the
#                  dependency directions encoded in .claude/workflow/boundaries.rules
#
# This drives the existing edit hook rather than reimplementing it. The hook is a per-file
# gate (one path in, exit 2 on violation); sweeping the tree is this script's job, so the
# rule holds for files nobody has edited since the rule was written.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
# Overridable so the stub case below is runnable as written: the spec (§Plan step 6, not
# step 0 — an earlier comment here said step 0 and no such check is in it) requires a hook
# that cannot run to be reported as "the hook did not run", never as a breach, and the only
# way to test that is to point HOOK at a stub. Defaults to the real hook.
HOOK="${HOOK:-$ROOT/.claude/hooks/boundary-check.sh}"

fail=0
rejected=0

# sweep <project-root> — runs the hook over every file below <project-root>/src, not just
# *.cpp/*.h: the hook decides for itself what an import line looks like, and a layer can be
# breached from a header with any extension or from a build fragment sitting in the tree.
# The hook itself always stays at its real path; only CLAUDE_PROJECT_DIR moves, which is
# how the rejection case below gets a tree of its own.
#
# Exit codes are kept apart on purpose. The hook returns 2 and only 2 for a layering
# breach; any other non-zero exit means the hook itself did not run (missing rules file,
# bad interpreter, a bug). Reporting the second as "forbidden dependency direction" would
# send the reader hunting for an import that does not exist.
#   0 — every file clean          1 — at least one real violation          2 — hook error
sweep( ) {
    sweep_root="$1"
    sweep_bad=0
    [ -d "$sweep_root/src" ] || return 0
    for f in $( find "$sweep_root/src" -type f 2>/dev/null ); do
        CLAUDE_PROJECT_DIR="$sweep_root" "$HOOK" "$f" 2>&1
        sweep_rc=$?
        case "$sweep_rc" in
            0 ) ;;
            2 ) [ "$sweep_bad" -eq 2 ] || sweep_bad=1 ;;
            * ) echo "boundary hook exited $sweep_rc on $f"; sweep_bad=2 ;;
        esac
    done
    return $sweep_bad
}

if [ ! -x "$HOOK" ]; then
    echo "  FAIL: R-ARCH-02: $HOOK is missing or not executable"
    exit 1
fi

# run_all <project-root> — the aggregate: sweep, then the verdict, then a return code. One
# function for the real tree and for the wiring case below, so no case can pass while the
# path the real run takes is broken. It sets `fail` ITSELF, which is the whole point: the
# flag the wiring case has to protect is the one on the rule's failure path, so it must live
# inside the aggregate where the case can run it. A first attempt put `|| fail=1` at the call
# site instead, and the wiring case then passed while deleting that flag still left this file
# exiting 0 — the defect moved rather than closed.
# The verdict itself stays where it was, in sweep( )'s case on the hook's 0/1/2 — this
# wrapper reports it, it does not re-decide it.
run_all( ) {
    ra_out=$( mktemp ) || return 1
    sweep "$1" >"$ra_out" 2>&1
    ra_rc=$?
    if [ "$ra_rc" -eq 0 ]; then
        if [ -d "$1/src" ] && [ -n "$( find "$1/src" -type f 2>/dev/null )" ]; then
            echo "  ok:   R-ARCH-02"
        else
            echo "  ok:   R-ARCH-02 (no sources yet)"
        fi
    elif [ "$ra_rc" -eq 1 ]; then
        echo "  FAIL: R-ARCH-02: forbidden dependency direction"
        sed 's/^/        /' "$ra_out"
        fail=1
    else
        echo "  FAIL: R-ARCH-02: the boundary hook did not run — this is not a layering result"
        sed 's/^/        /' "$ra_out"
        fail=1
    fi
    rm -f "$ra_out"
}

run_all "$ROOT"

# --- rejection case ------------------------------------------------------------------
# core may not depend on hal. Without this, a sweep over a tree with no sources would
# report exactly the same "ok" as a sweep whose hook silently does nothing.
tmp=$( mktemp -d ) || exit 1
mkdir -p "$tmp/.claude/workflow" "$tmp/src/core"
cp "$ROOT/.claude/workflow/boundaries.rules" "$tmp/.claude/workflow/boundaries.rules"
printf '#include "hal/bus.h"\n' > "$tmp/src/core/bad.cpp"
# Assert the exact code, never just "non-zero": sweep returns 2 when the hook could not
# run, and reading that as a successful rejection is how a check that enforces nothing
# looks green. A broken hook would then satisfy this case and the one below by itself.
sweep "$tmp" >/dev/null 2>&1
rc=$?
if [ "$rc" -eq 1 ]; then
    echo "  ok:   R-ARCH-02 rejection case (core -> hal is refused)"
    rejected=$(( rejected + 1 ))
else
    echo "  FAIL: R-ARCH-02 rejection case did not fire — core including hal returned $rc, expected 1"
    fail=1
fi
rm -rf "$tmp"

# A hook that cannot run must not be reported as a layering breach. Same tree as above,
# but the hook is a stub that exits 3: sweep has to come back 2, not 1.
tmp=$( mktemp -d ) || exit 1
mkdir -p "$tmp/src/core" "$tmp/stub"
printf '#include "hal/bus.h"\n' > "$tmp/src/core/bad.cpp"
printf '#!/bin/sh\nexit 3\n' > "$tmp/stub/hook"
chmod +x "$tmp/stub/hook"
if ( HOOK="$tmp/stub/hook"; sweep "$tmp" >/dev/null 2>&1; [ $? -eq 2 ] ); then
    echo "  ok:   R-ARCH-02 rejection case (a hook that cannot run is not a violation)"
    rejected=$(( rejected + 1 ))
else
    echo "  FAIL: R-ARCH-02 rejection case did not fire — a broken hook was reported as a breach"
    fail=1
fi
rm -rf "$tmp"

# --- wiring case -------------------------------------------------------------------------
# The rejection cases above assert sweep's verdict. This asserts that the verdict reaches
# run_all's RETURN CODE and names the rule, which is a different claim: without it, deleting
# `|| fail=1` from the real run prints `FAIL: R-ARCH-02` and this file still exits 0.
# Measured before it existed: that deletion left `make test` at exit 0.
tmp=$( mktemp -d ) || exit 1
mkdir -p "$tmp/.claude/workflow" "$tmp/src/core"
cp "$ROOT/.claude/workflow/boundaries.rules" "$tmp/.claude/workflow/boundaries.rules"
printf '#include "hal/bus.h"\n' > "$tmp/src/core/bad.cpp"
wiring_flag=$( fail=0; run_all "$tmp" >/dev/null 2>&1; echo "$fail" )
wiring_txt=$( fail=0; run_all "$tmp" 2>&1 )
if [ "$wiring_flag" = "1" ] && printf '%s' "$wiring_txt" | grep -q 'FAIL: R-ARCH-02'; then
    echo "  ok:   R-ARCH-02 wiring case (the verdict reaches the exit code)"
else
    echo "  FAIL: R-ARCH-02 is reported but never reaches the exit code (run_all left fail=$wiring_flag)"
    fail=1
fi
rm -rf "$tmp"

# A floor, not an equality: an equality breaks when a third case is added, which is a floor
# written backwards (§How counts are stated). Prefixed so the liveness harness can see it.
if [ "$rejected" -ge 2 ]; then
    echo "  ok:   rejection cases: $rejected/2"
else
    echo "  FAIL: rejection cases: $rejected/2"
    fail=1
fi

exit $fail
