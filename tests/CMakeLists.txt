set(CLIENT_TEST_SOURCE_FILES 
    "TestClient.cpp"
)

set(SINK_TEST_SOURCE_FILES
    "TestSink.cpp"
)

# Boost flags
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.64 COMPONENTS system filesystem REQUIRED)



# Sink test executable
set(SINK_TEST_TARGET TestSink)
add_executable (${SINK_TEST_TARGET} ${SINK_TEST_SOURCE_FILES}) 
add_dependencies(${SINK_TEST_TARGET} ${CORE_TARGET})
target_include_directories(${SINK_TEST_TARGET} PUBLIC ${ZST_INCLUDES})
target_link_libraries(${SINK_TEST_TARGET} ${CLIENT_TARGET})
add_test(NAME sink_test COMMAND ${SINK_TEST_TARGET})

# target_link_directories( ${Boost_LIBRARY_DIRS})

# CLient test executable
set(CLIENT_TEST_TARGET TestClient)
add_executable (${CLIENT_TEST_TARGET} ${CLIENT_TEST_SOURCE_FILES}) 
add_dependencies(${CLIENT_TEST_TARGET} ${CLIENT_TARGET} ${SINK_TEST_TARGET})
target_include_directories(${CLIENT_TEST_TARGET} PUBLIC 
    ${ZST_INCLUDES} 
    ${Boost_INCLUDE_DIRS})
target_link_libraries( ${CLIENT_TEST_TARGET}
    ${CLIENT_TARGET}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
add_test(NAME showtime_client_test COMMAND ${CLIENT_TEST_TARGET} WORKING_DIRECTORY $<TARGET_FILE_DIR:${CLIENT_TEST_TARGET}>)
