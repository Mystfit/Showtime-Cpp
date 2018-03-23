set(CLIENT_TEST_SOURCE_FILES 
    "TestClient.cpp"
)

set(SINK_TEST_SOURCE_FILES
    "TestSink.cpp"
)

# Sink test executable
set(SINK_TEST_TARGET TestSink)
message(STATUS "Configuring ${SINK_TEST_TARGET}")
add_executable (${SINK_TEST_TARGET} ${SINK_TEST_SOURCE_FILES}) 
add_dependencies(${SINK_TEST_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET})
target_include_directories(${SINK_TEST_TARGET} PUBLIC ${ZST_INCLUDES})
target_link_libraries(${SINK_TEST_TARGET} ${CLIENT_TARGET})

# CLient test executable
set(CLIENT_TEST_TARGET TestClient)
message(STATUS "Configuring ${CLIENT_TEST_TARGET}")
add_executable (${CLIENT_TEST_TARGET} ${CLIENT_TEST_SOURCE_FILES}) 
add_dependencies(${CLIENT_TEST_TARGET} ${CLIENT_TARGET} ${SINK_TEST_TARGET})
target_include_directories(${CLIENT_TEST_TARGET} PUBLIC 
    ${ZST_INCLUDES} 
    ${Boost_INCLUDE_DIRS})
target_link_libraries( ${CLIENT_TEST_TARGET}
    ${CLIENT_TARGET}
    ${CORE_ENTITIES_TARGET}
)
add_test(NAME ${CLIENT_TEST_TARGET} COMMAND ${CLIENT_TEST_TARGET} WORKING_DIRECTORY $<TARGET_FILE_DIR:${CLIENT_TEST_TARGET}>)


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
