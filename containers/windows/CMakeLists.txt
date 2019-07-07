message(STATUS "Configuring ${WINDOWS_CONTAINER_TARGET} target")

# Set file and folder vars
set(WINDOWS_CONTAINER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/windows-container")

# Create folder for container build system to live in - avoids long rebuilds
file(MAKE_DIRECTORY ${WINDOWS_CONTAINER_BUILD_DIR}/container-cmake-build)

# Install vagrant reload plugin
execute_process(COMMAND "${VAGRANT} list" RESULT_VARIABLE VAGRANT_PLUGIN_LIST)
string(FIND ${VAGRANT_PLUGIN_LIST} "reload" VAGRANT_RELOAD_PLUGIN_FOUND)
if(${VAGRANT_RELOAD_PLUGIN_FOUND} LESS 0)
	execute_process(COMMAND "${VAGRANT} plugin install vagrant-reload")
endif()

# Copy vagrantfile to build dir so we don't pollute the source tree with Vagrant images
configure_file(${CMAKE_CURRENT_LIST_DIR}/Vagrantfile.in ${WINDOWS_CONTAINER_BUILD_DIR}/Vagrantfile)

add_custom_command(
    OUTPUT ${WINDOWS_CONTAINER_OUTPUT_LIB}
    COMMAND ${CMAKE_COMMAND} -E echo "Building Showtime using Windows container"
    COMMAND ${VAGRANT} up
    COMMAND ${VAGRANT} halt
    WORKING_DIRECTORY ${WINDOWS_CONTAINER_BUILD_DIR}
    VERBATIM
)
