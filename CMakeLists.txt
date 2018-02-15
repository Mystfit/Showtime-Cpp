cmake_minimum_required(VERSION 3.8.4)

project(Showtime)

set(SHOWTIME_VERSION 0.9.5)

# Set build locations
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
    MESSAGE(STATUS "Platform is OSX")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    MESSAGE(STATUS "Platform is Windows")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    option(WINDOWS_USE_VCPKG "Look for sources using VCPKG" false)
    set(WINDOWS_VCPKG_ROOT "C:/Code/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# Use C11
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED)

# Set showtime include directories
set(Showtime_COMPILED_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include")
set(Showtime_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
set(Showtime_DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/src/dependencies")
set(Showtime_API_INTERFACE "")


# Core Library
# ------------
set(CORE_TARGET "${PROJECT_NAME}Core")
message(STATUS "Building ${CORE_TARGET} target")
add_library(${CORE_TARGET} SHARED "")
include(src/core/CMakeLists.txt)


# Client Library
# --------------
set(CLIENT_TARGET "${PROJECT_NAME}Client")
message(STATUS "Building ${CLIENT_TARGET} target")
add_library(${CLIENT_TARGET} SHARED "")
include(src/client/CMakeLists.txt)
get_target_property(CLIENT_INCLUDES ${CLIENT_TARGET} INCLUDE_DIRECTORIES)
message(STATUS ${Showtime_INTERFACE})


# Server binary
# -------------
set(SERVER_TARGET "${PROJECT_NAME}Server")
message(STATUS "Building ${SERVER_TARGET} target")
add_executable (${SERVER_TARGET} "") 
include(src/server/CMakeLists.txt)


# Tests
# -----
enable_testing()
add_subdirectory(tests)
add_subdirectory(src/bindings)
