cmake_minimum_required(VERSION 3.0)

# Install Hunter dependency tool
include("modules/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.18.30.tar.gz"
    SHA1 "6fc9258f984aed303eb960a95e36e19f2e7e83be"
)

project(Showtime)

# Include hunter dependencies
hunter_add_package(ZeroMQ)
find_package(ZeroMQ CONFIG REQUIRED)

# Set build locations
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add source dir
add_subdirectory(src)