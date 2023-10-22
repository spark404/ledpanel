add_library(gif_decoder INTERFACE)

target_sources(gif_decoder INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/gif_decoder.c
        ${CMAKE_CURRENT_LIST_DIR}/gif_lzw_decompress.c
)

target_include_directories(gif_decoder INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(gif_decoder INTERFACE
        pico_stdlib
)