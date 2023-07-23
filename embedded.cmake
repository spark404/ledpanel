set( RC_DEPENDS "" )

function( add_resource input )
    string( MAKE_C_IDENTIFIER ${input} input_identifier )
    set( output "${CMAKE_BINARY_DIR}/generated/${input_identifier}.o" )
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${output} )
    message("${CMAKE_LINKER} --relocatable --format binary --output ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${input}")
    add_custom_command(
            OUTPUT ${output}
            COMMAND ${CMAKE_LINKER} --relocatable --format binary --output ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${input}
            DEPENDS ${input}
            COMMENT "koffie"
    )

    set( RC_DEPENDS ${RC_DEPENDS} ${output} PARENT_SCOPE )
endfunction()

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()