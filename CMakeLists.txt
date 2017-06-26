cmake_minimum_required(VERSION 3.0)

project(Showtime)

set(SHOWTIME_VERSION 0.3.2)

# Set build locations
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

# Use C11
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED)

# Set showtime include directories
set(Showtime_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include)

# Packages
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(MsgPack REQUIRED)
find_package(SWIG)
include(UseSWIG)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_test(NAME section_test COMMAND SectionTest)
