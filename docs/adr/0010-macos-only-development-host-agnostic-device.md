# ADR-0010: Development is macOS-only; the device is host-agnostic

- Status: accepted
- Date: 2026-09-11

## Context

The repository already assumes macOS in three places and says so in none of them:

- `docs/constraints.md` R-TOOL-02 encodes a macOS-specific trap — "on macOS the sudo-free
  Homebrew formula is exactly that, and it shadows the working cask" — as if it were a
  general fact.
- `docs/constraints.md` §Observed conventions records, measured, that clang-tidy resolves no
  standard header without a compile database, and that the flag which fixes it is
  `-I/opt/homebrew/opt/llvm/include/c++/v1` — a Homebrew-on-Apple-Silicon path.
- `docs/phases/PHASES.md` promises 00-scaffold makes `make test` "run green from a clean
  clone with only clang++ and python3". That is platform-neutral phrasing and it is already
  false: on a clean Linux box with clang++ and python3, the suite does not run.

A fourth leak surfaced during 00-scaffold's round-13 review and is the same assumption
again: `tests/test_style.sh`'s accounting was measured on a machine where both clang tools
resolve. On the clean-clone floor, `OPTIONAL_TOOLS=1` turns its two sites into skips and the
honest result is `unproven`, not a tick.

So the decision is not new. What is missing is that it was never taken deliberately, which
leaves the repo claiming portability it does not have and leaves `01-ps2-codec` owing an
open design question — hardcoded flags versus a generated `compile_commands.json` versus
detecting the include root — that only exists because the platform was never fixed.

Development portability and device portability are independent. The firmware is
cross-compiled for the Pico either way, and a generic USB HID gamepad (ADR-0003) is
enumerated by macOS, Linux and Windows regardless of the machine that built it.
Constraining the development host costs the product nothing.

## Decision

Development is macOS-only, with Homebrew LLVM. Host tests, clang-tidy invocations, tool
detection and version floors may assume that platform and name its paths directly. The
device stays host-agnostic: nothing in `src/core/` or the USB descriptors may depend on the
build host, and the HID identity of ADR-0003 is unchanged.

The clang-tidy question `01-ps2-codec` inherited is settled by this: three flags, including
the Homebrew include path, rather than a generated compile database or include-root
detection.

## Consequences

`docs/constraints.md` R-TOOL-02 stops being an unexplained special case, and the
§Observed conventions entry stops describing a portability violation and starts describing a
documented assumption. The `PHASES.md` row's acceptance text gains its true qualifier —
green from a clean clone *on macOS with Homebrew LLVM* — which is a coarse-text correction
of a row whose cut is unchanged.

The clean-clone floor keeps its meaning but not its reach: it still proves the suite runs
without the optional tools, and `test_style.sh` is the one member of the accounting file set
whose rules nothing else covers, so that run proves strictly less about R-STYLE-01/02 and
R-CLEAN-02 than a development run does. No phase owns closing that; it is recorded here so
the next round of anyone reading a green clean-clone result knows what it did not check.

What is given up: the suite cannot run on a Linux box or a CI runner. There is no CI today,
so nothing breaks now; adding one later means either a macOS runner or reopening this ADR.
A second contributor on Linux would hit it immediately.

Rejected — leaving it implicit: it is the state today, and it is what left a false promise in
the phase row and an open design question in a phase that has not started.
Rejected — making the toolchain portable now: it prices a contributor and a CI runner that do
not exist against a question that blocks the next phase's first header.
