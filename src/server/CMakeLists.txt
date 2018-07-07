message(STATUS "Configuring ${SERVER_TARGET} target")

set(ZST_SERVER_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageSession.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageHierarchy.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageTransport.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformerStageProxy.h"
)

set(ZST_SERVER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstPerformerStageProxy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageRunner.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageHierarchy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageSession.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageTransport.cpp"
)

target_sources(${SERVER_TARGET} PRIVATE 
	${ZST_SERVER_HEADERS}
	${ZST_SERVER_SRC}
)

target_include_directories(${SERVER_TARGET} PRIVATE
  ${Showtime_SOURCE_DEPENDENCIES}
)

# Platform specific defines
IF(WIN32)
    target_compile_definitions(${SERVER_TARGET} PRIVATE -DZST_IMPORT_SERVER_API)
    target_compile_definitions(${SERVER_TARGET} INTERFACE -DZST_EXPORT_SERVER_API)
ENDIF(WIN32)

add_dependencies(${SERVER_TARGET} ${CORE_TARGET})
target_link_libraries(${SERVER_TARGET} 
					  ${CORE_TARGET} 
					  ${msgpackc_TARGET}
)
