set(SOURCE_FILES 
 "ZstSectionTest.cpp"
)

include_directories(
	${BASEPATH}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

IF(WIN32)
	add_definitions(-DIMPORTS_API)
ENDIF(WIN32)

add_executable (SectionTest ZstSectionTest.cpp) 
add_dependencies(SectionTest ${PROJECT_NAME})
target_link_libraries(SectionTest ${PROJECT_NAME})
