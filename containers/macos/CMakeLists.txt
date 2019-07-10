message(STATUS "Configuring ${MACOS_CONTAINER_TARGET} target")

# Set file and folder vars
set(MACOS_CONTAINER_NAME "macos-builder")
set(MACOS_CONTAINER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/macos-container")
set(MACOS_CONTAINER_INTERNAL_BUILD_DIR "/var/tmp/vagrant-showtime-build")

# Create native paths
file(TO_NATIVE_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/macOS NATIVE_BINARY_PATH)
file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/macOS NATIVE_LIBRARY_PATH)

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/macOS)
file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/macOS)

# Copy vagrantfile to build dir so we don't pollute the source tree with Vagrant images
configure_file(${CMAKE_CURRENT_LIST_DIR}/Vagrantfile.in ${MACOS_CONTAINER_BUILD_DIR}/Vagrantfile)

add_custom_command(
    OUTPUT ${MACOS_CONTAINER_OUTPUT_LIB}
    COMMAND ${CMAKE_COMMAND} -E echo "Building Showtime using MacOS container"
    COMMAND ${VAGRANTCOMMAND} up
    COMMAND ${CMAKE_COMMAND} -E echo "Copying files from container"
    COMMAND ${VAGRANTCOMMAND} scp ${MACOS_CONTAINER_NAME}:${MACOS_CONTAINER_INTERNAL_BUILD_DIR}/bin/* ${NATIVE_BINARY_PATH}
    COMMAND ${VAGRANTCOMMAND} scp ${MACOS_CONTAINER_NAME}:${MACOS_CONTAINER_INTERNAL_BUILD_DIR}/lib/* ${NATIVE_LIBRARY_PATH}
    COMMAND ${VAGRANTCOMMAND} halt
    WORKING_DIRECTORY ${MACOS_CONTAINER_BUILD_DIR}
    VERBATIM
)
