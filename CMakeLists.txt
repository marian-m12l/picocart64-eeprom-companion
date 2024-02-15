cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(picocart64_eeprom C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )



set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)

add_executable(picocart64_eeprom
        eeprom.cpp
        joybus-pio/src/joybus.c
        joybus-pio/src/N64EEPROM.cpp
        )

include_directories(joybus-pio/include)

pico_generate_pio_header(picocart64_eeprom ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/joybus.pio)

# pull in common dependencies
target_link_libraries(picocart64_eeprom pico_stdlib hardware_flash hardware_pio)

# create map/bin/hex file etc.
pico_add_extra_outputs(picocart64_eeprom)

pico_enable_stdio_usb(picocart64_eeprom 1)
pico_enable_stdio_uart(picocart64_eeprom 0)

pico_set_binary_type(picocart64_eeprom copy_to_ram)
