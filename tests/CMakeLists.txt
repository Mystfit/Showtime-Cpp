set(SECTION_SOURCE_FILES 
 "ShowtimeTest.cpp"
)

include_directories(
	${Showtime_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

IF(WIN32)
	add_definitions(-DIMPORTS_API)
ENDIF(WIN32)

# Section test executable
add_executable (ShowtimeTest ${SECTION_SOURCE_FILES}) 
add_dependencies(ShowtimeTest ${PROJECT_NAME})
target_link_libraries(ShowtimeTest ${PROJECT_NAME})
add_test(NAME showtime_test COMMAND ShowtimeTest)
