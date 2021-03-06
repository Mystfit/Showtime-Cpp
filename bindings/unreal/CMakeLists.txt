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
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${Showtime_INCLUDE_DIRS} to ${UNREAL_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Showtime_INCLUDE_DIRS} ${UNREAL_INCLUDE_DIR}
)

add_custom_command(
    PRE_BUILD
    TARGET ${UNREAL_TARGET}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${Showtime_COMPILED_INCLUDE_DIRS} to ${UNREAL_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Showtime_COMPILED_INCLUDE_DIRS} ${UNREAL_INCLUDE_DIR}
)

set(UNREAL_DEPENDENCIES "")


# Bundle copying
if(MACOSX OR BUILD_MACOS_CONTAINER)
    # Copy mac libs
    # Do stuff here

    if(BUILD_MACOS_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${MACOS_CONTAINER_TARGET})
    endif()
endif()

# Android lib copying
if(ANDROID OR BUILD_ANDROID_CONTAINER)
    set(UNREAL_ANDROID_LIBS
        "libshowtime.so"
        # "lib${CORE_TARGET}.so"
        # "lib${CLIENT_TARGET}.so"
        # "lib${SERVER_TARGET}.so"
        # "libzmq.so"
        # "libczmq.so"
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
    if(BUILD_ANDROID_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${ANDROID_CONTAINER_TARGET})
    endif()
endif()

# Windows lib copying
if(WIN32 OR BUILD_WINDOWS_CONTAINER)
    set(UNREAL_WIN_BINARIES
        "$<TARGET_FILE:${SHARED_FROM_STATIC_TARGET}>"
        # "$<TARGET_FILE:${CLIENT_TARGET}>"
        # "$<TARGET_FILE:${SERVER_TARGET}>"
        # "$<TARGET_FILE:czmq>"
        # "$<TARGET_FILE:libzmq>"
    )
    set(UNREAL_WIN_LIBRARIES
        "$<TARGET_LINKER_FILE:${SHARED_FROM_STATIC_TARGET}>"
        # "$<TARGET_LINKER_FILE:${CLIENT_TARGET}>"
        # "$<TARGET_LINKER_FILE:${SERVER_TARGET}>"
    )
    set(UNREAL_WIN_PDBS
        "$<TARGET_PDB_FILE:${SHARED_FROM_STATIC_TARGET}>"
        # "$<TARGET_PDB_FILE:${CLIENT_TARGET}>"
        # "$<TARGET_PDB_FILE:${SERVER_TARGET}>"
    )


    # Copy libraries
    foreach( bin ${UNREAL_WIN_BINARIES})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${bin} to ${UNREAL_BIN_DIR}/Win64"
            COMMAND ${CMAKE_COMMAND} -E copy ${bin} ${UNREAL_BIN_DIR}/Win64/
            DEPENDS ${WINDOWS_CONTAINER_TARGET}
        )
    endforeach()
    foreach( pdb ${UNREAL_WIN_PDBS})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${pdb} to ${UNREAL_BIN_DIR}/Win64"
            COMMAND ${CMAKE_COMMAND} -E copy ${pdb} ${UNREAL_BIN_DIR}/Win64/
            DEPENDS ${WINDOWS_CONTAINER_TARGET}
        )
    endforeach()
    foreach( lib ${UNREAL_WIN_LIBRARIES})
        add_custom_command(
            PRE_BUILD
            TARGET ${UNREAL_TARGET}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${lib} to ${UNREAL_BIN_DIR}/Win64/lib"
            COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${UNREAL_LIB_DIR}/Win64/
            DEPENDS ${WINDOWS_CONTAINER_TARGET}
        )
    endforeach()

    # Copy plugins
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${WINDOWS_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Win64/plugins
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )

    if(BUILD_WINDOWS_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${WINDOWS_CONTAINER_TARGET})
    endif()
endif()

if(BUILD_SHARED)
    add_dependencies(${UNREAL_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET} ${UNREAL_DEPENDENCIES})
else()
    add_dependencies(${UNREAL_TARGET} ${CLIENT_TARGET_STATIC} ${SERVER_TARGET_STATIC} ${UNREAL_DEPENDENCIES})
endif()
#install(FILES ${Unreal_PACKAGE_PATH} DESTINATION Unreal)
