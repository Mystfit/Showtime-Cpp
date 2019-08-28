message(STATUS "Configuring ${CORE_TARGET_STATIC} target")

# Generate version info header file
set(VERSION_FILE_IN "${CMAKE_CURRENT_LIST_DIR}/ZstVersion.h.in")
set(VERSION_FILE_OUT "${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h")
configure_file(${VERSION_FILE_IN} ${VERSION_FILE_OUT})
set_source_files_properties(${VERSION_FILE_OUT} PROPERTIES GENERATED ON)

# Generate empty source file to force some targets to compile
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp CONTENT "")

# Generated sources
set(GENERATED_SOURCES 
    ${VERSION_FILE_OUT}
    ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp
)

# Set generated sources property
set_source_files_properties(${GENERATED_SOURCES} PROPERTIES GENERATED ON)

# Public include files
set(ENTITY_HEADERS
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityFactory.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
)
set(ADAPTOR_HEADERS 
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEventAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstConnectionAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstComputeAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSynchronisableAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstFactoryAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstSessionAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstHierarchyAdaptor.hpp
    ${Showtime_INCLUDE_DIRS}/adaptors/ZstEntityAdaptor.hpp
)
set(FMT_HEADERS 
    ${Showtime_INCLUDE_DIRS}/fmt/format.hpp
    ${Showtime_INCLUDE_DIRS}/fmt/format.h
    ${Showtime_INCLUDE_DIRS}/fmt/format-inl.h
    ${Showtime_INCLUDE_DIRS}/fmt/core.h
)
set(API_HEADERS
    ${VERSION_FILE}
    ${Showtime_INCLUDE_DIRS}/ZstCore.h
    ${Showtime_INCLUDE_DIRS}/ZstLogging.h
    ${Showtime_INCLUDE_DIRS}/ZstCable.h
    ${Showtime_INCLUDE_DIRS}/ZstExceptions.h
    ${Showtime_INCLUDE_DIRS}/ZstCableAddress.h
    ${Showtime_INCLUDE_DIRS}/ZstConstants.h
    ${Showtime_INCLUDE_DIRS}/ZstExports.h
    ${Showtime_INCLUDE_DIRS}/ZstURI.h
    ${Showtime_INCLUDE_DIRS}/ZstSerialisable.h
    ${Showtime_INCLUDE_DIRS}/ZstSynchronisable.h
    ${Showtime_INCLUDE_DIRS}/ZstBundle.hpp
    ${Showtime_INCLUDE_DIRS}/ZstServerAddress.h
    ${Showtime_INCLUDE_DIRS}/ZstPointerUtils.h
    ${VERSION_FILE_OUT}
)
list(APPEND Showtime_API_INTERFACE 
    ${API_HEADERS}
    ${ENTITY_HEADERS}
    ${ADAPTOR_HEADERS}
    ${FMT_HEADERS}
)

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstZMQRefCounter.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageOptions.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSemaphore.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstIOLoop.h"
)
set(ZST_CORE_MODULE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisableModule.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.h"
)
set(ZST_CORE_TRANSPORT_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportHelpers.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayer.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayerBase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstServiceDiscoveryTransport.h"   
)
if(BUILD_DRAFTS)
    list(APPEND ZST_CORE_TRANSPORT_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/transports/ZstUDPGraphTransport.h"
    )
endif()
set(ZST_CORE_LIASON_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityFactoryLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.hpp"
)
set(ZST_CORE_ADAPTOR_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstTransportAdaptor.hpp"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstZMQRefCounter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstLogging.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCableAddress.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSerialisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"    
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstIOLoop.cpp"
)
set(ZST_CORE_TRANSPORT_SRC
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayerBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstServiceDiscoveryTransport.cpp"   

)
if(BUILD_DRAFTS)
    list(APPEND ZST_CORE_TRANSPORT_SRC
        "${CMAKE_CURRENT_LIST_DIR}/transports/ZstUDPGraphTransport.cpp"
    )
endif()

set(ZST_CORE_MODULE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisableModule.cpp"
)
set(ZST_CORE_ENTITY_SRC
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityFactory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPlug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPerformer.cpp"
)
set(ZST_CORE_ADAPTOR_SRC 
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstEventAdaptorStubs.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstEventAdaptor.cpp"

)
set(ZST_CORE_LIASON_SRC
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityFactoryLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityLiason.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.cpp"
)

# Source groups
source_group( "showtime\\entities" FILES ${ENTITY_HEADERS} ${ZST_CORE_ENTITY_SRC})
source_group( "showtime\\adaptors" FILES ${ADAPTOR_HEADERS} ${ZST_CORE_ADAPTOR_HEADERS} ${ZST_CORE_ADAPTOR_SRC})
source_group( "showtime\\fmt" FILES ${FMT_HEADERS})
source_group( "showtime\\headers" FILES ${ZST_CORE_HEADERS} ${API_HEADERS})
source_group( "showtime\\source" FILES ${ZST_CORE_SRC})
source_group( "showtime\\modules" FILES ${ZST_CORE_MODULE_HEADERS} ${ZST_CORE_MODULE_SRC})
source_group( "showtime\\transports" FILES ${ZST_CORE_TRANSPORT_HEADERS} ${ZST_CORE_TRANSPORT_SRC})
source_group( "showtime\\liasons" FILES ${ZST_CORE_LIASON_HEADERS} ${ZST_CORE_LIASON_SRC})

# Add sources to object library so that we don't have to recompile shared and static libs
target_sources(${CORE_TARGET_OBJECTS} PRIVATE 
    ${ZST_CORE_HEADERS} 
    ${ZST_CORE_SRC} 
    ${ZST_CORE_MODULE_HEADERS}
    ${ZST_CORE_MODULE_SRC}
    ${ZST_CORE_TRANSPORT_HEADERS} 
    ${ZST_CORE_TRANSPORT_SRC}
    ${ZST_CORE_LIASON_HEADERS}
    ${ZST_CORE_LIASON_SRC}
    ${ZST_CORE_ADAPTOR_HEADERS}
    ${ZST_CORE_ADAPTOR_SRC}
    ${ZST_CORE_ENTITY_SRC}
    ${Showtime_API_INTERFACE}
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

# Target definitions
set(GLOBAL_PUBLIC_DEFINES
    CZMQ_STATIC
    ZMQ_STATIC
    BOOST_THREAD_USE_LIB
)

if(BUILD_DRAFTS)
    list(APPEND GLOBAL_PUBLIC_DEFINES
        ZMQ_BUILD_DRAFT_API
        CZMQ_BUILD_DRAFT_API
        ZST_BUILD_DRAFT_API
    )
endif()

# Platform specific options
IF(WIN32)
    list(APPEND GLOBAL_PUBLIC_DEFINES
        _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
        _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
        _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
        _SCL_SECURE_NO_WARNINGS
        _WIN32_WINNT=0x0601
        BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
        BOOST_UUID_FORCE_AUTO_LINK
    )
endif()

# Object target includes

get_target_property(CZMQ_INCLUDE_DIR czmq INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${CORE_TARGET_OBJECTS} 
    PRIVATE
    ${Showtime_PRIVATE_HEADER_DEPENDENCIES}
    ${CZMQ_INCLUDE_DIR}
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${Showtime_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${Showtime_COMPILED_INCLUDE_DIRS}>
)

# Object private link libraries
set(${CORE_TARGET}_PRIVATE_LINKED_LIBS 
    ${msgpackc_TARGET}
)

# Link libs object target to pull in includes
target_link_libraries(${CORE_TARGET_OBJECTS} 
    PUBLIC
    ${SHOWTIME_PUBLIC_LINKED_LIBS}
    PRIVATE 
    ${${CORE_TARGET}_PRIVATE_LINKED_LIBS}
    ${SHOWTIME_PRIVATE_LINKED_LIBS}
)

# Object compile definitions
target_compile_definitions(${CORE_TARGET_OBJECTS} 
    PUBLIC 
    ${GLOBAL_PUBLIC_DEFINES}
)

# Make sure core objects are position independent for shared libs
set_target_properties(${CORE_TARGET_OBJECTS} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Object API exports
target_compile_definitions(${CORE_TARGET_OBJECTS} 
    PRIVATE 
    ZST_EXPORT_CORE_API
)

# Static/shared library specific options
if(BUILD_STATIC)
    list(APPEND CORE_LIBS ${CORE_TARGET_STATIC})
    target_link_libraries(${CORE_TARGET_STATIC} PRIVATE ${SHOWTIME_PRIVATE_STATIC_LINKED_LIBS})
endif()

if(BUILD_SHARED)
    list(APPEND CORE_LIBS ${CORE_TARGET})

    if(WIN32)
        # Copy runtime dependencies
        add_custom_command(TARGET ${CORE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:czmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:libzmq>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            VERBATIM
        )

        target_link_libraries(${CORE_TARGET} PRIVATE ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS})
    endif()

    # Library API exports
    target_compile_definitions(${CORE_TARGET} 
        PRIVATE
        ZST_EXPORT_CORE_API
        INTERFACE
        ZST_IMPORT_CORE_API
    )
endif()

# Options common to both static/shared libraries
foreach(TARGET ${CORE_LIBS})
    set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})
    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp)

    # Target link libs again to link actual libraries
    target_link_libraries(${TARGET}
        PUBLIC
        ${CORE_TARGET_OBJECTS} 
        PRIVATE
        ${${CORE_TARGET}_PRIVATE_LINKED_LIBS}
        ${SHOWTIME_PRIVATE_LINKED_LIBS}
    )
endforeach()
