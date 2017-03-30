set(SOURCE_FILES 
 "ZstSectionTest.cpp"
 "ZstStage.cpp"
)

include_directories(../lib)
# link_directories(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

add_executable (ShowtimeNodeTest ZstSectionTest.cpp) 
add_executable (ShowtimeStage ZstStage.cpp) 

add_dependencies(ShowtimeNodeTest ${PROJECT_NAME})

target_link_libraries(ShowtimeNodeTest ${PROJECT_NAME})

# target_link_libraries (TestProjectExecutable ${PROJECT_NAME})
