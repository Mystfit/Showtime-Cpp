set(TEST_COMMON_HEADERS "TestCommon.hpp")

set(TESTS
    "TestStartup"
    "TestEntities"
    "TestGraph"
    "TestExternalClients"
    "TestBenchmark"
    "TestSink"
)

set(TEST_DEPENDENCIES 
    ${CLIENT_TARGET} 
    ${CORE_ENTITIES_TARGET}
    ${Boost_SYSTEM_TARGET}
    ${Boost_FILESYSTEM_TARGET}
)

foreach(test ${TESTS})
    message(STATUS "Configuring ${test}")
    add_executable (${test} ${TEST_COMMON_HEADERS} "${test}.cpp") 
    add_dependencies(${test} ${TEST_DEPENDENCIES})
    target_link_libraries(${test} PRIVATE ${TEST_DEPENDENCIES})
    add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY $<TARGET_FILE_DIR:${test}>)
endforeach()


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
