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

set(PLUGIN_DEFINITIONS_PRIVATE "-DZST_EXPORT_PLUGIN_API")
set(PLUGIN_DEFINITIONS_INTERFACE "-DZST_IMPORT_PLUGIN_API")

# Target definitions
if(WIN32)
    # On windows we need to link against core since we can't do runtime symbol lookup
    #list(APPEND PLUGIN_LIBS ${CORE_TARGET})
else()
    # Since we skip linking against core we need to pull in the includes and definitions
    get_target_property(CORE_COMPILE_DEFS ${CORE_TARGET_OBJECTS} INTERFACE_COMPILE_DEFINITIONS)
    list(APPEND PLUGIN_DEFINITIONS_INTERFACE ${CORE_COMPILE_DEFS})

    # target_include_directories(${CORE_ENTITIES_TARGET} PUBLIC ${Showtime_ALL_INCLUDE_DIRS})

    # Enable runtime symbol lookups
    # target_link_options(${CORE_ENTITIES_TARGET} PRIVATE -undefined dynamic_lookup)
endif()

if(BUILD_SHARED)
  list(APPEND PLUGIN_LIBS ${CORE_TARGET})
elseif(BUILD_SHARED_FROM_STATIC_LIBS)
  list(APPEND PLUGIN_LIBS ${SHARED_FROM_STATIC_TARGET})
  message(STATUS "Linking ${CORE_ENTITIES_TARGET} against ${SHARED_FROM_STATIC_TARGET}")
else()
  list(APPEND PLUGIN_LIBS ${CORE_TARGET_STATIC})
endif()

# Compile definitions
target_compile_definitions(${CORE_ENTITIES_TARGET} PRIVATE ${PLUGIN_DEFINITIONS_PRIVATE})
target_compile_definitions(${CORE_ENTITIES_TARGET} INTERFACE ${PLUGIN_DEFINITIONS_INTERFACE})

# Link against boost headers explicitly
target_link_libraries(${CORE_ENTITIES_TARGET} PRIVATE ${PLUGIN_LIBS})
target_include_directories(${CORE_ENTITIES_TARGET} PUBLIC ${Showtime_ALL_INCLUDE_DIRS})


# Set plugin output dir
set_target_properties(${CORE_ENTITIES_TARGET} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_OUTPUT_DIR}
)
