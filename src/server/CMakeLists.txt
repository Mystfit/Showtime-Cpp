message(STATUS "Configuring ${SERVER_TARGET} target")

set(ZST_SERVER_HEADERS
    "${Showtime_INCLUDE_DIRS}/ShowtimeServer.h"
)

set(ZST_SERVER_PRIVATE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.h"
	"${CMAKE_CURRENT_LIST_DIR}/ZstStageModule.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageHierarchy.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformerStageProxy.h"
)

set(ZST_SERVER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ShowtimeServer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformerStageProxy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ZstStageModule.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageHierarchy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageSession.cpp"
)

set(ZST_SERVER_LAUNCHER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageRunner.cpp"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_SERVER_HEADERS})

# Server lib sources
target_sources(${SERVER_TARGET} PRIVATE 
    ${ZST_SERVER_HEADERS}
    ${ZST_SERVER_PRIVATE_HEADERS}
    ${ZST_SERVER_SRC}
)

# Server launcher sources
target_sources(${SERVER_LAUNCHER} PRIVATE ${ZST_SERVER_LAUNCHER_SRC})

# Server lib includes
target_include_directories(${SERVER_TARGET} PRIVATE
  ${Showtime_SOURCE_DEPENDENCIES}
  ${Showtime_PRIVATE_HEADER_DEPENDENCIES}
)

# Platform specific defines
IF(WIN32)
    target_compile_definitions(${SERVER_TARGET} 
        PRIVATE ZST_EXPORT_SERVER_API
        INTERFACE ZST_IMPORT_SERVER_API
    )
ENDIF(WIN32)

# Target dependencies
add_dependencies(${SERVER_TARGET} ${CORE_TARGET})
add_dependencies(${SERVER_LAUNCHER} ${SERVER_TARGET})

# Link libraries
target_link_libraries(${SERVER_TARGET} 
    PUBLIC 
    ${CORE_TARGET}
    PRIVATE
    ${SHOWTIME_PRIVATE_LINKED_LIBS}
)
target_link_libraries(${SERVER_LAUNCHER} PUBLIC ${SERVER_TARGET})

