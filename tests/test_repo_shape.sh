#!/bin/sh
# Repository-shape checks: rules that are properties of the source tree itself.
#
# RULE R-ARCH-01  — docs/constraints.md §Invariants — core includes no hardware header
# RULE R-ARCH-03  — docs/constraints.md §Invariants — no dynamic allocation under src/
# RULE R-ERR-03   — docs/constraints.md §Invariants — no throw/try/catch under src/
# RULE R-ERR-04   — docs/constraints.md §Invariants — no .value() on std::expected
# RULE R-CLEAN-03 — docs/constraints.md §Invariants — boolean names are assertions
# RULE R-CLEAN-05 — docs/constraints.md §Invariants — every TODO names what closes it
# RULE R-CLEAN-09 — docs/constraints.md §Invariants — no inheritance in core
# RULE R-PROTO-05 — docs/constraints.md §Invariants — src/ never reads tests/vectors/
#
# Every check takes the tree root as an argument. That is not decoration: this repo has no
# product code yet, so all eight pass vacuously, and a check that is silently broken would
# look identical to one that works. The rejection cases at the bottom point the same
# functions at a deliberately-bad temp tree and require them to fire.
#
# belay-debt: these are greps, not parsed C++. Most checks strip line comments first;
# R-CLEAN-05 deliberately does not, because it is a rule ABOUT comments. Block comments
# and string literals are stripped by neither. That is enough to catch the
# realistic violation (someone writes the forbidden thing) and will produce a false
# positive on a forbidden token inside a /* */ block or a string. Upgrade path if it ever
# bites: clang-query, which needs the compile_commands.json that .clang-tidy also wants,
# so 03-pio-bus is the earliest phase that can land it.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
fail=0

src_files( )  { find "$1/src" -type f \( -name '*.cpp' -o -name '*.h' \) 2>/dev/null; }
core_files( ) { find "$1/src/core" -type f \( -name '*.cpp' -o -name '*.h' \) 2>/dev/null; }

# hits <pattern> <exclude-pattern-or-empty> <file...>  — prints "path:line: text" per match
hits( ) {
    hits_pat="$1"; hits_not="$2"; shift 2
    for hits_f in "$@"; do
        [ -f "$hits_f" ] || continue
        hits_out=$( sed 's|//.*||' "$hits_f" | grep -nE "$hits_pat" 2>/dev/null )
        if [ -n "$hits_not" ] && [ -n "$hits_out" ]; then
            hits_out=$( printf '%s\n' "$hits_out" | grep -vE "$hits_not" 2>/dev/null )
        fi
        [ -n "$hits_out" ] && printf '%s\n' "$hits_out" | sed "s|^|${hits_f}:|"
    done
    return 0
}

# raw_hits <pattern> <file...> — same, but WITHOUT stripping comments. R-CLEAN-05 is a rule
# about comments, so the comment-stripping in hits() would delete the very text it looks
# for. Found by its own rejection case, which is why every check has one.
raw_hits( ) {
    raw_pat="$1"; shift
    for raw_f in "$@"; do
        [ -f "$raw_f" ] || continue
        raw_out=$( grep -nE "$raw_pat" "$raw_f" 2>/dev/null )
        [ -n "$raw_out" ] && printf '%s\n' "$raw_out" | sed "s|^|${raw_f}:|"
    done
    return 0
}

# report <rule-id> <found-text>  — one FAIL line naming the rule, or one ok line.
# Returns 1 when <found-text> is non-empty, i.e. when a violation was found.
#
# It does NOT set `fail` itself: the caller does. That is what lets the rejection and accept
# cases below run through this same function and assert its VERDICT, instead of re-deciding
# "is the finder's output non-empty" for themselves. They used to do exactly that, and the
# result was that gutting this function's branch — the line that decides whether anything is
# a violation at all — left the whole file green, because no case depended on it.
report( ) {
    if [ -n "$2" ]; then
        echo "  FAIL: $1"
        printf '%s\n' "$2" | head -8 | sed 's|^|        |'
        return 1
    fi
    echo "  ok:   $1"
    return 0
}

# --- the eight checks, each a function of the tree root ---------------------------------

# R-ARCH-01 has two clauses and needs both: no hardware header, and no hosted-only standard
# header. The second list is anchored on the closing '>' so <string_view> is not read as
# <string>; freestanding headers (<cstdint>, <array>, <span>, <expected>) are untouched.
find_arch01( ) { hits '^[[:space:]]*(#include[[:space:]]*[<"](pico/|hardware/|tusb|device/|class/|cmsis|core_cm)|#include[[:space:]]*<(iostream|fstream|sstream|iomanip|thread|mutex|condition_variable|future|filesystem|regex|locale|memory|new|stdexcept|exception|vector|string|map|set|unordered_map|unordered_set|deque|list|random)>)' '' $( core_files "$1" ); }
# std::string is anchored so that std::string_view — which owns nothing and allocates
# nothing — stays legal; the same header is accepted by R-ARCH-01 one line above, and the
# two halves must not disagree. No exclusion pattern: `= delete;` never matches the base
# alternation, which requires `delete` followed by whitespace and an identifier.
find_arch03( ) { hits '(\bnew[[:space:]]+[A-Za-z_]|\bmalloc[[:space:]]*\(|\bfree[[:space:]]*\(|std::vector|std::string([^_[:alnum:]]|$)|std::function|\bdelete[[:space:]]+[A-Za-z_])' '' $( src_files "$1" ); }
find_err03( )  { hits '(\bthrow\b|\btry[[:space:]]*\{|\bcatch[[:space:]]*\()' '' $( src_files "$1" ); }
find_err04( )  { hits '\.value[[:space:]]*\(' '' $( src_files "$1" ); }
find_clean03( ){ hits '\bbool[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*[;=]' 'bool[[:space:]]+(m_)?(is|has|can|should)_' $( src_files "$1" ); }
find_clean05( ){ raw_hits 'TODO([^(]|$)' $( src_files "$1" ); }
# The default-specifier forms (`struct A : B`) are inheritance too — the rule says "at all".
# `enum class Mode : uint8_t` is a fixed underlying type, not a base, so it is excluded.
find_clean09( ){ hits '(:[[:space:]]*(public|private|protected)[[:space:]]|(struct|class)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:|\bvirtual\b)' 'enum[[:space:]]+class' $( core_files "$1" ); }
find_proto05( ){ hits 'tests/vectors' '' $( src_files "$1" ); }

run_all( ) {
    report R-ARCH-01  "$( find_arch01  "$1" )" || fail=1
    report R-ARCH-03  "$( find_arch03  "$1" )" || fail=1
    report R-ERR-03   "$( find_err03   "$1" )" || fail=1
    report R-ERR-04   "$( find_err04   "$1" )" || fail=1
    report R-CLEAN-03 "$( find_clean03 "$1" )" || fail=1
    report R-CLEAN-05 "$( find_clean05 "$1" )" || fail=1
    report R-CLEAN-09 "$( find_clean09 "$1" )" || fail=1
    report R-PROTO-05 "$( find_proto05 "$1" )" || fail=1
}

run_all "$ROOT"

# --- rejection cases --------------------------------------------------------------------
# One bad tree per rule. Each must make its own check fire; a check that cannot be shown
# to reject anything is not a check.
# One scratch tree for every case, not one per case. tests/test_checks_are_live.py runs this
# file once per generated mutant, so a `mktemp -d` per case is paid ~60 times over ~60 runs
# and dominated the suite's wall clock. `src` is cleared between cases so a file written by
# one case cannot be seen by the next.
case_tmp=$( mktemp -d ) || exit 1
trap 'rm -rf "$case_tmp"' EXIT

reject( ) { # reject <rule-id> <finder> <relative-path> <content>
    rm -rf "$case_tmp/src"
    mkdir -p "$case_tmp/$( dirname "$3" )"
    printf '%s\n' "$4" > "$case_tmp/$3"
    # Through report(), not around it: the case asserts the verdict the real run depends on.
    if report "$1" "$( $2 "$case_tmp" )" >/dev/null 2>&1; then
        echo "  FAIL: $1 rejection case did not fire on: $4"
        fail=1
    else
        rejected=$(( rejected + 1 ))
    fi
}

rejected=0
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "pico/stdlib.h"'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'auto* p = new int;'
reject R-ERR-03   find_err03   src/core/x.cpp 'try { f( ); } catch ( ... ) { }'
# The combined line above matches try AND catch, so either alternative could be deleted
# with it still firing. One line each, reaching one alternative only.
reject R-ERR-03   find_err03   src/core/x.cpp 'try {'
reject R-ERR-03   find_err03   src/core/x.cpp '} catch ( const E& e ) {'
reject R-ERR-03   find_err03   src/core/x.cpp 'throw Status::kBad;'
reject R-ERR-04   find_err04   src/core/x.cpp 'auto v = result.value( );'
reject R-CLEAN-03 find_clean03 src/core/x.cpp 'bool flag = true;'
reject R-CLEAN-05 find_clean05 src/core/x.cpp '// TODO: fix this later'
reject R-CLEAN-09 find_clean09 src/core/x.h   'struct A : public B { };'
reject R-PROTO-05 find_proto05 src/core/x.cpp 'load( "tests/vectors/digital.hex" );'
# The two clauses validation found unchecked: each rule's second syntactic form.
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <iostream>'
reject R-CLEAN-09 find_clean09 src/core/x.h   'struct A : B { };'
# Anchoring std::string must not stop it matching the thing it is there for.
reject R-ARCH-03  find_arch03  src/core/x.cpp 'std::string s;'
# Cases for alternations that had none when round 7 mutation-tested this file. They are not
# a claim that every alternation is covered — counting that is tests/test_checks_are_live.py's
# job, and it is what names the next alternative to add a case for. Adding one here without
# running that harness proves nothing about the ones still uncovered.
reject R-ARCH-03  find_arch03  src/core/x.cpp 'void* p = malloc( 4 );'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'free( p );'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'std::vector<int> v;'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'std::function<void( )> cb;'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'delete p;'

# Every alternative of every pattern above needs a case that fires on it ALONE. This block
# is not a list somebody remembered to write: tests/test_checks_are_live.py generates one
# mutant per alternative, deletes it, and fails the build naming any alternative whose
# deletion nothing here notices. Add an alternative to a pattern and that harness will name
# it on the next run — which is the whole reason this phase exists.

reject R-ARCH-01  find_arch01  src/core/x.h   '#include "hardware/gpio.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "tusb.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "device/usbd.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "class/hid/hid_device.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "cmsis_gcc.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "core_cm0plus.h"'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <fstream>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <sstream>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <iomanip>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <thread>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <mutex>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <condition_variable>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <future>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <filesystem>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <regex>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <locale>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <memory>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <new>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <stdexcept>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <exception>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <vector>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <string>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <map>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <set>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <unordered_map>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <unordered_set>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <deque>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <list>'
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <random>'

# The end-of-line halves of two anchored patterns. `std::string s;` reaches std::string
# through [^_[:alnum:]]; a line that IS the identifier reaches it only through `$`, and
# without a case for it the anchor could be deleted with the suite still green.
reject R-ARCH-03  find_arch03  src/core/x.cpp 'std::string'
reject R-CLEAN-05 find_clean05 src/core/x.cpp '// TODO'

# Inheritance on a continuation line: the class name is on the line above, so the
# `(struct|class) Name :` alternative cannot see it and the access-specifier alternative is
# the only thing that can. Without these three, that whole alternative deletes clean —
# which is exactly what round 8's validation found.
reject R-CLEAN-09 find_clean09 src/core/x.h   '    : public Base'
reject R-CLEAN-09 find_clean09 src/core/x.h   '    : private Base'
reject R-CLEAN-09 find_clean09 src/core/x.h   '    : protected Base'

# `class A : B` reaches the second alternative only through `class`; `struct A : B` above
# covers `struct`.
reject R-CLEAN-09 find_clean09 src/core/x.h   'class A : B { };'
reject R-CLEAN-09 find_clean09 src/core/x.h   'virtual void poll( );'
# No floor. A count next to "every alternative is covered" is the round-8 defect: the two
# drift and the number is the one that stops being true. Sufficiency is asserted by
# tests/test_checks_are_live.py, which derives what is needed from the patterns themselves.
# No floor to compare against (see above), but a run that produced no case at all is a
# broken file, not a passing one — and the verdict has to reach the prefix. `ok:` above a
# comparison that may reject the same number is the shape §Plan step 0c bans.
if [ "$rejected" -gt 0 ]; then
    echo "  ok:   rejection cases: $rejected"
else
    echo "  FAIL: rejection cases: $rejected — no case ran"
    fail=1
fi

# A rule that fires on legitimate code is as broken as one that never fires. These must
# NOT be reported.
accept( ) { # accept <label> <finder> <relative-path> <content>
    rm -rf "$case_tmp/src"
    mkdir -p "$case_tmp/$( dirname "$3" )"
    printf '%s\n' "$4" > "$case_tmp/$3"
    if report "$1" "$( $2 "$case_tmp" )" >/dev/null 2>&1; then
        accepted=$(( accepted + 1 ))
    else
        echo "  FAIL: false positive — $1"
        fail=1
    fi
}

accepted=0
accept "= delete; is not an allocation" find_arch03 src/core/x.h   'Bus( const Bus& ) = delete;'
accept "std::string_view allocates nothing" find_arch03 src/core/x.h 'std::string_view sv;'
accept "is_ prefixed bool"            find_clean03 src/core/x.cpp 'bool is_ready = true;'
accept "m_has_ prefixed member bool"  find_clean03 src/core/x.cpp 'bool m_has_ack = false;'
accept "TODO with a phase reference"  find_clean05 src/core/x.cpp '// TODO(09-guitar-observe): confirm'
accept "freestanding <cstdint>"       find_arch01  src/core/x.h   '#include <cstdint>'
accept "freestanding <array>"         find_arch01  src/core/x.h   '#include <array>'
accept "freestanding <span>"          find_arch01  src/core/x.h   '#include <span>'
accept "freestanding <expected>"      find_arch01  src/core/x.h   '#include <expected>'
accept "<string_view> is not <string>" find_arch01 src/core/x.h   '#include <string_view>'
accept "enum with a fixed underlying type" find_clean09 src/core/x.h 'enum class Mode : uint8_t { kDigital };'
# The other two prefixes R-CLEAN-03 exempts. Demanded by tests/test_checks_are_live.py once
# it started mutating the EXCLUSION string as well as the pattern: dropping `can|` or
# `should|` from '(is|has|can|should)_' left this file green, and these are what fail when it
# happens. An accept case is what covers an exclusion alternative — a rejection case cannot.
accept "can_ prefixed bool"            find_clean03 src/core/x.cpp 'bool can_fire = true;'
accept "should_ prefixed bool"         find_clean03 src/core/x.cpp 'bool should_retry = false;'
if [ "$accepted" -ge 13 ]; then
    echo "  ok:   false-positive cases: $accepted (floor 13)"
else
    echo "  FAIL: false-positive cases: $accepted (floor 13)"
    fail=1
fi

# --- wiring cases -----------------------------------------------------------------------
# A rejection case proves the finder finds and report() judges. Neither proves the verdict
# reaches the EXIT CODE: dropping one `|| fail=1` from run_all leaves the violation printed,
# the FAIL: line on screen, and the file exiting 0. Found by mutating for it.
#
# One per rule, driving the real aggregate: run_all on a tree carrying exactly that rule's
# violation must both name the rule and leave fail set. Checking the rule name too, not just
# the flag, keeps the case precise when a fixture happens to trip a second rule.
wiring( ) { # wiring <rule-id> <relative-path> <content>
    rm -rf "$case_tmp/src"
    mkdir -p "$case_tmp/$( dirname "$2" )"
    printf '%s\n' "$3" > "$case_tmp/$2"
    wiring_out=$( fail=0; run_all "$case_tmp" >/dev/null 2>&1; echo "$fail" )
    wiring_txt=$( fail=0; run_all "$case_tmp" 2>&1 )
    if [ "$wiring_out" = "1" ] && printf '%s' "$wiring_txt" | grep -q "FAIL: $1"; then
        wired=$(( wired + 1 ))
    else
        echo "  FAIL: $1 is reported but never reaches the exit code (run_all left fail=$wiring_out)"
        fail=1
    fi
}

wired=0
wiring R-ARCH-01  src/core/x.h   '#include "pico/stdlib.h"'
wiring R-ARCH-03  src/core/x.cpp 'auto* p = new int;'
wiring R-ERR-03   src/core/x.cpp 'throw Status::kBad;'
wiring R-ERR-04   src/core/x.cpp 'auto v = result.value( );'
wiring R-CLEAN-03 src/core/x.cpp 'bool flag = true;'
wiring R-CLEAN-05 src/core/x.cpp '// TODO: fix this later'
wiring R-CLEAN-09 src/core/x.h   'struct A : public B { };'
wiring R-PROTO-05 src/core/x.cpp 'load( "tests/vectors/digital.hex" );'
if [ "$wired" -eq 8 ]; then
    echo "  ok:   wiring cases: $wired/8 (each rule's verdict reaches the exit code)"
else
    echo "  FAIL: wiring cases: $wired/8"
    fail=1
fi

exit $fail
