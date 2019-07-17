cmake_minimum_required(VERSION 3.13.0)

project(Showtime VERSION 0.21.0)
set(SHOWTIME_NAMESPACE "${PROJECT_NAME}::")

# Use C17
set(CMAKE_CXX_STANDARD 17)
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

if(${CMAKE_SYSTM_NAME} MATCHES "Android")
    set(ANDROID TRUE)
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
option(BUILD_SERVER_LAUNCHER "Build server launcher program" ON)


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
set(BOOST_COMPONENTS 
    log
    log_setup
    thread
    system
    context
    fiber
    date_time
    chrono
    atomic
    regex
    unit_test_framework
)
find_package(Boost COMPONENTS ${BOOST_COMPONENTS} CONFIG)
if(NOT Boost_FOUND)
    message(STATUS "Searching for Boost using cmake's FindBoost")
    find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
endif()

set(Boost_HEADER_TARGET "Boost::boost")
set(Boost_LOG_TARGET "Boost::log")
set(Boost_LOG_SETUP_TARGET "Boost::log_setup")
set(Boost_THREAD_TARGET "Boost::thread")
set(Boost_SYSTEM_TARGET "Boost::system")
set(Boost_CONTEXT_TARGET "Boost::context")
set(Boost_FIBER_TARGET "Boost::fiber")
set(Boost_DATE_TIME_TARGET "Boost::date_time")
set(Boost_CHRONO_TARGET "Boost::chrono")
set(Boost_ATOMIC_TARGET "Boost::atomic")
set(Boost_REGEX_TARGET "Boost::regex")
set(Boost_TEST_TARGET "Boost::unit_test_framework")

# Msgpack, swig
# ------------------------------
find_package(MsgPack CONFIG REQUIRED)
find_package(SWIG)
if(SWIG_FOUND)
    cmake_policy(SET CMP0078 NEW)
    cmake_policy(SET CMP0086 NEW)
    include(UseSWIG)
    set(UseSWIG_MODULE_VERSION 2)
endif()

# CZMQ, libZMQ
# ----
find_package(ZeroMQ CONFIG)
if(NOT ZeroMQ_FOUND)
    message(STATUS "Searching for libzmq using fallback Findlibzmq.cmake")
    find_package(libZMQ REQUIRED)
endif()

find_package(czmq CONFIG)
if(NOT czmq_FOUND)
    message(STATUS "Searching for czmq using fallback Findczmq.cmake")
    find_package(czmq REQUIRED)
endif()

# Set internal linked library lists for our targets
set(SHOWTIME_PRIVATE_LINKED_LIBS 
    ${Boost_SYSTEM_TARGET}
    ${Boost_THREAD_TARGET}
    ${Boost_DATE_TIME_TARGET}
    ${Boost_CHRONO_TARGET}
    ${Boost_ATOMIC_TARGET}
    ${Boost_REGEX_TARGET}
    ${Boost_LOG_TARGET}
    ${Boost_LOG_SETUP_TARGET}
    ${Boost_CONTEXT_TARGET}
    ${Boost_FIBER_TARGET}
    ${Boost_TEST_TARGET}
    ${Boost_HEADER_TARGET}
    Boost::disable_autolinking
    ${CMAKE_THREAD_LIBS_INIT}
)

# Boost libraries that MUST be linked shared
set(Showtime_PUBLIC_SHARED_BOOST_LINKED_LIBS
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
    msgpackc-static
)

set(SHOWTIME_PUBLIC_SHARED_LINKED_LIBS
    czmq
    libzmq
    msgpackc
)


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
if(BUILD_SERVER_LAUNCHER)
    add_executable (${SERVER_LAUNCHER} "")
    list(APPEND Showtime_APP_TARGETS ${SERVER_LAUNCHER})
endif()
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

# set(CORE_ENTITIES_TARGET "${PROJECT_NAME}CoreEntities")
# add_library(${CORE_ENTITIES_TARGET} MODULE "")
# include(src/plugins/core_entities/CMakeLists.txt)
# list(APPEND PLUGIN_TARGETS ${CORE_ENTITIES_TARGET})


option(PLUGINS_RTMIDI "Build RTMidi plugin" OFF)
if(PLUGINS_RTMIDI)
    add_subdirectory(src/plugins/rtmidi)
endif()


# Container compilation
# -----------------------------
option(BUILD_ANDROID_LIBS "Build Android libs using Singularity" OFF)
set(ANDROID_SINGULARITY_TARGET AndroidContainerBuild)
set(ANDROID_OUTPUT_LIB "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Android/lib${CLIENT_TARGET}.so")

if(NOT WIN32)
    option(BUILD_WINDOWS_CONTAINER "Build Windows libs using Vagrant" OFF)
endif()
set(WINDOWS_CONTAINER_OUTPUT_LIB "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Windows/${CLIENT_TARGET}.dll")
set(WINDOWS_CONTAINER_TARGET WindowsContainerBuild)

if(NOT MACOSX)
    option(BUILD_MACOS_CONTAINER "Build MacOS libs using Vagrant" OFF)
endif()
set(MACOS_CONTAINER_OUTPUT_LIB "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/MacOS/lib${CLIENT_TARGET}.dylib")
set(MACOS_CONTAINER_TARGET MacOSContainerBuild)

# Use Vagrant to create/run our build containers
if(BUILD_ANDROID_LIBS OR BUILD_WINDOWS_CONTAINER OR BUILD_MACOS_CONTAINER)
    find_program(VAGRANTCOMMAND vagrant)
    if(VAGRANTCOMMAND EQUAL vagrant-NOTFOUND)
        message(FATAL_ERROR "Could not find Vagrant")
    endif()

    # Install vagrant scp plugin
    execute_process(COMMAND ${VAGRANTCOMMAND} plugin list OUTPUT_VARIABLE VAGRANT_PLUGIN_LIST)
    string(FIND ${VAGRANT_PLUGIN_LIST} "vagrant-scp" VAGRANT_SCP_PLUGIN_FOUND)
    if(${VAGRANT_SCP_PLUGIN_FOUND} LESS 0)
        execute_process(COMMAND ${VAGRANTCOMMAND} plugin install vagrant-scp)
    endif()
    
    set(CONTAINER_CPU_COUNT 4 CACHE STRING "Number of cpu cores to run build containers with")
    set(CONTAINER_MEMORY_AMOUNT 4096 CACHE STRING "Amount of RAM to dedicate to the build containers")
endif()

if(BUILD_ANDROID_LIBS)
    add_custom_target(${ANDROID_SINGULARITY_TARGET} ALL DEPENDS "${ANDROID_OUTPUT_LIB}")
    include(containers/android/CMakeLists.txt)
endif()

if(BUILD_WINDOWS_CONTAINER)
    add_custom_target(${WINDOWS_CONTAINER_TARGET} ALL DEPENDS "${WINDOWS_CONTAINER_OUTPUT_LIB}")
    include(containers/windows/CMakeLists.txt)
endif()

if(BUILD_MACOS_CONTAINER)
    add_custom_target(${MACOS_CONTAINER_TARGET} ALL DEPENDS "${MACOS_CONTAINER_OUTPUT_LIB}")
    include(containers/macos/CMakeLists.txt)
endif()





# Bindings
# -------
include(bindings/CMakeLists.txt)


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
