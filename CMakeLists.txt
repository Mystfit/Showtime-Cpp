cmake_minimum_required(VERSION 3.8.2)

project(Showtime)

set(SHOWTIME_VERSION 0.5.3)

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
endif()

# Use C11
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED)

if (MSVC_VERSION GREATER_EQUAL "1900")
    
    #include(CheckCXXCompilerFlag)
    #CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_17_flag_supported)
    #if (_cpp_17_flag_supported)
    #    add_compile_options("/std:c++17")
    #endif()
endif()

# Set showtime include directories
set(Showtime_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include )

# Packages
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(MsgPack REQUIRED)
find_package(SWIG)
include(UseSWIG)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
