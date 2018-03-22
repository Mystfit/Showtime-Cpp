message(STATUS "Building ${CLIENT_TARGET} target")

set(ZST_CLIENT_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_HEADERS})

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
                 ${CORE_TARGET}
                 ${CORE_ENTITIES_TARGET})

if(MACOSX)
    set_target_properties(${CLIENT_TARGET} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION "A"
      VERSION "${SHOWTIME_VERSION}"
      PUBLIC_HEADER "${ZST_PUBLIC_HEADERS}"
    )
endif(MACOSX)

# Link libraries
target_link_libraries(${CLIENT_TARGET} PUBLIC
  ${CORE_TARGET}
)

target_link_libraries(${CLIENT_TARGET} PRIVATE
  ${CORE_TARGET}
  ${msgpackc_TARGET}
  ${fmt_TARGET}
)

if(WIN32)
  # Copy runtime dependencies
  add_custom_command(TARGET ${CLIENT_TARGET} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E echo "Copy CZMQ runtime $<TARGET_FILE:${CZMQ_TARGET}> to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
                      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${CZMQ_TARGET}>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
                      VERBATIM
  )
endif()
