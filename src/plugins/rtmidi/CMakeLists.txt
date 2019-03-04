cmake_minimum_required(VERSION 3.13)

project(ShowtimeRTMidi VERSION 0.0.1)

# Use C14
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED)

set(RTMIDI_PLUGIN_TARGET ShowtimeRTMidiPlugin)
message(STATUS "Configuring ${RTMIDI_PLUGIN_TARGET} target")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(NOT DEFINED PLUGIN_OUTPUT_DIR)
	set(PLUGIN_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins)
endif()

# Plugin source files
set(ZST_RTMIDI_PLUGIN_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/RtMidiEntity.h"
)

set(ZST_RTMIDI_PLUGIN_SRC
  "${CMAKE_CURRENT_LIST_DIR}/RtMidiEntity.cpp"
)

# Create Library
add_library(${RTMIDI_PLUGIN_TARGET} MODULE)

# Include files in target
target_sources(${RTMIDI_PLUGIN_TARGET} PRIVATE 
    ${ZST_RTMIDI_PLUGIN_HEADERS}
    ${ZST_RTMIDI_PLUGIN_SRC}
)

# Set compiler definitions
if(WIN32)
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} PRIVATE -DZST_EXPORT_ENTITY_API)
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} INTERFACE -DZST_IMPORT_ENTITY_API)
endif()

# Include packages
find_package(Showtime CONFIG REQUIRED)
find_package(rtmidi CONFIG REQUIRED)

# Library target dependencies
add_dependencies(${RTMIDI_PLUGIN_TARGET} ShowtimeCore)

# Set plugin output dir
set_target_properties(${RTMIDI_PLUGIN_TARGET} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGIN_OUTPUT_DIR}
)

# Link libraries
message(STATUS )
target_link_libraries(${RTMIDI_PLUGIN_TARGET} PUBLIC ShowtimeCore rtmidi)
