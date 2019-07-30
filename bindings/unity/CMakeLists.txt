message(STATUS "Configuring ${UNITY_TARGET} target")

# Set Unity project paths
set(UNITY_EXECUTABLE $ENV{PROGRAMFILES}/Unity/Editor/Unity.exe CACHE FILEPATH "Path to Unity executable")
set(UNITY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeExampleProject)
set(UNITY_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unity/ShowtimeExampleProject)
set(UNITY_BIN_DIR ${UNITY_BUILD_DIR}/Assets/${UNITY_TARGET}/Plugins)

# Path vars
set(UNITY_PACKAGE "${UNITY_TARGET}-${CMAKE_PROJECT_VERSION}.unitypackage")
set(UNITY_PACKAGE_PATH "${UNITY_BUILD_DIR}/${UNITY_PACKAGE}")
set(UNITY_TARGET_DEPENDS "")
set(UNITY_MANAGED_ASSEMBLIES "")

# Copy managed assemblies
list(APPEND UNITY_MANAGED_ASSEMBLIES
    ${DOTNET_OUTPUT_DLL}
)

# Create destination directories
file(MAKE_DIRECTORY ${UNITY_BUILD_DIR})
file(MAKE_DIRECTORY ${UNITY_BIN_DIR})


# Unity target build package
add_custom_target(${UNITY_TARGET} ALL DEPENDS ${UNITY_PACKAGE_PATH})

# Unity target copy/configure assets
add_custom_command(
    PRE_BUILD
    TARGET ${UNITY_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying project to ${UNITY_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNITY_SRC_DIR} ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying libraries to ${UNITY_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${UNITY_MANAGED_ASSEMBLIES}"
    COMMAND ${CMAKE_COMMAND} -E copy ${UNITY_MANAGED_ASSEMBLIES} ${UNITY_BIN_DIR}
    COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -noUpm -quit -projectPath ${UNITY_BUILD_DIR} -executeMethod Showtime.PluginPlatforms.RestorePluginDefaults
)

add_custom_command(
    OUTPUT ${UNITY_PACKAGE_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Exporting Unity package"
    COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -noUpm -quit -projectPath ${UNITY_BUILD_DIR} -exportPackage Assets/${UNITY_TARGET} ${UNITY_PACKAGE_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "...done"
)

# Bundle copying
if(MACOSX OR BUILD_MACOS_CONTAINER)
    set(UNITY_BUNDLE_PATH "${UNITY_BIN_DIR}/Darwin/$<TARGET_FILE_NAME:${DOTNET_TARGET_WRAPPER}>.bundle")
    add_custom_command(
        PRE_BUILD
        TARGET  ${UNITY_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying bundle to ${UNITY_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_BUNDLE_DIR:${DOTNET_TARGET_WRAPPER}> ${UNITY_BUNDLE_PATH}
        DEPENDS ${MACOS_CONTAINER_TARGET}
    )
endif()

# Android lib copying
if(ANDROID OR BUILD_ANDROID_CONTAINER)
    set(UNITY_ANDROID_PLUGIN_DIR ${UNITY_BIN_DIR}/Android)
    add_custom_command(
        PRE_BUILD
        TARGET ${UNITY_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying android files to ${UNITY_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_LIB_DIR}/libShowtimeDotnetWrapper.so ${UNITY_ANDROID_PLUGIN_DIR}/libShowtimeDotnetWrapper.so
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )
endif()

# Android lib copying
if(WIN32 OR BUILD_WINDOWS_CONTAINER)
    set(UNITY_WINDOWS_PLUGIN_DIR ${UNITY_BIN_DIR}/Windows)
    add_custom_command(
        PRE_BUILD
        TARGET ${UNITY_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying windows files to ${UNITY_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy ${WINDOWS_LIB_DIR}/ShowtimeDotnetWrapper.dll ${UNITY_WINDOWS_PLUGIN_DIR}/ShowtimeDotnetWrapper.dll
        DEPENDS ${WINDOWS_CONTAINER_TARGET}
    )
endif()

add_dependencies(${UNITY_TARGET} ${DOTNET_TARGET})

#install(FILES ${UNITY_PACKAGE_PATH} DESTINATION unity)
