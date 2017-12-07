enable_testing()

# Write version info into header file
set(COMPILED_INCLUDES "${CMAKE_BINARY_DIR}/include")
set(VERSION_FILE "${COMPILED_INCLUDES}/version.h")
FILE(WRITE ${COMPILED_INCLUDES}/version.h "#define SHOWTIME_VERSION \"${SHOWTIME_VERSION}\"")

set(CORE_TARGET "${PROJECT_NAME}Core")

set(ZST_PUBLIC_HEADERS
    ${Showtime_INCLUDE_DIRS}/Showtime.h
    ${Showtime_INCLUDE_DIRS}/ZstCable.h
    ${Showtime_INCLUDE_DIRS}/ZstConstants.h
    ${Showtime_INCLUDE_DIRS}/ZstExports.h
    ${Showtime_INCLUDE_DIRS}/ZstURI.h
    ${Showtime_INCLUDE_DIRS}/entities/AddFilter.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
)

set(ZST_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")

set(ZST_PRIVATE_HEADERS
    "${ZST_CORE_DIR}/Queue.h"
    "${ZST_CORE_DIR}/variant.hpp"
    "${ZST_CORE_DIR}/ZstActor.h"
    "${ZST_CORE_DIR}/ZstMessages.h"
    "${ZST_CORE_DIR}/ZstStreamable.h"
    "${ZST_CORE_DIR}/ZstGraphSender.h"
    "${ZST_CORE_DIR}/ZstUtils.hpp"
    "${ZST_CORE_DIR}/ZstValue.h"
    ${VERSION_FILE}
)

message(STATUS ${ZST_PRIVATE_HEADERS})

set(ZST_CORE_SRC
    "core/ZstActor.cpp"
    "core/ZstCable.cpp"
    "core/ZstURI.cpp"
    "core/ZstUtils.cpp"
    "core/ZstValue.cpp"
    "core/entities/ZstEntityBase.cpp"
    "core/entities/ZstComponent.cpp"
    "core/entities/ZstContainer.cpp"
    "core/entities/AddFilter.cpp"
    "core/entities/ZstPlug.cpp"
)

set(ZST_CORE_HEADERS ${ZST_PUBLIC_HEADERS} ${ZST_PRIVATE_HEADERS})

set(ZST_INCLUDES 
    ${COMPILED_INCLUDES}
    ${Showtime_INCLUDE_DIRS}
    ${ZeroMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

include_directories(${ZST_INCLUDES} ${ZST_CORE_DIR})

set(ZST_LINK_LIBS
    ${ZeroMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

# Set library definitions
IF(WIN32)
    add_definitions(-DEXPORTS_CORE_API)
ENDIF(WIN32)

# Create Showtime library target
message(STATUS "Building ${CORE_TARGET} target")
add_library(${CORE_TARGET} SHARED ${ZST_CORE_HEADERS} ${ZST_CORE_SRC})
target_link_libraries(${CORE_TARGET} ${ZST_LINK_LIBS})

if(MACOSX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION "A"
      VERSION "${SHOWTIME_VERSION}"
      PUBLIC_HEADER "${ZST_PUBLIC_HEADERS}"
    )
ENDIF(MACOSX)

add_subdirectory(bindings)
add_subdirectory(client)
add_subdirectory(server)
