# pico-sg2hid — the single entry point (ADR-0001).
#
#   make test      Host tests. Needs only a C++23 compiler and python3.
#                  No hardware, no network, no ARM toolchain, no Pico SDK.
#   make lint      Style gate: clang-format layout + clang-tidy naming. Fails if
#                  either tool is missing (`make test` only skips them).
#   make typecheck Compiles each src/core/*.h on its own with the host C++ compiler,
#                  then, when PICO_SDK_PATH is set, runs `make firmware` as the
#                  typecheck of the SDK layers (skipped with a `skip:` line when it is
#                  not). Not part of `make test`.
#   make firmware  Builds build/pico/sg2hid.uf2 (the master) and build/pico/sg2hid_emu.uf2
#                  (the emulator, 05-emulator). Needs cmake, arm-none-eabi-gcc and
#                  PICO_SDK_PATH pointing at pico-sdk 2.3.1 (ADR-0014). Also writes
#                  build/pico/compile_commands.json, which `make lint` reads for
#                  src/hal, src/usb, src/app and src/emu. Not required for `make test`.
#   make clean
#
# CMake is never invoked directly; this file is the seam between the two builds.

CXX      ?= c++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc
BUILD    := build/host

CORE_SRC  := $(wildcard src/core/*.cpp)
CORE_HDRS := $(wildcard src/core/*.h)
CPP_TESTS := $(wildcard tests/test_*.cpp)
CPP_BINS  := $(patsubst tests/%.cpp,$(BUILD)/%,$(CPP_TESTS))
SH_TESTS  := $(wildcard tests/test_*.sh)
PY_TESTS  := $(wildcard tests/test_*.py)

.PHONY: test lint typecheck firmware clean

test: $(CPP_BINS)
	@fail=0; \
	for t in $(CPP_BINS) ; do echo "--- $$t"       ; "$$t"           || fail=1; done; \
	for t in $(SH_TESTS) ; do echo "--- $$t"       ; OPTIONAL_TOOLS=1 sh "$$t" || fail=1; done; \
	for t in $(PY_TESTS) ; do echo "--- $$t"       ; OPTIONAL_TOOLS=1 python3 "$$t" || fail=1; done; \
	if [ $$fail -ne 0 ]; then echo "FAIL"; exit 1; fi; \
	echo "OK"

$(BUILD)/%: tests/%.cpp $(CORE_SRC)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $< $(CORE_SRC)

lint:
	@sh tests/test_style.sh

# Each header is included from a one-line TU on stdin, not compiled as the main file: that
# form trips -Wpragma-once-outside-header and -Wunused-const-variable under -Werror, which
# say nothing about missing includes. Every header runs even after one fails. The SDK layers
# have no typecheck but the firmware build itself, so that runs last when the SDK is there.
typecheck:
	@fail=0; \
	for h in $(CORE_HDRS) ; do \
	    printf '#include "%s"\n' "$$h" | $(CXX) $(CXXFLAGS) -fsyntax-only -xc++ - \
	        || { echo "typecheck: $$h does not compile on its own"; fail=1; }; \
	done; \
	if [ -n "$$PICO_SDK_PATH" ]; then \
	    $(MAKE) --no-print-directory firmware || { echo "typecheck: the firmware build failed"; fail=1; }; \
	else \
	    echo "skip: firmware typecheck: PICO_SDK_PATH unset"; \
	fi; \
	exit $$fail

firmware:
	@command -v cmake >/dev/null 2>&1 || { echo "firmware: cmake not installed. See docs/phases/00-scaffold/verify.md"; exit 1; }
	@[ -n "$$PICO_SDK_PATH" ] || { echo "firmware: PICO_SDK_PATH is unset."; exit 1; }
	cmake -S . -B build/pico -DPICO_BOARD=pico
	cmake --build build/pico

clean:
	rm -rf build
