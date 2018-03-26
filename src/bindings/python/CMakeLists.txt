message(STATUS "Configuring ${PYTHON_TARGET} target")

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

# Main swig interface
set(PYTHON_SWIG_INTERFACE "${CMAKE_SWIG_OUTDIR}/showtime_python.i")
set_source_files_properties(${PYTHON_SWIG_INTERFACE} PROPERTIES CPLUSPLUS ON)

# First pass variables for generated setup.py
set(ZST_RUNTIMES "@ZST_RUNTIMES@")
set(ZST_INCLUDE_DIRS "@ZST_INCLUDE_DIRS@")
set(ZST_BUILD_LIB_DIRS "@ZST_BUILD_LIB_DIRS@") 
set(ZST_RUNTIME_LIB_DIRS "@ZST_RUNTIME_LIB_DIRS@") 
set(ZST_LIBS "@ZST_LIBS@")
set(ZST_FLAGS "@ZST_FLAGS@")
set(ZST_DEFINITIONS "@ZST_DEFINITIONS@")
set(ZST_LINK_FLAGS "@ZST_LINK_FLAGS@")

# Generate first stage setup.py 
configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})

# Clear first pass vars
set(ZST_RUNTIMES "")
set(ZST_INCLUDE_DIRS "")
set(ZST_BUILD_LIB_DIRS "") 
set(ZST_RUNTIME_LIB_DIRS "") 
set(ZST_LIBS "")
set(ZST_FLAGS "")
set(ZST_DEFINITIONS "")
set(ZST_LINK_FLAGS "")

# Set up second pass variables for generated setup.py
set(ZST_BUILD_LIB_DIRS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>)
set(ZST_RUNTIME_LIB_DIRS ${CMAKE_INSTALL_PREFIX}/lib)
set(ZST_INCLUDE_DIRS $<TARGET_PROPERTY:${CLIENT_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
set(ZST_DEFINITIONS $<TARGET_PROPERTY:${CLIENT_TARGET},INTERFACE_COMPILE_DEFINITIONS>) #['-DMSGPACK_USE_BOOST']

# Platform specific definitions and paths
message(STATUS "Setting platform variables for ${CMAKE_SYSTEM_NAME}")

if(WIN32)
    set(BOOST_DLL_PATTERN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/boost*.dll)
    list(APPEND ZST_RUNTIMES 
        $<BUILD_INTERFACE:$<TARGET_FILE:${CLIENT_TARGET}>>
        $<BUILD_INTERFACE:$<TARGET_FILE:${CORE_TARGET}>>
        $<BUILD_INTERFACE:$<TARGET_FILE:${CORE_ENTITIES_TARGET}>>
        $<BUILD_INTERFACE:$<TARGET_FILE:${CZMQ_TARGET}>>
    )
    list(APPEND ZST_FLAGS $<$<CONFIG:Debug>:"/Zi">)
    list(APPEND ZST_LINK_FLAGS $<$<CONFIG:Debug>:/DEBUG>)
    list(APPEND ZST_LIBS
        $<BUILD_INTERFACE:$<TARGET_LINKER_FILE:${CORE_TARGET}>>
        $<BUILD_INTERFACE:$<TARGET_LINKER_FILE:${CORE_ENTITIES_TARGET}>>
        $<BUILD_INTERFACE:$<TARGET_LINKER_FILE:${CLIENT_TARGET}>>
    )
endif()

if(MACOSX OR LINUX)
    list(APPEND ZST_FLAGS "-O0;-g;-std=c++14")
    list(APPEND ZST_LIBS
        ${CORE_TARGET}
        ${CORE_ENTITIES_TARGET}
        ${CLIENT_TARGET}
    )
    list(APPEND ZST_LINK_FLAGS "-R${CMAKE_INSTALL_PREFIX}/lib")
endif()

set(PREFIX "")
if(LINUX)
    set(PREFIX "CC=gcc ")
endif()

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
                        "-DZST_BUILD_LIB_DIRS=${ZST_BUILD_LIB_DIRS}" 
                        "-DZST_RUNTIME_LIB_DIRS=${ZST_RUNTIME_LIB_DIRS}"
                        "-DZST_LIBS=${ZST_LIBS}" 
                        "-DZST_DEFINITIONS=${ZST_DEFINITIONS}"
                        "-DZST_FLAGS=${ZST_FLAGS}"
                        "-DZST_LINK_FLAGS=${ZST_LINK_FLAGS}"
                        "-DZST_INCLUDE_DIRS=${ZST_INCLUDE_DIRS}"
                        "-DBOOST_DLL_PATTERN=${BOOST_DLL_PATTERN}"
                        -P ${SETUPPY_PARSER_SCRIPT}
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SWIG_OUTDIR}/showtime"
                    COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext --verbose ${EXTENSION_COMPILER}
                    COMMAND ${PREFIX} ${PYTHON_EXECUTABLE} ${SETUP_PY} build ${EXTENSION_COMPILER}
                    DEPENDS ${SETUP_PY_PARTIAL}
                    VERBATIM
)

install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")

# Restore binding output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)
