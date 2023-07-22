cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(ledpanel)

pico_sdk_init()

add_executable(ledpanel
        src/main.c
        src/framebuffer.c src/framebuffer.h src/rgb565.h)

target_include_directories(ledpanel PRIVATE src)

target_link_libraries(ledpanel PRIVATE pico_stdlib pico_unique_id hardware_pio pico_multicore)

pico_add_extra_outputs(ledpanel)
pico_enable_stdio_uart(ledpanel 0)
pico_enable_stdio_usb(ledpanel 1)