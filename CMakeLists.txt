cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(embedded.cmake)

include_directories("util")

project(ledpanel)

pico_sdk_init()

add_executable(ledpanel
        src/main.c
        src/framebuffer.c src/gif_decoder.c src/gif_lzw_decompress.c src/animations/plasma.c src/animations/gif_animation.c)

add_resource( "images/seasons.gif" )
add_resource( "images/baloons.gif" )
add_resource( "images/chevrons.gif" )
add_resource( "images/fishandcat.gif" )
add_resource( "images/skull.gif" )
add_resource( "images/snowing.gif" )


target_include_directories(ledpanel PRIVATE src)

target_link_libraries(ledpanel PRIVATE pico_stdlib pico_unique_id hardware_pio pico_multicore ${RC_DEPENDS})

pico_add_extra_outputs(ledpanel)
pico_enable_stdio_uart(ledpanel 0)
pico_enable_stdio_usb(ledpanel 1)