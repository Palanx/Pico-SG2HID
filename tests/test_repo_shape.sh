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
# bites: clang-query, which needs the compile_commands.json that .clang-tidy also wants.
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

# report <rule-id> <found-text>  — one FAIL line naming the rule, or one ok line
report( ) {
    if [ -n "$2" ]; then
        echo "  FAIL: $1"
        printf '%s\n' "$2" | head -8 | sed 's|^|        |'
        fail=1
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
find_arch03( ) { hits '(\bnew[[:space:]]+[A-Za-z_]|\bmalloc[[:space:]]*\(|\bfree[[:space:]]*\(|std::vector|std::string|std::function|\bdelete[[:space:]]+[A-Za-z_])' '=[[:space:]]*delete' $( src_files "$1" ); }
find_err03( )  { hits '(\bthrow\b|\btry[[:space:]]*\{|\bcatch[[:space:]]*\()' '' $( src_files "$1" ); }
find_err04( )  { hits '\.value[[:space:]]*\(' '' $( src_files "$1" ); }
find_clean03( ){ hits '\bbool[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*[;=]' 'bool[[:space:]]+(m_)?(is|has|can|should)_' $( src_files "$1" ); }
find_clean05( ){ raw_hits 'TODO([^(]|$)' $( src_files "$1" ); }
# The default-specifier forms (`struct A : B`) are inheritance too — the rule says "at all".
# `enum class Mode : uint8_t` is a fixed underlying type, not a base, so it is excluded.
find_clean09( ){ hits '(:[[:space:]]*(public|private|protected)[[:space:]]|(struct|class)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:|\bvirtual\b)' 'enum[[:space:]]+class' $( core_files "$1" ); }
find_proto05( ){ hits 'tests/vectors' '' $( src_files "$1" ); }

run_all( ) {
    report R-ARCH-01  "$( find_arch01  "$1" )"
    report R-ARCH-03  "$( find_arch03  "$1" )"
    report R-ERR-03   "$( find_err03   "$1" )"
    report R-ERR-04   "$( find_err04   "$1" )"
    report R-CLEAN-03 "$( find_clean03 "$1" )"
    report R-CLEAN-05 "$( find_clean05 "$1" )"
    report R-CLEAN-09 "$( find_clean09 "$1" )"
    report R-PROTO-05 "$( find_proto05 "$1" )"
}

run_all "$ROOT"

# --- rejection cases --------------------------------------------------------------------
# One bad tree per rule. Each must make its own check fire; a check that cannot be shown
# to reject anything is not a check.
reject( ) { # reject <rule-id> <finder> <relative-path> <content>
    reject_tmp=$( mktemp -d ) || return 1
    mkdir -p "$reject_tmp/$( dirname "$3" )"
    printf '%s\n' "$4" > "$reject_tmp/$3"
    if [ -n "$( $2 "$reject_tmp" )" ]; then
        rejected=$(( rejected + 1 ))
    else
        echo "  FAIL: $1 rejection case did not fire on: $4"
        fail=1
    fi
    rm -rf "$reject_tmp"
}

rejected=0
reject R-ARCH-01  find_arch01  src/core/x.h   '#include "pico/stdlib.h"'
reject R-ARCH-03  find_arch03  src/core/x.cpp 'auto* p = new int;'
reject R-ERR-03   find_err03   src/core/x.cpp 'try { f( ); } catch ( ... ) { }'
reject R-ERR-04   find_err04   src/core/x.cpp 'auto v = result.value( );'
reject R-CLEAN-03 find_clean03 src/core/x.cpp 'bool flag = true;'
reject R-CLEAN-05 find_clean05 src/core/x.cpp '// TODO: fix this later'
reject R-CLEAN-09 find_clean09 src/core/x.h   'struct A : public B { };'
reject R-PROTO-05 find_proto05 src/core/x.cpp 'load( "tests/vectors/digital.hex" );'
# The two clauses validation found unchecked: each rule's second syntactic form.
reject R-ARCH-01  find_arch01  src/core/x.h   '#include <iostream>'
reject R-CLEAN-09 find_clean09 src/core/x.h   'struct A : B { };'
echo "  ok:   rejection cases: $rejected/10"
[ "$rejected" -eq 10 ] || fail=1

# A rule that fires on legitimate code is as broken as one that never fires. These must
# NOT be reported.
accept( ) { # accept <label> <finder> <relative-path> <content>
    accept_tmp=$( mktemp -d ) || return 1
    mkdir -p "$accept_tmp/$( dirname "$3" )"
    printf '%s\n' "$4" > "$accept_tmp/$3"
    if [ -n "$( $2 "$accept_tmp" )" ]; then
        echo "  FAIL: false positive — $1"
        fail=1
    else
        accepted=$(( accepted + 1 ))
    fi
    rm -rf "$accept_tmp"
}

accepted=0
accept "= delete is legal C++"        find_arch03  src/core/x.h   'Bus( const Bus& ) = delete;'
accept "is_ prefixed bool"            find_clean03 src/core/x.cpp 'bool is_ready = true;'
accept "m_has_ prefixed member bool"  find_clean03 src/core/x.cpp 'bool m_has_ack = false;'
accept "TODO with a phase reference"  find_clean05 src/core/x.cpp '// TODO(09-guitar-observe): confirm'
accept "freestanding <cstdint>"       find_arch01  src/core/x.h   '#include <cstdint>'
accept "freestanding <array>"         find_arch01  src/core/x.h   '#include <array>'
accept "freestanding <span>"          find_arch01  src/core/x.h   '#include <span>'
accept "freestanding <expected>"      find_arch01  src/core/x.h   '#include <expected>'
accept "<string_view> is not <string>" find_arch01 src/core/x.h   '#include <string_view>'
accept "enum with a fixed underlying type" find_clean09 src/core/x.h 'enum class Mode : uint8_t { kDigital };'
echo "  ok:   false-positive cases: $accepted/10"
[ "$accepted" -eq 10 ] || fail=1

exit $fail
