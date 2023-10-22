cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(embedded.cmake)
include(libraries/i2c_slave/i2c_slave.cmake)
include(libraries/gif_decoder/gif_decoder.cmake)

include_directories("util")

project(ledpanel)

pico_sdk_init()

add_executable(ledpanel
        src/main.c
        src/framebuffer.c
        src/animations/plasma.c
        src/animations/gif_animation.c
)

add_resource( "images/baloons.gif" )
add_resource( "images/chevrons.gif" )
add_resource( "images/fishandcat.gif" )
add_resource( "images/lattice.gif" )
add_resource( "images/numbers.gif" )
add_resource( "images/pattern.gif" )
add_resource( "images/seasons.gif" )
add_resource( "images/skull.gif" )
add_resource( "images/snafu.gif" )
add_resource( "images/snowing.gif" )
add_resource( "images/pnp2000.gif" )


target_include_directories(ledpanel PRIVATE src)

target_link_libraries(ledpanel PRIVATE
        pico_stdlib pico_unique_id hardware_pio pico_multicore hardware_i2c
        ${RC_DEPENDS}
        i2c_slave gif_decoder
)

pico_add_extra_outputs(ledpanel)
pico_enable_stdio_uart(ledpanel 1)
pico_enable_stdio_usb(ledpanel 0)