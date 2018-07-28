message(STATUS "Configuring ${CORE_TARGET} target")

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
    ${Showtime_INCLUDE_DIRS}/ZstEventDispatcher.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEventAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstComputeAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSynchronisableAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSessionAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstHierarchyAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEntityAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
    ${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h
)
list(APPEND Showtime_API_INTERFACE ${API_FILES})

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstTransportLayer.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstTransportLayerBase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstModule.h"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstTransportAdaptor.hpp"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstLogging.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstTransportLayerBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstModule.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstContainer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPlug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPerformer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstEventAdaptorStubs.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.cpp"
)

target_sources(${CORE_TARGET} PUBLIC ${ZST_PUBLIC_HEADERS} )
target_sources(${CORE_TARGET} PRIVATE ${ZST_CORE_SRC} ${ZST_CORE_HEADERS} ${Showtime_API_INTERFACE})
target_include_directories(${CORE_TARGET}
    PUBLIC 
        $<BUILD_INTERFACE:${Showtime_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${Showtime_COMPILED_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Showtime_SOURCE_DEPENDENCIES}
)

IF(WIN32)
    target_compile_definitions(${CORE_TARGET} PRIVATE -DZST_EXPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} INTERFACE -DZST_IMPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} PUBLIC 
                                -D_WIN32_WINNT=0x0601
                                -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
else()
    # Use shared boost on non-windows platforms
    if(Boost_USE_LOCAL)
        target_compile_definitions(${CORE_TARGET} PUBLIC -DBOOST_ALL_DYN_LINK)
    endif()

    # Turn off C11 ABI compatibility in glibc
    #target_compile_definitions(${CORE_TARGET} PUBLIC -D_GLIBCXX_USE_CXX11_ABI=0)
endif(WIN32)

if(NOT HUNTER_ENABLED)
    if(USE_STATIC_CZMQ)
        target_compile_definitions(${CORE_TARGET} PRIVATE 
                                    -DZMQ_STATIC
                                    -DCZMQ_STATIC
        )
    endif()
    target_include_directories(${CORE_TARGET} PUBLIC 
                                ${CZMQ_INCLUDE_DIRS}
    )
endif()

# Set threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_WIN32_THREADS_INIT)
    message(STATUS "Using windows threads")
elseif(CMAKE_USE_PTHREADS_INIT)
    message(STATUS "Using posix threads")
endif()
message(STATUS "Setting link thread library to ${CMAKE_THREAD_LIBS_INIT}")

# Link libraries
list(APPEND SHOWTIME_PUBLIC_LINKED_LIBS 
    ${CZMQ_TARGET}
    ${fmt_TARGET}
    ${Boost_HEADER_TARGET}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries(${CORE_TARGET} PUBLIC 
                        ${SHOWTIME_PUBLIC_LINKED_LIBS}
)
target_link_libraries(${CORE_TARGET} PRIVATE 
                        ${msgpackc_TARGET}
                        ${Boost_DATE_TIME_TARGET}
                        ${Boost_CHRONO_TARGET}
                        ${Boost_ATOMIC_TARGET}
                        ${Boost_LOG_TARGET}
                        ${Boost_LOG_SETUP_TARGET}
                        ${Boost_THREAD_TARGET}
                        ${Boost_SYSTEM_TARGET}
                        ${Boost_FILESYSTEM_TARGET}
)
list(APPEND SHOWTIME_PUBLIC_LINKED_LIBS ${CORE_TARGET})

if(WIN32)
    # Copy runtime dependencies
    add_custom_command(TARGET ${CORE_TARGET} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E echo "Copy CZMQ runtime $<TARGET_FILE:${CZMQ_TARGET}> to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
                      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${CZMQ_TARGET}>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
                      VERBATIM
    )
endif()
