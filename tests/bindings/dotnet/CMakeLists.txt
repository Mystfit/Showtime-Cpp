# Build dotnet tests
set(DOTNET_TEST_TARGET TestDotNet)
set(DOTNET_TEST_SRC
    "${CMAKE_CURRENT_LIST_DIR}/TestDotnet.cs"
)
set(DOTNET_TEST_COMMAND "")
message(STATUS "Adding test ${DOTNET_TEST_TARGET}")

if(WIN32 AND NOT BINDINGS_DOTNET_CSPROJ)
    # Create executable
    add_executable(${DOTNET_TEST_TARGET} ${DOTNET_TEST_SRC})
    set_target_properties(${DOTNET_TEST_TARGET} PROPERTIES 
        VS_DOTNET_REFERENCE_System ON 
        DOTNET_TARGET_FRAMEWORK_VERSION ${BINDINGS_DOTNET_FRAMEWORK_VERSION}
    )
    target_link_libraries(${DOTNET_TEST_TARGET} ${DOTNET_TARGET})
    set(DOTNET_TEST_COMMAND "${DOTNET_TEST_TARGET}")
else()
    # Configure project file
    set(DOTNET_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build)
    set(DOTNET_TEST_OUTPUT ${DOTNET_TEST_BINARY_DIR}/${DOTNET_TEST_TARGET}.dll)
    set(DOTNET_TEST_CSPROJ_IN ${CMAKE_CURRENT_LIST_DIR}/${DOTNET_TEST_TARGET}.csproj.in)
    set(DOTNET_TEST_CSPROJ_OUT ${CMAKE_CURRENT_BINARY_DIR}/${DOTNET_TEST_TARGET}.csproj)

    # Add source files to csproj
    set(DOTNET_TEST_SOURCE_BLOCK "")
    foreach(file ${DOTNET_TEST_SRC})
        string(APPEND DOTNET_TEST_SOURCE_BLOCK "<Compile\ Include=\"${file}\"\ />\n    ")
    endforeach()

    # Generate csproj file
    configure_file(${DOTNET_TEST_CSPROJ_IN} ${DOTNET_TEST_CSPROJ_OUT})

    # Directory to build out tests into
    file(MAKE_DIRECTORY ${DOTNET_TEST_BINARY_DIR}) 

    # Create custom target to build our test
    add_custom_target(${DOTNET_TEST_TARGET} ALL DEPENDS ${DOTNET_TEST_OUTPUT})

    if(ADD_GENERATED_MSVC_PROJECTS)
        include_external_msproject(${DOTNET_TEST_TARGET} ${DOTNET_TEST_CSPROJ_OUT})
    endif()

    # Create command to build the csproj
    add_custom_command(
        OUTPUT ${DOTNET_TEST_OUTPUT}
        COMMAND ${DOTNETCOMMAND} "build" "-o" "${DOTNET_TEST_BINARY_DIR}" "${DOTNET_TEST_CSPROJ_OUT}"
        DEPENDS ${DOTNET_TEST_CSPROJ_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )

    set(DOTNET_TEST_COMMAND "${DOTNETCOMMAND}" "test")
endif()

add_dependencies(${DOTNET_TEST_TARGET} ${DOTNET_TARGET})

add_test(NAME ${DOTNET_TEST_TARGET} COMMAND ${DOTNET_TEST_COMMAND} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
