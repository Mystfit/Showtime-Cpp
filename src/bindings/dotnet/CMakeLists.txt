message(STATUS "Configuring ${DOTNET_TARGET} target")

set(BUILD_STAMP "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

enable_language(CSharp)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/dotnet)

# Set include directories from client dependency
include_directories(${Showtime_INCLUDE_DIRS} ${Showtime_COMPILED_INCLUDE_DIRS})

# Set include files
set(SWIG_MODULE_${DOTNET_TARGET}_EXTRA_DEPS ${Showtime_API_INTERFACE})

# Define language root interface file
set(DOTNET_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_dotnet.i")
set_source_files_properties(${DOTNET_SWIG_INTERFACES} PROPERTIES CPLUSPLUS ON)

# Clear the debug suffix for generated files
set(CMAKE_DEBUG_POSTFIX "")

# Create the swig dotnet target
swig_add_library(${DOTNET_TARGET} LANGUAGE csharp TYPE SHARED SOURCES ${DOTNET_SWIG_INTERFACES})

# Set dependant projects
add_dependencies(${SWIG_MODULE_${DOTNET_TARGET}_REAL_NAME} ${CLIENT_TARGET})

# Link libraries to swig target
swig_link_libraries(
    ${DOTNET_TARGET}
    ${CLIENT_TARGET}
)

# Group together generated binding scripts
file(GLOB DOTNET_GENERATED_BINDINGS "${CMAKE_SWIG_OUTDIR}/*.cs")

# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})
