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

file( GLOB SG2HID_SOURCES CONFIGURE_DEPENDS src/core/*.cpp src/hal/*.cpp src/app/*.cpp )
add_executable( sg2hid ${SG2HID_SOURCES} )
target_include_directories( sg2hid PRIVATE src )
# Our sources only. The SDK's libraries are INTERFACE libraries, so their .c files compile
# inside this target too, and target_compile_options would put -Werror on them.
set_source_files_properties( ${SG2HID_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror" )
target_link_libraries( sg2hid pico_stdlib )

# UART stdio would claim GP0 and GP1, which src/core/pins.h does not declare (R-SAFETY-02).
pico_enable_stdio_usb( sg2hid 1 )
pico_enable_stdio_uart( sg2hid 0 )
pico_add_extra_outputs( sg2hid )
