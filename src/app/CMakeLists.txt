set(SOURCE_FILES 
 "ZstSectionTest.cpp"
 "ZstStage.cpp"
)

include_directories(${ZST_ROOT})

add_executable (ShowtimeNodeTest ZstSectionTest.cpp ${ZST_HEADERS}) 
add_executable (ShowtimeStage ZstStage.cpp ${ZST_HEADERS}) 

add_dependencies(ShowtimeNodeTest ${PROJECT_NAME})

target_link_libraries(ShowtimeNodeTest ${PROJECT_NAME})
target_link_libraries(ShowtimeStage ${PROJECT_NAME})
