message(STATUS "Configuring ${DOTNET_TARGET} target")

if(NOT BUILD_STATIC)
    message(FATAL_ERROR "Dotnet bindings require BUILD_STATIC to be enabled")
endif()


# ShowtimeDotnetWrapper
# ---------------------

set(DOTNET_TARGET_WRAPPER ${DOTNET_TARGET}Wrapper)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)

# Custom .csproj project files
set(DOTNET_CSPROJ_IN "${CMAKE_CURRENT_LIST_DIR}/${DOTNET_TARGET}.csproj.in")
set(DOTNET_CSPROJ_OUT "${SWIG_LANGUAGE_DIR}/${DOTNET_TARGET}.csproj")

# Define language root interface file
set(DOTNET_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_dotnet.i")
set_source_files_properties(${DOTNET_SWIG_INTERFACES} PROPERTIES CPLUSPLUS ON)
message(STATUS "Running swig to generate dotnet wrappers")

# Set name of dllimport library for generated cs pinvoke calls
set(DOTNET_WRAPPER_FILENAME ${DOTNET_TARGET_WRAPPER})

set(DOTNET_BINDINGS
    "${CMAKE_CURRENT_LIST_DIR}/ZstDelegateSessionAdaptor.cs"
    "${CMAKE_CURRENT_LIST_DIR}/ZstDelegateHierarchyAdaptor.cs"
    "${CMAKE_CURRENT_LIST_DIR}/ZstDelegateSynchronisableAdaptor.cs"
    "${CMAKE_CURRENT_LIST_DIR}/ZstDelegateEntityAdaptor.cs"
)

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

# Add hand-written CS files
list(APPEND DOTNET_GENERATED_BINDINGS ${DOTNET_BINDINGS})

# Set include directories from client dependency
include_directories(${Showtime_INCLUDE_DIRS} ${Showtime_COMPILED_INCLUDE_DIRS})

# Set include files
set(SWIG_MODULE_${DOTNET_TARGET_WRAPPER}_EXTRA_DEPS ${Showtime_API_INTERFACE})

# Clear the debug suffix for generated files
set(CMAKE_DEBUG_POSTFIX "")
set(DOTNET_STAMP ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnet/dotnet.stamp)

# Dependant projects
set(DOTNET_DEPENDENCIES ${CLIENT_TARGET_STATIC} ${SERVER_TARGET_STATIC})

# Create the swig dotnet target
swig_add_library(${DOTNET_TARGET_WRAPPER}
    LANGUAGE csharp
    TYPE MODULE
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${DOTNET_SWIG_INTERFACES}
)

# Fix library prefixes on non-windows platforms
if(NOT WIN32)
    set_target_properties(${DOTNET_TARGET_WRAPPER} PROPERTIES PREFIX "lib")
endif()

# Enable Ahead of Time compilation on mobile platforms for swig directors
if(ANDROID)
    set_target_properties(${DOTNET_TARGET_WRAPPER} PROPERTIES SWIG_COMPILE_OPTIONS -monoaotcompat)
endif()

if(MACOSX)
    set_target_properties(${SWIG_MODULE_${DOTNET_TARGET_WRAPPER}_REAL_NAME} PROPERTIES BUNDLE TRUE)
    # set(DOTNET_BUNDLE_COPY_LIBS
    #     $<TARGET_FILE:${CORE_TARGET}>
    #     $<TARGET_FILE:${SERVER_TARGET}>
    #     $<TARGET_FILE:${CLIENT_TARGET}> 
    # )
    # add_custom_command(TARGET ${DOTNET_TARGET_WRAPPER} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_BUNDLE_COPY_LIBS} $<TARGET_BUNDLE_CONTENT_DIR:${DOTNET_TARGET_WRAPPER}>/MacOS
    #     VERBATIM
    # )
endif()

# Set dependant projects
add_dependencies(${DOTNET_TARGET_WRAPPER} ${DOTNET_DEPENDENCIES})

# Link libraries to swig target
swig_link_libraries(${DOTNET_TARGET_WRAPPER} PUBLIC ${DOTNET_DEPENDENCIES})


# ShowtimeDotnet 
# --------------

# Get the library output directory
get_filename_component(DOTNET_OUTPUT_DIR ${DOTNET_OUTPUT_PACKAGE} DIRECTORY)

# Create output directories
file(MAKE_DIRECTORY ${DOTNET_OUTPUT_DIR})

if(WIN32 AND NOT BINDINGS_DOTNET_CSPROJ)
    # Make sure all included C# files are set to compile
    foreach(file ${DOTNET_GENERATED_BINDINGS})
        set_source_files_properties(${file} PROPERTIES VS_TOOL_OVERRIDE "Compile")
    endforeach()
    set_target_properties(${DOTNET_TARGET} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${DOTNET_OUTPUT_DIR}
        DOTNET_TARGET_FRAMEWORK_VERSION ${BINDINGS_DOTNET_FRAMEWORK_VERSION}
    )
    target_sources(${DOTNET_TARGET} PRIVATE ${DOTNET_GENERATED_BINDINGS})
else()
    message(STATUS "Configuring Dotnet project")
    set(DOTNET_SRC_BLOCK "")
    foreach(file ${DOTNET_BINDINGS})
        string(APPEND DOTNET_SRC_BLOCK "    <Compile\ Include=\"${file}\"\ />\n")
    endforeach()

    set(DOTNET_NATIVE_RUNTIMES_BLOCK "")
    set(DOTNET_RUNTIME_BLOCK_TEMPLATE "")
    string(CONCAT DOTNET_RUNTIME_BLOCK_TEMPLATE
            "<Content\ Include=\"@runtime_path@\">\n"
            "      <PackagePath>runtimes/@runtime@/native/@runtime_file@</PackagePath>\n"
            "      <Pack>true</Pack>\n"
            "      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>"
            "      <PackageCopyToOutput>true</PackageCopyToOutput>\n"
            "    </Content>\n"
    )

    set(RID_Windows "win-x64")
    set(RID_Darwin "osx-x64")
    set(RID_Linux "linux-x64")
    set(RID_Android "android-arm64")
    set(ALL_RIDS
        ${RID_Windows}
        ${RID_Darwin}
        ${RID_Linux}
        ${RID_Android}
    )

    if(WIN32 OR BUILD_WINDOWS_CONTAINER)
        set(runtime ${RID_Windows})
        set(runtime_file "${DOTNET_TARGET_WRAPPER}.dll")
        set(runtime_path "${WINDOWS_LIB_DIR}/${runtime_file}")
        string(CONFIGURE ${DOTNET_RUNTIME_BLOCK_TEMPLATE} runtime_block_win @ONLY)
        string(APPEND DOTNET_NATIVE_RUNTIMES_BLOCK ${runtime_block_win}${runtime_block_wrapper})
    endif()
    if(MACOSX OR BUILD_MACOS_CONTAINER)
        set(runtime ${RID_Darwin})
        set(runtime_file "${DOTNET_TARGET_WRAPPER}.bundle/")
        set(runtime_path "${MACOS_LIB_DIR}/${runtime_file}**")
        string(CONFIGURE ${DOTNET_RUNTIME_BLOCK_TEMPLATE} runtime_block @ONLY)
        string(APPEND DOTNET_NATIVE_RUNTIMES_BLOCK ${runtime_block})
    endif()
    if(ANDROID OR BUILD_ANDROID_CONTAINER)
        set(runtime ${RID_Android})
        set(runtime_file "lib${DOTNET_TARGET_WRAPPER}.so")
        set(runtime_path "${ANDROID_LIB_DIR}/${runtime_file}")
        string(CONFIGURE ${DOTNET_RUNTIME_BLOCK_TEMPLATE} runtime_block @ONLY)
        string(APPEND DOTNET_NATIVE_RUNTIMES_BLOCK ${runtime_block})
    endif()
    if(LINUX OR BUILD_LINUX_CONTAINER)
        set(runtime ${RID_Linux})
        set(runtime_file "lib${DOTNET_TARGET_WRAPPER}.so")
        set(runtime_path "${LINUX_LIB_DIR}/${runtime_file}")
        string(CONFIGURE ${DOTNET_RUNTIME_BLOCK_TEMPLATE} runtime_block @ONLY)
        string(APPEND DOTNET_NATIVE_RUNTIMES_BLOCK ${runtime_block})
    endif()
    configure_file(${DOTNET_CSPROJ_IN} ${DOTNET_CSPROJ_OUT})

    if(ADD_GENERATED_MSVC_PROJECTS)
        include_external_msproject(${DOTNET_TARGET} ${DOTNET_CSPROJ_OUT})
    endif()

     set(DOTNET_PACK_ARGS 
        "--include-source"
        "--include-symbols"
        "${DOTNET_CSPROJ_OUT}"
    )

    # Build csproj
    add_custom_command(
        OUTPUT ${DOTNET_OUTPUT_PACKAGE}
        COMMAND ${DOTNETCOMMAND} restore
        COMMAND ${CMAKE_COMMAND} -E echo "Building Dotnet package"
        COMMAND ${DOTNETCOMMAND} build ${DOTNET_CSPROJ_OUT}
        COMMAND ${DOTNETCOMMAND} pack ${DOTNET_ARGS}
        WORKING_DIRECTORY ${SWIG_LANGUAGE_DIR}
        DEPENDS ${CSPROJ_OUT} ${PLATFORM_TARGETS} ${DOTNET_DEPENDENCIES}
        VERBATIM
    )
    # add_custom_command(
    #     TARGET ${DOTNET_TARGET}
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/${DOTNET_OUTPUT_PACKAGENAME} ${DOTNET_OUTPUT_PACKAGE}
    #     WORKING_DIRECTORY ${SWIG_LANGUAGE_DIR}
    #     DEPENDS ${DOTNET_OUTPUT_PACKAGE}
    #     VERBATIM
    # )
endif()

# Add dependencies
add_dependencies(${DOTNET_TARGET} ${DOTNET_TARGET_WRAPPER})

# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})
