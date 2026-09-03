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
# Overridable so the spec's own step-0 check ("point HOOK at a stub that exits 3 and
# this file must exit non-zero") is runnable as written; defaults to the real hook.
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

out=$( mktemp ) || exit 1
sweep "$ROOT" >"$out" 2>&1
rc=$?
if [ "$rc" -eq 0 ]; then
    if [ -d "$ROOT/src" ] && [ -n "$( find "$ROOT/src" -type f 2>/dev/null )" ]; then
        echo "  ok:   R-ARCH-02"
    else
        echo "  ok:   R-ARCH-02 (no sources yet)"
    fi
elif [ "$rc" -eq 1 ]; then
    echo "  FAIL: R-ARCH-02: forbidden dependency direction"
    sed 's/^/        /' "$out"
    fail=1
else
    echo "  FAIL: R-ARCH-02: the boundary hook did not run — this is not a layering result"
    sed 's/^/        /' "$out"
    fail=1
fi
rm -f "$out"

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

echo "  rejection cases: $rejected/2"
[ "$rejected" -eq 2 ] || fail=1

exit $fail
