set(SOURCE_FILES 
 "ZstInstrument.h"
 "ZstPlug.h"
 "ZstSection.h"
 "ZstExports.h"
 "ZstStage.h"
 
 "ZstInstrument.cpp"
 "ZstPlug.cpp"
 "ZstSection.cpp"
 "ZstStage.cpp"
)

include_directories(
	${ZeroMQ_INCLUDE_DIRS}
	${CZMQ_INCLUDE_DIRS}
)

add_definitions(-DEXPORTS_API)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} 
	${ZeroMQ_LIBRARIES}
	${CZMQ_LIBRARIES}
)