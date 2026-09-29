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
# RULE R-SAFETY-09 — docs/constraints.md §Invariants — no pin-configuring SDK call under src/
#                   outside src/hal/
#
# Every check takes the tree root as an argument. That is not decoration: when this file was
# written the repo had no product code, so every check passed vacuously and one that was
# silently broken looked identical to one that worked. The rejection cases at the bottom point
# the same functions at a deliberately-bad temp tree and require them to fire. src/core/ exists
# as of phase 01-ps2-codec, so the real run now examines something — and the cases are still
# what prove it would notice if it did not.
#
# belay-debt: these are greps, not parsed C++. Most checks strip line comments first — a `//`
# outside any string or character literal, through strip_line_comments( ); R-CLEAN-05 and
# R-PROTO-05 deliberately do not. Block comments are never stripped and string literals are
# never blanked (blanking would hide quoted #include paths from R-ARCH-01). That is enough to
# catch the realistic violation (someone writes the forbidden thing) and will produce a false
# positive on a forbidden token inside a /* */ block or a string. strip_line_comments( ) also
# takes every ' as a character-literal quote, so a C++14 digit separator (1'000) or an
# apostrophe in a /* */ block flips its tracking for the rest of the line: a later // comment
# is kept (false positive), or a // inside a real string is cut, hiding the code after it
# (a miss: `1'000; u = "a'"; v = "http://x"; throw E;` loses the throw). Upgrade path if it ever
# bites: clang-query, which needs the compile_commands.json that .clang-tidy also wants,
# so 03-pio-bus is the earliest phase that can land it.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )
fail=0

# One alternation, not -name clauses, so tests/test_checks_are_live.py mutates each extension.
src_files( )  { find "$1/src" -type f 2>/dev/null | grep -E '\.(cpp|h|hpp|cc|inl)$'; }
core_files( ) { find "$1/src/core" -type f 2>/dev/null | grep -E '\.(cpp|h|hpp|cc|inl)$'; }
# R-ERR-02 scans HEADERS only, and that is a property of the rule rather than a shortcut: a
# [[nodiscard]] belongs on the declaration, where it governs every call, and C++ does not
# repeat it on the definition. Scanning .cpp too would report every correct out-of-line
# definition — `LinkState step( … ) {` in src/core/link.cpp is exactly that shape. `.cc` and
# `.inl` are left out for the same reason: both hold out-of-line definitions, so a declaration
# placed in a `.inl` is outside this check.
#
# belay-debt: a function with no declaration at all — one defined only inside an anonymous
# namespace in a .cpp — is therefore outside this check. Those are internal to one translation
# unit and cannot be called by a caller who could ignore the result, but the gap is real. The
# clang-query upgrade in 03-pio-bus is what closes it.
core_headers( ) { find "$1/src/core" -type f 2>/dev/null | grep -E '\.(h|hpp)$'; }

# strip_line_comments <file> — each line cut at the first `//` that sits outside a "…" string
# (honouring \" escapes) and outside a '…' character literal; every other character is kept,
# so `#include "pico/stdlib.h"` still reaches R-ARCH-01 and `"http://x"; throw E;` still
# reaches R-ERR-03. A plain `s|//.*||` cut that line at the URL and hid the throw.
strip_line_comments( ) {
    awk -v sq="'" '{
        q = ""
        for ( i = 1; i <= length( $0 ); i++ ) {
            c = substr( $0, i, 1 )
            if ( q != "" ) {
                if ( c == "\\" ) i++
                else if ( c == q ) q = ""
            } else if ( c == "\"" || c == sq ) {
                q = c
            } else if ( substr( $0, i, 2 ) == "//" ) {
                $0 = substr( $0, 1, i - 1 )
                break
            }
        }
        print
    }' "$1"
}

# hits <pattern> <exclude-pattern-or-empty> <file-list>  — prints "path:line: text" per match.
# <file-list> is ONE argument, one path per line, read line by line: every finder passes its
# list quoted ("$( src_files "$1" )"), so a path with a space reaches [ -f ] whole. Passed
# unquoted, `src/core/a b.cpp` split into two paths that do not exist and the file was never
# read. A newline in a file name still splits (POSIX sh has no NUL read; see R-ARCH-02).
hits( ) {
    printf '%s\n' "$3" | while IFS= read -r hits_f; do
        [ -f "$hits_f" ] || continue
        hits_out=$( strip_line_comments "$hits_f" | grep -nE "$1" 2>/dev/null )
        if [ -n "$2" ] && [ -n "$hits_out" ]; then
            hits_out=$( printf '%s\n' "$hits_out" | grep -vE "$2" 2>/dev/null )
        fi
        [ -n "$hits_out" ] && printf '%s\n' "$hits_out" | sed "s|^|${hits_f}:|"
    done
    return 0
}

# raw_hits <pattern> <file-list> — same, but WITHOUT stripping comments. R-CLEAN-05 is a rule
# about comments, so the comment-stripping in hits() would delete the very text it looks
# for. Found by its own rejection case, which is why every check has one.
raw_hits( ) {
    printf '%s\n' "$2" | while IFS= read -r raw_f; do
        [ -f "$raw_f" ] || continue
        raw_out=$( grep -nE "$1" "$raw_f" 2>/dev/null )
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
find_arch01( ) { hits '^[[:space:]]*(#include[[:space:]]*[<"](pico/|hardware/|tusb|device/|class/|cmsis|core_cm)|#include[[:space:]]*<(iostream|fstream|sstream|iomanip|thread|mutex|condition_variable|future|filesystem|regex|locale|memory|new|stdexcept|exception|vector|string|map|set|unordered_map|unordered_set|deque|list|random)>)' '' "$( core_files "$1" )"; }
# std::string is anchored so that std::string_view — which owns nothing and allocates
# nothing — stays legal; the same header is accepted by R-ARCH-01 one line above, and the
# two halves must not disagree. No exclusion pattern: `= delete;` never matches the base
# alternation, which requires `delete` followed by whitespace and an identifier.
find_arch03( ) { hits '(\bnew[[:space:]]+[A-Za-z_]|\bmalloc[[:space:]]*\(|\bfree[[:space:]]*\(|std::vector|std::string([^_[:alnum:]]|$)|std::function|\bdelete[[:space:]]+[A-Za-z_])' '' "$( src_files "$1" )"; }
find_err03( )  { hits '(\bthrow\b|\btry[[:space:]]*\{|\bcatch[[:space:]]*\()' '' "$( src_files "$1" )"; }
find_err04( )  { hits '\.value[[:space:]]*\(' '' "$( src_files "$1" )"; }
find_clean03( ){ hits '\bbool[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*[;=]' 'bool[[:space:]]+(m_)?(is|has|can|should)_' "$( src_files "$1" )"; }
find_clean05( ){ raw_hits 'TODO([^(]|$)' "$( src_files "$1" )"; }
# The default-specifier forms (`struct A : B`) are inheritance too — the rule says "at all".
# `enum class Mode : uint8_t` is a fixed underlying type, not a base, so it is excluded.
find_clean09( ){ hits '(:[[:space:]]*(public|private|protected)[[:space:]]|(struct|class)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:|\bvirtual\b)' 'enum[[:space:]]+class' "$( core_files "$1" )"; }
# R-PROTO-05 uses raw_hits, NOT hits, and that is the rule's meaning rather than a detail of
# the scanner. The rule says no file under src/ may REFERENCE tests/vectors/, and a comment
# naming a vector is a reference. Decided 2026-09-15 after validation #3, where the acceptance
# criterion (a plain grep, which counts comments) and this check (which stripped them) gave
# opposite verdicts on the same tree. The literal reading is what both now use, so there is no
# judgement left to interpret about what counts as a reference.
find_proto05( ){ raw_hits 'tests/vectors' "$( src_files "$1" )"; }
# R-ERR-01, narrowed on 2026-09-11 to its greppable core: a status enum is never a return type
# of its own. It lives in std::expected's error slot (where it follows a `,` or a `<`, never the
# start of a line) or as a member of Link (where the identifier is followed by `=` or `;`, never
# `(` ). So "line starts with the enum name, then an identifier, then `(`" is exactly the
# forbidden shape and nothing else. `enum class DecodeStatus : …` does not match because a `:`
# follows the name, not an identifier.
find_err01( ){ hits '^[[:space:]]*(\[\[nodiscard\]\][[:space:]]*)?(constexpr[[:space:]]+)?(DecodeStatus|FaultCause)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(' '' "$( core_files "$1" )"; }
# R-ERR-02: a declaration whose return type is a result or a LinkState and which does not open
# with [[nodiscard]]. The attribute must precede the return type, so a compliant declaration
# starts with `[` and cannot match the anchor at all — which is why this needs no exclusion
# pattern. DecodeOutcome is included because it is the alias for the std::expected form, and a
# rule that named only the spelled-out type would be silent on the name everyone actually uses.
#
# Every alternative ends in an identifier followed by `(`, which is what makes this a check on
# RETURN types. Without it the pattern reported `LinkState state = LinkState::Absent;` — the
# member of Link, which is not a function at all. Found by running it against src/core/link.h.
find_err02( ){ hits '^[[:space:]]*(constexpr[[:space:]]+)?(std::expected<[^;]*>[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(|DecodeOutcome[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(|LinkState[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\()' '' "$( core_headers "$1" )"; }

# R-SAFETY-09: a Pico SDK call that configures a pin, anywhere under src/ but src/hal/. The
# name must be followed by optional whitespace and `(`, so a mention in prose or a pointer to
# the function is not a call. `\b` keeps `pio_gpio_init` from matching the `gpio_init`
# alternative; each has its own. The trailing `[a-z0-9_]*` is the rule's `*`: gpio_init_mask,
# the gpio_set_dir_*masked forms and pio_sm_set_pindirs_with_mask64.
find_safety09( ){ hits '\b(gpio_init[a-z0-9_]*|gpio_set_dir[a-z0-9_]*|gpio_set_function[a-z0-9_]*|gpio_set_pulls|gpio_pull_up|gpio_pull_down|gpio_disable_pulls|gpio_set_oeover|pio_gpio_init|pio_sm_set_pindirs_with_mask[a-z0-9_]*|pio_sm_set_consecutive_pindirs)[[:space:]]*\(' '' "$( src_files "$1" | grep -vF "$1/src/hal/" )"; }

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
    report R-SAFETY-09 "$( find_safety09 "$1" )" || fail=1
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

# reject <finder> <relative-path> <content> — the finder's name is report()'s label, so a
# FAIL line names the finder; which rule a finder backs is in run_all above.
reject( ) {
    rm -rf "$case_tmp/src"
    mkdir -p "$case_tmp/$( dirname "$2" )"
    printf '%s\n' "$3" > "$case_tmp/$2"
    # Through report(), not around it: the case asserts the verdict the real run depends on.
    if report "$1" "$( $1 "$case_tmp" )" >/dev/null 2>&1; then
        echo "  FAIL: $1 rejection case did not fire on: $3"
        fail=1
    else
        rejected=$(( rejected + 1 ))
    fi
}

rejected=0
reject find_arch01  src/core/x.h   '#include "pico/stdlib.h"'
reject find_arch03  src/core/x.cpp 'auto* p = new int;'
reject find_err03   src/core/x.cpp 'try { f( ); } catch ( ... ) { }'
# The combined line above matches try AND catch, so either alternative could be deleted
# with it still firing. One line each, reaching one alternative only.
reject find_err03   src/core/x.cpp 'try {'
reject find_err03   src/core/x.cpp '} catch ( const E& e ) {'
reject find_err03   src/core/x.cpp 'throw Status::kBad;'
# One per extension src_files( ) lists beyond .cpp. `.h` included: no other case of a
# src_files( )-backed finder plants a header, so `h` could be dropped with the suite green.
reject find_err03   src/core/x.h   'throw Status::kBad;'
reject find_err03   src/core/x.hpp 'throw Status::kBad;'
reject find_err03   src/core/x.cc  'throw Status::kBad;'
reject find_err03   src/core/x.inl 'throw Status::kBad;'
# Same for core_files( ) and core_headers( ). `.cpp` included: every other core_files( )-backed
# case plants a `.h`.
reject find_clean09 src/core/x.cpp 'virtual void poll( );'
reject find_clean09 src/core/x.hpp 'virtual void poll( );'
reject find_clean09 src/core/x.cc  'virtual void poll( );'
reject find_clean09 src/core/x.inl 'virtual void poll( );'
reject find_err02   src/core/x.hpp 'LinkState step( Link& link );'
# One per file list, in a path with a space: each list reaches hits( ) as one quoted argument
# read one line per path. Passed unquoted, `a b.cpp` split into `a` and `b.cpp`, neither of
# which exists, and all three of these passed.
reject find_err03   'src/core/a b.cpp' 'throw Status::kBad;'
reject find_clean09 'src/core/a b.hpp' 'virtual void poll( );'
reject find_err02   'src/core/a b.h'   'LinkState step( Link& link );'
# A `//` inside a string literal is not a comment: the throw after it must still be seen.
reject find_err03   src/core/x.cpp 'const char* u = "http://x"; throw E;'
reject find_err04   src/core/x.cpp 'auto v = result.value( );'
reject find_clean03 src/core/x.cpp 'bool flag = true;'
reject find_clean05 src/core/x.cpp '// TODO: fix this later'
reject find_clean09 src/core/x.h   'struct A : public B { };'
reject find_proto05 src/core/x.cpp 'load( "tests/vectors/digital.hex" );'
# The scope case, not a duplicate of the one above: a reference inside a COMMENT. Until
# 2026-09-15 find_proto05 went through hits(), which strips // before grepping, so this exact
# tree passed while the acceptance criterion failed on it. Deleting this case would let the
# check silently revert to the narrower scanner.
reject find_proto05 src/core/x.cpp '// see tests/vectors/digital.h for the bytes'
# R-ERR-01: one case per alternative of (DecodeStatus|FaultCause). Neither alternative can be
# reached by the other's case, which is what tests/test_checks_are_live.py requires.
reject find_err01   src/core/x.h   'DecodeStatus decode_it( const std::uint8_t* p );'
reject find_err01   src/core/x.h   'FaultCause cause_of( const Link& link );'
# ...and the two optional prefixes a real violation would carry.
reject find_err01   src/core/x.h   '[[nodiscard]] DecodeStatus decode_it( int n );'
reject find_err01   src/core/x.h   'constexpr FaultCause cause_of( int n );'
# R-ERR-02: one case per alternative of (std::expected<|DecodeOutcome …|LinkState …). All three
# are headers, because that is the only place this check looks.
reject find_err02   src/core/x.h   'std::expected<Ps2Frame, DecodeStatus> decode( int n );'
reject find_err02   src/core/x.h   'DecodeOutcome poll_once( Link& link );'
reject find_err02   src/core/x.h   'LinkState step( Link& link );'
reject find_err02   src/core/x.h   'constexpr LinkState step( Link& link );'
# The two clauses validation found unchecked: each rule's second syntactic form.
reject find_arch01  src/core/x.h   '#include <iostream>'
reject find_clean09 src/core/x.h   'struct A : B { };'
# Anchoring std::string must not stop it matching the thing it is there for.
reject find_arch03  src/core/x.cpp 'std::string s;'
# Cases for alternations that had none when round 7 mutation-tested this file. They are not
# a claim that every alternation is covered — counting that is tests/test_checks_are_live.py's
# job, and it is what names the next alternative to add a case for. Adding one here without
# running that harness proves nothing about the ones still uncovered.
reject find_arch03  src/core/x.cpp 'void* p = malloc( 4 );'
reject find_arch03  src/core/x.cpp 'free( p );'
reject find_arch03  src/core/x.cpp 'std::vector<int> v;'
reject find_arch03  src/core/x.cpp 'std::function<void( )> cb;'
reject find_arch03  src/core/x.cpp 'delete p;'

# Every alternative of every pattern above needs a case that fires on it ALONE. This block
# is not a list somebody remembered to write: tests/test_checks_are_live.py generates one
# mutant per alternative, deletes it, and fails the build naming any alternative whose
# deletion nothing here notices. Add an alternative to a pattern and that harness will name
# it on the next run — which is the whole reason this phase exists.

reject find_arch01  src/core/x.h   '#include "hardware/gpio.h"'
reject find_arch01  src/core/x.h   '#include "tusb.h"'
reject find_arch01  src/core/x.h   '#include "device/usbd.h"'
reject find_arch01  src/core/x.h   '#include "class/hid/hid_device.h"'
reject find_arch01  src/core/x.h   '#include "cmsis_gcc.h"'
reject find_arch01  src/core/x.h   '#include "core_cm0plus.h"'
reject find_arch01  src/core/x.h   '#include <fstream>'
reject find_arch01  src/core/x.h   '#include <sstream>'
reject find_arch01  src/core/x.h   '#include <iomanip>'
reject find_arch01  src/core/x.h   '#include <thread>'
reject find_arch01  src/core/x.h   '#include <mutex>'
reject find_arch01  src/core/x.h   '#include <condition_variable>'
reject find_arch01  src/core/x.h   '#include <future>'
reject find_arch01  src/core/x.h   '#include <filesystem>'
reject find_arch01  src/core/x.h   '#include <regex>'
reject find_arch01  src/core/x.h   '#include <locale>'
reject find_arch01  src/core/x.h   '#include <memory>'
reject find_arch01  src/core/x.h   '#include <new>'
reject find_arch01  src/core/x.h   '#include <stdexcept>'
reject find_arch01  src/core/x.h   '#include <exception>'
reject find_arch01  src/core/x.h   '#include <vector>'
reject find_arch01  src/core/x.h   '#include <string>'
reject find_arch01  src/core/x.h   '#include <map>'
reject find_arch01  src/core/x.h   '#include <set>'
reject find_arch01  src/core/x.h   '#include <unordered_map>'
reject find_arch01  src/core/x.h   '#include <unordered_set>'
reject find_arch01  src/core/x.h   '#include <deque>'
reject find_arch01  src/core/x.h   '#include <list>'
reject find_arch01  src/core/x.h   '#include <random>'

# The end-of-line halves of two anchored patterns. `std::string s;` reaches std::string
# through [^_[:alnum:]]; a line that IS the identifier reaches it only through `$`, and
# without a case for it the anchor could be deleted with the suite still green.
reject find_arch03  src/core/x.cpp 'std::string'
reject find_clean05 src/core/x.cpp '// TODO'

# Inheritance on a continuation line: the class name is on the line above, so the
# `(struct|class) Name :` alternative cannot see it and the access-specifier alternative is
# the only thing that can. Without these three, that whole alternative deletes clean —
# which is exactly what round 8's validation found.
reject find_clean09 src/core/x.h   '    : public Base'
reject find_clean09 src/core/x.h   '    : private Base'
reject find_clean09 src/core/x.h   '    : protected Base'

# `class A : B` reaches the second alternative only through `class`; `struct A : B` above
# covers `struct`.
reject find_clean09 src/core/x.h   'class A : B { };'
reject find_clean09 src/core/x.h   'virtual void poll( );'

# R-SAFETY-09: one case per alternative, each reaching that alternative alone, then the
# starred forms, then one per layer outside src/hal/ so the path filter cannot widen.
reject find_safety09 src/app/x.cpp  'gpio_init( 2 );'
reject find_safety09 src/app/x.cpp  'gpio_set_dir( 2, false );'
reject find_safety09 src/app/x.cpp  'gpio_set_function( 5, GPIO_FUNC_PIO0 );'
reject find_safety09 src/app/x.cpp  'gpio_set_pulls( 2, true, false );'
reject find_safety09 src/app/x.cpp  'gpio_pull_up( 2 );'
reject find_safety09 src/app/x.cpp  'gpio_pull_down( 2 );'
reject find_safety09 src/app/x.cpp  'gpio_disable_pulls( 2 );'
reject find_safety09 src/app/x.cpp  'gpio_set_oeover( 2, GPIO_OVERRIDE_HIGH );'
reject find_safety09 src/app/x.cpp  'pio_gpio_init( pio0, 2 );'
reject find_safety09 src/app/x.cpp  'pio_sm_set_pindirs_with_mask( pio0, 0, 0, 0 );'
reject find_safety09 src/app/x.cpp  'pio_sm_set_consecutive_pindirs( pio0, 0, 2, 5, true );'
reject find_safety09 src/app/x.cpp  'gpio_init_mask( 0x7c );'
reject find_safety09 src/app/x.cpp  'gpio_set_dir_out_masked( 0x38 );'
reject find_safety09 src/app/x.cpp  'pio_sm_set_pindirs_with_mask64( pio0, 0, 0, 0 );'
reject find_safety09 src/app/x.cpp  'gpio_init ( 2 );'
reject find_safety09 src/emu/x.cpp  'gpio_set_dir( 2, true );'
reject find_safety09 src/usb/x.cpp  'gpio_pull_up( 6 );'
reject find_safety09 src/core/x.h   'gpio_init( 2 );'
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
# accept <finder> <relative-path> <content> — what each case proves is the comment above it.
accept( ) {
    rm -rf "$case_tmp/src"
    mkdir -p "$case_tmp/$( dirname "$2" )"
    printf '%s\n' "$3" > "$case_tmp/$2"
    if report "$1" "$( $1 "$case_tmp" )" >/dev/null 2>&1; then
        accepted=$(( accepted + 1 ))
    else
        echo "  FAIL: $1 false-positive case fired on: $3"
        fail=1
    fi
}

accepted=0
# = delete; is not an allocation
accept find_arch03  src/core/x.h   'Bus( const Bus& ) = delete;'
# std::string_view allocates nothing
accept find_arch03  src/core/x.h   'std::string_view sv;'
# is_ prefixed bool
accept find_clean03 src/core/x.cpp 'bool is_ready = true;'
# m_has_ prefixed member bool
accept find_clean03 src/core/x.cpp 'bool m_has_ack = false;'
# TODO with a phase reference
accept find_clean05 src/core/x.cpp '// TODO(09-guitar-observe): confirm'
# freestanding <cstdint>
accept find_arch01  src/core/x.h   '#include <cstdint>'
# freestanding <array>
accept find_arch01  src/core/x.h   '#include <array>'
# freestanding <span>
accept find_arch01  src/core/x.h   '#include <span>'
# freestanding <expected>
accept find_arch01  src/core/x.h   '#include <expected>'
# <string_view> is not <string>
accept find_arch01  src/core/x.h   '#include <string_view>'
# enum with a fixed underlying type
accept find_clean09 src/core/x.h   'enum class Mode : uint8_t { kDigital };'
# The other two prefixes R-CLEAN-03 exempts. Demanded by tests/test_checks_are_live.py once
# it started mutating the EXCLUSION string as well as the pattern: dropping `can|` or
# `should|` from '(is|has|can|should)_' left this file green, and these are what fail when it
# happens. An accept case is what covers an exclusion alternative — a rejection case cannot.
# can_ prefixed bool
accept find_clean03 src/core/x.cpp 'bool can_fire = true;'
# should_ prefixed bool
accept find_clean03 src/core/x.cpp 'bool should_retry = false;'
# R-ERR-01 must not fire on the two shapes the narrowed rule explicitly permits — a status in
# std::expected's error slot, and a status as a member of Link — nor on the enum's own
# declaration. Without these the anchor and the trailing `(` could be dropped from the pattern
# with every rejection case above still firing.
# status in the expected error slot
accept find_err01   src/core/x.h   'std::expected<Ps2Frame, DecodeStatus> decode( int n );'
# status as a Link member
accept find_err01   src/core/x.h   'FaultCause last_fault = FaultCause::None;'
# the enum declaration itself
accept find_err01   src/core/x.h   'enum class DecodeStatus : std::uint8_t { AckTimeout };'
# R-ERR-02 must not fire on a compliant declaration. One per alternative, because a false
# positive on any one of the three would be as broken as a missed violation.
# nodiscard std::expected return
accept find_err02   src/core/x.h   '[[nodiscard]] std::expected<Ps2Frame, DecodeStatus> decode( int n );'
# nodiscard DecodeOutcome return
accept find_err02   src/core/x.h   '[[nodiscard]] DecodeOutcome poll_once( Link& link );'
# nodiscard LinkState return
accept find_err02   src/core/x.h   '[[nodiscard]] LinkState step( Link& link );'
# a DecodeOutcome parameter, not a return
accept find_err02   src/core/x.h   'void trace( const DecodeOutcome& outcome );'
# The three finders that had no accept case at all until 2026-09-17. None of them carries an
# exclusion pattern, so there is no alternative to cover here and that is not what these are
# for: each is the legitimate NEIGHBOUR of the violation its reject case plants, and nothing
# else proves the finder stays silent on it. A pattern loosened by one character — `\bthrow\b`
# to `throw`, `\.value[[:space:]]*\(` to `\.value`, `tests/vectors` to `tests/vector` — passes
# every rejection case above and fails exactly these.
# an identifier containing throw
accept find_err03   src/core/x.cpp 'int throwaway = 0;'
# a comment after a string literal is still stripped
accept find_err03   src/core/x.cpp 'const char* u = "http://x"; // throw later'
# a `"` inside a character literal does not open a string, so the `//` after it is a comment
accept find_err03   src/core/x.cpp 'char q = '\''"'\''; // throw'
# values( ) is not value( )
accept find_err04   src/core/x.cpp 'const auto n = report.values( );'
# a tests/ path that is not the vectors
accept find_proto05 src/core/x.cpp '#include "tests/vector_math.h"'
# R-SAFETY-09: src/hal/ is the one place allowed to configure pins
accept find_safety09 src/hal/x.cpp 'gpio_init( 2 );'
# reading or writing a pin's level is not configuring it
accept find_safety09 src/app/x.cpp 'gpio_put( 2, true );'
# gpio_get outside hal
accept find_safety09 src/app/x.cpp 'gpio_get( 6 );'
if [ "$accepted" -ge 28 ]; then
    echo "  ok:   false-positive cases: $accepted (floor 28)"
else
    echo "  FAIL: false-positive cases: $accepted (floor 28)"
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
wiring R-SAFETY-09 src/app/x.cpp 'gpio_init( 2 );'
if [ "$wired" -eq 11 ]; then
    echo "  ok:   wiring cases: $wired/11 (each rule's verdict reaches the exit code)"
else
    echo "  FAIL: wiring cases: $wired/11"
    fail=1
fi

exit $fail
