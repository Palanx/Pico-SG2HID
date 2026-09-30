#!/bin/sh
# Firmware C++ flags, read from the build's compile database.
#
# RULE R-ERR-05 — docs/constraints.md §Invariants — firmware builds pass -fno-exceptions -fno-rtti
#
# The flags come from the Pico SDK (pico_cxx_options, driven by PICO_CXX_ENABLE_EXCEPTIONS and
# PICO_CXX_ENABLE_RTTI, both pinned off in CMakeLists.txt), not from any line in this repo. So
# the check reads what CMake actually passed the compiler — build/pico/compile_commands.json,
# written by `make firmware` — rather than grepping CMakeLists.txt, which would prove only that
# a variable was set. It therefore judges the last configured build.
#
# Only C++ entries (.cpp .cc .hpp) whose file is under this repository's src/ are judged. The
# SDK's own sources are not ours, and build/pico holds generated files whose paths also contain
# /src/. A database with no such entry is a failure, never a vacuous pass. GCC takes the last
# of -fexceptions / -fno-exceptions, so a later -fexceptions overrides an earlier -fno-.
#
# Two modes, as in tests/test_style.sh: OPTIONAL_TOOLS=1 (make test) skips when no build has
# been configured; unset, a missing database is a failure.
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$( pwd )

fail=0

# check_err05 <compile-db> <src-dir> — one line per entry under <src-dir> that lacks either flag
# in effect, or one line saying there is no such entry. Silence is a pass.
check_err05( ) {
    python3 - "$1" "$2" <<'PY'
import json, os, shlex, sys
db, src = sys.argv[1], os.path.join(sys.argv[2], "")
judged = 0
for e in json.load(open(db)):
    path = os.path.normpath(os.path.join(e.get("directory", ""), e["file"]))
    if not path.startswith(src) or not path.endswith((".cpp", ".cc", ".hpp")):
        continue
    judged += 1
    args = e["arguments"] if "arguments" in e else shlex.split(e["command"])
    for off, on in (("-fno-exceptions", "-fexceptions"), ("-fno-rtti", "-frtti")):
        last = {f: max((i for i, a in enumerate(args) if a == f), default=-1) for f in (off, on)}
        if last[off] < 0:
            print("%s: no %s" % (os.path.relpath(path, sys.argv[2]), off))
        elif last[on] > last[off]:
            print("%s: %s overridden by a later %s" % (os.path.relpath(path, sys.argv[2]), off, on))
if judged == 0:
    print("no C++ entry under %s in %s" % (src, db))
PY
}

# --- real run ---------------------------------------------------------------------------
DB=${COMPILE_DB:-build/pico/compile_commands.json}
# Inline rather than tests/test_style.sh's skip_or_fail( ): a function here is mutated by
# tests/test_checks_are_live.py, and this branch never runs while a build exists.
if [ ! -f "$DB" ] && [ "${OPTIONAL_TOOLS:-0}" = "1" ]; then
    echo "  skip: R-ERR-05: no compile database at $DB — run make firmware"
elif [ ! -f "$DB" ]; then
    echo "  FAIL: R-ERR-05: no compile database at $DB — run make firmware"
    fail=1
elif out=$( check_err05 "$DB" "$ROOT/src" ); [ -n "$out" ]; then
    echo "  FAIL: R-ERR-05: firmware C++ compiles without -fno-exceptions -fno-rtti in effect"
    printf '%s\n' "$out" | sed 's/^/        /'
    fail=1
else
    echo "  ok:   R-ERR-05 ($DB)"
fi

# --- cases ------------------------------------------------------------------------------
# Hand-written databases, fed to the same function. /repo stands in for the repository root.
case_tmp=$( mktemp -d ) || exit 1
trap 'rm -rf "$case_tmp"' EXIT
passed=0

# err05_case <accept|rejection> <label> <json> — rejection: must print something; accept: nothing.
err05_case( ) {
    printf '%s\n' "$3" > "$case_tmp/db.json"
    ec_out=$( check_err05 "$case_tmp/db.json" /repo/src )
    if { [ "$1" = rejection ] && [ -n "$ec_out" ]; } || { [ "$1" = accept ] && [ -z "$ec_out" ]; }; then
        echo "  ok:   R-ERR-05 $1 case: $2"
        passed=$(( passed + 1 ))
    else
        echo "  FAIL: R-ERR-05 $1 case: $2"
        printf '%s\n' "$ec_out" | sed 's/^/        /'
        fail=1
    fi
}

err05_case rejection "-fno-rtti missing" '[
  {"directory": "/repo/build/pico", "file": "/repo/src/core/link.cpp",
   "command": "arm-none-eabi-g++ -std=gnu++23 -fno-exceptions -c /repo/src/core/link.cpp"}
]'
err05_case rejection "-fno-exceptions overridden by a later -fexceptions" '[
  {"directory": "/repo/build/pico", "file": "/repo/src/app/main.cpp",
   "command": "arm-none-eabi-g++ -fno-exceptions -fno-rtti -fexceptions -c /repo/src/app/main.cpp"}
]'
err05_case rejection "no entry under src/" '[
  {"directory": "/repo/build/pico", "file": "/sdk/src/rp2_common/pico_stdlib/stdlib.c",
   "command": "arm-none-eabi-gcc -c /sdk/src/rp2_common/pico_stdlib/stdlib.c"},
  {"directory": "/repo/build/pico", "file": "/repo/build/pico/src/gen.cpp",
   "command": "arm-none-eabi-g++ -c /repo/build/pico/src/gen.cpp"}
]'
err05_case accept "SDK entry without the flags beside a compliant src/ entry" '[
  {"directory": "/repo/build/pico", "file": "/sdk/src/rp2_common/pico_cxx_options/new_delete.cpp",
   "command": "arm-none-eabi-g++ -c /sdk/src/rp2_common/pico_cxx_options/new_delete.cpp"},
  {"directory": "/repo/build/pico", "file": "/repo/src/core/link.cpp",
   "arguments": ["arm-none-eabi-g++", "-fexceptions", "-fno-exceptions", "-fno-rtti", "-c", "/repo/src/core/link.cpp"]}
]'

# Counted, so an err05_case( ) that checks nothing and prints nothing is still a failure.
if [ "$passed" -eq 4 ]; then
    echo "  ok:   R-ERR-05 rejection and accept cases: $passed/4"
else
    echo "  FAIL: R-ERR-05 rejection and accept cases: $passed/4"
    fail=1
fi

exit $fail
