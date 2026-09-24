#!/bin/sh
# The tools the other checks depend on are present, recent enough, and actually work.
#
# RULE R-TOOL-01 — docs/constraints.md §Invariants — minimum versions
# R-TOOL-01 is four independent probes behind one rule id, so deleting any one of them
# leaves the other three still reporting it. One LIVE label per probe; the harness requires
# each to prefix exactly one result line. Same shape, and same fix, as R-SEC-01's two scans.
# LIVE R-TOOL-01: clang-format
# LIVE R-TOOL-01: clang-tidy
# LIVE R-TOOL-01: arm-none-eabi-g++
# LIVE R-TOOL-01: python3
#
# RULE R-TOOL-02 — docs/constraints.md §Invariants — the arm-none-eabi-g++ first on PATH
#                  can compile a translation unit that includes <cstdint> for cortex-m0plus
#
# R-TOOL-02 is not redundant with R-TOOL-01, and the reason is a trap this project walked
# into: on macOS the sudo-free Homebrew formula `arm-none-eabi-gcc` reports a HIGHER
# version than the cask that works, and ships no target C library. It looks installed, it
# passes any version check, and it cannot compile. The only thing that catches it is
# compiling something. See docs/adr/0008-cpp23.md §Context.
set -u
cd "$(dirname "$0")/.." || exit 1
fail=0

# ver_num <text> — a banner's version as a comparable integer, major*100+minor: the first
# N or N.N after the first word "version" when the text has that word, else the first N or
# N.N in the text. The first number alone read "x86_64-apple clang-format version 14.0.6"
# as 86 and "clang-format 99 version 14.0.6" as 99, both clearing a floor of 23.
# Handles all four shapes, and a floor written the same way ("23" -> 2300, "3.8" -> 308):
#   "Homebrew clang-format version 23.1.0"   "Homebrew LLVM version 23.1.0"
#   "Python 3.14.6"                          "15.3.1" (gcc -dumpversion, may be bare "15")
# Major alone is not enough: R-TOOL-01's python3 floor is 3.8, and every Python this
# project will ever meet is major 3.
ver_num( ) {
    vn_t="$1"
    case "$vn_t" in *version*) vn_t="${vn_t#*version}" ;; esac
    vn_v=$( printf '%s' "$vn_t" | grep -oE '[0-9]+(\.[0-9]+)?' | head -1 )
    [ -n "$vn_v" ] || return 1
    vn_maj=$( printf '%s' "$vn_v" | cut -d. -f1 )
    vn_min=$( printf '%s' "$vn_v" | cut -s -d. -f2 )
    printf '%s' $(( vn_maj * 100 + ${vn_min:-0} ))
}

# resolve <name> — PATH first, then the keg-only LLVM prefixes Homebrew does not link.
# Deliberately NOT applied to arm-none-eabi-*: R-TOOL-02 is a claim about what is first on
# PATH, and probing alternative locations would paper over the exact trap it exists to
# catch. The early return is what makes that true — without it R-TOOL-01 could version-check
# a binary R-TOOL-02 never probes.
resolve( ) {
    case "$1" in arm-none-eabi-*) command -v "$1"; return ;; esac
    for r_c in "$1" "/opt/homebrew/opt/llvm/bin/$1" "/usr/local/opt/llvm/bin/$1"; do
        command -v "$r_c" >/dev/null 2>&1 && { command -v "$r_c"; return 0; }
    done
    return 1
}

# check_version <binary> <min-major> <version-flag> — messages name <binary> as given, not
# the path resolve( ) found, so each LIVE label above still prefixes exactly one line.
# Absent tool: skip under OPTIONAL_TOOLS, fail otherwise. Present tool: must meet the floor.
check_version( ) {
    cv_name="$1"; cv_min="$2"; cv_flag="$3"
    cv_bin=$( resolve "$cv_name" ) || cv_bin=""
    if [ -z "$cv_bin" ]; then
        if [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
            echo "  skip: R-TOOL-01: $cv_name not on PATH"
            return 0
        fi
        echo "  FAIL: R-TOOL-01: $cv_name not on PATH"
        return 1
    fi
    cv_out=$( "$cv_bin" "$cv_flag" 2>&1 | head -1 )
    cv_have=$( ver_num "$cv_out" ) || cv_have=""
    cv_want=$( ver_num "$cv_min" )
    if [ -z "$cv_have" ]; then
        echo "  FAIL: R-TOOL-01: cannot parse a version from $cv_name: $cv_out"
        return 1
    fi
    if [ "$cv_have" -lt "$cv_want" ]; then
        echo "  FAIL: R-TOOL-01: $cv_name is below the floor of $cv_min ($cv_out)"
        return 1
    fi
    echo "  ok:   R-TOOL-01: $cv_name $cv_min+ ($cv_out)"
    return 0
}

# arm_compiles <binary> — 0 if it can build a TU that includes <cstdint> for the target.
arm_compiles( ) {
    ac_tmp=$( mktemp -d ) || return 1
    printf '#include <cstdint>\nuint8_t f( ) { return 1; }\n' > "$ac_tmp/probe.cpp"
    # Exactly the two flags R-TOOL-02 names. No -std=: that spelling is GCC 13+, so adding
    # it would fail a compliant GCC 12 — R-TOOL-01's own floor — and report the failure as
    # "no target C library", which is the wrong diagnosis entirely.
    if "$1" -mcpu=cortex-m0plus -mthumb \
         -c "$ac_tmp/probe.cpp" -o "$ac_tmp/probe.o" >"$ac_tmp/err" 2>&1; then
        rm -rf "$ac_tmp"; return 0
    fi
    ac_msg=$( head -2 "$ac_tmp/err" )
    rm -rf "$ac_tmp"
    printf '%s' "$ac_msg"
    return 1
}

# --- R-TOOL-01 ---------------------------------------------------------------------------
# Floors: clang-format/clang-tidy 23, because .clang-format uses key shapes introduced in
# 23 (PackArguments, the struct forms of AllowShortFunctionsOnASingleLine and SortIncludes).
# arm-none-eabi-g++ 12, the release in which libstdc++ gained <expected> — inferred from
# the library's history, NOT verified here; only 15.3.1 has been measured (ADR-0008
# §Verification). python3 3.8 for the meta-test — floors carry a minor number for a reason.
# probe <binary> <min> <flag> — one probe, and the ONLY place R-TOOL-01's failure
# flag is set. Four call sites used to carry `|| fail=1` each, which is four independent
# failure paths and would need four wiring cases to cover; collapsing them to one path is
# cheaper than writing four cases and proves the same thing.
probe( ) { check_version "$@" || fail=1; }

# run_all — the aggregate: every probe, both rules, and the flags they set. One function for
# the real machine and for the wiring cases at the bottom, which is what makes those cases
# able to prove that a printed FAIL reaches the exit code.
run_all( ) {
    probe clang-format      23 --version
    probe clang-tidy        23 --version
    probe arm-none-eabi-g++ 12 -dumpversion
    probe python3          3.8 --version

    # --- R-TOOL-02 -----------------------------------------------------------------------
    if command -v arm-none-eabi-g++ >/dev/null 2>&1; then
        if ra_err=$( arm_compiles arm-none-eabi-g++ ); then
            echo "  ok:   R-TOOL-02: $( command -v arm-none-eabi-g++ ) compiles <cstdint> for cortex-m0plus"
        else
            echo "  FAIL: R-TOOL-02: the arm-none-eabi-g++ first on PATH cannot compile <cstdint>"
            echo "        $( command -v arm-none-eabi-g++ )"
            printf '        %s\n' "$ra_err"
            echo "        A cross-compiler with no target C library. See docs/adr/0008-cpp23.md."
            fail=1
        fi
    elif [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
        echo "  skip: R-TOOL-02: arm-none-eabi-g++ not on PATH"
    else
        echo "  FAIL: R-TOOL-02: arm-none-eabi-g++ not on PATH"
        fail=1
    fi
}

run_all

# --- rejection cases ----------------------------------------------------------------------
# Stub binaries, because the real tools on this machine pass. Without these the whole file
# is a check nobody has ever seen reject anything.
stub_dir=$( mktemp -d ) || exit 1
rejected=0

cat > "$stub_dir/clang-format" <<'STUB'
#!/bin/sh
echo "clang-format version 14.0.6"
STUB
chmod +x "$stub_dir/clang-format"
if PATH="$stub_dir:$PATH" check_version clang-format 23 --version >/dev/null 2>&1; then
    echo "  FAIL: R-TOOL-01 rejection case did not fire — version 14 passed a floor of 23"
    fail=1
else
    rejected=$(( rejected + 1 ))
fi

cat > "$stub_dir/fake-gcc" <<'STUB'
#!/bin/sh
echo "fatal error: cstdint: No such file or directory" >&2
exit 1
STUB
chmod +x "$stub_dir/fake-gcc"
if arm_compiles "$stub_dir/fake-gcc" >/dev/null 2>&1; then
    echo "  FAIL: R-TOOL-02 rejection case did not fire — a compiler with no libc passed"
    fail=1
else
    rejected=$(( rejected + 1 ))
fi

cat > "$stub_dir/noversion" <<'STUB'
#!/bin/sh
echo "no digits here"
STUB
chmod +x "$stub_dir/noversion"
if PATH="$stub_dir:$PATH" check_version noversion 1 --version >/dev/null 2>&1; then
    echo "  FAIL: R-TOOL-01 rejection case did not fire — an unparsable banner passed"
    fail=1
else
    rejected=$(( rejected + 1 ))
fi

# The floor that needs a minor number: 3.7.9 is below 3.8 and major-only would miss it.
cat > "$stub_dir/oldpython" <<'STUB'
#!/bin/sh
echo "Python 3.7.9"
STUB
chmod +x "$stub_dir/oldpython"
if PATH="$stub_dir:$PATH" check_version oldpython 3.8 --version >/dev/null 2>&1; then
    echo "  FAIL: R-TOOL-01 rejection case did not fire — 3.7.9 passed a floor of 3.8"
    fail=1
else
    rejected=$(( rejected + 1 ))
fi

# A banner whose first number is not the version. Both were read as that first number
# (86, 99) and cleared the floor before ver_num( ) looked after the word "version". Their
# own stub name: the wiring case below needs the clang-format stub above to stay at 14.
for vb in "x86_64-apple clang-format version 14.0.6" "clang-format 99 version 14.0.6"; do
    printf '#!/bin/sh\necho "%s"\n' "$vb" > "$stub_dir/banner-format"
    chmod +x "$stub_dir/banner-format"
    if PATH="$stub_dir:$PATH" check_version banner-format 23 --version >/dev/null 2>&1; then
        echo "  FAIL: R-TOOL-01 rejection case did not fire — '$vb' passed a floor of 23"
        fail=1
    else
        rejected=$(( rejected + 1 ))
    fi
done

# The same shape at a passing version must still pass, or the two cases above could be
# satisfied by a ver_num( ) that rejects every banner with a leading number.
printf '#!/bin/sh\necho "x86_64-apple clang-format version 23.1.0"\n' > "$stub_dir/banner-format"
if PATH="$stub_dir:$PATH" check_version banner-format 23 --version >/dev/null 2>&1; then
    echo "  ok:   R-TOOL-01 accept case (a leading number before \"version\" is not the version)"
else
    echo "  FAIL: R-TOOL-01 accept case — 'x86_64-apple clang-format version 23.1.0' failed a floor of 23"
    fail=1
fi

# A floor, not an equality (§How counts are stated): the six names are clang-format below
# its floor, a cross-compiler with no target libc, an unparsable banner, a python3 below a
# floor that needs its minor number, and two banners whose first number is not the version.
if [ "$rejected" -ge 6 ]; then
    echo "  ok:   rejection cases: $rejected/6"
else
    echo "  FAIL: rejection cases: $rejected/6"
    fail=1
fi

# --- wiring cases --------------------------------------------------------------------------
# One per rule, and each stub is chosen so that ONLY that rule fails — otherwise the other
# rule's flag satisfies the case and deleting the flag under test goes unnoticed.
#
# (1) R-TOOL-01: a clang-format that reports 14. Nothing else on PATH changes.
wiring_flag=$( fail=0; PATH="$stub_dir:$PATH" run_all >/dev/null 2>&1; echo "$fail" )
wiring_txt=$( fail=0; PATH="$stub_dir:$PATH" run_all 2>&1 )
if [ "$wiring_flag" = "1" ] && printf '%s' "$wiring_txt" | grep -q 'FAIL: R-TOOL-01'; then
    echo "  ok:   R-TOOL-01 wiring case (the verdict reaches the exit code)"
else
    echo "  FAIL: R-TOOL-01 is reported but never reaches the exit code (run_all left fail=$wiring_flag)"
    fail=1
fi

# (2) R-TOOL-02: an arm-none-eabi-g++ whose -dumpversion clears R-TOOL-01's floor of 12 and
# which still cannot compile. R-TOOL-01 therefore passes and only R-TOOL-02's flag can fire.
cat > "$stub_dir/arm-none-eabi-g++" <<'STUB'
#!/bin/sh
case "$1" in
    -dumpversion ) echo "15.3.1" ;;
    * ) echo "fatal error: cstdint: No such file or directory" >&2 ; exit 1 ;;
esac
STUB
chmod +x "$stub_dir/arm-none-eabi-g++"
rm -f "$stub_dir/clang-format"
wiring_flag=$( fail=0; PATH="$stub_dir:$PATH" run_all >/dev/null 2>&1; echo "$fail" )
wiring_txt=$( fail=0; PATH="$stub_dir:$PATH" run_all 2>&1 )
if [ "$wiring_flag" = "1" ] && printf '%s' "$wiring_txt" | grep -q 'FAIL: R-TOOL-02'; then
    echo "  ok:   R-TOOL-02 wiring case (the verdict reaches the exit code)"
else
    echo "  FAIL: R-TOOL-02 is reported but never reaches the exit code (run_all left fail=$wiring_flag)"
    fail=1
fi

rm -rf "$stub_dir"

exit $fail
