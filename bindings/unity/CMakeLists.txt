message(STATUS "Configuring ${UNITY_TARGET} target")

# Set Unity project paths
set(UNITY_EXECUTABLE $ENV{PROGRAMFILES}/Unity/Editor/Unity.exe CACHE FILEPATH "Path to Unity executable")
set(UNITY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeExampleProject)
set(UNITY_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unity/ShowtimeExampleProject)

if(MACOSX)
    set(UNITY_BIN_DIR ${UNITY_BUILD_DIR}/Assets/${UNITY_TARGET}/Plugins)
else()
    set(UNITY_BIN_DIR ${UNITY_BUILD_DIR}/Assets/${UNITY_TARGET}/Plugins/x86_64)
endif()

# Path vars
set(UNITY_PACKAGE "${UNITY_TARGET}-${CMAKE_PROJECT_VERSION}.unitypackage")
set(UNITY_PACKAGE_PATH "${UNITY_BUILD_DIR}/${UNITY_PACKAGE}")
set(UNITY_TARGET_DEPENDS "")
set(UNITY_BINARY_FILES "")

if(WIN32)
    # Copy dotnet target outputs on windows
    list(APPEND UNITY_BINARY_FILES
        $<TARGET_FILE:${CORE_TARGET}>
        # $<TARGET_FILE:${CORE_ENTITIES_TARGET}>
        $<TARGET_FILE:${CLIENT_TARGET}>
        $<TARGET_FILE:${SERVER_TARGET}>
        $<TARGET_FILE:${DOTNET_TARGET}>
        $<TARGET_FILE:${DOTNET_TARGET_WRAPPER}>
    )
else()
    # Copy dll files on unix
    list(APPEND UNITY_BINARY_FILES
        ${DOTNET_OUTPUT_FILE}
    )
endif()

# Bundle copying
if(MACOSX)
    set(UNITY_BUNDLE_PATH "${UNITY_BIN_DIR}/OSX/$<TARGET_FILE_NAME:${DOTNET_TARGET_WRAPPER}>.bundle")
    set(UNITY_BUNDLE_STAMP ${CMAKE_CURRENT_BINARY_DIR}/unity-copy-bundle.stamp)

    add_custom_command(
        OUTPUT  ${UNITY_BUNDLE_STAMP}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying bundle to ${UNITY_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_BUNDLE_DIR:${DOTNET_TARGET_WRAPPER}> ${UNITY_BUNDLE_PATH}
        COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_BUNDLE_STAMP}
    )
    add_custom_target(UnityCopyBundle ALL DEPENDS ${UNITY_BUNDLE_STAMP})
    add_dependencies(UnityCopyBundle ${DOTNET_TARGET_WRAPPER})
    list(APPEND UNITY_TARGET_DEPENDS UnityCopyBundle)
endif()

# Android lib copying
if(BUILD_ANDROID_CONTAINER)
    set(UNITY_ANDROID_PLUGIN_DIR ${UNITY_BIN_DIR}/Android)
    set(UNITY_COPY_ANDROID_STAMP ${CMAKE_CURRENT_BINARY_DIR}/unity-copy-android.stamp)

    add_custom_command(
        OUTPUT ${UNITY_COPY_ANDROID_STAMP}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying android files to ${UNITY_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Android ${UNITY_ANDROID_PLUGIN_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_COPY_ANDROID_STAMP}
    )
    add_custom_target(UnityCopyAndroidLibs ALL DEPENDS ${UNITY_COPY_ANDROID_STAMP})
    add_dependencies(UnityCopyAndroidLibs ${DOTNET_TARGET_WRAPPER} ${ANDROID_CONTAINER_TARGET})
    list(APPEND UNITY_TARGET_DEPENDS UnityCopyAndroidLibs)
endif()

# Android lib copying
if(BUILD_WINDOWS_CONTAINER)
    set(UNITY_WINDOWS_PLUGIN_DIR ${UNITY_BIN_DIR}/windows)
    set(UNITY_COPY_WINDOWS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/unity-copy-windows.stamp)

    add_custom_command(
        OUTPUT ${UNITY_COPY_WINDOWS_STAMP}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying windows files to ${UNITY_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/windows ${UNITY_WINDOWS_PLUGIN_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_COPY_WINDOWS_STAMP}
    )
    add_custom_target(UnityCopyWindowsLibs ALL DEPENDS ${UNITY_COPY_WINDOWS_STAMP})
    add_dependencies(UnityCopyWindowsLibs ${DOTNET_TARGET_WRAPPER} ${WINDOWS_CONTAINER_TARGET})
    list(APPEND UNITY_TARGET_DEPENDS UnityCopyWindowsLibs)
endif()

# Unity target copy/configure assets
set(UNITY_SETUP_PROJECT_STAMP ${CMAKE_CURRENT_BINARY_DIR}/unity-setup-project.stamp)
add_custom_command(
    OUTPUT ${UNITY_SETUP_PROJECT_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying project to ${UNITY_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNITY_SRC_DIR} ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying libraries to ${UNITY_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${UNITY_BINARY_FILES}"
    COMMAND ${CMAKE_COMMAND} -E copy ${UNITY_BINARY_FILES} ${UNITY_BIN_DIR}
    COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -noUpm -quit -projectPath ${UNITY_BUILD_DIR} -executeMethod Showtime.PluginPlatforms.RestorePluginDefaults
    COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_SETUP_PROJECT_STAMP}
)
add_custom_target(UnitySetupProject ALL DEPENDS ${UNITY_SETUP_PROJECT_STAMP})
add_dependencies(UnitySetupProject ${UNITY_TARGET_DEPENDS})
list(APPEND UNITY_TARGET_DEPENDS UnitySetupProject)


# Unity target build package
add_custom_command(
    OUTPUT ${UNITY_PACKAGE_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Exporting Unity package"
    COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -noUpm -quit -projectPath ${UNITY_BUILD_DIR} -exportPackage Assets/${UNITY_TARGET} ${UNITY_PACKAGE_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "...done"
)
add_custom_target(${UNITY_TARGET} ALL DEPENDS ${UNITY_PACKAGE_PATH})
add_dependencies(${UNITY_TARGET} ${DOTNET_TARGET} ${UNITY_TARGET_DEPENDS})

#install(FILES ${UNITY_PACKAGE_PATH} DESTINATION unity)
