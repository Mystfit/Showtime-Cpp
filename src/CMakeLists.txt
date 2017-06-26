enable_testing()

set(ZST_PUBLIC_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Showtime.h
 ${Showtime_INCLUDE_DIRS}/ZstURI.h
 ${Showtime_INCLUDE_DIRS}/ZstURIWire.h
 ${Showtime_INCLUDE_DIRS}/ZstUtils.hpp
 ${Showtime_INCLUDE_DIRS}/ZstPlug.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformer.h
 ${Showtime_INCLUDE_DIRS}/ZstStage.h
 ${Showtime_INCLUDE_DIRS}/ZstMessages.h
 ${Showtime_INCLUDE_DIRS}/ZstEvent.h

)

set(ZST_PRIVATE_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Queue.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpoint.h
 ${Showtime_INCLUDE_DIRS}/ZstActor.h	
 ${Showtime_INCLUDE_DIRS}/ZstPlugRef.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformerRef.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpointRef.h
 ${Showtime_INCLUDE_DIRS}/ZstExports.h
)

set(ZST_HEADERS ${ZST_PUBLIC_HEADERS} ${ZST_PRIVATE_HEADERS})

set(ZST_SOURCE 
 "Showtime.cpp"
 "ZstEndpoint.cpp"
 "ZstURI.cpp"
 "ZstURIWire.cpp"
 "ZstUtils.cpp"
 "ZstActor.cpp"
 "ZstMessages.cpp"
 "ZstEvent.cpp"
 "ZstPlug.cpp"
 "ZstPlugRef.cpp"
 "ZstPerformer.cpp"
 "ZstPerformerRef.cpp"
 "ZstEndpointRef.cpp"
 "ZstStage.cpp"
)

set(ZST_INCLUDES 
	${Showtime_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

set(ZST_LINK_LIBS
	${ZeroMQ_LIBRARIES}
	${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

# SWIG bindings
option(BINDINGS_RUBY "Build python bindings" OFF)
option(BINDINGS_PYTHON "Build ruby bindings" OFF)
option(BINDINGS_DOTNET "Build C# bindings" OFF)

# Set up libzmq and czmq runtime dependencies
set(RUNTIMES_LIBZMQ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to libzmq dll/dylib/so")
set(RUNTIMES_CZMQ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE FILEPATH "Path to czmq .dll/.dylib/.so")

include_directories(${ZST_INCLUDES})

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
ENDIF(WIN32)
add_executable (${SHOWTIME_STAGE_NAME} "ZstStageRunner.cpp") 
add_dependencies(${SHOWTIME_STAGE_NAME} ${PROJECT_NAME})
target_link_libraries(${SHOWTIME_STAGE_NAME} ${PROJECT_NAME})

if(MACOSX)
	# set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${ZST_HEADERS})
	set_target_properties(${PROJECT_NAME} PROPERTIES
	  FRAMEWORK TRUE
	  FRAMEWORK_VERSION "${SHOWTIME_VERSION}"
	  PUBLIC_HEADER "${ZST_HEADERS}"
  	)

	install(TARGETS ${PROJECT_NAME}
		FRAMEWORK DESTINATION "/Library/Frameworks/"
		PUBLIC_HEADER DESTINATION "/usr/local/include/${PROJECT_NAME}"
	)
ENDIF(MACOSX)

add_test( section_heartbeat Exename arg1 arg2 ... )

if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_PYTHON)
    FIND_PACKAGE(PythonLibs 2.7.12 REQUIRED)
	INCLUDE(FindPythonInterp)
	get_filename_component(PYTHON_DIR ${PYTHON_EXECUTABLE} DIRECTORY)	
	add_subdirectory(bindings/python)
endif(BINDINGS_PYTHON)

if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_DOTNET)
	add_subdirectory(bindings/dotnet)
endif(BINDINGS_DOTNET)
