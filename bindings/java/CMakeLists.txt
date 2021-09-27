message(STATUS "Configuring ${JAVA_TARGET} target")

if(NOT BUILD_STATIC)
    message(FATAL_ERROR "Java bindings require BUILD_STATIC to be enabled")
endif()

set(JAVA_TARGET_WRAPPER ${JAVA_TARGET}Wrapper)
set(JAVA_PACKAGE showtime)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/java)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/java/classes)

# Set java specific swig interfaces
set(JAVA_SWIG_INTERFACES 
    "${CMAKE_CURRENT_LIST_DIR}/showtime_java.i"
)

# Set include directories for swig
set(JAVA_INCLUDES 
    ${Showtime_INCLUDE_DIRS}
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2}
)

# Set common swig flags
#set(CMAKE_SWIG_FLAGS -package ${JAVA_PACKAGE})

# Run SWIG externally to generate java files during configure step for add_jar
foreach(SWIG_INTERFACE_FILE ${JAVA_SWIG_INTERFACES})
    get_filename_component(OUTFILE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)
    set(SWIG_ARGS 
        "-c++"
        "-java"
        "-I${Showtime_INCLUDE_DIRS}"
        "-package" "${JAVA_PACKAGE}"
        "-outdir" "${SWIG_LANGUAGE_DIR}"
        "-o" "${CMAKE_SWIG_OUTDIR}/${OUTFILE_NAME}_wrap.cxx"
        "${SWIG_INTERFACE_FILE}"
    )
    set_source_files_properties(${SWIG_INTERFACE_FILE} PROPERTIES 
        CPLUSPLUS ON
        #SWIG_MODULE_NAME ${JAVA_TARGET}
    )
    message(STATUS "Java SWIG flags: ${SWIG_ARGS}")
    file(MAKE_DIRECTORY ${SWIG_LANGUAGE_DIR})
    execute_process(COMMAND ${SWIG_EXECUTABLE} ${SWIG_ARGS} RESULT_VARIABLE SWIG_OUTPUT)
endforeach()

# Group together generated binding scripts
file(GLOB JAVA_GENERATED_BINDINGS CONFIGURE_DEPENDS "${SWIG_LANGUAGE_DIR}/*.java")

# Clear the debug suffix for generated files
set(CMAKE_DEBUG_POSTFIX "")
set(JAVA_STAMP ${CMAKE_CURRENT_BINARY_DIR}/JAVA.stamp)

# Create the swig JAVA target
swig_add_library(${JAVA_TARGET_WRAPPER} 
    LANGUAGE java 
    TYPE SHARED 
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${JAVA_SWIG_INTERFACES}
)

separate_arguments(SWIG_ARGS UNIX_COMMAND "-package ${JAVA_PACKAGE}")
set_target_properties(${JAVA_TARGET_WRAPPER} PROPERTIES 
    SWIG_COMPILE_OPTIONS "${SWIG_ARGS}"
    SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
    SWIG_SUPPORT_FILES_DIRECTORY ${SWIG_LANGUAGE_DIR}
)
target_include_directories(${JAVA_TARGET_WRAPPER} PRIVATE 
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2}
)

# Set dependant projects
add_dependencies(${JAVA_TARGET_WRAPPER} 
    ${CLIENT_TARGET_STATIC} 
    ${SERVER_TARGET_STATIC}
)

# Link libraries to swig target
swig_link_libraries(${JAVA_TARGET_WRAPPER} 
    PUBLIC 
    ${SHARED_FROM_STATIC_TARGET}
)

# Create jar library
add_jar(${JAVA_TARGET} 
    SOURCES ${JAVA_GENERATED_BINDINGS}
    OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Add dependencies
add_dependencies(${JAVA_TARGET} ${JAVA_TARGET_WRAPPER})

# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})
