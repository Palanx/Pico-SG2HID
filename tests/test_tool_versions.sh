#!/bin/sh
# The tools the other checks depend on are present, recent enough, and actually work.
#
# RULE R-TOOL-01 — docs/constraints.md §Invariants — minimum versions
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

# ver_num <text> — the first version in a banner as a comparable integer, major*100+minor.
# Handles all four shapes, and a floor written the same way ("23" -> 2300, "3.8" -> 308):
#   "Homebrew clang-format version 23.1.0"   "Homebrew LLVM version 23.1.0"
#   "Python 3.14.6"                          "15.3.1" (gcc -dumpversion, may be bare "15")
# Major alone is not enough: R-TOOL-01's python3 floor is 3.8, and every Python this
# project will ever meet is major 3.
ver_num( ) {
    vn_v=$( printf '%s' "$1" | grep -oE '[0-9]+(\.[0-9]+)?' | head -1 )
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

# check_version <label> <binary> <min-major> <version-flag>
# Absent tool: skip under OPTIONAL_TOOLS, fail otherwise. Present tool: must meet the floor.
check_version( ) {
    cv_label="$1"; cv_bin="$2"; cv_min="$3"; cv_flag="$4"
    cv_bin=$( resolve "$cv_bin" ) || cv_bin=""
    if [ -z "$cv_bin" ]; then
        if [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
            echo "  skip: R-TOOL-01: $cv_label not on PATH"
            return 0
        fi
        echo "  FAIL: R-TOOL-01: $cv_label not on PATH"
        return 1
    fi
    cv_out=$( "$cv_bin" "$cv_flag" 2>&1 | head -1 )
    cv_have=$( ver_num "$cv_out" ) || cv_have=""
    cv_want=$( ver_num "$cv_min" )
    if [ -z "$cv_have" ]; then
        echo "  FAIL: R-TOOL-01: cannot parse a version from $cv_label: $cv_out"
        return 1
    fi
    if [ "$cv_have" -lt "$cv_want" ]; then
        echo "  FAIL: R-TOOL-01: $cv_label is below the floor of $cv_min ($cv_out)"
        return 1
    fi
    echo "  ok:   R-TOOL-01: $cv_label $cv_min+ ($cv_out)"
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
check_version "clang-format"      clang-format      23 --version   || fail=1
check_version "clang-tidy"        clang-tidy        23 --version   || fail=1
check_version "arm-none-eabi-g++" arm-none-eabi-g++ 12 -dumpversion || fail=1
check_version "python3"           python3          3.8 --version   || fail=1

# --- R-TOOL-02 ---------------------------------------------------------------------------
if command -v arm-none-eabi-g++ >/dev/null 2>&1; then
    if err=$( arm_compiles arm-none-eabi-g++ ); then
        echo "  ok:   R-TOOL-02: $( command -v arm-none-eabi-g++ ) compiles <cstdint> for cortex-m0plus"
    else
        echo "  FAIL: R-TOOL-02: the arm-none-eabi-g++ first on PATH cannot compile <cstdint>"
        echo "        $( command -v arm-none-eabi-g++ )"
        printf '        %s\n' "$err"
        echo "        A cross-compiler with no target C library. See docs/adr/0008-cpp23.md."
        fail=1
    fi
elif [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
    echo "  skip: R-TOOL-02: arm-none-eabi-g++ not on PATH"
else
    echo "  FAIL: R-TOOL-02: arm-none-eabi-g++ not on PATH"
    fail=1
fi

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
if PATH="$stub_dir:$PATH" check_version "clang-format" clang-format 23 --version >/dev/null 2>&1; then
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
if PATH="$stub_dir:$PATH" check_version "noversion" noversion 1 --version >/dev/null 2>&1; then
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
if PATH="$stub_dir:$PATH" check_version "oldpython" oldpython 3.8 --version >/dev/null 2>&1; then
    echo "  FAIL: R-TOOL-01 rejection case did not fire — 3.7.9 passed a floor of 3.8"
    fail=1
else
    rejected=$(( rejected + 1 ))
fi

rm -rf "$stub_dir"
echo "  ok:   rejection cases: $rejected/4"
[ "$rejected" -eq 4 ] || fail=1

exit $fail
