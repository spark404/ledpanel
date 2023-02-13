cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(ledpanel)

pico_sdk_init()

add_executable(ledpanel
        src/main.c
        src/framebuffer.c src/framebuffer.h)

target_include_directories(ledpanel PRIVATE src)

target_link_libraries(ledpanel PRIVATE pico_stdlib pico_unique_id hardware_pio)

pico_add_extra_outputs(ledpanel)
