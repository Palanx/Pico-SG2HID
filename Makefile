# pico-sg2hid — the single entry point (ADR-0001).
#
#   make test      Host tests. Needs only a C++23 compiler and python3.
#                  No hardware, no network, no ARM toolchain, no Pico SDK.
#   make lint      Style gate: clang-format layout + clang-tidy naming. Fails if
#                  either tool is missing (`make test` only skips them).
#   make firmware  Builds the .uf2 images. Needs cmake, arm-none-eabi-gcc and
#                  PICO_SDK_PATH. Not required for `make test` to pass.
#   make clean
#
# CMake is never invoked directly; this file is the seam between the two builds.

CXX      ?= c++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Werror -Og -g -UNDEBUG -Isrc
BUILD    := build/host

CORE_SRC  := $(wildcard src/core/*.cpp)
CPP_TESTS := $(wildcard tests/test_*.cpp)
CPP_BINS  := $(patsubst tests/%.cpp,$(BUILD)/%,$(CPP_TESTS))
SH_TESTS  := $(wildcard tests/test_*.sh)
PY_TESTS  := $(wildcard tests/test_*.py)

.PHONY: test lint firmware clean

test: $(CPP_BINS)
	@fail=0; \
	for t in $(CPP_BINS) ; do echo "--- $$t"       ; "$$t"           || fail=1; done; \
	for t in $(SH_TESTS) ; do echo "--- $$t"       ; OPTIONAL_TOOLS=1 sh "$$t" || fail=1; done; \
	for t in $(PY_TESTS) ; do echo "--- $$t"       ; python3 "$$t"   || fail=1; done; \
	if [ $$fail -ne 0 ]; then echo "FAIL"; exit 1; fi; \
	echo "OK"

$(BUILD)/%: tests/%.cpp $(CORE_SRC)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $< $(CORE_SRC)

lint:
	@sh tests/test_style.sh

firmware:
	@command -v cmake >/dev/null 2>&1 || { echo "firmware: cmake not installed. See docs/phases/00-scaffold/verify.md"; exit 1; }
	@[ -n "$$PICO_SDK_PATH" ] || { echo "firmware: PICO_SDK_PATH is unset."; exit 1; }
	cmake -S . -B build/pico -DPICO_BOARD=pico
	cmake --build build/pico

clean:
	rm -rf build
