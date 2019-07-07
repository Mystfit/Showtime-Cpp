message(STATUS "Configuring ${ANDROID_SINGULARITY_TARGET} target")

# Folder and file vars
set(ANDROID_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/android-container")
set(ANDROID_STAMP ${ANDROID_BUILD_DIR}/android.stamp)

# Copy vagrantfile to build dir so we don't pollute the source tree with Vagrant images
configure_file(${CMAKE_CURRENT_LIST_DIR}/Vagrantfile.in ${ANDROID_BUILD_DIR}/Vagrantfile)
configure_file(${CMAKE_CURRENT_LIST_DIR}/build-android.def.in ${ANDROID_BUILD_DIR}/build-android.def)

add_custom_command(
    OUTPUT ${ANDROID_OUTPUT_LIB}
    COMMAND ${CMAKE_COMMAND} -E echo "Building Showtime for Android using Linux container"
    COMMAND ${VAGRANT} up
    COMMAND ${VAGRANT} halt
    WORKING_DIRECTORY ${ANDROID_BUILD_DIR}
    VERBATIM
)
