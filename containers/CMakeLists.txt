message(STATUS "Configuring ${ANDROID_SINGULARITY_TARGET} target")

# We use Vagrant to create/run the Singularity image
find_program(VAGRANT vagrant)
if(VAGRANT EQUAL vagrant-NOTFOUND)
    message(ERROR "Could not find Vagrant")
endif()

# Create folder for container build system to live in - avoids long rebuilds
file(MAKE_DIRECTORY ${ANDROID_BUILD_DIR}/container-cmake-build)

# Copy vagrantfile to build dir so we don't pollute the source tree with Vagrant images
configure_file(${CMAKE_CURRENT_LIST_DIR}/Vagrantfile ${ANDROID_BUILD_DIR}/Vagrantfile COPYONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/build-android.def ${ANDROID_BUILD_DIR}/build-android.def COPYONLY)

add_custom_command(
    OUTPUT ${ANDROID_OUTPUT_LIB}
    COMMAND ${CMAKE_COMMAND} -E echo "Building Showtime for Android"
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/build_vagrant_android.sh
    WORKING_DIRECTORY ${ANDROID_BUILD_DIR}
    VERBATIM
)

add_dependencies(${ANDROID_SINGULARITY_TARGET} 
    ${CLIENT_TARGET}
    ${SERVER_TARGET}
)
