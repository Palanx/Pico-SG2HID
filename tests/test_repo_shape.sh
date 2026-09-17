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
# RULE R-ERR-01   — docs/constraints.md §Invariants — core returns no bare status enum
# RULE R-ERR-02   — docs/constraints.md §Invariants — a result or LinkState return is
#                   marked [[nodiscard]]
#
# Every check takes the tree root as an argument. That is not decoration: when this file was
# written the repo had no product code, so every check passed vacuously and one that was
# silently broken looked identical to one that worked. The rejection cases at the bottom point
# the same functions at a deliberately-bad temp tree and require them to fire. src/core/ exists
# as of phase 01-ps2-codec, so the real run now examines something — and the cases are still
# what prove it would notice if it did not.
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
# R-ERR-02 scans HEADERS only, and that is a property of the rule rather than a shortcut: a
# [[nodiscard]] belongs on the declaration, where it governs every call, and C++ does not
# repeat it on the definition. Scanning .cpp too would report every correct out-of-line
# definition — `LinkState step( … ) {` in src/core/link.cpp is exactly that shape.
#
# belay-debt: a function with no declaration at all — one defined only inside an anonymous
# namespace in a .cpp — is therefore outside this check. Those are internal to one translation
# unit and cannot be called by a caller who could ignore the result, but the gap is real. The
# clang-query upgrade in 03-pio-bus is what closes it.
core_headers( ) { find "$1/src/core" -type f -name '*.h' 2>/dev/null; }

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

# --- the ten checks, each a function of the tree root -----------------------------------

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
# R-PROTO-05 uses raw_hits, NOT hits, and that is the rule's meaning rather than a detail of
# the scanner. The rule says no file under src/ may REFERENCE tests/vectors/, and a comment
# naming a vector is a reference. Decided 2026-09-15 after validation #3, where the acceptance
# criterion (a plain grep, which counts comments) and this check (which stripped them) gave
# opposite verdicts on the same tree. The literal reading is what both now use, so there is no
# judgement left to interpret about what counts as a reference.
find_proto05( ){ raw_hits 'tests/vectors' $( src_files "$1" ); }
# R-ERR-01, narrowed on 2026-09-11 to its greppable core: a status enum is never a return type
# of its own. It lives in std::expected's error slot (where it follows a `,` or a `<`, never the
# start of a line) or as a member of Link (where the identifier is followed by `=` or `;`, never
# `(` ). So "line starts with the enum name, then an identifier, then `(`" is exactly the
# forbidden shape and nothing else. `enum class DecodeStatus : …` does not match because a `:`
# follows the name, not an identifier.
find_err01( ){ hits '^[[:space:]]*(\[\[nodiscard\]\][[:space:]]*)?(constexpr[[:space:]]+)?(DecodeStatus|FaultCause)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(' '' $( core_files "$1" ); }
# R-ERR-02: a declaration whose return type is a result or a LinkState and which does not open
# with [[nodiscard]]. The attribute must precede the return type, so a compliant declaration
# starts with `[` and cannot match the anchor at all — which is why this needs no exclusion
# pattern. DecodeOutcome is included because it is the alias for the std::expected form, and a
# rule that named only the spelled-out type would be silent on the name everyone actually uses.
#
# Every alternative ends in an identifier followed by `(`, which is what makes this a check on
# RETURN types. Without it the pattern reported `LinkState state = LinkState::Absent;` — the
# member of Link, which is not a function at all. Found by running it against src/core/link.h.
find_err02( ){ hits '^[[:space:]]*(constexpr[[:space:]]+)?(std::expected<[^;]*>[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(|DecodeOutcome[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(|LinkState[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\()' '' $( core_headers "$1" ); }

run_all( ) {
    report R-ARCH-01  "$( find_arch01  "$1" )" || fail=1
    report R-ARCH-03  "$( find_arch03  "$1" )" || fail=1
    report R-ERR-03   "$( find_err03   "$1" )" || fail=1
    report R-ERR-04   "$( find_err04   "$1" )" || fail=1
    report R-CLEAN-03 "$( find_clean03 "$1" )" || fail=1
    report R-CLEAN-05 "$( find_clean05 "$1" )" || fail=1
    report R-CLEAN-09 "$( find_clean09 "$1" )" || fail=1
    report R-PROTO-05 "$( find_proto05 "$1" )" || fail=1
    report R-ERR-01   "$( find_err01   "$1" )" || fail=1
    report R-ERR-02   "$( find_err02   "$1" )" || fail=1
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
# The scope case, not a duplicate of the one above: a reference inside a COMMENT. Until
# 2026-09-15 find_proto05 went through hits(), which strips // before grepping, so this exact
# tree passed while the acceptance criterion failed on it. Deleting this case would let the
# check silently revert to the narrower scanner.
reject R-PROTO-05 find_proto05 src/core/x.cpp '// see tests/vectors/digital.h for the bytes'
# R-ERR-01: one case per alternative of (DecodeStatus|FaultCause). Neither alternative can be
# reached by the other's case, which is what tests/test_checks_are_live.py requires.
reject R-ERR-01   find_err01   src/core/x.h   'DecodeStatus decode_it( const std::uint8_t* p );'
reject R-ERR-01   find_err01   src/core/x.h   'FaultCause cause_of( const Link& link );'
# ...and the two optional prefixes a real violation would carry.
reject R-ERR-01   find_err01   src/core/x.h   '[[nodiscard]] DecodeStatus decode_it( int n );'
reject R-ERR-01   find_err01   src/core/x.h   'constexpr FaultCause cause_of( int n );'
# R-ERR-02: one case per alternative of (std::expected<|DecodeOutcome …|LinkState …). All three
# are headers, because that is the only place this check looks.
reject R-ERR-02   find_err02   src/core/x.h   'std::expected<Ps2Frame, DecodeStatus> decode( int n );'
reject R-ERR-02   find_err02   src/core/x.h   'DecodeOutcome poll_once( Link& link );'
reject R-ERR-02   find_err02   src/core/x.h   'LinkState step( Link& link );'
reject R-ERR-02   find_err02   src/core/x.h   'constexpr LinkState step( Link& link );'
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
# R-ERR-01 must not fire on the two shapes the narrowed rule explicitly permits — a status in
# std::expected's error slot, and a status as a member of Link — nor on the enum's own
# declaration. Without these the anchor and the trailing `(` could be dropped from the pattern
# with every rejection case above still firing.
accept "status in the expected error slot"  find_err01 src/core/x.h 'std::expected<Ps2Frame, DecodeStatus> decode( int n );'
accept "status as a Link member"            find_err01 src/core/x.h 'FaultCause last_fault = FaultCause::None;'
accept "the enum declaration itself"        find_err01 src/core/x.h 'enum class DecodeStatus : std::uint8_t { AckTimeout };'
# R-ERR-02 must not fire on a compliant declaration. One per alternative, because a false
# positive on any one of the three would be as broken as a missed violation.
accept "nodiscard std::expected return"     find_err02 src/core/x.h '[[nodiscard]] std::expected<Ps2Frame, DecodeStatus> decode( int n );'
accept "nodiscard DecodeOutcome return"     find_err02 src/core/x.h '[[nodiscard]] DecodeOutcome poll_once( Link& link );'
accept "nodiscard LinkState return"         find_err02 src/core/x.h '[[nodiscard]] LinkState step( Link& link );'
accept "a DecodeOutcome parameter, not a return" find_err02 src/core/x.h 'void trace( const DecodeOutcome& outcome );'
# The three finders that had no accept case at all until 2026-09-17. None of them carries an
# exclusion pattern, so there is no alternative to cover here and that is not what these are
# for: each is the legitimate NEIGHBOUR of the violation its reject case plants, and nothing
# else proves the finder stays silent on it. A pattern loosened by one character — `\bthrow\b`
# to `throw`, `\.value[[:space:]]*\(` to `\.value`, `tests/vectors` to `tests/vector` — passes
# every rejection case above and fails exactly these.
accept "an identifier containing throw"     find_err03   src/core/x.cpp 'int throwaway = 0;'
accept "values( ) is not value( )"          find_err04   src/core/x.cpp 'const auto n = report.values( );'
accept "a tests/ path that is not the vectors" find_proto05 src/core/x.cpp '#include "tests/vector_math.h"'
if [ "$accepted" -ge 23 ]; then
    echo "  ok:   false-positive cases: $accepted (floor 23)"
else
    echo "  FAIL: false-positive cases: $accepted (floor 23)"
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
wiring R-ERR-01   src/core/x.h   'DecodeStatus decode_it( const std::uint8_t* p );'
wiring R-ERR-02   src/core/x.h   'LinkState step( Link& link );'
if [ "$wired" -eq 10 ]; then
    echo "  ok:   wiring cases: $wired/10 (each rule's verdict reaches the exit code)"
else
    echo "  FAIL: wiring cases: $wired/10"
    fail=1
fi

exit $fail
