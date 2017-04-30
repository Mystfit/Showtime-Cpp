set(ZST_HEADERS 
 ${BASEPATH}/ZstInstrument.h
 ${BASEPATH}/ZstPlug.h
 ${BASEPATH}/ZstSection.h
 ${BASEPATH}/ZstExports.h
 ${BASEPATH}/ZstStage.h
 ${BASEPATH}/ZstMessages.hpp
)

set(ZST_SOURCE 
 "ZstInstrument.cpp"
 "ZstPlug.cpp"
 "ZstSection.cpp"
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

add_subdirectory(bindings)
