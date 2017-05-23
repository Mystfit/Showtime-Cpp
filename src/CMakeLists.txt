enable_testing()

set(ZST_HEADERS 
 ${Showtime_INCLUDE_DIRS}/Showtime.h
 ${Showtime_INCLUDE_DIRS}/ZstPerformer.h
 ${Showtime_INCLUDE_DIRS}/ZstActor.h	
 ${Showtime_INCLUDE_DIRS}/ZstPlug.h
 ${Showtime_INCLUDE_DIRS}/ZstExports.h
 ${Showtime_INCLUDE_DIRS}/ZstStage.h
 ${Showtime_INCLUDE_DIRS}/ZstMessages.h
)

set(ZST_SOURCE 
 "Showtime.cpp"
 "ZstPerformer.cpp"
 "ZstActor.cpp"
 "ZstMessages.cpp"
 "ZstPlug.cpp"
 "ZstStage.cpp"
)

include_directories(
	${Showtime_INCLUDE_DIRS}
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
	${CZMQ_LIBRARIES}
    ${MSGPACK_LIBRARIES}
)

add_test( section_heartbeat Exename arg1 arg2 ... )

# SWIG bindings
add_subdirectory(bindings)
