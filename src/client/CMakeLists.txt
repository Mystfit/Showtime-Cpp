set(ZST_CLIENT_PUBLIC_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_PUBLIC_HEADERS})

set(ZST_CLIENT_PRIVATE_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
	"${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
	"${CMAKE_CURRENT_LIST_DIR}/ZstClientEvents.h"
)

set(ZST_CLIENT_SRC
    "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstClientEvents.cpp"
)

set(ZST_CLIENT_INCLUDES 
	${ZST_CLIENT_PUBLIC_HEADERS}
)
target_sources(${CLIENT_TARGET} PUBLIC ${ZST_CLIENT_PUBLIC_HEADERS})
target_sources(${CLIENT_TARGET} PRIVATE ${ZST_CLIENT_PRIVATE_HEADERS} ${ZST_CLIENT_SRC})
target_include_directories(${CLIENT_TARGET} PUBLIC ${ZST_INCLUDES})

# Platform specific definitions
if(WIN32)
    target_compile_definitions(${CLIENT_TARGET} PRIVATE -DEXPORT_CLIENT_API)
    target_compile_definitions(${CLIENT_TARGET} INTERFACE -DIMPORT_CLIENT_API)
endif(WIN32)

add_dependencies(${CLIENT_TARGET} ${CORE_TARGET})

if(MACOSX)
    set_target_properties(${CLIENT_TARGET} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION "A"
      VERSION "${SHOWTIME_VERSION}"
      PUBLIC_HEADER "${ZST_PUBLIC_HEADERS}"
    )
endif(MACOSX)

# Link client to core
target_link_libraries(${CLIENT_TARGET} ${CORE_TARGET})
