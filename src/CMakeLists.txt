enable_testing()

set(ZST_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Showtime.h
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

include_directories(
	${Showtime_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
)

IF(WIN32)
	add_definitions(-DEXPORTS_API)
ENDIF(WIN32)

add_library(${PROJECT_NAME} SHARED ${ZST_HEADERS} ${ZST_SOURCE})

target_link_libraries(${PROJECT_NAME} 
	${ZeroMQ_LIBRARIES}
	${Boost_LIBRARIES}
	${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

add_test( section_heartbeat Exename arg1 arg2 ... )

# SWIG bindings
option(BINDINGS_RUBY "Build python bindings" OFF)
option(BINDINGS_PYTHON "Build ruby bindings" OFF)
option(BINDINGS_DOTNET "Build C# bindings" OFF)

if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_PYTHON)
	INCLUDE(FindPythonInterp)
	FIND_PACKAGE(PythonLibs 3.6 REQUIRED)
	get_filename_component(PYTHON_DIR ${PYTHON_EXECUTABLE} DIRECTORY)
	set(PYTHON_SITE_PACKAGES ${PYTHON_DIR}/lib/site-packages)

	add_subdirectory(bindings/python)
	install(TARGETS ${PROJECT_NAME} DESTINATION ${PYTHON_DIR})
endif(BINDINGS_PYTHON)

if(BINDINGS_RUBY)
	add_subdirectory(bindings/ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_DOTNET)
	add_subdirectory(bindings/dotnet)
endif(BINDINGS_DOTNET)
