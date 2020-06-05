set(TEST_COMMON_HEADERS "TestCommon.hpp")

set(HELPER_PREFIX "TestHelper")

set(TESTS
    "TestURI"
    "TestStartup"
    "TestEntities"
    "TestExternalClients"
    "TestEntityFactories"
    "TestPlugins"
    "TestGraph"
    "TestBenchmark"
    #"TestHelperSink"
    # "TestHelperExternalFactory"
)

set(TEST_DEPENDENCIES 
    ${CLIENT_TARGET}
    ${SERVER_TARGET}
    Boost::unit_test_framework
    Boost::thread
    Boost::system
    Boost::filesystem
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

    # Generate PDB files for release
    target_compile_options(${test} PRIVATE ${SHOWTIME_RELEASE_PDB_COMPILE_DEFINITIONS})
    target_link_options(${test} PRIVATE ${SHOWTIME_RELEASE_PDB_LINK_FLAGS})
endforeach()

# Test helper dependencies
add_dependencies(TestPlugins ${CORE_ENTITIES_TARGET})
# add_dependencies(TestEntityFactories TestHelperExternalFactory)
# add_dependencies(TestExternalClients TestHelperSink)


# -------------
# Language tests
# -------------

if(BINDINGS_DOTNET)
    add_subdirectory(bindings/dotnet)
endif()

if(BINDINGS_PYTHON)
    add_subdirectory(bindings/python)
endif()

if(BINDINGS_JAVA OR BINDINGS_JRUBY)
    add_subdirectory(bindings/java)
endif()

if(BINDINGS_JRUBY)
    add_subdirectory(bindings/jruby)
endif()

if(BINDINGS_UNITY)
    add_subdirectory(bindings/unity)
endif()
