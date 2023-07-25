function( add_resource input )
    string( MAKE_C_IDENTIFIER ${input} input_identifier )
    set( output "${input_identifier}.S" )

    target_sources( ${PROJECT_NAME} PRIVATE ${output} )
    add_custom_command(
            OUTPUT ${output}
            COMMAND util/build/bin2asm ${input_identifier} > ${PROJECT_BINARY_DIR}/${output} < ${PROJECT_SOURCE_DIR}/${input}
            DEPENDS ${input}
            COMMENT "util/build/bin2asm ${input_identifier}"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endfunction()