#!/bin/sh
# Style gate: layout (clang-format) and naming (clang-tidy).
#
# RULE R-STYLE-01 — docs/constraints.md §Invariants — layout matches .clang-format
# RULE R-STYLE-02 — docs/constraints.md §Invariants — naming matches .clang-tidy
# RULE R-CLEAN-02 — docs/constraints.md §Invariants — function size, params, nesting
#
# Two modes, one script:
#   STYLE_OPTIONAL=1  missing tools are reported and skipped   (used by `make test`,
#                     which must run with only a C++17 compiler and python3, R-PROC-04)
#   unset             missing tools are a failure              (used by `make lint`)
#
# clang-tidy scope is src/core/ and tests/ only: everything else includes Pico SDK or
# TinyUSB headers and needs a compile_commands.json that does not exist yet. See the
# header of .clang-tidy for the upgrade path.
set -u
cd "$(dirname "$0")/.." || exit 1

fail=0
skip_or_fail() {
  if [ "${STYLE_OPTIONAL:-0}" = "1" ]; then
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
sources() { $LS '*.cpp' '*.h' 2>/dev/null; }
tidy_sources() { $LS 'src/core/*.cpp' 'src/core/*.h' 'tests/*.cpp' 2>/dev/null; }

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
  elif echo "$files" | xargs clang-format --style=file --dry-run --Werror 2>&1 | grep -q .; then
    echo "  FAIL: R-STYLE-01: formatting differs. Fix: clang-format -i \$(git ls-files '*.cpp' '*.h')"
    echo "$files" | xargs clang-format --style=file --dry-run --Werror 2>&1 | sed 's/^/        /'
    fail=1
  else
    echo "  ok:   R-STYLE-01"
  fi
fi

# --- R-STYLE-02: naming -----------------------------------------------------------
if ! TIDY=$(find_tidy); then
  skip_or_fail "R-STYLE-02 / R-CLEAN-02: clang-tidy not found (brew install llvm)"
else
  files=$(tidy_sources)
  if [ -z "$files" ]; then
    echo "  ok:   R-STYLE-02, R-CLEAN-02 (no checkable sources yet)"
  else
    # One invocation per file, never xargs: xargs appends the file list AFTER the
    # `--`, where clang-tidy reads it as compiler flags and silently checks nothing.
    out=$(for f in $files; do "$TIDY" --quiet "$f" -- -std=c++17 -Isrc 2>&1; done \
          | grep -E ': (warning|error): ')
    if [ -n "$out" ]; then
      echo "  FAIL: R-STYLE-02 / R-CLEAN-02: naming or function-size violations"
      echo "$out" | sed 's/^/        /'
      fail=1
    else
      echo "  ok:   R-STYLE-02, R-CLEAN-02"
    fi
  fi
fi

exit $fail
