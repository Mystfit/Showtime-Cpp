cmake_minimum_required(VERSION 3.8.4)

if(NOT DEFINED USE_HUNTER)
    if(WIN32)
        set(USE_HUNTER ON)
    else()
        set(USE_HUNTER OFF)
    endif()
endif()

if(USE_HUNTER)
    include("cmake/HunterGate.cmake")
    HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.20.9.tar.gz"
        SHA1 "8f7cf6d719d05c49224a4ebeba90c6bb90b90f4a"
        LOCAL
    )
    set(HUNTER_ENABLED ON)
endif()

project(Showtime)
set(SHOWTIME_VERSION_MAJOR 0)
set(SHOWTIME_VERSION_MINOR 9)
set(SHOWTIME_VERSION_PATCH 9)
set(SHOWTIME_VERSION "${SHOWTIME_VERSION_MAJOR}.${SHOWTIME_VERSION_MINOR}.${SHOWTIME_VERSION_PATCH}")
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

# Set boost common parameters
# On windows, compile with the following (stage/install):
#bjam.exe threading=multi architecture=x86 address-model=64 link=static runtime-link=static --with-system --with-filesystem
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_STATIC_LIBS OFF)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    MESSAGE(STATUS "Platform is Windows")
    set(Boost_USE_STATIC_LIBS ON)
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
set(Showtime_RUNTIME_DEPENDENCIES "")

if(NOT DEFINED USE_LOCAL_BOOST)
    set(USE_LOCAL_BOOST OFF)
endif()

# Packages
# --------
set(LIBZMQ_TARGET "")
set(CZMQ_TARGET "")
if(USE_HUNTER AND HUNTER_ENABLED)
    if(NOT USE_LOCAL_BOOST)
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
    # hunter_add_package(MsgPack) # Use updated packaged at https://github.com/mystfit/msgpack-c.git
    hunter_add_package(fmt)
    # hunter_add_package(ZeroMQ)  # Only include ZeroMQ until CZMQ gets CZMQConfig.cmake added to Hunter
    # hunter_add_package(czmq)
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
find_package(MsgPack CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SWIG)
if(SWIG_FOUND)
    include(UseSWIG)
endif()
set(fmt_TARGET "fmt::fmt")
set(msgpackc_TARGET "msgpackc-static") # set(msgpackc_LIBRARY "msgpack::msgpackc-static")

if(WIN32 AND USE_HUNTER AND HUNTER_ENABLED)
    find_package(ZeroMQ CONFIG REQUIRED) # Only include ZeroMQ until CZMQ gets CZMQConfig.cmake added to Hunter - windows only
    find_package(czmq CONFIG REQUIRED)
    if(NOT USE_LOCAL_BOOST)
        set(USE_LOCAL_BOOST OFF)
        set(Boost_LOG_TARGET "Boost::log")
        set(Boost_LOG_SETUP_TARGET "Boost::log_setup")
        set(Boost_THREAD_TARGET "Boost::thread")
        set(Boost_FILESYSTEM_TARGET "Boost::filesystem")
        set(Boost_SYSTEM_TARGET "Boost::system")
        set(Boost_DATE_TIME_TARGET "Boost::date_time")
        set(Boost_CHRONO_TARGET "Boost::chrono")
        set(Boost_ATOMIC_TARGET "Boost::atomic")
    endif()
    
    set(CZMQ_TARGET "czmq::czmq")
else()
    find_package(czmq REQUIRED)
    set(USE_LOCAL_BOOST ON)
    set(CZMQ_TARGET ${CZMQ_LIBRARIES})
    # set(ZEROMQ_TARGET ${ZeroMQ_LIBRARIES})
endif()

if(USE_LOCAL_BOOST)
    set(Boost_LOG_TARGET ${Boost_LOG_LIBRARY})
    set(Boost_LOG_SETUP_TARGET ${Boost_LOG_LIBRARY})
    set(Boost_THREAD_TARGET ${Boost_THREAD_LIBRARY})
    set(Boost_FILESYSTEM_TARGET ${Boost_FILESYSTEM_LIBRARY})
    set(Boost_SYSTEM_TARGET ${Boost_SYSTEM_LIBRARY})
    set(Boost_DATE_TIME_TARGET ${Boost_DATE_TIME_LIBRARY})
    set(Boost_CHRONO_TARGET ${Boost_CHRONO_LIBRARY})
    set(Boost_ATOMIC_TARGET ${Boost_ATOMIC_LIBRARY})
endif()


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
option(BINDINGS_UNITY "Generate bindings for Unity" OFF)

if(BINDINGS_DOTNET OR BINDINGS_UNITY)
    set(BINDINGS_DOTNET ON)
    set(DOTNET_GENERATED_BINDINGS)
    set(DOTNET_TARGET ${PROJECT_NAME}Dotnet)
    include(src/bindings/dotnet/CMakeLists.txt)
endif()

if(BINDINGS_UNITY)
    set(UNITY_TARGET ${PROJECT_NAME}Unity)
    include(src/bindings/unity/CMakeLists.txt)
endif()

if(BINDINGS_PYTHON)
    set(PYTHON_TARGET ${PROJECT_NAME}Python)
    set(PYTHON_VERSION 3.6 CACHE STRING "Python version to search for")
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
install(FILES ${Showtime_API_INTERFACE} DESTINATION include)

# Install targets
set(Showtime_INSTALL_TARGETS 
    ${Showtime_LIBRARY_TARGETS} 
    ${Showtime_APP_TARGETS}
)

install(TARGETS ${Showtime_INSTALL_TARGETS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    EXPORT showtime-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Export targets
if(NOT CMAKE_VERSION VERSION_LESS 3.0)
    export(EXPORT showtime-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/showtime-targets.cmake"
        NAMESPACE ${SHOWTIME_NAMESPACE}
    )
endif()

# Copy targets
install(EXPORT showtime-targets
    FILE showtime-targets.cmake
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
)

# Set up CMake package
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    showtime-config-version.cmake
    VERSION ${SHOWTIME_VERSION}
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

