cmake_minimum_required(VERSION 3.13.0)

project(Showtime VERSION 0.18.0)
set(SHOWTIME_NAMESPACE "${PROJECT_NAME}::")

# Use C14
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Set build locations
set(CMAKE_BASE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;${CMAKE_MODULE_PATH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SWIG_ROOT_OUTDIR ${CMAKE_BINARY_DIR}/bindings)
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})

# Set CMake vars
set(CMAKE_FIND_FRAMEWORKS LAST)
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_SUPPRESS_REGENERATION true)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set platform flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
    MESSAGE(STATUS "Platform is OSX")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    MESSAGE(STATUS "Platform is Linux")
endif()

if(LINUX OR MACOSX)
    if(NOT DEFINED CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH} "/usr/local")
        if(LINUX)
            list(APPEND CMAKE_PREFIX_PATH "/home/linuxbrew/.linuxbrew")
        endif()
    endif() 
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    MESSAGE(STATUS "Platform is Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")

    option(WINDOWS_USE_DLL_RUNTIME "Use the Visual C++ DLL runtime" ON)
    if(WINDOWS_USE_DLL_RUNTIME)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
        set(Boost_USE_STATIC_RUNTIME OFF)
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(Boost_USE_STATIC_RUNTIME ON)
    endif()
endif()

# Set shared and static library flags
option(BUILD_STATIC "Build static Showtime library" OFF)
option(BUILD_SHARED "Build shared Showtime library" ON)
option(BUILD_DRAFTS "Build with experimental draft support" OFF)

# Set showtime variables
set(Showtime_COMPILED_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include/")
set(Showtime_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include/showtime/")
set(Showtime_ALL_INCLUDE_DIRS ${Showtime_COMPILED_INCLUDE_DIRS} ${Showtime_INCLUDE_DIRS})
set(Showtime_HEADER_DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/src/dependencies")

set(Showtime_API_INTERFACE "")
set(Showtime_LIBRARY_TARGETS "")
set(Showtime_APP_TARGETS "")
set(Showtime_INSTALL_TARGETS "")

# Boost properties
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_SYSTEM_PATHS ON)
if(NOT Boost_USE_STATIC_LIBS)
    set(Boost_USE_STATIC_LIBS ON)
endif()

if(NOT BUILD_SHARED AND NOT BUILD_STATIC)
    message(FATAL_ERROR "Need to set either BUILD_SHARED or BUILD_STATIC")
endif()

# --------
# Packages
# --------
set(LIBZMQ_TARGET "")
set(CZMQ_TARGET "")
message(STATUS "Finding packages")

# Boost
# --------
find_package(Boost COMPONENTS
    log
    log_setup
    thread
    system
    context
    fiber
    filesystem 
    date_time
    chrono
    atomic
    regex
    REQUIRED
)
set(Boost_HEADER_TARGET "Boost::boost")
set(Boost_LOG_TARGET "Boost::log")
set(Boost_LOG_SETUP_TARGET "Boost::log_setup")
set(Boost_THREAD_TARGET "Boost::thread")
set(Boost_FILESYSTEM_TARGET "Boost::filesystem")
set(Boost_SYSTEM_TARGET "Boost::system")
set(Boost_CONTEXT_TARGET "Boost::context")
set(Boost_FIBER_TARGET "Boost::fiber")
set(Boost_DATE_TIME_TARGET "Boost::date_time")
set(Boost_CHRONO_TARGET "Boost::chrono")
set(Boost_ATOMIC_TARGET "Boost::atomic")
set(Boost_REGEX_TARGET "Boost::regex")


# Msgpack, fmt, swig, json
# ------------------------------
find_package(MsgPack CONFIG REQUIRED)
set(msgpackc_TARGET "msgpackc-static")

# find_package(nlohmann_json CONFIG REQUIRED)
# set(json_TARGET "nlohmann_json")

if(WIN32)
    # Find swigwin downloaded using install_dependencies script
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}/swig")
endif()
find_package(SWIG)
if(SWIG_FOUND)
    cmake_policy(SET CMP0078 NEW)
    include(UseSWIG)
    set(UseSWIG_MODULE_VERSION 2)
endif()

# CZMQ, libZMQ
# ----
if(WIN32)
    # Use cmake versions of ZeroMQ and CZMQ
    find_package(ZeroMQ CONFIG REQUIRED)
    find_package(czmq CONFIG REQUIRED)
else()
    # Use homebrew versions of libzmq and CZMQ
    find_package(libzmq REQUIRED)
    find_package(czmq REQUIRED)
endif()

# Set internal linked library lists for our targets
set(SHOWTIME_PRIVATE_LINKED_LIBS 
    ${Boost_SYSTEM_TARGET}
    ${Boost_FILESYSTEM_TARGET}
    ${Boost_THREAD_TARGET}
    ${Boost_DATE_TIME_TARGET}
    ${Boost_CHRONO_TARGET}
    ${Boost_ATOMIC_TARGET}
    ${Boost_REGEX_TARGET}
    ${Boost_LOG_TARGET}
    ${Boost_LOG_SETUP_TARGET}
    ${Boost_CONTEXT_TARGET}
    ${Boost_FIBER_TARGET}
    ${Boost_HEADER_TARGET}
    Boost::disable_autolinking
    ${CMAKE_THREAD_LIBS_INIT}
)

# Boost libraries that MUST be linked shared
set(Showtime_PUBLIC_SHARED_BOOST_LINKED_LIBS
    ${Boost_FILESYSTEM_LIBRARY_DEBUG}
    ${Boost_CONTEXT_LIBRARY_DEBUG}
)

# Boost shared DLLs
set(Showtime_SHARED_BOOST_LIBS)
if(WIN32)
    # Copy boost dlls
    foreach(BOOST_LIB ${Showtime_PUBLIC_SHARED_BOOST_LINKED_LIBS})
        get_filename_component(BOOST_LIB_DIR ${BOOST_LIB} DIRECTORY)
        get_filename_component(BOOST_LIB_NAME ${BOOST_LIB} NAME_WE)
        string(REPLACE "libboost" "boost" BOOST_DLL_NAME ${BOOST_LIB_NAME})
        list(APPEND Showtime_SHARED_BOOST_LIBS ${BOOST_LIB_DIR}/${BOOST_DLL_NAME}.dll)
    endforeach()
    message(STATUS "Additional Boost DLLs that need to be copied: ${Showtime_SHARED_BOOST_LIBS}")
endif()

set(Showtime_PRIVATE_HEADER_DEPENDENCIES
    $<BUILD_INTERFACE:${Showtime_HEADER_DEPENDENCIES_DIR}>
    $<BUILD_INTERFACE:${Showtime_HEADER_DEPENDENCIES_DIR}/cf>
    $<BUILD_INTERFACE:${Showtime_HEADER_DEPENDENCIES_DIR}/concurrentqueue> 
)

set(SHOWTIME_PRIVATE_STATIC_LINKED_LIBS
    czmq-static 
    libzmq-static
)

set(SHOWTIME_PUBLIC_SHARED_LINKED_LIBS
    czmq
    libzmq
)

# set(SHOWTIME_PUBLIC_LINKED_LIBS
#     nlohmann_json
# )


# ----------------
# Showtime targets
# ----------------

# Core Library
# ------------
set(CORE_TARGET "${PROJECT_NAME}Core")
set(CORE_TARGET_STATIC "${PROJECT_NAME}CoreStatic")
set(CORE_TARGET_OBJECTS ${CORE_TARGET}Objects)
add_library(${CORE_TARGET_OBJECTS} OBJECT "")
list(APPEND Showtime_LIBRARY_TARGETS ${CORE_TARGET_OBJECTS})

if(BUILD_STATIC)
    add_library(${CORE_TARGET_STATIC} STATIC "")
    list(APPEND Showtime_LIBRARY_TARGETS ${CORE_TARGET_STATIC})
endif()
if(BUILD_SHARED)
    add_library(${CORE_TARGET} SHARED "")
    list(APPEND Showtime_LIBRARY_TARGETS ${CORE_TARGET})
endif()
include(src/core/CMakeLists.txt)


# Client Library
# --------------
set(CLIENT_TARGET "${PROJECT_NAME}Client")
set(CLIENT_TARGET_STATIC "${PROJECT_NAME}ClientStatic")
if(BUILD_STATIC)
    add_library(${CLIENT_TARGET_STATIC} STATIC "")
    list(APPEND Showtime_LIBRARY_TARGETS ${CLIENT_TARGET_STATIC})
endif()
if(BUILD_SHARED)
    add_library(${CLIENT_TARGET} SHARED "")
    list(APPEND Showtime_LIBRARY_TARGETS ${CLIENT_TARGET})
endif()
include(src/client/CMakeLists.txt)


# Server binary
# -------------
set(SERVER_TARGET "${PROJECT_NAME}Server")
set(SERVER_TARGET_STATIC "${PROJECT_NAME}ServerStatic")
set(SERVER_LAUNCHER "${PROJECT_NAME}ServerLauncher")
if(BUILD_STATIC)
    add_library (${SERVER_TARGET_STATIC} STATIC "")
    list(APPEND Showtime_LIBRARY_TARGETS ${SERVER_TARGET_STATIC})
endif()
if(BUILD_SHARED)
    add_library (${SERVER_TARGET} SHARED "")
    list(APPEND Showtime_LIBRARY_TARGETS ${SERVER_TARGET})
endif()
add_executable (${SERVER_LAUNCHER} "")
list(APPEND Showtime_APP_TARGETS ${SERVER_LAUNCHER})
include(src/server/CMakeLists.txt)


# Web monitor
# -------------
set(MONITOR_TARGET "${PROJECT_NAME}Monitor")
include(src/monitor/CMakeLists.txt)
#list(APPEND Showtime_APP_TARGETS ${MONITOR_TARGET})


# ------
# Plugins
# ------

set(PLUGIN_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins)
set(PLUGIN_TARGETS "")


# Core entity plugin
# -------------------

set(CORE_ENTITIES_TARGET "${PROJECT_NAME}CoreEntities")
add_library(${CORE_ENTITIES_TARGET} MODULE "")
include(src/plugins/core_entities/CMakeLists.txt)
list(APPEND PLUGIN_TARGETS ${CORE_ENTITIES_TARGET})


option(PLUGINS_RTMIDI "Build RTMidi plugin" OFF)
if(PLUGINS_RTMIDI)
    add_subdirectory(src/plugins/rtmidi)
endif()


# Bindings
# -------

# Dotnet bindings
option(BINDINGS_DOTNET "Generate bindings for C#" OFF)
option(BINDINGS_PYTHON "Generate bindings for Python" OFF)
option(BINDINGS_UNITY "Generate bindings for Unity" OFF)
option(BINDINGS_RUBY "Generate bindings for ruby" OFF)
option(BINDINGS_JAVA "Generate bindings for java" OFF)
option(BINDINGS_JRUBY "Generate bindings for JRuby" OFF)

if(BINDINGS_DOTNET OR BINDINGS_UNITY)
    set(BINDINGS_DOTNET_FRAMEWORK_VERSION v4.7.2 CACHE STRING "dotnet Framework version")
    set(DOTNET_FRAMEWORK_VERSIONS
        v3.5 
        v4 
        v4.5 
        v4.5.1 
        v4.5.2 
        v4.6 
        v4.6.1 
        v4.7.1 
        v4.7.2
        netstandard1.0 
        netstandard2.0
    )
    set_property(CACHE BINDINGS_DOTNET_FRAMEWORK_VERSION PROPERTY STRINGS ${DOTNET_FRAMEWORK_VERSIONS})
    set(BINDINGS_DOTNET ON)
    set(DOTNET_GENERATED_BINDINGS)
    set(DOTNET_TARGET ${PROJECT_NAME}Dotnet)
    set(DOTNET_STAMP ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnet/dotnet.stamp)
    set(DOTNET_OUTPUT_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DOTNET_TARGET}.dll)

    if(WIN32)
        enable_language(CSharp)
        add_library(${DOTNET_TARGET} SHARED "")
    else()
        add_custom_target(${DOTNET_TARGET} ALL DEPENDS ${DOTNET_OUTPUT_FILE})
    endif()
    include(src/bindings/dotnet/CMakeLists.txt)
endif()

if(BINDINGS_JRUBY)
    find_package(JRuby REQUIRED)

    # JRuby bindings require 
    if(NOT BINDINGS_JAVA)
        message(FATAL_ERROR "JRuby bindings require BINDINGS_JAVA to be enabled")
    endif()
endif()

if(BINDINGS_JAVA)
    set(JAVA_TARGET ${PROJECT_NAME}Java)
    find_package(Java REQUIRED)
    find_package(JNI REQUIRED)
    include(UseJava)
    include(src/bindings/java/CMakeLists.txt)
endif()


if(BINDINGS_UNITY)
    set(UNITY_TARGET ${PROJECT_NAME}Unity)
    include(src/bindings/unity/CMakeLists.txt)
endif()

if(BINDINGS_PYTHON)
    set(PYTHON_TARGET ${PROJECT_NAME}Python)
    option(BINDINGS_PYTHON_LINK_AT_RUNTIME "Link against Python at runtime" OFF)
    set(BINDINGS_PYTHON_VERSION_MAJOR 3 CACHE STRING "Python major version")
    set(BINDINGS_PYTHON_VERSION_MINOR 7 CACHE STRING "Python minor version")
    set(BINDINGS_PYTHON_VERSION_PATH 0 CACHE STRING "Python patch version")
    set(BINDINGS_PYTHON_VERSION ${BINDINGS_PYTHON_VERSION_MAJOR}.${BINDINGS_PYTHON_VERSION_MINOR}.${BINDINGS_PYTHON_VERSION_PATCH})
    find_package(Python${BINDINGS_PYTHON_VERSION_MAJOR} ${BINDINGS_PYTHON_VERSION} COMPONENTS Interpreter Development REQUIRED)
    set(Python_EXECUTABLE ${Python${BINDINGS_PYTHON_VERSION_MAJOR}_EXECUTABLE})
    set(PYTHON_LIB_TARGET Python${BINDINGS_PYTHON_VERSION_MAJOR}::Python)
    set(PYTHON_INCLUDE_DIR ${Python${BINDINGS_PYTHON_VERSION_MAJOR}_INCLUDE_DIR})
    add_custom_target(${PYTHON_TARGET} ALL)
    include(src/bindings/python/CMakeLists.txt)
endif()

if(BINDINGS_RUBY)
    set(RUBY_TARGET ${PROJECT_NAME}Ruby)
    set(RUBY_VERSION 2.5 CACHE STRING "Ruby version to search for")
    find_package(Ruby ${RUBY_VERSION} REQUIRED)
    include(src/bindings/ruby/CMakeLists.txt)
endif()


# Tests
# -----
include( CTest ) 
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()


# Installation
# ------------
set(CMAKE_INSTALL_CMAKEDIR "lib/cmake/showtime")

# Install targets
set(Showtime_INSTALL_TARGETS 
    ${Showtime_LIBRARY_TARGETS} 
    ${Showtime_APP_TARGETS}
)

# Set files to copy
install(DIRECTORY ${Showtime_ALL_INCLUDE_DIRS} DESTINATION include/showtime)
#install(FILES ${Showtime_API_INTERFACE} DESTINATION include/showtime)
foreach(lib ${SHOWTIME_PUBLIC_SHARED_LINKED_LIBS})
    install(FILES $<TARGET_FILE:${lib}> DESTINATION bin)
endforeach()

if(MACOSX)
    # Install framework
    install(TARGETS ${Showtime_INSTALL_TARGETS}
        EXPORT showtime-targets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION /Library/Frameworks
    )
else()
    # Install targets
    install(TARGETS ${Showtime_INSTALL_TARGETS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        EXPORT showtime-targets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

# Export targets
if(NOT CMAKE_VERSION VERSION_LESS 3.0)
    export(EXPORT showtime-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/showtime-targets.cmake"
        NAMESPACE ${SHOWTIME_NAMESPACE}
    )
endif()

# Install target exports
install(EXPORT showtime-targets
    FILE showtime-targets.cmake
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
)

# Set up CMake package
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    showtime-config-version.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file (showtime-config.cmake.in
    showtime-config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
)

# Install package files
install (
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/showtime-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/showtime-config-version.cmake"
    DESTINATION
        "${CMAKE_INSTALL_CMAKEDIR}"
)
