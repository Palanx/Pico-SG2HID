---
paths:
  - "tests/test_repo_shape.sh"
  - "docs/constraints.md"
  - "Makefile"
  - ".claude/workflow/toolchain.manual.json"
  - "src/core/**"
---

# Tech debt log

## No typecheck gate configured (reviewed 2026-09-28)

Files: `Makefile`, `.claude/workflow/toolchain.manual.json`, `src/core/*.h`

`scripts/check.sh` prints `workflow gap: no 'typecheck' tool configured` on every run, because
neither `toolchain.json` nor `toolchain.manual.json` declares a `typecheck` command.

Nothing breaks today because `make test` compiles every `src/core/*.cpp` and every
`tests/test_*.cpp` with `-Werror`, and `tests/test_pin_table.py` compiles
`tests/pin_table_cases.cpp`. So every `src/core/` header is type-checked inside some `.cpp`.
What nobody checks is whether each header compiles *standalone*: a header that relies on an
include its user happened to add first passes today. That stops being true when a header is
included from a new file first, and it stops covering the tree at all once `src/hal/`,
`src/usb/`, `src/app/` or `src/emu/` exist (from `03-pio-bus`), since those need the Pico SDK
and `make test` must never need it.

Fix, cheapest first:
- A `make typecheck` target: `$(CXX) $(CXXFLAGS) -fsyntax-only -xc++ <header>` for each
  `src/core/*.h` (the same form phase `02-wiring`'s acceptance criterion already runs on
  `pins.h`). Register it as `"typecheck": "make typecheck"` in `toolchain.manual.json`.
  Seconds to run, no new dependency. It does not cover SDK-dependent code.
- `make firmware` as the typecheck for `src/hal/` and the other SDK layers. It is the only
  real check for that code, but it needs cmake, `arm-none-eabi-gcc` and `PICO_SDK_PATH`,
  which the gates must not require. Revisit in `03-pio-bus`.
- Not recommended: `"typecheck": "make test"`. It silences the gap without checking anything
  new, and it doubles the ~10-minute `make test` inside `check.sh`. Also not recommended: mypy
  for the `tests/*.py` drivers, which are unannotated scripts, so it adds a dependency for
  little gain.

Where it was found: `/validate-phase 02-wiring`, 2026-09-28. The operator deferred it to a
`/plan-feature` after `02-wiring` closes.

