message(STATUS "Configuring ${CORE_TARGET_STATIC} target")

# Generate version info header file
set(VERSION_FILE_IN "${CMAKE_CURRENT_LIST_DIR}/ZstVersion.h.in")
set(VERSION_FILE_OUT "${Showtime_COMPILED_INCLUDE_DIRS}/showtime/ZstVersion.h")
configure_file(${VERSION_FILE_IN} ${VERSION_FILE_OUT})
set_source_files_properties(${VERSION_FILE_OUT} PROPERTIES GENERATED ON)

# Generate empty source file to force some targets to compile
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp CONTENT "")

# Generated sources
set(GENERATED_SOURCES 
    ${VERSION_FILE_OUT}
    ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp
)

# Generated flatbuffer schemas
list(APPEND GENERATED_SOURCES ${FLATBUFFER_HEADER_OUTPUT})

# Set generated sources property
set_source_files_properties(${GENERATED_SOURCES} PROPERTIES GENERATED ON)

# Public include files
set(CORE_INCLUDE_DIR ${Showtime_INCLUDE_DIRS}/showtime)
set(ENTITY_HEADERS
    ${CORE_INCLUDE_DIR}/entities/ZstEntityFactory.h
    ${CORE_INCLUDE_DIR}/entities/ZstEntityBase.h
    ${CORE_INCLUDE_DIR}/entities/ZstComponent.h
    ${CORE_INCLUDE_DIR}/entities/ZstComputeComponent.h
    ${CORE_INCLUDE_DIR}/entities/ZstPlug.h
    ${CORE_INCLUDE_DIR}/entities/ZstPerformer.h
)
set(ADAPTOR_HEADERS 
    ${CORE_INCLUDE_DIR}/adaptors/ZstEventAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstLogAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstConnectionAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstComputeAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstSynchronisableAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstFactoryAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstSessionAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstHierarchyAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstEntityAdaptor.hpp
    ${CORE_INCLUDE_DIR}/adaptors/ZstPluginAdaptor.hpp
)

set(API_HEADERS
    ${CORE_INCLUDE_DIR}/ZstCore.h
    ${CORE_INCLUDE_DIR}/ZstLogging.h
    ${CORE_INCLUDE_DIR}/ZstCable.h
    ${CORE_INCLUDE_DIR}/ZstExceptions.h
    ${CORE_INCLUDE_DIR}/ZstCableAddress.h
    ${CORE_INCLUDE_DIR}/ZstConstants.h
    ${CORE_INCLUDE_DIR}/ZstExports.h
    ${CORE_INCLUDE_DIR}/ZstURI.h
    ${CORE_INCLUDE_DIR}/ZstPlugin.h
    ${CORE_INCLUDE_DIR}/ZstSerialisable.h
    ${CORE_INCLUDE_DIR}/ZstSynchronisable.h
    ${CORE_INCLUDE_DIR}/ZstBundle.hpp
    ${CORE_INCLUDE_DIR}/ZstServerAddress.h
    ${CORE_INCLUDE_DIR}/ZstEventDispatcher.h
    ${CORE_INCLUDE_DIR}/ZstPointerUtils.h
    ${CORE_INCLUDE_DIR}/ZstFilesystemUtils.h
    ${CORE_INCLUDE_DIR}/ZstIValue.h
    ${VERSION_FILE_OUT}
)
list(APPEND Showtime_API_INTERFACE 
    ${API_HEADERS}
    ${ENTITY_HEADERS}
    ${ADAPTOR_HEADERS}
    # ${FMT_HEADERS}
    ${FLATBUFFER_HEADERS}
)

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstZMQRefCounter.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstDynamicValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstFixedValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValueUtils.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstServerBeaconMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSemaphore.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstIOLoop.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCorePointerUtils.h"
    "${CMAKE_CURRENT_LIST_DIR}/boost_use_future.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/boost_use_future_imp.hpp"
)
set(ZST_CORE_MODULE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisableModule.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstHierarchy.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPluginLoader.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventModule.h"
)
set(ZST_CORE_TRANSPORT_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstSTUNService.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportHelpers.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTransportLayerBase.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstStageTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.h"
    #"${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstServiceDiscoveryTransport.h"   
)
if(BUILD_DRAFTS)
    list(APPEND ZST_CORE_TRANSPORT_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/transports/ZstUDPGraphTransport.h"
    )
endif()

set(ZST_CORE_SCHEMA_HEADERS "")
list(APPEND ZST_CORE_SCHEMA_HEADERS ${FLATBUFFER_HEADER_OUTPUT})

set(ZST_CORE_LIASON_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstSynchronisableLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityFactoryLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstPlugLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstEntityLiason.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/liasons/ZstCableLiason.hpp"
)
set(ZST_CORE_ADAPTOR_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstTransportAdaptor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstGraphTransportAdaptor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstStageTransportAdaptor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/adaptors/ZstServiceDiscoveryAdaptor.hpp"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstZMQRefCounter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMsgID.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstLogging.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstServerAddress.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCableAddress.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSerialisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstDynamicValue.cpp"   
    "${CMAKE_CURRENT_LIST_DIR}/ZstFixedValue.cpp"    
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstServerBeaconMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformanceMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstReaper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessageSupervisor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstIOLoop.cpp"
)
set(ZST_CORE_TRANSPORT_SRC
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstSTUNService.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstStageTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstGraphTransport.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPGraphTransport.cpp"
    #"${CMAKE_CURRENT_LIST_DIR}/transports/ZstTCPSession.cpp"
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
    "${CMAKE_CURRENT_LIST_DIR}/ZstPluginLoader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventModule.cpp"
)
set(ZST_CORE_ENTITY_SRC
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityFactory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComputeComponent.cpp"    
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
# source_group( "showtime\\fmt" FILES ${FMT_HEADERS})
source_group( "showtime\\headers" FILES ${ZST_CORE_HEADERS} ${API_HEADERS})
source_group( "showtime\\source" FILES ${ZST_CORE_SRC})
source_group( "showtime\\modules" FILES ${ZST_CORE_MODULE_HEADERS} ${ZST_CORE_MODULE_SRC})
source_group( "showtime\\transports" FILES ${ZST_CORE_TRANSPORT_HEADERS} ${ZST_CORE_TRANSPORT_SRC})
source_group( "showtime\\liasons" FILES ${ZST_CORE_LIASON_HEADERS} ${ZST_CORE_LIASON_SRC})
source_group( "showtime\\schemas" FILES ${ZST_CORE_SCHEMA_HEADERS})

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
    # ${ZST_CORE_ADAPTOR_HEADERS}
    ${ZST_CORE_ADAPTOR_SRC}
    ${ZST_CORE_ENTITY_SRC}
    # ${ZST_CORE_SCHEMA_HEADERS}
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

# Target definitions
set(GLOBAL_PUBLIC_DEFINES
    BOOST_THREAD_USE_LIB
    # BOOST_THREAD_VERSION=4
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
        BOOST_ALL_NO_LIB
    )
endif()

target_include_directories(${CORE_TARGET_OBJECTS} PUBLIC ${CORE_INCLUDES})

# Link libs object target to pull in includes
target_link_libraries(${CORE_TARGET_OBJECTS} 
    PUBLIC
    ${SHOWTIME_PUBLIC_LINKED_LIBS}
    PRIVATE 
    ${SHOWTIME_PRIVATE_LINKED_LIBS}
)

# Object compile definitions
target_compile_definitions(${CORE_TARGET_OBJECTS} 
    PUBLIC 
    ${GLOBAL_PUBLIC_DEFINES}
    PRIVATE 
    ZST_EXPORT_CORE_API
    CZMQ_STATIC
    ZMQ_STATIC
    ${SHOWTIME_PRIVATE_DEFS}
)

# Generate PDB files for release
target_compile_options(${CORE_TARGET_OBJECTS} 
    PRIVATE ${SHOWTIME_RELEASE_PDB_COMPILE_DEFINITIONS}
)

# Make sure core objects are position independent for shared libs
set_target_properties(${CORE_TARGET_OBJECTS} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Static/shared library specific options
if(BUILD_STATIC)
    list(APPEND CORE_LIBS ${CORE_TARGET_STATIC})
    target_link_libraries(${CORE_TARGET_STATIC} PRIVATE ${SHOWTIME_PRIVATE_STATIC_LINKED_LIBS})
    target_compile_definitions(${CORE_TARGET_STATIC} 
        PUBLIC
        CZMQ_STATIC
        ZMQ_STATIC
    )
        
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
    endif()

    target_link_libraries(${CORE_TARGET} PRIVATE ${SHOWTIME_PRIVATE_SHARED_LINKED_LIBS})

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
    # Generate PDB files for release
    target_compile_options(${TARGET} PRIVATE ${SHOWTIME_RELEASE_PDB_COMPILE_DEFINITIONS})
    target_link_options(${TARGET} PRIVATE ${SHOWTIME_RELEASE_PDB_LINK_FLAGS})
    target_include_directories(${TARGET} PUBLIC ${CORE_INCLUDES})
    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/dummy.cpp)
    set_target_properties(${TARGET} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})

    # Target link libs again to link actual libraries
    target_link_libraries(${TARGET}
        PUBLIC 
        ${SHOWTIME_PUBLIC_LINKED_LIBS}
        PRIVATE
        ${CORE_TARGET_OBJECTS}
        ${SHOWTIME_PRIVATE_LINKED_LIBS}
    )
endforeach()
