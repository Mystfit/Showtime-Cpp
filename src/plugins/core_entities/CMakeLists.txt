message(STATUS "Configuring ${CORE_ENTITIES_TARGET} target")

# Entity source files
set(ZST_CORE_ENTITIES_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/plugin.h"
  "${CMAKE_CURRENT_LIST_DIR}/Adder.h"
  "${CMAKE_CURRENT_LIST_DIR}/MathEntityFactory.h"
)

set(ZST_CORE_ENTITIES_SRC
  "${CMAKE_CURRENT_LIST_DIR}/plugin.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/Adder.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/MathEntityFactory.cpp"
)

# Include files in target
target_sources(${CORE_ENTITIES_TARGET} PRIVATE 
    ${ZST_CORE_ENTITIES_SRC}
    ${ZST_CORE_ENTITIES_HEADERS}
)

set(PLUGIN_LIBS Boost::boost)

# Target definitions
if(WIN32)
    target_compile_definitions(${CORE_ENTITIES_TARGET} PRIVATE -DZST_EXPORT_PLUGIN_API)
    target_compile_definitions(${CORE_ENTITIES_TARGET} INTERFACE -DZST_IMPORT_PLUGIN_API)

    # On windows we need to link against core since we can't do runtime symbol lookup
    list(APPEND PLUGIN_LIBS ${CORE_TARGET})
else()
    # Since we skip linking against core we need to pull in the includes and definitions
    get_target_property(CORE_COMPILE_DEFS ${CORE_TARGET_OBJECTS} INTERFACE_COMPILE_DEFINITIONS)
    target_include_directories(${CORE_ENTITIES_TARGET} PUBLIC ${Showtime_ALL_INCLUDE_DIRS})

    # Enable runtime symbol lookups
    target_link_options(${PYTHON_TARGET_WRAPPER} PRIVATE -undefined dynamic_lookup)
endif()

# Link against boost headers explicitly
target_link_libraries(${CORE_ENTITIES_TARGET} PRIVATE ${PLUGIN_LIBS})

# Set plugin output dir
set_target_properties(${CORE_ENTITIES_TARGET} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_OUTPUT_DIR}
)
