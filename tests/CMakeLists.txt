set(LIBRARY_TEST_SOURCE_FILES 
 "ShowtimeTest.cpp"
)

set(SINK_TEST_SOURCE_FILES
	"SinkTest.cpp"
)

IF(WIN32)
	add_definitions(-DIMPORTS_API)
ENDIF(WIN32)

# Boost flags
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.64 COMPONENTS system filesystem REQUIRED)

include_directories(
	${Showtime_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

link_directories(${Boost_LIBRARY_DIRS})

# Sink test executable
add_executable (SinkTest ${SINK_TEST_SOURCE_FILES}) 
add_dependencies(SinkTest ${PROJECT_NAME})
target_link_libraries(SinkTest ${PROJECT_NAME})
add_test(NAME sink_test COMMAND SinkTest)

# Main library test executable
add_executable (ShowtimeLibraryTest ${LIBRARY_TEST_SOURCE_FILES}) 
add_dependencies(ShowtimeLibraryTest ${PROJECT_NAME} SinkTest)
target_link_libraries(ShowtimeLibraryTest
	${PROJECT_NAME}
	${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
add_test(NAME showtime_library_test COMMAND ShowtimeLibraryTest WORKING_DIRECTORY $<TARGET_FILE_DIR:ShowtimeLibraryTest>)
