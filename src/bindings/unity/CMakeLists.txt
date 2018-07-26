message(STATUS "Building ${UNITY_TARGET} target")

# Set Unity project paths
set(UNITY_EXECUTABLE $ENV{PROGRAMFILES}/Unity/Editor/Unity.exe CACHE FILEPATH "Path to Unity executable")
set(UNITY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeExampleProject)
set(UNITY_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unity/ShowtimeExampleProject)
set(UNITY_BIN_DIR ${UNITY_BUILD_DIR}/Assets/${UNITY_TARGET}/Plugins/x86_64)
set(UNITY_BINARY_FILES 
    $<TARGET_FILE:${CORE_TARGET}> 
    $<TARGET_FILE:${CLIENT_TARGET}> 
    $<TARGET_FILE:${SERVER_TARGET}> 
    $<TARGET_FILE:${DOTNET_TARGET}>
    $<TARGET_FILE:${DOTNET_TARGET_WRAPPER}>
    ${RUNTIMES_CZMQ}
)

add_custom_command(
    OUTPUT ${BUILD_STAMP} 
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying project to ${UNITY_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNITY_SRC_DIR} ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying libraries to ${UNITY_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy ${UNITY_BINARY_FILES} ${UNITY_BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Exporting Unity package"
    COMMAND ${UNITY_EXECUTABLE} -batchmode -quit -projectPath ${UNITY_BUILD_DIR} -exportPackage Assets/${UNITY_TARGET} ${UNITY_TARGET}-${SHOWTIME_VERSION}.unitypackage
    DEPENDS ${DOTNET_TARGET} ${SERVER_TARGET}
)
add_custom_target(${UNITY_TARGET} ALL DEPENDS ${BUILD_STAMP})
