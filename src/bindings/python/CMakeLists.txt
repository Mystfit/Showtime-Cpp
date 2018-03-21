message(STATUS "Building ${PYTHON_TARGET} target")

set(Python_ADDITIONAL_VERSIONS 3.6)
FIND_PACKAGE(PythonLibs 3.6 REQUIRED)
#FIND_PACKAGE(PythonLibs 2.7.12 REQUIRED)
INCLUDE(FindPythonInterp)

# Set binding output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)

# paths
set(SETUP_PY_IN         "${CMAKE_CURRENT_LIST_DIR}/setup.py.in")
set(SETUP_PY_PARTIAL    "${CMAKE_SWIG_OUTDIR}/setup-partial.py")
set(SETUP_PY            "${CMAKE_SWIG_OUTDIR}/setup.py")
set(BUILD_STAMP         "${CMAKE_CURRENT_BINARY_DIR}/build/python/timestamp")

# Copy showtime module files
file(GLOB PYTHON_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/../*.i")
list(APPEND PYTHON_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_python.i")
file(COPY ${PYTHON_INTERFACES} DESTINATION ${CMAKE_SWIG_OUTDIR})

# First pass variables for generated setup.py
set(ZST_SWIG_INTERFACE "${CMAKE_SWIG_OUTDIR}/showtime_python.i")
set(ZST_RUNTIMES "@ZST_RUNTIMES@")
set(ZST_INCLUDE_DIRS "@ZST_INCLUDE_DIRS@")
set(ZST_LIB_DIRS "@ZST_LIB_DIRS@") 
set(ZST_LIBS "@ZST_LIBS@")
set(ZST_FLAGS "@ZST_FLAGS@")

# Generate first stage setup.py 
configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})

# Set up second pass variables for generated setup.py
set(ZST_LIB_DIRS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>)

set(ZST_LIBS
    $<TARGET_LINKER_FILE:${CORE_TARGET}>
    $<TARGET_LINKER_FILE:${CORE_ENTITIES_TARGET}>
    $<TARGET_LINKER_FILE:${CLIENT_TARGET}>
)

# Manually set runtimes to copy
set(ZST_RUNTIMES 
    $<TARGET_FILE:${CLIENT_TARGET}>
    $<TARGET_FILE:${CORE_TARGET}>
    $<TARGET_FILE:${CORE_ENTITIES_TARGET}>
    $<TARGET_FILE:${CZMQ_TARGET}>
)

if(WIN32)
    set(BOOST_DLL_PATTERN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/boost*.dll)
endif()

set(ZST_FLAGS $<TARGET_PROPERTY:${CLIENT_TARGET},INTERFACE_COMPILE_DEFINITIONS>)
set(ZST_INCLUDE_DIRS $<TARGET_PROPERTY:${CLIENT_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)

# Set second pass setup.py parser
get_filename_component(SETUPPY_PARSER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/generate-setuppy.cmake REALPATH)

# Create python target
add_custom_target(${PYTHON_TARGET} ALL DEPENDS ${CLIENT_TARGET})

# Command for building second-pass setup.py
add_custom_command(TARGET ${PYTHON_TARGET} PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} 
                        "-DSUBST_SOURCE=${SETUP_PY_PARTIAL}" 
                        "-DSUBST_DEST=${SETUP_PY}" 
                        "-DZST_RUNTIMES=${ZST_RUNTIMES}" 
                        "-DZST_LIB_DIRS=${ZST_LIB_DIRS}" 
                        "-DZST_LIBS=${ZST_LIBS}" 
                        "-DZST_FLAGS=${ZST_FLAGS}"
                        "-DZST_INCLUDE_DIRS=${ZST_INCLUDE_DIRS}" 
                        "-DBOOST_DLL_PATTERN=${BOOST_DLL_PATTERN}"
                        -P ${SETUPPY_PARSER_SCRIPT}
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SWIG_OUTDIR}/showtime"
                    COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build ${EXTENSION_COMPILER}
                    DEPENDS ${SETUP_PY_PARTIAL}
                    VERBATIM
)

install(CODE "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${SETUP_PY}\" install)")

# Restore binding output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)
