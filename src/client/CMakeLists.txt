set(ZST_CLIENT_PUBLIC_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

set(ZST_CLIENT_PRIVATE_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
)

set(ZST_CLIENT_SRC
    "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
)

target_sources(${CLIENT_TARGET} PUBLIC ${ZST_CLIENT_PUBLIC_HEADERS})
target_sources(${CLIENT_TARGET} PRIVATE ${ZST_CLIENT_PRIVATE_HEADERS} ${ZST_CLIENT_SRC})

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
