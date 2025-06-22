
# Prints a list.
function(printList name list)
    message(STATUS "${name}:")
    foreach(item IN LISTS ${list})
        message(STATUS "  ${item}")
    endforeach()
endfunction()


# Adds a library or executable target.
function(addTarget targetName)
    set(options)
    set(oneArgs TYPE)
    set(multiArgs HEADERS SOURCES LIBRARIES)
    list(APPEND multiArgs SOURCES_WIN SOURCES_MAC)
    list(APPEND multiArgs LIBRARIES_WIN    LIBRARIES_MAC)
    list(APPEND multiArgs FRAMEWORKS_MAC)
    list(APPEND multiArgs SHADERS_VK SHADERS_D3D SHADERS_MTL)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "${arg_TYPE}: ${targetName} processor:${CMAKE_SYSTEM_PROCESSOR} system:${CMAKE_SYSTEM_NAME}")
    printList("HEADERS"         arg_HEADERS)
    printList("SOURCES"         arg_SOURCES)
    printList("SOURCES WIN"     arg_SOURCES_WIN)
    printList("SOURCES MAC"     arg_SOURCES_MAC)
    printList("SHADERS_D3D"     arg_SHADERS_D3D)
    printList("SHADERS_VK"      arg_SHADERS_VK)
    printList("SHADERS_MTL"     arg_SHADERS_MTL)
    printList("LIBRARIES"       arg_LIBRARIES)
    printList("LIBRARIES WIN"   arg_LIBRARIES_WIN)
    printList("LIBRARIES MAC"   arg_LIBRARIES_MAC)
    printList("FRAMEWORKS MAC"  arg_FRAMEWORKS_MAC)
    list(TRANSFORM arg_FRAMEWORKS_MAC  PREPEND "-framework ")

    if (${arg_TYPE} STREQUAL LIB)
        add_library(${targetName} STATIC)
    elseif(${arg_TYPE} STREQUAL EXE)
        add_executable(${targetName})
    endif()

    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_include_directories  (${targetName} PUBLIC  ${CMAKE_SOURCE_DIR}/src/libs)
    target_link_libraries       (${targetName} PUBLIC  ${arg_LIBRARIES})
    target_sources              (${targetName} PUBLIC  ${arg_HEADERS})
    target_sources              (${targetName} PRIVATE ${arg_SOURCES})
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_WIN})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBRARIES_WIN})
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_MAC})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBRARIES_MAC})
        target_link_libraries   (${targetName} PUBLIC  ${arg_FRAMEWORKS_MAC})
    endif()
    
    file(COPY ${arg_SHADERS_D3D} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    set(shadersSpv ${arg_SHADERS_VK})
    list(TRANSFORM shadersSpv PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)
    list(TRANSFORM shadersSpv APPEND .spv)
    foreach(shader shaderSpv IN ZIP_LISTS arg_SHADERS_VK shadersSpv)
        add_custom_command(
            TARGET ${targetName} PRE_BUILD
            COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${shader}.spv
            # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
            BYPRODUCTS ${shaderSpv}
            COMMENT "Compile ${shader}"
            VERBATIM
        )
    endforeach()
    set_target_properties(${targetName} PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    # install(FILES ${shadersSpv}      DESTINATION bin/${targetName})
    # install(FILES ${arg_SHADERS_D3D} DESTINATION bin/${targetName})
    # install(
    #     TARGETS ${targetName}
    #     EXPORT ${targetName}Targets
    #     PUBLIC_HEADER DESTINATION include
    #     LIBRARY DESTINATION lib
    # )
endfunction()