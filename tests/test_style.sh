#!/bin/sh
# Style gate: layout (clang-format) and naming (clang-tidy).
#
# RULE R-STYLE-01 — docs/constraints.md §Invariants — layout matches .clang-format
# RULE R-STYLE-02 — docs/constraints.md §Invariants — naming matches .clang-tidy
# RULE R-CLEAN-02 — docs/constraints.md §Invariants — function size, params, nesting
# RULE R-CLEAN-04 — docs/constraints.md §Invariants — no magic numbers in logic
#
# R-CLEAN-04 is reported on the same line as R-STYLE-02 and R-CLEAN-02 because all three
# come out of one clang-tidy invocation: there is one real run, so there is one result
# line, and splitting it into three would claim three independent checks where there is
# one. What that line does NOT cover is recorded in docs/constraints.md §Observed
# conventions (2026-09-11): readability-magic-numbers ignores every literal inside a
# `const` or `constexpr` variable's initializer, and `const bool is_ok = <expr>;` is this
# repo's dominant idiom — so a magic number in one of those is invisible here.
#
# Two modes, one script:
#   OPTIONAL_TOOLS=1  missing tools are reported and skipped   (used by `make test`,
#                     which must run with only a C++23 compiler and python3, R-PROC-04)
#   unset             missing tools are a failure              (used by `make lint`)
#
# clang-tidy scope is every .cpp, .h, .hpp, .cc and .inl under src/ (any layer, any depth)
# plus tests/*.cpp. clang-format's scope is the same five extensions anywhere in the repo.
# A file under src/hal, src/usb, src/app or src/emu that includes a Pico SDK or TinyUSB
# header fails lint with clang-diagnostic-error until 03-pio-bus supplies its include flags;
# that failure is loud, which is the point — skipping those layers was a silent hole. See the
# header of .clang-tidy for the upgrade path.
#
# Two flags, for two different failures, both measured 2026-09-11 with Homebrew LLVM 23.1.0
# and both recorded in docs/constraints.md §Observed conventions:
#
#   -xc++     A .h with no compile database is compiled as C: `invalid argument '-std=c++23'
#             not allowed with 'C'`, then `'cstdint' file not found`. A .inl with no -x
#             fails with `unable to handle compilation, expected exactly one compiler job`
#             (measured 2026-09-23). .h and .inl only — .cpp, .hpp and .cc are already C++.
#   -isysroot Homebrew's libc++ finds no platform C library without it, so any header that
#             reaches its platform layer dies on "We don't know how to get the definition of
#             mbstate_t on your platform" — <array>, <optional>, <string_view>, <algorithm>,
#             <functional> and <variant> all do. <cstdint>, <cstddef>, <span>, <bit>,
#             <limits>, <type_traits>, <concepts>, <utility>, <tuple> and <expected> do not,
#             which is why a probe built from those two alone reports the flag unnecessary.
#             Both kinds of file need it.
#
# Every diagnostic above is a clang-diagnostic-error, which WarningsAsErrors reports as a
# FAIL: R-STYLE-02 line — so a broken invocation looks exactly like a naming violation. That
# is the reason the flags are explained here rather than just set.
#
# File lists reach both tools one whole line per path, through format_files( ) and
# tidy_files( ). Split on spaces (`xargs`, `for f in $files`), `src/core/a b.cpp` became two
# paths that do not exist, and the FAIL line carried `No such file` instead of the file's real
# diagnostic. Each block therefore has one rejection case: a violation in a scratch
# `a b.cpp`, run through the same function as the real run, which must report that file's
# diagnostic and no missing file. A newline in a file name still splits (no NUL read in sh).
set -u
cd "$(dirname "$0")/.." || exit 1

fail=0
skip_or_fail() {
  if [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
    echo "  skip: $1"
  else
    echo "  FAIL: $1"
    fail=1
  fi
}

# clang-tidy ships in the keg-only llvm formula on macOS, so PATH alone is not enough.
find_tidy() {
  for c in clang-tidy /opt/homebrew/opt/llvm/bin/clang-tidy /usr/local/opt/llvm/bin/clang-tidy; do
    command -v "$c" >/dev/null 2>&1 && { echo "$c"; return 0; }
  done
  return 1
}

# --cached --others --exclude-standard: tracked AND new-but-not-ignored files. Plain
# `git ls-files` would let a brand-new .cpp slip past the gate until someone `git add`ed it.
LS="git ls-files --cached --others --exclude-standard"
sources() { $LS '*.cpp' '*.h' '*.hpp' '*.cc' '*.inl' 2>/dev/null; }
tidy_sources() { $LS 'src/*.cpp' 'src/*.h' 'src/*.hpp' 'src/*.cc' 'src/*.inl' 'tests/*.cpp' 2>/dev/null; }

# tidy_lang <file> — the -x flag this file needs, empty for a .cpp, .hpp or .cc. A function and not an
# inline `case`, because bash 3.2 — which is /bin/sh on macOS, and macOS is the only
# development platform (ADR-0010) — reads the `)` closing a case pattern inside $( ) as the
# end of the substitution and dies with `syntax error near unexpected token ';;'`.
tidy_lang() {
  case "$1" in
    *.h|*.inl) echo "-xc++" ;;
    *)   echo "" ;;
  esac
}

# The macOS SDK, or empty when xcrun cannot name one. Resolved once, and never allowed to
# expand to a bare `-isysroot` with nothing after it: that consumes the next flag as its
# argument and the failure that follows names -std, not the missing SDK.
tidy_sysroot_flag() {
  ts_sdk=$( xcrun --show-sdk-path 2>/dev/null ) || ts_sdk=""
  [ -n "$ts_sdk" ] && [ -d "$ts_sdk" ] && echo "-isysroot $ts_sdk"
  return 0
}

# The scratch dir for the rejection cases. Both tools find their config by walking up from the
# file, so it holds copies of .clang-format and .clang-tidy.
case_tmp=$( mktemp -d ) || exit 1
trap 'rm -rf "$case_tmp"' EXIT
cp .clang-format .clang-tidy "$case_tmp/"

# style_case <label> <output> <diagnostic> <missing-file-text> — ok when <output> carries
# <diagnostic> on a line naming `a b.cpp:` and never <missing-file-text>.
style_case() {
  if printf '%s\n' "$2" | grep -q "a b\.cpp:.*$3" && ! printf '%s\n' "$2" | grep -q "$4"; then
    echo "  ok:   $1 rejection case: file name with a space"
  else
    echo "  FAIL: $1 rejection case: file name with a space not diagnosed as $3"
    printf '%s\n' "$2" | head -8 | sed 's/^/        /'
    fail=1
  fi
}

# format_files <file-list> — one clang-format run per line of <file-list>. Returns 0: the
# verdict is its output. The loop's status is only the LAST file's, so `&&` on it passed a
# tree whose last file was misformatted.
format_files() {
  printf '%s\n' "$1" | while IFS= read -r f; do
    clang-format --style=file --dry-run --Werror "$f" 2>&1
  done
  return 0
}

# --- R-STYLE-01: layout -----------------------------------------------------------
if ! command -v clang-format >/dev/null 2>&1; then
  skip_or_fail "R-STYLE-01: clang-format not found (brew install clang-format)"
elif ! clang-format --style=file --dump-config >/dev/null 2>&1; then
  echo "  FAIL: R-STYLE-01: .clang-format is not valid for clang-format $(clang-format --version | awk '{print $NF}')"
  fail=1
else
  files=$(sources)
  if [ -z "$files" ]; then
    echo "  ok:   R-STYLE-01 (no C++ sources yet)"
  elif out=$( format_files "$files" ); [ -n "$out" ]; then
    echo "  FAIL: R-STYLE-01: formatting differs. Fix: clang-format -i \$(git ls-files '*.cpp' '*.h' '*.hpp' '*.cc' '*.inl')"
    echo "$out" | sed 's/^/        /'
    fail=1
  else
    echo "  ok:   R-STYLE-01"
  fi
  printf 'int  x=1;\n' > "$case_tmp/a b.cpp"
  style_case R-STYLE-01 "$( format_files "$case_tmp/a b.cpp" )" clang-format-violations "No such file"
fi

# --- R-STYLE-02: naming -----------------------------------------------------------
if ! TIDY=$(find_tidy); then
  skip_or_fail "R-STYLE-02 / R-CLEAN-02 / R-CLEAN-04: clang-tidy not found (brew install llvm)"
else
  files=$(tidy_sources)
  # One invocation per file, never xargs: xargs appends the file list AFTER the
  # `--`, where clang-tidy reads it as compiler flags and silently checks nothing.
  # -xc++ on .h and .inl only, per the note at the top of this file, and not as a single
  # unconditional flag: -xc++ on a .cpp is accepted but then the language comes from this
  # line rather than from the file, which is the kind of flag that outlives a rename.
  SYSROOT=$( tidy_sysroot_flag )
  if [ -z "$SYSROOT" ]; then
    echo "  note: no macOS SDK from xcrun; any header reaching libc++'s platform layer"
    echo "        will report clang-diagnostic-error (see the flag note at the top)"
  fi
  # tidy_files <file-list> — one clang-tidy run per line of <file-list>, diagnostics only.
  # $( tidy_lang ) and $SYSROOT stay unquoted: empty is no argument, the sysroot is two.
  tidy_files() {
    printf '%s\n' "$1" | while IFS= read -r f; do
      "$TIDY" --quiet "$f" -- $( tidy_lang "$f" ) -std=c++23 -Isrc $SYSROOT 2>&1
    done | grep -E '(: (warning|error): |^error: )'
  }
  if [ -z "$files" ]; then
    echo "  ok:   R-STYLE-02, R-CLEAN-02, R-CLEAN-04 (no checkable sources yet)"
  else
    out=$( tidy_files "$files" )
    if [ -n "$out" ]; then
      echo "  FAIL: R-STYLE-02 / R-CLEAN-02 / R-CLEAN-04: naming, function-size or magic-number violations"
      echo "$out" | sed 's/^/        /'
      fail=1
    else
      echo "  ok:   R-STYLE-02, R-CLEAN-02, R-CLEAN-04"
    fi
  fi
  printf 'void BadName( ) {}\n' > "$case_tmp/a b.cpp"
  style_case R-STYLE-02 "$( tidy_files "$case_tmp/a b.cpp" )" readability-identifier-naming "no such file or directory"
fi

exit $fail
