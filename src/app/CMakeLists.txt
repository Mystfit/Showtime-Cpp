set(SOURCE_FILES 
 "zst_node_test.cpp"
)

include_directories(../lib)
# link_directories(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

add_executable (ShowtimeNodeTest zst_node_test.cpp) 
add_dependencies(ShowtimeNodeTest ${PROJECT_NAME})

target_link_libraries(ShowtimeNodeTest ${PROJECT_NAME})

# target_link_libraries (TestProjectExecutable ${PROJECT_NAME})
