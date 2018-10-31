message(STATUS "Configuring ${CLIENT_TARGET} target")

set(ZST_CLIENT_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

set(ZST_CLIENT_PRIVATE_HEADERS
  "${Showtime_INCLUDE_DIRS}/Showtime.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientTransport.h"
  #"${CMAKE_CURRENT_LIST_DIR}/ZstHTTPTransport.h"
)

set(ZST_CLIENT_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientTransport.cpp"
  #"${CMAKE_CURRENT_LIST_DIR}/ZstHTTPTransport.cpp"
)


# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_HEADERS})

set(CLIENT_LIBS "")

if(BUILD_STATIC)
  target_link_libraries(${CLIENT_TARGET_STATIC} PUBLIC ${CORE_TARGET_STATIC})
  add_dependencies(${CLIENT_TARGET_STATIC} ${CORE_TARGET_STATIC})
  list(APPEND CLIENT_LIBS ${CLIENT_TARGET_STATIC})
endif()

if(BUILD_SHARED)
  # Platform specific definitions
  if(WIN32)
      target_compile_definitions(${CLIENT_TARGET} PRIVATE ZST_EXPORT_CLIENT_API)
      target_compile_definitions(${CLIENT_TARGET} INTERFACE ZST_IMPORT_CLIENT_API)
  endif(WIN32)

  target_link_libraries(${CLIENT_TARGET} PUBLIC ${CORE_TARGET})
  add_dependencies(${CLIENT_TARGET} ${CORE_TARGET})
  list(APPEND CLIENT_LIBS ${CLIENT_TARGET})
endif()

foreach(TARGET ${CLIENT_LIBS})
  set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

  # Include files in client target
  target_sources(${TARGET} PRIVATE 
    ${ZST_CLIENT_HEADERS}
    ${ZST_CLIENT_PRIVATE_HEADERS}
    ${ZST_CLIENT_SRC}
  )

  # Include source dependencies in client target
  target_include_directories(${TARGET} PRIVATE ${Showtime_SOURCE_DEPENDENCIES})
endforeach()
