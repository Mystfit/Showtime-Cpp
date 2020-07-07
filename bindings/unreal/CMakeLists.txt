message(STATUS "Configuring ${UNREAL_TARGET} target")

# Set Unreal project paths
set(UNREAL_EXECUTABLE $ENV{PROGRAMFILES}/Unreal/Editor/Unreal.exe CACHE FILEPATH "Path to Unreal executable")
set(UNREAL_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeUnreal)
set(UNREAL_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unreal/ShowtimeUnreal)
set(UNREAL_BIN_DIR ${UNREAL_BUILD_DIR}/external/bin)
set(UNREAL_LIB_DIR ${UNREAL_BUILD_DIR}/external/lib)
set(UNREAL_INCLUDE_DIR ${UNREAL_BUILD_DIR}/external/include)

# Path vars
set(UNREAL_TARGET_DEPENDS "")

# Create destination directories
file(MAKE_DIRECTORY ${UNREAL_BUILD_DIR})

# Unreal target build package
add_custom_target(${UNREAL_TARGET} ALL DEPENDS ${UNREAL_BUILD_DIR}/ShowtimeUnreal.uplugin)

# Unreal project template
add_custom_command(
    PRE_BUILD
    TARGET ${UNREAL_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${UNREAL_SRC_DIR} to ${UNREAL_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNREAL_SRC_DIR} ${UNREAL_BUILD_DIR}
)

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


# Bundle copying
if(MACOSX OR BUILD_MACOS_CONTAINER)
    # Copy mac libs
endif()

# Android lib copying
if(ANDROID OR BUILD_ANDROID_CONTAINER)
    set(UNREAL_ANDROID_LIBS
        "lib${CORE_TARGET}.so"
        "lib${CLIENT_TARGET}.so"
        "lib${SERVER_TARGET}.so"
    )

    # Copy libraries
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

add_dependencies(${UNREAL_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET})
#install(FILES ${Unreal_PACKAGE_PATH} DESTINATION Unreal)
