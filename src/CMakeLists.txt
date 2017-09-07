enable_testing()

# Write version info into header file
set(COMPILED_INCLUDES "${CMAKE_BINARY_DIR}/include")
set(VERSION_FILE "${COMPILED_INCLUDES}/version.h")
FILE(WRITE ${COMPILED_INCLUDES}/version.h "#define SHOWTIME_VERSION \"${SHOWTIME_VERSION}\"")

set(ZST_PUBLIC_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Showtime.h
 ${Showtime_INCLUDE_DIRS}/ZstURI.h
 ${Showtime_INCLUDE_DIRS}/ZstUtils.hpp
 ${Showtime_INCLUDE_DIRS}/ZstPlug.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformer.h
 ${Showtime_INCLUDE_DIRS}/ZstStage.h
 ${Showtime_INCLUDE_DIRS}/ZstMessages.h
 ${Showtime_INCLUDE_DIRS}/ZstEvent.h
 ${Showtime_INCLUDE_DIRS}/ZstCable.h
 ${Showtime_INCLUDE_DIRS}/ZstValue.h
 ${Showtime_INCLUDE_DIRS}/ZstCallbacks.h
 ${Showtime_INCLUDE_DIRS}/ZstConstants.h
)

set(ZST_ENTITIES
 ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
 ${Showtime_INCLUDE_DIRS}/entities/ZstFilter.h
 ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
 ${Showtime_INCLUDE_DIRS}/entities/ZstProxyComponent.h
 ${Showtime_INCLUDE_DIRS}/entities/AddFilter.h
)

set(ZST_PRIVATE_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Queue.h
 ${Showtime_INCLUDE_DIRS}/variant.hpp
 ${Showtime_INCLUDE_DIRS}/ZstEndpoint.h
 ${Showtime_INCLUDE_DIRS}/ZstActor.h	
 ${Showtime_INCLUDE_DIRS}/ZstPlugRef.h
 ${Showtime_INCLUDE_DIRS}/ZstURIWire.h
 ${Showtime_INCLUDE_DIRS}/ZstValueWire.h
 ${Showtime_INCLUDE_DIRS}/ZstEventWire.h
 ${Showtime_INCLUDE_DIRS}/ZstPlugEvent.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpointRef.h
 ${Showtime_INCLUDE_DIRS}/ZstExports.h
 ${Showtime_INCLUDE_DIRS}/ZstEntityRef.h
 ${Showtime_INCLUDE_DIRS}/ZstCallbackQueue.h
 ${VERSION_FILE}
)

source_group(Entities FILES ${ZST_ENTITIES})

set(ZST_HEADERS ${ZST_PUBLIC_HEADERS} ${ZST_PRIVATE_HEADERS} ${ZST_ENTITIES})

set(ZST_SOURCE 
 "Showtime.cpp"
 "ZstEndpoint.cpp"
 "ZstEndpointRef.cpp"
 "ZstURI.cpp"
 "ZstURIWire.cpp"
 "ZstUtils.cpp"
 "ZstActor.cpp"
 "ZstMessages.cpp"
 "ZstEvent.cpp"
 "ZstPlugEvent.cpp"
 "ZstEventWire.cpp"
 "ZstPlug.cpp"
 "ZstPlugRef.cpp"
 "ZstStage.cpp"
 "ZstCable.cpp"
 "ZstValue.cpp"
 "ZstValueWire.cpp"
 "ZstCallbacks.cpp"
 "ZstCallbackQueueInstantiations.cpp"
 "ZstEntityBase.cpp"
 "ZstEntityRef.cpp"
 "ZstFilter.cpp"
 "ZstComponent.cpp"
 "ZstProxyComponent.cpp"
 "AddFilter.cpp"
)

set(ZST_INCLUDES 
	${COMPILED_INCLUDES}
	${Showtime_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

include_directories(${ZST_INCLUDES})

set(ZST_LINK_LIBS
	${ZeroMQ_LIBRARIES}
	${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

# Set library definitions
IF(WIN32)
	add_definitions(-DEXPORTS_API)
ENDIF(WIN32)

# Create Showtime library target
add_library(${PROJECT_NAME} SHARED ${ZST_HEADERS} ${ZST_SOURCE})
target_link_libraries(${PROJECT_NAME} ${ZST_LINK_LIBS})

# Create standalone stage executable
SET(SHOWTIME_STAGE_NAME ShowtimeStage)
IF(WIN32)
	add_definitions(-DIMPORTS_API)

	# Set up libzmq and czmq runtime dependencies
	set(RUNTIMES_LIBZMQ_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to release libzmq dll")
	set(RUNTIMES_LIBZMQ_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to debug libzmq dll")
	set(RUNTIMES_LIBZMQ ${RUNTIMES_LIBZMQ_RELEASE} ${RUNTIMES_LIBZMQ_DEBUG})
	set(RUNTIMES_CZMQ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to czmq .dll")
ENDIF(WIN32)

add_executable (${SHOWTIME_STAGE_NAME} "ZstStageRunner.cpp") 
add_dependencies(${SHOWTIME_STAGE_NAME} ${PROJECT_NAME})
target_link_libraries(${SHOWTIME_STAGE_NAME} ${PROJECT_NAME})

if(MACOSX)
	# set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${ZST_HEADERS})
	set_target_properties(${PROJECT_NAME} PROPERTIES
	  FRAMEWORK TRUE
	  FRAMEWORK_VERSION "A"
	  VERSION "${SHOWTIME_VERSION}"
	  PUBLIC_HEADER "${ZST_HEADERS}"
  	)
ENDIF(MACOSX)

add_subdirectory(bindings)
