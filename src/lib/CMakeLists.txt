set(ZST_SOURCES 
 ${ZST_ROOT}/ZstInstrument.cpp
 ${ZST_ROOT}/ZstPlug.cpp
 ${ZST_ROOT}/ZstSection.cpp
)

set(ZST_HEADERS
 ${ZST_ROOT}/ZstInstrument.h
 ${ZST_ROOT}/ZstPlug.h
 ${ZST_ROOT}/ZstSection.h
 ${ZST_ROOT}/ZstUtils.h
)

add_definitions(-DEXPORTS_API)

add_library(${PROJECT_NAME} SHARED ${ZST_SOURCES} ${ZST_HEADERS})

target_link_libraries(${PROJECT_NAME} 
	ZeroMQ::libzmq
)