message(STATUS "Configuring ${DOTNET_TARGET} target")

# if(NOT BUILD_STATIC)
#     message(FATAL_ERROR "Dotnet bindings require BUILD_STATIC to be enabled")
# endif()

set(DOTNET_TARGET_WRAPPER ${DOTNET_TARGET}Wrapper)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)

# Define language root interface file
set(DOTNET_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_dotnet.i")
set_source_files_properties(${DOTNET_SWIG_INTERFACES} PROPERTIES CPLUSPLUS ON)
message(STATUS "Running swig to generate dotnet wrappers")

# Set name of dllimport library for generated cs pinvoke calls
set(DOTNET_WRAPPER_FILENAME ${DOTNET_TARGET_WRAPPER})

foreach(SWIG_INTERFACE_FILE ${DOTNET_SWIG_INTERFACES})
    get_filename_component(OUTFILE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)
    set(SWIG_ARGS
        "-c++"
        "-csharp"
        "-I${Showtime_INCLUDE_DIRS}"
        "-dllimport" "${DOTNET_WRAPPER_FILENAME}"
        "-outdir" "${SWIG_LANGUAGE_DIR}"
        "-o" "${CMAKE_SWIG_OUTDIR}/${OUTFILE_NAME}CSHARP_wrap.cxx"
    )
    if(ANDROID)
        list(APPEND SWIG_ARGS "-monoaotcompat")
    endif()
    list(APPEND SWIG_ARGS "${SWIG_INTERFACE_FILE}")

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
set(DOTNET_STAMP ${CMAKE_CURRENT_BINARY_DIR}/dotnet.stamp)

# Create the swig dotnet target
swig_add_library(${DOTNET_TARGET_WRAPPER}
    LANGUAGE csharp
    TYPE MODULE
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${DOTNET_SWIG_INTERFACES}
)

# Fix library prefixes on unix platforms
if(LINUX OR ANDROID)
    set_target_properties(${DOTNET_TARGET_WRAPPER} PROPERTIES 
        PREFIX "lib"
    )
endif()

# Enable Ahead of Time compilation on mobile platforms for swig directors
if(ANDROID)
    set_target_properties(${DOTNET_TARGET_WRAPPER} PROPERTIES 
        PREFIX "lib"
        SWIG_COMPILE_OPTIONS -monoaotcompat
    )
endif()

if(MACOSX)
    set_target_properties(${SWIG_MODULE_${DOTNET_TARGET_WRAPPER}_REAL_NAME} PROPERTIES BUNDLE TRUE)
    if(NOT BUILD_STATIC)
        set(DOTNET_BUNDLE_COPY_LIBS
            $<TARGET_FILE:${CORE_TARGET}>
            $<TARGET_FILE:${SERVER_TARGET}>
            # $<TARGET_FILE:${CORE_ENTITIES_TARGET}>
            $<TARGET_FILE:${CLIENT_TARGET}> 
        )
        add_custom_command(TARGET ${DOTNET_TARGET_WRAPPER} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_BUNDLE_COPY_LIBS} $<TARGET_BUNDLE_CONTENT_DIR:${DOTNET_TARGET_WRAPPER}>/MacOS
            VERBATIM
        )
    endif()
endif()

# Set dependant projects
add_dependencies(${DOTNET_TARGET_WRAPPER} 
    ${CLIENT_TARGET}
    ${SERVER_TARGET}
)

# Link libraries to swig target
swig_link_libraries(${DOTNET_TARGET_WRAPPER} 
    PUBLIC
    ${CLIENT_TARGET} 
    ${SERVER_TARGET}
)

if(WIN32)
    # Make sure all included C# files are set to compile
    foreach(file ${DOTNET_GENERATED_BINDINGS})
        set_source_files_properties(${file} PROPERTIES VS_TOOL_OVERRIDE "Compile")
    endforeach()

    target_sources(${DOTNET_TARGET} PRIVATE ${DOTNET_GENERATED_BINDINGS})
else()
    message(STATUS "Configuring Dotnet unix project")

    # Create custom .csproj file for unix platforms
    set(CSPROJ_IN "${CMAKE_CURRENT_LIST_DIR}/${DOTNET_TARGET}.csproj.in")
    set(CSPROJ_OUT "${SWIG_LANGUAGE_DIR}/${DOTNET_TARGET}.csproj")
    set(DOTNET_SRC_BLOCK "")
    foreach(file ${DOTNET_GENERATED_BINDINGS})
        string(APPEND DOTNET_SRC_BLOCK "<Compile\ Include=\"${file}\"\ />\n    ")
    endforeach()
    configure_file(${CSPROJ_IN} ${CSPROJ_OUT})

    # Build the project using the dotnet sdk
    set(DOTNET_COMMAND dotnet CACHE FILEPATH "Path to dotnet sdk")
    set(DOTNET_ARGS 
        "build" "-o" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        "--framework" "${BINDINGS_DOTNET_FRAMEWORK_VERSION}"
        "${CSPROJ_OUT}")
    add_custom_command(
        OUTPUT ${DOTNET_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "Building Dotnet unix dll"
        COMMAND ${DOTNET_COMMAND} ${DOTNET_ARGS}
        DEPENDS ${CSPROJ_OUT}
        VERBATIM
    )
endif()

# Set net framework version
set_target_properties(${DOTNET_TARGET} PROPERTIES DOTNET_TARGET_FRAMEWORK_VERSION ${BINDINGS_DOTNET_FRAMEWORK_VERSION})

# Add dependencies
add_dependencies(${DOTNET_TARGET} ${DOTNET_TARGET_WRAPPER})

# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})
