message(STATUS "Configuring ${CORE_ENTITIES_TARGET} target")

# Entity source files
set(ZST_CORE_ENTITIES_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/Adder.h"
)

set(ZST_CORE_ENTITIES_SRC
  "${CMAKE_CURRENT_LIST_DIR}/Adder.cpp"
)

# Include files in target
target_sources(${CORE_ENTITIES_TARGET} PRIVATE 
    ${ZST_CORE_ENTITIES_SRC}
    ${ZST_CORE_ENTITIES_HEADERS}
)

target_include_directories(${CORE_ENTITIES_TARGET}
    PRIVATE
        ${Showtime_SOURCE_DEPENDENCIES}
)

# Library target dependencies
add_dependencies(${CORE_ENTITIES_TARGET} ${CORE_TARGET})

# Target definitions
if(WIN32)
    target_compile_definitions(${CORE_ENTITIES_TARGET} PRIVATE -DZST_EXPORT_ENTITY_API)
    target_compile_definitions(${CORE_ENTITIES_TARGET} INTERFACE -DZST_IMPORT_ENTITY_API)
endif()

# Set plugin output dir

# Set plugin output dir
set_target_properties(${CORE_ENTITIES_TARGET} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_OUTPUT_DIR}
)

# Link libraries
target_link_libraries(${CORE_ENTITIES_TARGET} PUBLIC ${CORE_TARGET})
