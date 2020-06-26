message(STATUS "Configuring ${UNREAL_TARGET} target")

# Set Unreal project paths
set(UNREAL_ROOT "" CACHE PATH "Path to UE4 folder") 
set(UNREAL_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeUnreal)
set(UNREAL_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unreal/ShowtimeUnreal)
set(UNREAL_BIN_DIR ${UNREAL_BUILD_DIR}/external/bin)
set(UNREAL_LIB_DIR ${UNREAL_BUILD_DIR}/external/lib)
set(UNREAL_INCLUDE_DIR ${UNREAL_BUILD_DIR}/external/include)

# Path vars
set(UNREAL_TARGET_DEPENDS "")

# Program vars
set(RUNUAT_COMMAND ${UNREAL_ROOT}/Engine/Build/BatchFiles/RunUAT.sh)

# Plugin output dir
set(UNREAL_PLUGIN_OUT_PATH ${UNREAL_BUILD_DIR}/Binaries/packaged)

# Create destination directories
file(MAKE_DIRECTORY ${UNREAL_BUILD_DIR})
file(MAKE_DIRECTORY ${UNREAL_INCLUDE_DIR})
file(MAKE_DIRECTORY ${UNREAL_BIN_DIR})
file(MAKE_DIRECTORY ${UNREAL_LIB_DIR})
file(MAKE_DIRECTORY ${UNREAL_PLUGIN_OUT_PATH})

# Unreal target build package
add_custom_target(${UNREAL_TARGET} ALL DEPENDS ${UNREAL_BUILD_DIR}/Binaries/fake_file.dll)

# Unreal project template
add_custom_command(
    PRE_BUILD
    TARGET ${UNREAL_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${UNREAL_SRC_DIR} to ${UNREAL_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNREAL_SRC_DIR} ${UNREAL_BUILD_DIR}
)

# Include folders
add_custom_command(
    PRE_BUILD
    TARGET ${UNREAL_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${Showtime_INCLUDE_DIRS} to ${UNREAL_INCLUDE_DIR}/showtime"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Showtime_INCLUDE_DIRS} ${UNREAL_INCLUDE_DIR}/showtime
)
add_custom_command(
    PRE_BUILD
    TARGET ${UNREAL_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${Showtime_COMPILED_INCLUDE_DIRS} to ${UNREAL_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Showtime_COMPILED_INCLUDE_DIRS} ${UNREAL_INCLUDE_DIR}
)


# Dylib copying
if(MACOSX OR BUILD_MACOS_CONTAINER)
    file(MAKE_DIRECTORY ${UNREAL_BIN_DIR}/Mac/plugins)
    file(MAKE_DIRECTORY ${UNREAL_LIB_DIR}/Mac)

    # Copy libraries
    set(UNREAL_MAC_LIBS "libshowtime.dylib")
    foreach( lib ${UNREAL_MAC_LIBS})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${MACOS_LIB_DIR}/${lib} to ${UNREAL_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy ${MACOS_LIB_DIR}/${lib} ${UNREAL_LIB_DIR}/Mac/${lib}
            DEPENDS ${MAC_CONTAINER_TARGET}
        )
    endforeach()

    # Copy plugins
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MACOS_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Mac/plugins
        DEPENDS ${MAC_CONTAINER_TARGET}
    )
endif()

# Android lib copying
if(ANDROID OR BUILD_ANDROID_CONTAINER)
    file(MAKE_DIRECTORY ${UNREAL_BIN_DIR}/Android/plugins)
    file(MAKE_DIRECTORY ${UNREAL_LIB_DIR}/Android)

    # Copy libraries
    set(UNREAL_ANDROID_LIBS "libshowtime.so")
    foreach( lib ${UNREAL_ANDROID_LIBS})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${lib} to ${UNREAL_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_LIB_DIR}/${lib} ${UNREAL_LIB_DIR}/Android/${lib}
            DEPENDS ${ANDROID_CONTAINER_TARGET}
        )
    endforeach()

    # Copy plugins
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ANDROID_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Android/plugins
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )
endif()

# Windows lib copying
if(WIN32 OR BUILD_WINDOWS_CONTAINER)
    file(MAKE_DIRECTORY ${UNREAL_BIN_DIR}/Win64/plugins)
    file(MAKE_DIRECTORY ${UNREAL_LIB_DIR}/Win64)

    set(UNREAL_WIN_BINARIES
        "${CORE_TARGET}.dll"
        "${CLIENT_TARGET}.dll"
        "${SERVER_TARGET}.dll"
        "$<TARGET_FILE_NAME:czmq>"
        "$<TARGET_FILE_NAME:libzmq>"
    )
    set(UNREAL_WIN_LIBRARIES
        "${CORE_TARGET}.lib"
        "${CLIENT_TARGET}.lib"
        "${SERVER_TARGET}.lib"
    )

    # Copy libraries
    foreach( bin ${UNREAL_WIN_BINARIES})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${lib} to ${UNREAL_BIN_DIR}/Win64"
            COMMAND ${CMAKE_COMMAND} -E copy ${WINDOWS_BIN_DIR}/${bin} ${UNREAL_BIN_DIR}/Win64/${bin}
            DEPENDS ${WINDOWS_CONTAINER_TARGET}
        )
    endforeach()
    foreach( lib ${UNREAL_WIN_LIBRARIES})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${bin} to ${UNREAL_BIN_DIR}/Win64/lib"
            COMMAND ${CMAKE_COMMAND} -E copy ${WINDOWS_LIB_DIR}/${lib} ${UNREAL_LIB_DIR}/Win64/${lib}
            DEPENDS ${WINDOWS_CONTAINER_TARGET}
        )
    endforeach()

    # Copy plugins
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ANDROID_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Win64/plugins
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )
endif()

# Build the plugin
add_custom_command(
    OUTPUT ${UNREAL_BUILD_DIR}/Binaries/fake_file.dll
    COMMAND ${RUNUAT_COMMAND} BuildPlugin -Plugin="${UNREAL_BUILD_DIR}/ShowtimeUnreal.uplugin" -Package="${UNREAL_PLUGIN_OUT_PATH}" -Rocket
)
# 

add_dependencies(${UNREAL_TARGET} ${SHARED_FROM_STATIC_TARGET})
#install(FILES ${Unreal_PACKAGE_PATH} DESTINATION Unreal)
