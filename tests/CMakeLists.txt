set(TEST_COMMON_HEADERS "TestCommon.hpp")

set(HELPER_PREFIX "TestHelper")

set(TESTS
    "TestStartup"
    "TestEntities"
    "TestExternalClients"
    "TestEntityFactories"
    "TestPlugins"
    "TestGraph"
    "TestBenchmark"
    "TestHelperSink"
    "TestHelperExternalFactory"
)

set(TEST_DEPENDENCIES 
    ${CLIENT_TARGET}
    ${Boost_THREAD_TARGET}
    ${Boost_SYSTEM_TARGET}
    ${Boost_FILESYSTEM_TARGET}
)

foreach(test ${TESTS})
    add_executable (${test} ${TEST_COMMON_HEADERS} "${test}.cpp") 
    add_dependencies(${test} ${TEST_DEPENDENCIES} ${SERVER_TARGET})
    target_link_libraries(${test} PRIVATE ${TEST_DEPENDENCIES})
    
    # Only include tests that don't have the helper prefix
    string(FIND ${test} ${HELPER_PREFIX} FOUND_PREFIX)
    if(FOUND_PREFIX LESS 0)
        message(STATUS "Adding test ${test}")
        add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY $<TARGET_FILE_DIR:${test}>)
    endif()
endforeach()

# Test helper dependencies
add_dependencies(TestEntityFactories TestHelperExternalFactory)
add_dependencies(TestExternalClients TestHelperSink)


# -------------
# Language tests
# -------------

option (BINDINGS_DOTNET_TESTS "Build Dotnet tests" OFF)
if(BINDINGS_DOTNET_TESTS)
    add_subdirectory(bindings/dotnet)
endif(BINDINGS_DOTNET_TESTS)


option (BINDINGS_PYTHON_TESTS "Build Python tests" OFF)
if(BINDINGS_PYTHON_TESTS)
    add_subdirectory(bindings/python)
endif(BINDINGS_PYTHON_TESTS)
