message(STATUS "Configuring ${PYTHON_TARGET} target")

if(NOT BUILD_STATIC)
    message(FATAL_ERROR "Python bindings require BUILD_STATIC to be enabled")
endif()

set(PYTHON_TARGET_WRAPPER ${PYTHON_TARGET}Wrapper)

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/python/showtime)

# paths 
set(SETUP_PY_IN             "${CMAKE_CURRENT_LIST_DIR}/setup.py.in")
set(SETUP_PY                "${CMAKE_SWIG_OUTDIR}/setup.py")
set(MANIFEST_PARSER_SCRIPT  "${CMAKE_CURRENT_LIST_DIR}/generate-manifest.cmake")
set(MANIFEST_IN_IN          "${CMAKE_CURRENT_LIST_DIR}/MANIFEST.in.in")
set(MANIFEST_IN_PARTIAL     "${CMAKE_SWIG_OUTDIR}/MANIFEST-partial.in")
set(MANIFEST_IN             "${CMAKE_SWIG_OUTDIR}/MANIFEST.in")
set(BUILD_STAMP             "${CMAKE_CURRENT_BINARY_DIR}/build/python/timestamp")
set(PYTHON_LINK_LIBS 
    ${CLIENT_TARGET_STATIC}
    ${SERVER_TARGET_STATIC}
)

# Create setup.py
configure_file(${SETUP_PY_IN} ${SETUP_PY} @ONLY)

# Create manifest
set(SHOWTIME_WRAPPER_LIB "@SHOWTIME_WRAPPER_LIB@")
configure_file(${MANIFEST_IN_IN} ${MANIFEST_IN_PARTIAL})

# Define language root interface file
set(PYTHON_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_python.i")
set_source_files_properties(${PYTHON_SWIG_INTERFACES} PROPERTIES 
    CPLUSPLUS ON
    SWIG_COMPILE_OPTIONS "-directors"
)

# Create the swig PYTHON target
swig_add_library(${PYTHON_TARGET_WRAPPER} 
    LANGUAGE python
    TYPE SHARED
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${PYTHON_SWIG_INTERFACES}
)
set_target_properties(${PYTHON_TARGET_WRAPPER} PROPERTIES 
    SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
    SWIG_SUPPORT_FILES_DIRECTORY ${SWIG_LANGUAGE_DIR}
    OUTPUT_NAME showtime
    DEBUG_POSTFIX ""
    CXX_STANDARD 14
)

if(BINDINGS_PYTHON_LINK_AT_RUNTIME)
    # Since we don't link against the Python libs we need to explicitly add the python include dir
    target_include_directories(${PYTHON_TARGET_WRAPPER} PRIVATE ${PYTHON_INCLUDE_DIR})
    if(MACOSX OR LINUX)
        # Compiler option to ignore unresolved externals, resolve at runtime
        target_link_options(${PYTHON_TARGET_WRAPPER} PRIVATE -undefined dynamic_lookup)
    elseif(WIN32)
        message(WARNING "Windows runtime linking not implemented")
        target_link_options(${PYTHON_TARGET_WRAPPER} PRIVATE -undefined dynamic_lookup)
    endif()
else()
    list(APPEND PYTHON_LINK_LIBS ${PYTHON_LIB_TARGET})
endif()

# Set dependant projects
add_dependencies(${PYTHON_TARGET_WRAPPER} 
    ${CLIENT_TARGET_STATIC} 
    ${SERVER_TARGET_STATIC}
)
add_dependencies(${PYTHON_TARGET} ${PYTHON_TARGET_WRAPPER})

# Link libraries to swig target
swig_link_libraries(${PYTHON_TARGET_WRAPPER} PUBLIC ${PYTHON_LINK_LIBS})

# Copy files to build tree
add_custom_command(TARGET ${PYTHON_TARGET} PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/showtime ${CMAKE_SWIG_OUTDIR}/showtime
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYTHON_TARGET_WRAPPER}> ${CMAKE_SWIG_OUTDIR}/showtime
                    VERBATIM
)

# Run setuptools to build wheels
message(STATUS "${CMAKE_SWIG_OUTDIR}")
message(STATUS "${Python_EXECUTABLE} ${SETUP_PY} bdist_wheel")
add_custom_command(OUTPUT ${PYTHON_WHEEL}
                    COMMAND ${CMAKE_COMMAND} 
                        "-DSUBST_SOURCE=${MANIFEST_IN_PARTIAL}"
                        "-DSUBST_DEST=${MANIFEST_IN}"
                        "-DSHOWTIME_WRAPPER_LIB=showtime/$<TARGET_FILE_NAME:${PYTHON_TARGET_WRAPPER}>"
                        -P ${MANIFEST_PARSER_SCRIPT}
                    COMMAND ${Python_EXECUTABLE} ${SETUP_PY} bdist_wheel WORKING_DIRECTORY ${CMAKE_SWIG_OUTDIR}
                    DEPENDS ${SETUP_PY}
                    VERBATIM
)

# Install library
message(STATUS "${PIP_COMMAND} install ${PYTHON_WHEEL}")
install(CODE "execute_process(COMMAND ${PIP_COMMAND} install --ignore-installed ${PYTHON_WHEEL})")

# Restore binding output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)
