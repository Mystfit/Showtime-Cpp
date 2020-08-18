cmake_minimum_required(VERSION 3.13)

project(ShowtimePluginMidi VERSION 0.0.1)


# Use C14
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED)

set(RTMIDI_PLUGIN_TARGET ${PROJECT_NAME})
message(STATUS "Configuring ${RTMIDI_PLUGIN_TARGET} target")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(NOT DEFINED PLUGIN_OUTPUT_DIR)
    set(PLUGIN_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins)
endif()

# Plugin dependant libs
find_package(Boost COMPONENTS ${BOOST_COMPONENTS} CONFIG)
if(NOT Boost_FOUND)
    message(STATUS "Searching for Boost using cmake's FindBoost")
    find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
endif()
set(PLUGIN_LIBS 
    Boost::boost
    ShowtimeCore
    RtMidi::rtmidi
)

# Pull content
include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


# Plugin source files
set(ZST_RTMIDI_PLUGIN_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/plugin.h"
  "${CMAKE_CURRENT_LIST_DIR}/MidiFactory.h"
  "${CMAKE_CURRENT_LIST_DIR}/MidiPort.h"
)

set(ZST_RTMIDI_PLUGIN_SRC
  "${CMAKE_CURRENT_LIST_DIR}/plugin.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/MidiFactory.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/MidiPort.cpp"
)

# Plugin compile defs
set(PLUGIN_DEFINITIONS_PRIVATE "-DZST_EXPORT_PLUGIN_API")
set(PLUGIN_DEFINITIONS_INTERFACE "-DZST_IMPORT_PLUGIN_API")

# Create Library
add_library(${RTMIDI_PLUGIN_TARGET} MODULE)

# Include files in target
target_sources(${RTMIDI_PLUGIN_TARGET} PRIVATE 
    ${ZST_RTMIDI_PLUGIN_HEADERS}
    ${ZST_RTMIDI_PLUGIN_SRC}
)

# Set compiler definitions
if(WIN32)
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} PRIVATE ${PLUGIN_DEFINITIONS_PRIVATE})
    target_compile_definitions(${RTMIDI_PLUGIN_TARGET} INTERFACE ${PLUGIN_DEFINITIONS_INTERFACE})
endif()

# Include packages
# find_package(Showtime CONFIG REQUIRED)
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
target_link_libraries(${RTMIDI_PLUGIN_TARGET} PUBLIC ${PLUGIN_LIBS} nlohmann_json::nlohmann_json)
