enable_testing()

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
)

set(ZST_PRIVATE_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Queue.h
 ${Showtime_INCLUDE_DIRS}/ZstEndpoint.h
 ${Showtime_INCLUDE_DIRS}/ZstActor.h	
 ${Showtime_INCLUDE_DIRS}/ZstPlugRef.h
 ${Showtime_INCLUDE_DIRS}/ZstURIWire.h
 ${Showtime_INCLUDE_DIRS}/ZstValueWire.h
 ${Showtime_INCLUDE_DIRS}/ZstEventWire.h
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
 "ZstEventWire.cpp"
 "ZstPlug.cpp"
 "ZstPlugRef.cpp"
 "ZstPerformer.cpp"
 "ZstPerformerRef.cpp"
 "ZstEndpointRef.cpp"
 "ZstStage.cpp"
 "ZstCable.cpp"
 "ZstValue.cpp"
 "ZstValueWire.cpp"
)

set(ZST_INCLUDES 
	${Showtime_INCLUDE_DIRS}
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

include_directories(${ZST_INCLUDES})

link_directories(${Boost_LIBRARY_DIRS})
set(ZST_LINK_LIBS
	${ZeroMQ_LIBRARIES}
	${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

IF(WIN32)
	add_definitions(-DEXPORTS_API)
ENDIF(WIN32)
add_definitions(-DMSGPACK_USE_BOOST)

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

	# option(FRAMEWORK_SYMLINK "Symlink Showtime.framework into /Library/Frameworks" OFF)


	# IF(FRAMEWORK_SYMLINK)
	# 	add_custom_command(
	# 		OUTPUT /Library/Frameworks/Showtime.framework
 #            COMMAND ln -s ${SOURCE} ${TARGET}
 #            DEPENDS ${PROJECT_NAME}
 #        )

	# 	# Create target and install
	# 	add_custom_target(INSTALL_PYTHON ALL DEPENDS ${BUILD_STAMP})
	# 	install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")
	# ENDIF()

	# IF(NOT FRAMEWORK_SYMLINK)
	# 	install(
	# 		TARGETS ${PROJECT_NAME}
	# 		FRAMEWORK DESTINATION "/Library/Frameworks/"
	# 		PUBLIC_HEADER DESTINATION "/usr/local/include/${PROJECT_NAME}"
	# 	)
	# ENDIF()

ENDIF(MACOSX)

add_subdirectory(bindings)
