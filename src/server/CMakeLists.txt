message(STATUS "Configuring ${SERVER_TARGET} target")

set(ZST_SERVER_HEADERS
    "${CORE_INCLUDE_DIR}/ShowtimeServer.h"
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

  # Server lib sources
target_sources(${SERVER_TARGET_OBJECTS} PRIVATE 
  ${ZST_SERVER_HEADERS}
  ${ZST_SERVER_PRIVATE_HEADERS}
  ${ZST_SERVER_SRC}
)

set(SERVER_LIBS "")

# Link libs object target to pull in includes
target_link_libraries(${SERVER_TARGET_OBJECTS} 
  PRIVATE
  ${CORE_TARGET_OBJECTS}
  ${SHOWTIME_PRIVATE_LINKED_LIBS}
)

target_compile_definitions(${SERVER_TARGET_OBJECTS} PRIVATE ZST_EXPORT_SERVER_API)

# Prefer linking against shared library if possible over static
if(BUILD_SHARED)
  target_link_libraries(${SERVER_TARGET} 
    PUBLIC 
    ${CORE_TARGET}
    PRIVATE
    ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS}
  )

  target_compile_definitions(${SERVER_TARGET} 
    INTERFACE 
    ZST_IMPORT_SERVER_API
    PRIVATE 
    ZST_EXPORT_SERVER_API
    ${SHOWTIME_PRIVATE_DEFS}
    $<$<CONFIG:Release>:${SHOWTIME_RELEASE_PDB_COMPILE_DEFINITIONS}>
  )

  add_dependencies(${SERVER_TARGET} ${CORE_TARGET})
  list(APPEND SERVER_LIBS ${SERVER_TARGET})
endif()

if(BUILD_STATIC)
  target_link_libraries(${SERVER_TARGET_STATIC} 
    PUBLIC
    ${CORE_TARGET_STATIC}
    # PRIVATE
    # ${SHOWTIME_PRIVATE_STATIC_LINKED_LIBS}
  )
  add_dependencies(${SERVER_TARGET_STATIC} ${CORE_TARGET_STATIC})
  list(APPEND SERVER_LIBS ${SERVER_TARGET_STATIC})
endif()

foreach(TARGET ${SERVER_LIBS})
  # Generate PDB files for release
  target_compile_options(${TARGET} PRIVATE ${SHOWTIME_RELEASE_PDB_COMPILE_DEFINITIONS})
  target_link_options(${TARGET} PRIVATE ${SHOWTIME_RELEASE_PDB_LINK_FLAGS})
  
  set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

  # Server lib includes
  target_include_directories(${TARGET} PRIVATE
    ${SERVER_TARGET_OBJECTS}
    ${Showtime_SOURCE_DEPENDENCIES}
  )

  # Dummy cpp file to give compiler something to do
  target_sources(${TARGET} PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp
  )

  # Link libraries
  target_link_libraries(${TARGET} PRIVATE ${SHOWTIME_PRIVATE_LINKED_LIBS} ${SERVER_TARGET_OBJECTS})
endforeach()


if(BUILD_SERVER_LAUNCHER)
  # Server launcher sources
  target_sources(${SERVER_LAUNCHER} PRIVATE ${ZST_SERVER_LAUNCHER_SRC})

  # Server launcher dependencies
  if(BUILD_SHARED)
    target_link_libraries(${SERVER_LAUNCHER} PUBLIC ${SERVER_TARGET})
    add_dependencies(${SERVER_LAUNCHER} ${SERVER_TARGET})
  else()
    target_link_libraries(${SERVER_LAUNCHER} PUBLIC ${SERVER_TARGET_STATIC})
    add_dependencies(${SERVER_LAUNCHER} ${SERVER_TARGET_STATIC})
  endif()
endif()
