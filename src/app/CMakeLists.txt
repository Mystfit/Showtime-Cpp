set(SOURCE_FILES 
 "ZstSectionTest.cpp"
 "ZstStageRunner.cpp"
)

include_directories(
	../lib
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
)

add_executable (ShowtimeNodeTest ZstSectionTest.cpp) 
add_executable (ShowtimeStage ZstStageRunner.cpp) 

add_dependencies(ShowtimeNodeTest ${PROJECT_NAME})
add_dependencies(ShowtimeStage ${PROJECT_NAME})

target_link_libraries(ShowtimeNodeTest ${PROJECT_NAME})
target_link_libraries(ShowtimeStage ${PROJECT_NAME})

