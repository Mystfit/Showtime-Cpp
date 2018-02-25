# Entity source files
set(ZST_CORE_ENTITIES_PUBLIC_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/Adder.h"
)

set(ZST_CORE_ENTITIES_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Adder.cpp"
)

# Include files in target
target_sources(${CORE_ENTITIES_TARGET} PUBLIC ${ZST_CORE_ENTITIES_PUBLIC_HEADERS})
target_sources(${CORE_ENTITIES_TARGET} PRIVATE ${ZST_CORE_ENTITIES_SRC})

# Library target dependencies
add_dependencies(${CORE_ENTITIES_TARGET} ${CORE_TARGET})

# Link libraries
target_link_libraries(${CORE_ENTITIES_TARGET} ${CORE_TARGET})
