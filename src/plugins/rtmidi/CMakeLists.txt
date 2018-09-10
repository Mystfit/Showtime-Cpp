message(STATUS "Configuring ${RTMIDI_PLUGIN_TARGET} target")

# Plugin source files
set(ZST_RTMIDI_PLUGIN_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/RtMidiEntity.h"
)

set(ZST_RTMIDI_PLUGIN_SRC
  "${CMAKE_CURRENT_LIST_DIR}/RtMidiEntity.cpp"
)

# Include files in target
target_sources(${RTMIDI_PLUGIN_TARGET} PRIVATE 
    ${ZST_RTMIDI_PLUGIN_HEADERS}
    ${ZST_RTMIDI_PLUGIN_SRC}
)

if(WIN32)
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} PRIVATE -DZST_EXPORT_ENTITY_API)
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} INTERFACE -DZST_IMPORT_ENTITY_API)
endif()

# Library target dependencies
add_dependencies(${RTMIDI_PLUGIN_TARGET} ${CORE_TARGET})

# Set plugin output dir
set_target_properties(${RTMIDI_PLUGIN_TARGET} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_OUTPUT_DIR}
)

# Link libraries
target_link_libraries(${RTMIDI_PLUGIN_TARGET} PUBLIC ${CORE_TARGET} ${RTMIDI_TARGET})
