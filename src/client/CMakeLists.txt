message(STATUS "Configuring ${CLIENT_TARGET} target")

set(ZST_CLIENT_HEADERS
	"${Showtime_INCLUDE_DIRS}/Showtime.h"
)

# Add our top level header to the public API
list(APPEND Showtime_API_INTERFACE ${ZST_CLIENT_HEADERS})

set(ZST_CLIENT_PRIVATE_HEADERS
  "${Showtime_INCLUDE_DIRS}/Showtime.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstMessageDispatcher.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstTransportLayer.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstCZMQTransportLayer.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstCZMQMessage.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.h"
  "${CMAKE_CURRENT_LIST_DIR}/ZstSession.h"
  "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstMessageAdaptor.hpp"
)

set(ZST_CLIENT_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Showtime.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClient.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstClientModule.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstMessageDispatcher.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstTransportLayer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstCZMQTransportLayer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstCZMQMessage.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZstSession.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstClientAdaptorStubs.cpp"
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

# Link libraries
target_link_libraries(${CLIENT_TARGET} PUBLIC
  ${CORE_TARGET}
)

target_link_libraries(${CLIENT_TARGET} PRIVATE
  ${CORE_TARGET}
  ${msgpackc_TARGET}
  ${fmt_TARGET}
)
