message(STATUS "Configuring ${CLIENT_TARGET} target")

set_target_properties(${CLIENT_TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

set(ZST_CLIENT_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_HEADERS})

set(ZST_CLIENT_PRIVATE_HEADERS
  "${Showtime_INCLUDE_DIRS}/Showtime.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientTransport.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstGraphTransport.h"
)

set(ZST_CLIENT_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientHierarchy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientTransport.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstGraphTransport.cpp"
)

# Include files in client target
target_sources(${CLIENT_TARGET} PRIVATE 
  ${ZST_CLIENT_HEADERS}
  ${ZST_CLIENT_PRIVATE_HEADERS}
  ${ZST_CLIENT_SRC}
)

# Include source dependencies in client target
target_include_directories(${CLIENT_TARGET} PRIVATE
  ${Showtime_SOURCE_DEPENDENCIES}
)

# Platform specific definitions
if(WIN32)
    target_compile_definitions(${CLIENT_TARGET} PRIVATE -DZST_EXPORT_CLIENT_API)
    target_compile_definitions(${CLIENT_TARGET} INTERFACE -DZST_IMPORT_CLIENT_API)
endif(WIN32)

# Target dependencies
add_dependencies(${CLIENT_TARGET}
                 ${CORE_TARGET})

# Link libraries
list(APPEND SHOWTIME_PUBLIC_LINKED_LIBS ${CLIENT_TARGET})
target_link_libraries(${CLIENT_TARGET} PUBLIC ${CORE_TARGET})
