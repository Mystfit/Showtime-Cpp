message(STATUS "Configuring ${DOTNET_TARGET} target")

set(BUILD_STAMP "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

set(DOTNET_TARGET_WRAPPER ${DOTNET_TARGET}Wrapper)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)

# Define language root interface file
set(DOTNET_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_dotnet.i")
set_source_files_properties(${DOTNET_SWIG_INTERFACES} PROPERTIES CPLUSPLUS ON)

message(STATUS "Running swig to generate dotnet wrappers")

foreach(SWIG_INTERFACE_FILE ${DOTNET_SWIG_INTERFACES})
    get_filename_component(OUTFILE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)
    set(SWIG_ARGS 
        "-c++"
        "-csharp"
        "-I${Showtime_INCLUDE_DIRS}"
        "-dllimport" "${DOTNET_TARGET_WRAPPER}"
        "-outdir" "${SWIG_LANGUAGE_DIR}"
        "-o" "${CMAKE_SWIG_OUTDIR}/${OUTFILE_NAME}CSHARP_wrap.cxx"
        "${SWIG_INTERFACE_FILE}"
    )
    file(MAKE_DIRECTORY ${SWIG_LANGUAGE_DIR})
    execute_process(COMMAND ${SWIG_EXECUTABLE} ${SWIG_ARGS} RESULT_VARIABLE SWIG_OUTPUT)
endforeach()

# Group together generated binding scripts
file(GLOB DOTNET_GENERATED_BINDINGS CONFIGURE_DEPENDS "${SWIG_LANGUAGE_DIR}/*.cs")

# Set include directories from client dependency
include_directories(${Showtime_INCLUDE_DIRS} ${Showtime_COMPILED_INCLUDE_DIRS})

# Set include files
set(SWIG_MODULE_${DOTNET_TARGET_WRAPPER}_EXTRA_DEPS ${Showtime_API_INTERFACE})

# Clear the debug suffix for generated files
set(CMAKE_DEBUG_POSTFIX "")

# Create the swig dotnet target
swig_add_library(${DOTNET_TARGET_WRAPPER} 
    LANGUAGE csharp 
    TYPE SHARED 
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${DOTNET_SWIG_INTERFACES}
)

# Set dependant projects
add_dependencies(${SWIG_MODULE_${DOTNET_TARGET_WRAPPER}_REAL_NAME} ${CLIENT_TARGET})

# Link libraries to swig target
swig_link_libraries(
    ${DOTNET_TARGET_WRAPPER}
    ${CLIENT_TARGET}
)

if(WIN32)
    # C# is a first class citizen for CMake on Windows, we can build the project normally
    enable_language(CSharp)
    target_sources(${DOTNET_TARGET} PRIVATE ${DOTNET_GENERATED_BINDINGS})
else()
    # Create custom .csproj file for unix platforms
    set(CSPROJ_IN "${CMAKE_CURRENT_LIST_DIR}/${DOTNET_TARGET}.csproj.in")
    set(CSPROJ_OUT "${SWIG_LANGUAGE_DIR}/${DOTNET_TARGET}.csproj")
    set(DOTNET_SRC_BLOCK "")
    foreach(file ${DOTNET_GENERATED_BINDINGS})
        string(APPEND DOTNET_SRC_BLOCK "<Compile\ Include=\"${file}\"\ />\n    ")
    endforeach()
    configure_file(${CSPROJ_IN} ${CSPROJ_OUT})

    # Build the dotnet project
    set(DOTNET_COMMAND "dotnet")
    add_custom_command(
        OUTPUT ${DOTNET_OUTPUT_FILE}
        COMMAND ${DOTNET_COMMAND} "build" "-o" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" "${CSPROJ_OUT}"
        DEPENDS ${CSPROJ_OUT} ${DOTNET_TARGET_WRAPPER}
        VERBATIM
    )

    # Create custom target for dotnet
    add_custom_target(${DOTNET_TARGET} DEPENDS ${DOTNET_OUTPUT_FILE})
endif()

add_dependencies(${DOTNET_TARGET} ${DOTNET_TARGET_WRAPPER})


# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})
