message(STATUS "Configuring ${UNITY_TARGET} target")

# Set Unity project paths
set(UNITY_EXECUTABLE $ENV{PROGRAMFILES}/Unity/Editor/Unity.exe CACHE FILEPATH "Path to Unity executable")
set(UNITY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ShowtimeExampleProject)
set(UNITY_BUILD_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/unity/ShowtimeExampleProject)
set(UNITY_BIN_DIR ${UNITY_BUILD_DIR}/Assets/${UNITY_TARGET}/Plugins/x86_64)

set(UNITY_BINARY_FILES "")
# foreach(LIB ${SHOWTIME_PUBLIC_LINKED_LIBS})
#     list(APPEND UNITY_BINARY_FILES $<TARGET_FILE:LIB>)
# endforeach()

set(UNITY_BINARY_FILES)
if(WIN32)
    # Copy dotnet target outputs on windows
    list(APPEND UNITY_BINARY_FILES
        $<TARGET_FILE:${CZMQ_TARGET}>
        $<TARGET_FILE:${CORE_TARGET}>
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


set(UNITY_PACKAGE "${UNITY_TARGET}-${CMAKE_PROJECT_VERSION}.unitypackage")
set(UNITY_PACKAGE_PATH "${UNITY_BUILD_DIR}/${UNITY_PACKAGE}")
set(UNITY_STAMP {CMAKE_CURRENT_BINARY_DIR}/unity.stamp)
if(MACOSX)
    add_custom_command(
    OUTPUT  ${UNITY_STAMP}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying bundle to ${UNITY_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_BUNDLE_DIR:${DOTNET_TARGET_WRAPPER}> ${UNITY_BIN_DIR}/$<TARGET_FILE_NAME:${DOTNET_TARGET_WRAPPER}>
    COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_STAMP}
)
endif()
add_custom_command(
    OUTPUT ${UNITY_PACKAGE_PATH} 
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${UNITY_BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying project to ${UNITY_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${UNITY_SRC_DIR} ${UNITY_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying libraries to ${UNITY_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy ${UNITY_BINARY_FILES} ${UNITY_BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Exporting Unity package"
    COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -quit -projectPath ${UNITY_BUILD_DIR} -exportPackage Assets/${UNITY_TARGET} ${UNITY_PACKAGE_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "...done"
    COMMAND ${CMAKE_COMMAND} -E touch ${UNITY_STAMP}
    DEPENDS ${UNITY_STAMP}
)

add_custom_target(${UNITY_TARGET} ALL DEPENDS ${UNITY_PACKAGE_PATH})
add_dependencies(${UNITY_TARGET} ${DOTNET_TARGET} ${SERVER_TARGET})

message(STATUS ${UNITY_PACKAGE_PATH})
#install(FILES ${UNITY_PACKAGE_PATH} DESTINATION unity)
