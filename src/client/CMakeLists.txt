set(CLIENT_TARGET ${PROJECT_NAME}Client)

set(ZST_CLIENT_HEADERS
    "ZstCallbackQueue.h"
    "ZstCallbacks.h"
    "ZstClient.h"
)

set(ZST_CLIENT_SRC
    "Showtime.cpp"
    "ZstClient.cpp"
    "ZstCallbackQueueInstantiations.cpp"
)

include_directories(${ZST_CORE_DIR})

# Set library definitions
IF(WIN32)
	add_definitions(-DIMPORTS_CORE_API)
    add_definitions(-DEXPORTS_CLIENT_API)
ENDIF(WIN32)

# Create Showtime library target
message(STATUS "Building ${CLIENT_TARGET} target")
add_library(${CLIENT_TARGET} SHARED  ${ZST_CLIENT_HEADERS} ${ZST_CLIENT_SRC})
target_link_libraries(${CLIENT_TARGET} ${CORE_TARGET} ${ZST_LINK_LIBS})