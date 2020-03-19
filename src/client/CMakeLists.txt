message(STATUS "Configuring ${CLIENT_TARGET} target")

set(ZST_CLIENT_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

set(ZST_CLIENT_PRIVATE_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientCommon.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstZMQClientTransport.h"
)

set(ZST_CLIENT_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstZMQClientTransport.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientAdaptors.cpp"
)


# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_HEADERS})

set(CLIENT_LIBS "")

if(BUILD_STATIC)
  target_link_libraries(${CLIENT_TARGET_STATIC} 
    PUBLIC
    ${CORE_TARGET_STATIC}
    #PRIVATE
    #{SHOWTIME_PRIVATE_STATIC_LINKED_LIBS}
  )
  add_dependencies(${CLIENT_TARGET_STATIC} ${CORE_TARGET_STATIC})
  list(APPEND CLIENT_LIBS ${CLIENT_TARGET_STATIC})
endif()

if(BUILD_SHARED) 
  target_link_libraries(${CLIENT_TARGET} 
    PUBLIC 
    ${CORE_TARGET}
    PRIVATE
    ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS}
  )

  target_compile_definitions(${CLIENT_TARGET} 
    PRIVATE ZST_EXPORT_CLIENT_API
    INTERFACE ZST_IMPORT_CLIENT_API
  )

  if(WIN32)
    # Copy runtime dependencies
    add_custom_command(TARGET ${CLIENT_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:czmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:libzmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        VERBATIM
    )
  endif()

  add_dependencies(${CLIENT_TARGET} ${CORE_TARGET_STATIC})
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

  target_link_libraries(${TARGET} PRIVATE ${SHOWTIME_PRIVATE_LINKED_LIBS})

  # Include source dependencies in client target
  target_include_directories(${TARGET} PRIVATE 
    ${Showtime_SOURCE_DEPENDENCIES}
    ${Showtime_PRIVATE_HEADER_DEPENDENCIES}
  )
endforeach()
