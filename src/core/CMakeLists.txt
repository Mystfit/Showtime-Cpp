# Write version info into header file
set(COMPILED_INCLUDES "${CMAKE_BINARY_DIR}/include")
set(VERSION_FILE "${COMPILED_INCLUDES}/ZstVersion.h")
FILE(WRITE ${VERSION_FILE} "#define SHOWTIME_VERSION \"${SHOWTIME_VERSION}\"")

set(ZST_INTERFACE_HEADERS
    ${Showtime_INCLUDE_DIRS}/ZstCore.h
    ${Showtime_INCLUDE_DIRS}/ZstLogging.h
    ${Showtime_INCLUDE_DIRS}/ZstCable.h
    ${Showtime_INCLUDE_DIRS}/ZstConstants.h
    ${Showtime_INCLUDE_DIRS}/ZstExports.h
    ${Showtime_INCLUDE_DIRS}/ZstURI.h
    ${Showtime_INCLUDE_DIRS}/ZstStreamable.h
    ${Showtime_INCLUDE_DIRS}/ZstEvents.h
    ${Showtime_INCLUDE_DIRS}/entities/AddFilter.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
)

set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/Queue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstGraphSender.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstUtils.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCallbackQueue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.h"
    ${VERSION_FILE}
)

set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstUtils.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCallbackQueueInstantiations.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstContainer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/AddFilter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPlug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPerformer.cpp"
)

# Packages
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(MsgPack REQUIRED)

set(ZST_CORE_INCLUDES 
    ${COMPILED_INCLUDES}
    ${Showtime_INCLUDE_DIRS}
    "${Showtime_SUBMODULES_DIR}/spdlog/include"
    "${Showtime_SUBMODULES_DIR}/variant/include/mpark"
    "${Showtime_SUBMODULES_DIR}/cf"
    ${ZeroMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)
#include_directories(${ZST_INCLUDES})
target_sources(${CORE_TARGET} PUBLIC ${ZST_PUBLIC_HEADERS} ${ZST_CORE_HEADERS})
target_sources(${CORE_TARGET} PRIVATE ${ZST_CORE_SRC} ${ZST_INTERFACE_HEADERS})
target_include_directories(${CORE_TARGET} PUBLIC ${ZST_CORE_INCLUDES})

set(ZST_LINK_LIBS
    ${ZeroMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

IF(WIN32)
    IF(WINDOWS_USE_VCPKG)
        target_compile_definitions(${CORE_TARGET} PRIVATE "-DCMAKE_TOOLCHAIN_FILE=${WINDOWS_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    ENDIF(WINDOWS_USE_VCPKG)
    target_compile_definitions(${CORE_TARGET} PRIVATE -DEXPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} INTERFACE -DIMPORT_CORE_API)
ENDIF(WIN32)


target_link_libraries(${CORE_TARGET} ${ZST_LINK_LIBS})
