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
file(MAKE_DIRECTORY ${UNREAL_BIN_DIR})
file(MAKE_DIRECTORY ${UNREAL_LIB_DIR})

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
    COMMAND ${CMAKE_COMMAND} -E echo "Copying ${Showtime_ALL_INCLUDE_DIRS} to ${UNREAL_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Showtime_ALL_INCLUDE_DIRS} ${UNREAL_INCLUDE_DIR}
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
    #set(UNREAL_ANDROID_LIBS
        #"libShowtime.so"
        # "lib${CORE_TARGET}.so"
        # "lib${CLIENT_TARGET}.so"
        # "lib${SERVER_TARGET}.so"
        # "libzmq.so"
        # "libczmq.so"
    #)

    # Copy libraries
    #foreach( lib ${UNREAL_ANDROID_LIBS})
        # add_custom_command(
        #     PRE_BUILD
        #     TARGET ${UNREAL_TARGET}
        #     COMMAND ${CMAKE_COMMAND} -E echo "Copying ${lib} to ${UNREAL_LIB_DIR}"
        #     COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_LIB_DIR}/${lib} ${UNREAL_LIB_DIR}/Android/${lib}
        #     DEPENDS ${ANDROID_CONTAINER_TARGET}
        # )
    #endforeach()
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${UNREAL_LIB_DIR}/Android/${ANDROID_ABI}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying ${ANDROID_LIB_DIR} to ${UNREAL_LIB_DIR}/Android/${ANDROID_ABI}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ANDROID_LIB_DIR} ${UNREAL_LIB_DIR}/Android/${ANDROID_ABI}
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )

    # Copy plugins
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${UNREAL_BIN_DIR}/Android/${ANDROID_ABI}/plugins
        COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ANDROID_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Android/${ANDROID_ABI}/plugins
        DEPENDS ${ANDROID_CONTAINER_TARGET}
    )
    if(BUILD_ANDROID_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${ANDROID_CONTAINER_TARGET})
    endif()
endif()

# Windows lib copying
if(WIN32 OR BUILD_WINDOWS_CONTAINER)
    # Copy windows platform files
    add_custom_command(
        PRE_BUILD
        TARGET ${UNREAL_TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${UNREAL_BIN_DIR}/Win64
        COMMAND ${CMAKE_COMMAND} -E make_directory ${UNREAL_BIN_DIR}/Win64/plugins
        COMMAND ${CMAKE_COMMAND} -E make_directory ${UNREAL_LIB_DIR}/Win64
        COMMAND ${CMAKE_COMMAND} -E echo "Copying Windows platform files to ${UNREAL_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${SHARED_FROM_STATIC_TARGET}> ${UNREAL_BIN_DIR}/Win64/$<TARGET_FILE_NAME:${SHARED_FROM_STATIC_TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_LINKER_FILE:${SHARED_FROM_STATIC_TARGET}> ${UNREAL_LIB_DIR}/Win64/$<TARGET_LINKER_FILE_NAME:${SHARED_FROM_STATIC_TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${SHARED_FROM_STATIC_TARGET}> ${UNREAL_BIN_DIR}/Win64/$<TARGET_PDB_FILE_NAME:${SHARED_FROM_STATIC_TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${WINDOWS_BIN_DIR}/plugins ${UNREAL_BIN_DIR}/Win64/plugins
        DEPENDS ${WINDOWS_CONTAINER_TARGET} ${ANDROID_CONTAINER_TARGET} ${SHARED_FROM_STATIC_TARGET}
    )

    if(BUILD_WINDOWS_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${WINDOWS_CONTAINER_TARGET})
    endif()
    if(BUILD_ANDROID_CONTAINER)
        list(APPEND UNREAL_DEPENDENCIES ${ANDROID_CONTAINER_TARGET})
    endif()
endif()

if(BUILD_SHARED)
    add_dependencies(${UNREAL_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET} ${UNREAL_DEPENDENCIES})
else()
    add_dependencies(${UNREAL_TARGET} ${CLIENT_TARGET_STATIC} ${SERVER_TARGET_STATIC} ${UNREAL_DEPENDENCIES})
endif()
#install(FILES ${Unreal_PACKAGE_PATH} DESTINATION Unreal)
