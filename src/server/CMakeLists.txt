set(SERVER_TARGET ${PROJECT_NAME}Server)

set(ZST_SERVER_HEADERS
    "ZstStage.h"
)

set(ZST_SERVER_SRC
    "ZstStageRunner.cpp"
    "ZstStage.cpp"
)

# Create standalone stage executable
IF(WIN32)
    add_definitions(-DIMPORTS_CORE_API)

    # Set up libzmq and czmq runtime dependencies
    set(RUNTIMES_LIBZMQ_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to release libzmq dll")
    set(RUNTIMES_LIBZMQ_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to debug libzmq dll")
    set(RUNTIMES_LIBZMQ ${RUNTIMES_LIBZMQ_RELEASE} ${RUNTIMES_LIBZMQ_DEBUG})
    set(RUNTIMES_CZMQ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to czmq .dll")
ENDIF(WIN32)

message(STATUS "Building ${SERVER_TARGET} target")
add_executable (${SERVER_TARGET} ${ZST_SERVER_HEADERS} ${ZST_SERVER_SRC}) 
add_dependencies(${SERVER_TARGET} ${CORE_TARGET})
target_link_libraries(${SERVER_TARGET} ${CORE_TARGET})