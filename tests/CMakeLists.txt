set(SECTION_SOURCE_FILES 
 "ZstSectionTest.cpp"
)

include_directories(
	${Showtime_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

IF(WIN32)
	add_definitions(-DIMPORTS_API)
ENDIF(WIN32)

# Section test executable
add_executable (SectionTest ${SECTION_SOURCE_FILES}) 
add_dependencies(SectionTest ${PROJECT_NAME})
target_link_libraries(SectionTest ${PROJECT_NAME})
add_test(NAME section_test COMMAND SectionTest)

