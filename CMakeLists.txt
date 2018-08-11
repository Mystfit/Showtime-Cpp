cmake_minimum_required(VERSION 3.12.0)

if(NOT DEFINED USE_HUNTER)
    if(WIN32)
        set(USE_HUNTER ON)
    else()
        set(USE_HUNTER OFF)
    endif()
endif()

set(CMAKE_BASE_MODULE_PATH ${CMAKE_MODULE_PATH})
if(USE_HUNTER)
    include("cmake/HunterGate.cmake")
    HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.3.tar.gz"
        SHA1 "2c421912930d183c24512590014f955ff073424b"
    )
    set(HUNTER_ENABLED ON)
endif()

project(Showtime VERSION 0.12.0)
set(SHOWTIME_NAMESPACE "${PROJECT_NAME}::")

# Use C14
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build locations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;${CMAKE_MODULE_PATH}")
set(CMAKE_SWIG_ROOT_OUTDIR ${CMAKE_BINARY_DIR}/bindings)
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
    MESSAGE(STATUS "Platform is OSX")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    MESSAGE(STATUS "Platform is Linux")
endif()

if(LINUX OR MACOSX)
    # set(CMAKE_INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "Library Install Name Destination Directory" FORCE) 
    # option(BUILD_RELOCATABLE_LIBRARY "Build relocatatable library" OFF)
    # set(CMAKE_BUILD_RPATH "@loader_path")
    # set(CMAKE_MACOSX_RPATH "@loader_path")
    # set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "Library Install RPath") 
    # set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF CACHE BOOL "Do Build Using Library Install RPath")
    if(NOT DEFINED CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH} "/usr/local")
        if(LINUX)
            list(APPEND CMAKE_PREFIX_PATH "/home/linuxbrew/.linuxbrew")
        endif()
    endif() 
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    MESSAGE(STATUS "Platform is Windows")
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

# Set showtime variables
set(Showtime_COMPILED_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include")
set(Showtime_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
set(Showtime_ALL_INCLUDE_DIRS ${Showtime_COMPILED_INCLUDE_DIRS} ${Showtime_INCLUDE_DIRS})
set(Showtime_DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/src/dependencies")
set(Showtime_SOURCE_DEPENDENCIES 
	"${Showtime_DEPENDENCIES_DIR}"
    "${Showtime_DEPENDENCIES_DIR}/cf"
    "${Showtime_DEPENDENCIES_DIR}/variant/include"
)

set(Showtime_API_INTERFACE "")
set(Showtime_LIBRARY_TARGETS "")
set(Showtime_APP_TARGETS "")
set(Showtime_INSTALL_TARGETS "")

# Boost properties
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_SYSTEM_PATHS ON)
option(Boost_USE_LOCAL "Use local boost" OFF)
if(WIN32)
    if(NOT DEFINED Boost_USE_STATIC_LIBS)
        set(Boost_USE_STATIC_LIBS ON)
    endif()
else()
    if(NOT DEFINED Boost_USE_STATIC_LIBS)
        set(Boost_USE_STATIC_LIBS OFF)
    endif()
endif()


# --------
# Packages
# --------
set(LIBZMQ_TARGET "")
set(CZMQ_TARGET "")
if(USE_HUNTER AND HUNTER_ENABLED)
    message(STATUS "Installing hunter dependencies")
    if(NOT Boost_USE_LOCAL)
        hunter_add_package(boost COMPONENTS 
            system 
            filesystem 
            log
            thread
            date_time
            chrono
            atomic
        )
    endif()

    # hunter_add_package(MsgPack) # Use updated package at https://github.com/mystfit/msgpack-c.git
    message(STATUS "Adding hunter packages")
    hunter_add_package(fmt)
    # hunter_add_package(ZeroMQ)  # Only include ZeroMQ until CZMQ gets CZMQConfig.cmake added to Hunter
    # hunter_add_package(czmq)
endif()

message(STATUS "Finding packages")

# Boost
# --------
if(Boost_USE_LOCAL)
    # Reset the CMake module path to avoid using Hunter's FindBoost.cmake
    set(HUNTER_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH ${CMAKE_BASE_MODULE_PATH})
    message(STATUS "Current module path: ${CMAKE_MODULE_PATH}")
endif()
find_package(Boost COMPONENTS
    log
    log_setup
    thread
    system
    filesystem 
    date_time
    chrono
    atomic
    REQUIRED
)
if(Boost_USE_LOCAL)
   set(CMAKE_MODULE_PATH ${HUNTER_CMAKE_MODULE_PATH}) 
endif()
set(Boost_HEADER_TARGET "Boost::boost")
set(Boost_LOG_TARGET "Boost::log")
set(Boost_LOG_SETUP_TARGET "Boost::log_setup")
set(Boost_THREAD_TARGET "Boost::thread")
set(Boost_FILESYSTEM_TARGET "Boost::filesystem")
set(Boost_SYSTEM_TARGET "Boost::system")
set(Boost_DATE_TIME_TARGET "Boost::date_time")
set(Boost_CHRONO_TARGET "Boost::chrono")
set(Boost_ATOMIC_TARGET "Boost::atomic")

# Msgpack, fmt, swig
# ------------------
find_package(MsgPack CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SWIG)
if(SWIG_FOUND)
    include(UseSWIG)
endif()
set(msgpackc_TARGET "msgpackc-static") # set(msgpackc_LIBRARY "msgpack::msgpackc-static")

if(WIN32 AND USE_HUNTER AND HUNTER_ENABLED)
    find_package(ZeroMQ CONFIG REQUIRED) # Only include ZeroMQ until CZMQ gets CZMQConfig.cmake added to Hunter - windows only
    find_package(czmq CONFIG REQUIRED)
    set(CZMQ_TARGET czmq-static)
    set(fmt_TARGET "fmt::fmt")
else()
    find_package(czmq REQUIRED)
    set(Boost_USE_LOCAL ON)
    set(CZMQ_TARGET czmq-static)
    set(fmt_TARGET "fmt::fmt-header-only")
    # set(ZEROMQ_TARGET ${ZeroMQ_LIBRARIES})
endif()


# ----------------
# Showtime targets
# ----------------
set(SHOWTIME_PUBLIC_LINKED_LIBS "")

# Core Library
# ------------
set(CORE_TARGET "${PROJECT_NAME}Core")
add_library(${CORE_TARGET} SHARED "")
include(src/core/CMakeLists.txt)
list(APPEND Showtime_LIBRARY_TARGETS ${CORE_TARGET})

# Core entity plugin
# --------------------
set(CORE_ENTITIES_TARGET "${PROJECT_NAME}CoreEntities")
add_library(${CORE_ENTITIES_TARGET} SHARED "")
include(src/core_entities/CMakeLists.txt)
list(APPEND Showtime_LIBRARY_TARGETS ${CORE_ENTITIES_TARGET})


# Client Library
# --------------
set(CLIENT_TARGET "${PROJECT_NAME}Client")
add_library(${CLIENT_TARGET} SHARED "")
include(src/client/CMakeLists.txt)
list(APPEND Showtime_LIBRARY_TARGETS ${CLIENT_TARGET})


# Server binary
# -------------
set(SERVER_TARGET "${PROJECT_NAME}Server")
add_executable (${SERVER_TARGET} "") 
include(src/server/CMakeLists.txt)
list(APPEND Showtime_APP_TARGETS ${SERVER_TARGET})


# Bindings
# -------
#include (${SWIG_USE_FILE})

# Dotnet bindings
option(BINDINGS_DOTNET "Generate bindings for C#" OFF)
option(BINDINGS_PYTHON "Generate bindings for Python" OFF)
option(BINDINGS_UNITY "Generate bindings for Unity" OFF)

if(BINDINGS_DOTNET OR BINDINGS_UNITY)
    set(BINDINGS_DOTNET_FRAMEWORK_VERSION "4.6.1" CACHE STRING "dotnet Framework version")
    set_property(CACHE BINDINGS_DOTNET_FRAMEWORK_VERSION PROPERTY STRINGS 3.5 4 4.5 4.5.1 4.5.2 4.6 4.6.1 4.7.1)
    set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION "v${BINDINGS_DOTNET_FRAMEWORK_VERSION}")
    set(BINDINGS_DOTNET ON)
    set(DOTNET_GENERATED_BINDINGS)
    set(DOTNET_TARGET ${PROJECT_NAME}Dotnet)
    if(WIN32)
        add_library(${DOTNET_TARGET} SHARED "")
    else()
        set(DOTNET_OUTPUT_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DOTNET_TARGET}.dll)
    endif()
    include(src/bindings/dotnet/CMakeLists.txt)
    if(WIN32)
        list(APPEND Showtime_LIBRARY_TARGETS ${DOTNET_TARGET})
    endif()

endif()

if(BINDINGS_UNITY)
    set(UNITY_TARGET ${PROJECT_NAME}Unity)
    include(src/bindings/unity/CMakeLists.txt)
endif()

if(BINDINGS_PYTHON)
    set(PYTHON_TARGET ${PROJECT_NAME}Python)
    set(PYTHON_VERSION 2.7 CACHE STRING "Python version to search for")
    find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
    include(src/bindings/python/CMakeLists.txt)
endif()


# Tests
# -----
enable_testing()
add_subdirectory(tests)


# Installation
# ------------
set(CMAKE_INSTALL_CMAKEDIR "lib/cmake/showtime")

# Install targets
set(Showtime_INSTALL_TARGETS 
    ${Showtime_LIBRARY_TARGETS} 
    ${Showtime_APP_TARGETS}
)

install(FILES ${Showtime_API_INTERFACE} DESTINATION include/showtime)

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

install (
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/showtime-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/showtime-config-version.cmake"
    DESTINATION
        "${CMAKE_INSTALL_CMAKEDIR}"
)




