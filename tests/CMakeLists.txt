set(CLIENT_TEST_SOURCE_FILES 
    "TestClient.cpp"
)

set(SINK_TEST_SOURCE_FILES
    "TestSink.cpp"
)

# Sink test executable
set(SINK_TEST_TARGET TestSink)
add_executable (${SINK_TEST_TARGET} ${SINK_TEST_SOURCE_FILES}) 
add_dependencies(${SINK_TEST_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET})
target_include_directories(${SINK_TEST_TARGET} PUBLIC ${ZST_INCLUDES})
target_link_libraries(${SINK_TEST_TARGET} ${CLIENT_TARGET})
add_test(NAME sink_test COMMAND ${SINK_TEST_TARGET})

# CLient test executable
set(CLIENT_TEST_TARGET TestClient)
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
