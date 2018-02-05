# Write version info into header file
set(VERSION_FILE "${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h")
FILE(WRITE ${VERSION_FILE} "#define SHOWTIME_VERSION \"${SHOWTIME_VERSION}\"")

# Public include files
set(API_FILES
    ${VERSION_FILE}
    ${Showtime_INCLUDE_DIRS}/ZstCore.h
    ${Showtime_INCLUDE_DIRS}/ZstLogging.h
    ${Showtime_INCLUDE_DIRS}/ZstCable.h
    ${Showtime_INCLUDE_DIRS}/ZstConstants.h
    ${Showtime_INCLUDE_DIRS}/ZstExports.h
    ${Showtime_INCLUDE_DIRS}/ZstURI.h
    ${Showtime_INCLUDE_DIRS}/ZstSerialisable.h
    ${Showtime_INCLUDE_DIRS}/ZstSynchronisable.h
    ${Showtime_INCLUDE_DIRS}/ZstEvents.h
    ${Showtime_INCLUDE_DIRS}/entities/AddFilter.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
)
list(APPEND Showtime_API_INTERFACE ${API_FILES})

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstINetworkInteractor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.h"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEvents.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.cpp"
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
find_package(log4cplus REQUIRED)

# Include directories
set(ZST_CORE_INCLUDES 
    ${Showtime_INCLUDE_DIRS}
    ${Showtime_COMPILED_INCLUDE_DIRS}
    ${Showtime_DEPENDENCIES_DIR}
	"${Showtime_DEPENDENCIES_DIR}/spdlog/include"
    "${Showtime_DEPENDENCIES_DIR}/cf/"
    "${Showtime_DEPENDENCIES_DIR}/variant/include"
    ${ZeroMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
    ${log4cplus_INCLUDE_DIRS}
)

target_sources(${CORE_TARGET} PUBLIC ${ZST_PUBLIC_HEADERS} )
target_sources(${CORE_TARGET} PRIVATE ${ZST_CORE_SRC} ${ZST_CORE_HEADERS} ${Showtime_API_INTERFACE})
target_include_directories(${CORE_TARGET} PUBLIC ${ZST_CORE_INCLUDES})

IF(WIN32)
    IF(WINDOWS_USE_VCPKG)
        target_compile_definitions(${CORE_TARGET} PRIVATE "-DCMAKE_TOOLCHAIN_FILE=${WINDOWS_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    ENDIF(WINDOWS_USE_VCPKG)
    target_compile_definitions(${CORE_TARGET} PRIVATE -DEXPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} INTERFACE -DIMPORT_CORE_API)
ENDIF(WIN32)

set(ZST_LINK_LIBS
    ${ZeroMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
    log4cplus::log4cplusU
)
target_link_libraries(${CORE_TARGET} ${ZST_LINK_LIBS})
