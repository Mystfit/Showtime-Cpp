# Entity source files
set(ZST_COMMON_ENTITIES_PRIVATE_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/Adder.h"
)

set(ZST_COMMON_ENTITIES_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Adder.cpp"
)

# Include files in target
target_sources(${COMMON_ENTITIES_TARGET} PRIVATE ${ZST_COMMON_ENTITIES_PRIVATE_HEADERS} ${ZST_COMMON_ENTITIES_SRC})

# Library target dependencies
add_dependencies(${COMMON_ENTITIES_TARGET} ${CORE_TARGET})

# Link libraries
target_link_libraries(${COMMON_ENTITIES_TARGET} ${CORE_TARGET})CZMQ_INCLUDE_DIR
