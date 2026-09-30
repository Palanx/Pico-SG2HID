# ADR-0014: Build against an external pico-sdk clone pinned at tag 2.3.1

- Status: accepted
- Date: 2026-09-30

## Context

ADR-0001 chose the Pico SDK and fixed `make` as the only entry point. It did not say where
the SDK comes from or which version is used. Phase `23-firmware-build` is the first phase
to build firmware, so that choice is now due. An unpinned SDK means the same commit can
build different firmware on two days. That matters here because the SDK supplies the
firmware's C++ flags (`-fno-exceptions -fno-rtti`, R-ERR-05) and the pin-configuring
functions R-SAFETY-09 names. The latest release on 2026-09-30 is 2.3.1, published
2026-09-04.

## Decision

The SDK is a clone outside this repository, at the exact tag `2.3.1`, with its submodules
initialised (`lib/tinyusb` is needed for USB stdio). The build finds it only through the
environment variable `PICO_SDK_PATH`. `CMakeLists.txt` pins the version in
`SG2HID_PICO_SDK_VERSION`, which defaults to `2.3.1`, and stops the configure step with a
`FATAL_ERROR` naming both versions when `PICO_SDK_VERSION_STRING` differs. The operator
installs and upgrades the clone (R-PROC-05). An upgrade is a new ADR and a changed default,
made in the same commit.

## Consequences

Easier: the repository stays small; a version drift fails loudly at configure time instead
of silently changing the firmware; one clone serves every Pico project on the machine.

Harder: the build depends on a per-machine setup step that nothing in the repo performs,
and a fresh shell must export `PICO_SDK_PATH`. `make test` never needs the SDK (R-PROC-04),
so only `make firmware`, `make lint` on SDK-layer files and the firmware half of
`make typecheck` feel it.

Rejected:
- A git submodule: it adds the SDK's history to every clone of this repo, and `lib/tinyusb`
  becomes a submodule nested inside a submodule.
- CMake `FetchContent`: a network fetch inside the build, and the build is supposed to be
  reproducible offline once set up.
- `PICO_SDK_FETCH_FROM_GIT`, the SDK's own fetch helper: the same network fetch, and it
  follows a branch unless it is pinned in a second place.
