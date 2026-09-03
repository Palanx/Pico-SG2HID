#!/bin/sh
# NOT A CHECK. This is the liveness harness's bootstrap floor — read
# tests/test_checks_are_live.py, function bootstrap(), before changing anything here.
#
# It is a miniature check file that is DELIBERATELY INCOMPLETE: find_thing looks for two
# forbidden forms and the rejection case below exercises only the first. The harness must
# report exactly one uncovered alternative. If the harness is ever silently broken it will
# report none, and its own floor is what fails.
#
# Do not "fix" the missing rejection case. Do not move this file under tests/test_*.sh —
# the Makefile glob would run it as a real check, and it is designed to be incomplete.
# It carries no RULE marker on purpose: it binds no rule, and a marker here would make the
# traceability meta-test report a rule the catalogue never declared.
set -u
fail=0

find_thing( ) { grep -nE '(forbidden_alpha|forbidden_beta)' "$1/probe.txt" 2>/dev/null; }

tmp=$( mktemp -d ) || exit 1
printf 'nothing here\n' > "$tmp/probe.txt"
if [ -n "$( find_thing "$tmp" )" ]; then
    echo "  FAIL: fixture: a clean file was reported"
    fail=1
else
    echo "  ok:   fixture: clean file"
fi
rm -rf "$tmp"

# The only rejection case: forbidden_alpha. forbidden_beta deliberately has none.
tmp=$( mktemp -d ) || exit 1
printf 'forbidden_alpha\n' > "$tmp/probe.txt"
if [ -n "$( find_thing "$tmp" )" ]; then
    echo "  ok:   fixture: forbidden_alpha rejected"
else
    echo "  FAIL: fixture: forbidden_alpha not rejected"
    fail=1
fi
rm -rf "$tmp"

exit $fail
