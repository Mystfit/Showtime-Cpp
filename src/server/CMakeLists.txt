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
  "${CMAKE_CURRENT_LIST_DIR}/ZstZMQServerTransport.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstWebsocketServerTransport.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstWebsocketSession.h"
)

set(ZST_SERVER_SRC
  "${CMAKE_CURRENT_LIST_DIR}/ShowtimeServer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstPerformerStageProxy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstStage.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstStageModule.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstStageHierarchy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstStageSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstZMQServerTransport.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstWebsocketServerTransport.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstWebsocketSession.cpp"
)

set(ZST_SERVER_LAUNCHER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageRunner.cpp"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_SERVER_HEADERS})

set(SERVER_LIBS "")

if(BUILD_STATIC)
  target_link_libraries(${SERVER_TARGET_STATIC} 
    PUBLIC
    ${CORE_TARGET_STATIC}
    PRIVATE
    ${SHOWTIME_PRIVATE_STATIC_LINKED_LIBS}
  )
  add_dependencies(${SERVER_TARGET_STATIC} ${CORE_TARGET_STATIC})
  list(APPEND SERVER_LIBS ${SERVER_TARGET_STATIC})
endif()

if(BUILD_SHARED)
  target_link_libraries(${SERVER_TARGET} 
    PUBLIC 
    ${CORE_TARGET}
    PRIVATE
    ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS}
  )

  add_dependencies(${SERVER_TARGET} ${CORE_TARGET_STATIC})
  list(APPEND SERVER_LIBS ${SERVER_TARGET})
endif()

foreach(TARGET ${SERVER_LIBS})
  set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

  target_compile_definitions(${TARGET} 
    PRIVATE ZST_EXPORT_SERVER_API
    INTERFACE ZST_IMPORT_SERVER_API
  )

  # Server lib sources
  target_sources(${TARGET} PRIVATE 
    ${ZST_SERVER_HEADERS}
    ${ZST_SERVER_PRIVATE_HEADERS}
    ${ZST_SERVER_SRC}
  )

  # Server lib includes
  target_include_directories(${TARGET} PRIVATE
    ${Showtime_SOURCE_DEPENDENCIES}
    ${Showtime_PRIVATE_HEADER_DEPENDENCIES}
  )

  # Link libraries
  target_link_libraries(${TARGET} PRIVATE ${SHOWTIME_PRIVATE_LINKED_LIBS})
endforeach()


if(BUILD_SERVER_LAUNCHER)
  # Server launcher sources
  target_sources(${SERVER_LAUNCHER} PRIVATE ${ZST_SERVER_LAUNCHER_SRC})

  # Server launcher dependencies
  add_dependencies(${SERVER_LAUNCHER} ${SERVER_TARGET})
  target_link_libraries(${SERVER_LAUNCHER} PUBLIC ${SERVER_TARGET})
endif()

