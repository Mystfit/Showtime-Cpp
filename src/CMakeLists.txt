enable_testing()

set(ZST_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Showtime.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpoint.h
 ${Showtime_INCLUDE_DIRS}/ZstURI.h
 ${Showtime_INCLUDE_DIRS}/ZstUtils.hpp
 ${Showtime_INCLUDE_DIRS}/ZstActor.h	
 ${Showtime_INCLUDE_DIRS}/ZstPlug.h
 ${Showtime_INCLUDE_DIRS}/ZstPlugRef.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformer.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformerRef.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpointRef.h
 ${Showtime_INCLUDE_DIRS}/ZstExports.h
 ${Showtime_INCLUDE_DIRS}/ZstStage.h
 ${Showtime_INCLUDE_DIRS}/ZstMessages.h
)

set(ZST_SOURCE 
 "Showtime.cpp"
 "ZstEndpoint.cpp"
 "ZstURI.cpp"
 "ZstUtils.cpp"
 "ZstActor.cpp"
 "ZstMessages.cpp"
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

if(BINDINGS_PYTHON)
	add_definitions(-DUSEPYTHON)
	FIND_PACKAGE(PythonLibs 3.6 REQUIRED)
	list(APPEND ZST_INCLUDES ${PYTHON_INCLUDE_PATH})
	list(APPEND ZST_LINK_LIBS ${PYTHON_LIBRARIES})
ENDIF(BINDINGS_PYTHON)

include_directories(${ZST_INCLUDES})

IF(WIN32)
	add_definitions(-DEXPORTS_API)
ENDIF(WIN32)

add_library(${PROJECT_NAME} SHARED ${ZST_HEADERS} ${ZST_SOURCE})
target_link_libraries(${PROJECT_NAME} ${ZST_LINK_LIBS})
add_test( section_heartbeat Exename arg1 arg2 ... )


if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_PYTHON)
	INCLUDE(FindPythonInterp)
	get_filename_component(PYTHON_DIR ${PYTHON_EXECUTABLE} DIRECTORY)
	set(PYTHON_SITE_PACKAGES ${PYTHON_DIR}/lib/site-packages)

	add_subdirectory(bindings/python)
endif(BINDINGS_PYTHON)

if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_DOTNET)
	add_subdirectory(bindings/dotnet)
endif(BINDINGS_DOTNET)
