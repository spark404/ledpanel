function( add_resource input )
    string( MAKE_C_IDENTIFIER ${input} input_identifier )
    set( output "${PROJECT_BINARY_DIR}/generated/${input_identifier}${CMAKE_C_OUTPUT_EXTENSION}" )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${output} )
    message("${CMAKE_LINKER} --relocatable --format binary --output ${PROJECT_BINARY_DIR}/generated/${output} ${PROJECT_SOURCE_DIR}/${input}")

    add_custom_command(
            OUTPUT ${output}
            COMMAND ${CMAKE_LINKER} --relocatable --format binary --output ${PROJECT_BINARY_DIR}/generated/${output} ${PROJECT_SOURCE_DIR}/${input}
            DEPENDS ${input}
            COMMENT "koffie"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endfunction()