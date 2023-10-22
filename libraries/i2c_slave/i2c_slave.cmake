add_library(i2c_slave INTERFACE)

target_sources(i2c_slave INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/i2c_slave.c
)

target_include_directories(i2c_slave INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(i2c_slave INTERFACE
        pico_stdlib hardware_i2c
)