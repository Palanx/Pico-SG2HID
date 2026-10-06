# pico-sg2hid firmware. Never run cmake by hand: `make firmware` is the entry point (ADR-0001).
# The SDK is an external clone at the tag pinned below, found through PICO_SDK_PATH (ADR-0014).
cmake_minimum_required( VERSION 3.13 )

# The SDK defaults to C++17; this project is C++23 (ADR-0008).
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Normal variables, not cache entries, so a -D on the command line cannot turn either on.
# The SDK turns them into -fno-exceptions and -fno-rtti (R-ERR-05, checked from the compile
# database by tests/test_firmware_flags.sh).
set( PICO_CXX_ENABLE_EXCEPTIONS 0 )
set( PICO_CXX_ENABLE_RTTI 0 )

include( $ENV{PICO_SDK_PATH}/pico_sdk_init.cmake )

project( sg2hid C CXX ASM )

# A cache entry only so the mismatch path can be exercised; the default is the pin.
set( SG2HID_PICO_SDK_VERSION "2.3.1" CACHE STRING "The pico-sdk version this firmware is pinned to (ADR-0014)" )

pico_sdk_init( )

if( NOT PICO_SDK_VERSION_STRING STREQUAL SG2HID_PICO_SDK_VERSION )
    message( FATAL_ERROR "pico-sdk ${PICO_SDK_VERSION_STRING} found at $ENV{PICO_SDK_PATH}, "
                         "but this firmware is pinned to ${SG2HID_PICO_SDK_VERSION} (ADR-0014)" )
endif( )

# Two firmwares, each from an explicit list: a glob over src/hal/ would link the master's and the
# emulator's PIO ports into both, and each claims the bus pins from its own table.
file( GLOB SG2HID_CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp )
file( GLOB SG2HID_APP_SOURCES CONFIGURE_DEPENDS src/app/*.cpp )
file( GLOB SG2HID_EMU_SOURCES CONFIGURE_DEPENDS src/emu/*.cpp )

# The master: build/pico/sg2hid.uf2.
set( SG2HID_SOURCES ${SG2HID_CORE_SOURCES} src/hal/bus_frame.cpp src/hal/pio_port.cpp ${SG2HID_APP_SOURCES} )
add_executable( sg2hid ${SG2HID_SOURCES} )
target_include_directories( sg2hid PRIVATE src )
# Our sources only. The SDK's libraries are INTERFACE libraries, so their .c files compile
# inside this target too, and target_compile_options would put -Werror on them.
set_source_files_properties( ${SG2HID_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror" )
# The PS2 bus master's PIO program, assembled into build/pico/ps2_master.pio.h.
pico_generate_pio_header( sg2hid ${CMAKE_CURRENT_LIST_DIR}/src/hal/ps2_master.pio )
target_link_libraries( sg2hid pico_stdlib hardware_pio )

# UART stdio would claim GP0 and GP1, which src/core/pins.h does not declare (R-SAFETY-02).
pico_enable_stdio_usb( sg2hid 1 )
pico_enable_stdio_uart( sg2hid 0 )
pico_add_extra_outputs( sg2hid )

# The emulator, flashed to a second Pico (ADR-0004): build/pico/sg2hid_emu.uf2.
set( SG2HID_EMU_ALL_SOURCES ${SG2HID_CORE_SOURCES} src/hal/pio_device.cpp ${SG2HID_EMU_SOURCES} )
add_executable( sg2hid_emu ${SG2HID_EMU_ALL_SOURCES} )
target_include_directories( sg2hid_emu PRIVATE src )
set_source_files_properties( ${SG2HID_EMU_ALL_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror" )
# The emulator's PIO program, assembled into build/pico/ps2_device.pio.h.
pico_generate_pio_header( sg2hid_emu ${CMAKE_CURRENT_LIST_DIR}/src/hal/ps2_device.pio )
target_link_libraries( sg2hid_emu pico_stdlib hardware_pio )

# The command port is USB; UART stdio would claim GP0 and GP1, as above.
pico_enable_stdio_usb( sg2hid_emu 1 )
pico_enable_stdio_uart( sg2hid_emu 0 )
pico_add_extra_outputs( sg2hid_emu )
