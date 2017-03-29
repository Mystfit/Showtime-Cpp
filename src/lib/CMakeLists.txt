set(SOURCE_FILES 
 "zst_node.h"
 "zst_method.h"
 "zst_node.cpp"
 "zst_method.cpp"
)

find_package(ZeroMQ REQUIRED)

include_directories(
	${ZeroMQ_INCLUDE_DIRS}
)

add_definitions(-DEXPORTS_API)

# link_directories(
# 	${ZeroMQ_LIBRARIES}
# )

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} 
	${ZeroMQ_LIBRARIES}
)