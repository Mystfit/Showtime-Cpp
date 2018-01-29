set(ZST_SERVER_PUBLIC_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.h"
)

set(ZST_SERVER_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstStageRunner.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstStage.cpp"
)

target_sources(${SERVER_TARGET} PUBLIC ${ZST_SERVER_PUBLIC_HEADERS})
target_sources(${SERVER_TARGET} PRIVATE ${ZST_SERVER_SRC})

# Platform specific defines
IF(WIN32)
    target_compile_definitions(${SERVER_TARGET} PRIVATE -DIMPORT_SERVER_API)
    target_compile_definitions(${SERVER_TARGET} PRIVATE -DEXPORT_SERVER_API)
ENDIF(WIN32)

add_dependencies(${SERVER_TARGET} ${CORE_TARGET})
target_link_libraries(${SERVER_TARGET} ${CORE_TARGET})