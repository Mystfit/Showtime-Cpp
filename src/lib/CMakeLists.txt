enable_testing()

set(ZST_HEADERS 
 ${BASEPATH}/ZstActor.h	
 ${BASEPATH}/ZstPlug.h
 ${BASEPATH}/ZstPerformance.h
 ${BASEPATH}/ZstExports.h
 ${BASEPATH}/ZstStage.h
 ${BASEPATH}/ZstMessages.h
)

set(ZST_SOURCE 
 "ZstActor.cpp"
 "ZstMessages.cpp"
 "ZstPlug.cpp"
 "ZstPerformance.cpp"
 "ZstStage.cpp"
)

include_directories(
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
