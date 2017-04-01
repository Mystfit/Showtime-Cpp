set(SOURCE_FILES 
 "ZstInstrument.h"
 "ZstPlug.h"
 "ZstSection.h"
 
 "ZstInstrument.cpp"
 "ZstPlug.cpp"
 "ZstSection.cpp"
)

include_directories(
	ZeroMQ_INCLUDE_DIRS
)

add_definitions(-DEXPORTS_API)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} 
	ZeroMQ::libzmq
)