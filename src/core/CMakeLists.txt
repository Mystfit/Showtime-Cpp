message(STATUS "Configuring ${CORE_TARGET} target")

# Write version info into header file
set(VERSION_FILE "${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h")
FILE(WRITE ${VERSION_FILE} "#define SHOWTIME_VERSION \"${CMAKE_PROJECT_VERSION}\"")

# Directories
set(ZST_HEADER_DEPENDENCIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../dependencies")

# Target definitions
set(GLOBAL_PUBLIC_DEFINES
    ZMQ_BUILD_DRAFT_API
    CZMQ_BUILD_DRAFT_API
    BOOST_ALL_NO_LIB
    BOOST_THREAD_USE_LIB
)

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
    ${Showtime_INCLUDE_DIRS}/ZstBundle.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEventAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstComputeAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSynchronisableAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstFactoryAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSessionAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstHierarchyAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEntityAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityFactory.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
    ${Showtime_INCLUDE_DIRS}/fmt/format.hpp
    ${Showtime_INCLUDE_DIRS}/fmt/format.h
    ${Showtime_INCLUDE_DIRS}/fmt/format-inl.h
    ${Showtime_INCLUDE_DIRS}/fmt/core.h
    ${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h
)
list(APPEND Showtime_API_INTERFACE ${API_FILES})

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageOptions.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstModule.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventWakeup.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstBoostEventWakeup.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayer.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayerBase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstUDPGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityFactoryLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstTransportAdaptor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstModuleAdaptor.hpp"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstLogging.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSerialisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstModule.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstBoostEventWakeup.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayerBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstUDPGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityFactory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstContainer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPlug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPerformer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstEventAdaptorStubs.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityFactoryLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.cpp"
)

# Set threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_WIN32_THREADS_INIT)
    message(STATUS "Using windows threads")
elseif(CMAKE_USE_PTHREADS_INIT)
    message(STATUS "Using posix threads")
endif()
message(STATUS "Setting link thread library to ${CMAKE_THREAD_LIBS_INIT}")

# Platform specific options
IF(WIN32)
    list(APPEND GLOBAL_PUBLIC_DEFINES 
        _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
        _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
    )
    list(APPEND GLOBAL_PUBLIC_DEFINES
        _WIN32_WINNT=0x0601
        BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
    )
    if(BUILD_SHARED)
        target_compile_definitions(${CORE_TARGET} PRIVATE ZST_EXPORT_CORE_API)
        target_compile_definitions(${CORE_TARGET} INTERFACE ZST_IMPORT_CORE_API)
    endif()
endif()
#target_compile_definitions(${CORE_TARGET} PUBLIC -D_GLIBCXX_USE_CXX11_ABI=0)

# Link libraries
set(${CORE_TARGET}_PRIVATE_LINKED_LIBS 
    ${msgpackc_TARGET}
)

if(BUILD_STATIC)
    list(APPEND CORE_LIBS ${CORE_TARGET_STATIC})
    target_link_libraries(${CORE_TARGET_STATIC} PRIVATE 
        ${SHOWTIME_PRIVATE_STATIC_LINKED_LIBS}
    )
    target_compile_definitions(${CORE_TARGET_STATIC} PUBLIC ${GLOBAL_PUBLIC_DEFINES} CZMQ_STATIC ZMQ_STATIC)
endif()

if(BUILD_SHARED)
    list(APPEND CORE_LIBS ${CORE_TARGET})
    target_link_libraries(${CORE_TARGET} PRIVATE 
        ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS}
    )
    target_compile_definitions(${CORE_TARGET} PUBLIC ${GLOBAL_PUBLIC_DEFINES})

  if(WIN32)
    # Copy runtime dependencies
    add_custom_command(TARGET ${CORE_TARGET} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E echo "Copy CZMQ runtime $<TARGET_FILE:czmq> to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:czmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                      COMMAND ${CMAKE_COMMAND} -E echo "Copy libZMQ runtime $<TARGET_FILE:libzmq> to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:libzmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                      VERBATIM
    )
    endif()
endif()

foreach(TARGET ${CORE_LIBS})
    set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

    # Target sources
    target_sources(${TARGET} PRIVATE ${ZST_CORE_SRC} ${ZST_CORE_HEADERS})

    # Target includes
    target_include_directories(${TARGET} PUBLIC
            $<BUILD_INTERFACE:${Showtime_INCLUDE_DIRS}>
            $<BUILD_INTERFACE:${Showtime_COMPILED_INCLUDE_DIRS}>
            $<BUILD_INTERFACE:${ZST_HEADER_DEPENDENCIES_DIR}>
            $<BUILD_INTERFACE:${ZST_HEADER_DEPENDENCIES_DIR}/cf>
            $<BUILD_INTERFACE:${ZST_HEADER_DEPENDENCIES_DIR}/concurrentqueue>
            $<INSTALL_INTERFACE:include>
    )

    # Target link libs
    target_link_libraries(${TARGET} 
        PUBLIC
        ${SHOWTIME_PUBLIC_LINKED_LIBS}
        PRIVATE 
        ${${CORE_TARGET}_PRIVATE_LINKED_LIBS}
        ${SHOWTIME_PRIVATE_LINKED_LIBS}
    )
endforeach()
