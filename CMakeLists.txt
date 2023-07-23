cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(embedded.cmake)

project(ledpanel)

pico_sdk_init()

add_executable(ledpanel
        src/main.c
        src/framebuffer.c src/gif_decoder.c src/gif_lzw_decompress.c)

add_resource( "seasons.gif" )

target_include_directories(ledpanel PRIVATE src)

target_link_libraries(ledpanel PRIVATE pico_stdlib pico_unique_id hardware_pio pico_multicore)
dump_cmake_variables()
pico_add_extra_outputs(ledpanel)
pico_enable_stdio_uart(ledpanel 0)
pico_enable_stdio_usb(ledpanel 1)