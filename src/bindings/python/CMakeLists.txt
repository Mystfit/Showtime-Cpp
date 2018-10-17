message(STATUS "Configuring ${PYTHON_TARGET} target")

set(PYTHON_VERSION 2.7 CACHE STRING "Python version to search for")
find_package(Python COMPONENTS Interpreter Development REQUIRED)

if(NOT BUILD_STATIC)
    message(FATAL_ERROR "Python bindings require BUILD_STATIC to be enabled")
endif()

# Save original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)
set(SWIG_LANGUAGE_DIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)

# Define language root interface file
set(PYTHON_SWIG_INTERFACES "${CMAKE_CURRENT_LIST_DIR}/showtime_python.i")
set_source_files_properties(${PYTHON_SWIG_INTERFACES} PROPERTIES CPLUSPLUS ON)

# Set include files
set(SWIG_MODULE_${PYTHON_TARGET}_EXTRA_DEPS ${Showtime_API_INTERFACE})

# Clear the debug suffix for generated files
set(CMAKE_DEBUG_POSTFIX "")
set(PYTHON_STAMP ${CMAKE_CURRENT_BINARY_DIR}/python.stamp)

swig_add_library(${PYTHON_TARGET} 
    LANGUAGE python 
    TYPE MODULE 
    OUTPUT_DIR ${SWIG_LANGUAGE_DIR}
    OUTFILE_DIR ${CMAKE_SWIG_OUTDIR}
    SOURCES ${PYTHON_SWIG_INTERFACES}
)

# Set include directories from client dependency
target_include_directories(${SWIG_MODULE_${PYTHON_TARGET}_REAL_NAME} PRIVATE 
    ${Showtime_INCLUDE_DIRS} 
    ${Showtime_COMPILED_INCLUDE_DIRS}
)

# Set dependant projects
add_dependencies(${SWIG_MODULE_${PYTHON_TARGET}_REAL_NAME} ${CLIENT_TARGET_STATIC})

# Link libraries to swig target
swig_link_libraries(${PYTHON_TARGET} 
    ${CLIENT_TARGET_STATIC} 
    Python::Python
)

# # paths
# set(SETUP_PY_IN         "${CMAKE_CURRENT_LIST_DIR}/setup.py.in")
# set(SETUP_PY_PARTIAL    "${CMAKE_SWIG_OUTDIR}/setup-partial.py")
# set(SETUP_PY            "${CMAKE_SWIG_OUTDIR}/setup.py")

# # First pass variables for generated setup.py
# set(ZST_RUNTIMES "@ZST_RUNTIMES@")
# set(ZST_INCLUDE_DIRS "@ZST_INCLUDE_DIRS@")
# set(ZST_BUILD_LIB_DIRS "@ZST_BUILD_LIB_DIRS@") 
# set(ZST_RUNTIME_LIB_DIRS "@ZST_RUNTIME_LIB_DIRS@") 
# set(ZST_LIBS "@ZST_LIBS@")
# set(ZST_FLAGS "@ZST_FLAGS@")
# set(ZST_DEFINITIONS "@ZST_DEFINITIONS@")
# set(ZST_LINK_FLAGS "@ZST_LINK_FLAGS@")
# set(ZST_VERSION "@ZST_VERSION@")

# # Generate first stage setup.py 
# configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})

# # Clear first pass vars
# set(ZST_RUNTIMES "")
# set(ZST_INCLUDE_DIRS "")
# set(ZST_BUILD_LIB_DIRS "") 
# set(ZST_RUNTIME_LIB_DIRS "") 
# set(ZST_LIBS "")
# set(ZST_FLAGS "")
# set(ZST_DEFINITIONS "")
# set(ZST_LINK_FLAGS "")
# set(ZST_BUILD_LIB_DIRS "")

# # Set up second pass variables for generated setup.py
# set(ZST_RUNTIME_LIB_DIRS ${CMAKE_INSTALL_PREFIX}/lib)
# set(ZST_INCLUDE_DIRS $<TARGET_PROPERTY:${CLIENT_TARGET_STATIC},INTERFACE_INCLUDE_DIRECTORIES>)
# set(ZST_DEFINITIONS $<TARGET_PROPERTY:${CLIENT_TARGET_STATIC},INTERFACE_COMPILE_DEFINITIONS>) #['-DMSGPACK_USE_BOOST']

# # Platform specific definitions and paths
# message(STATUS "Setting platform variables for ${CMAKE_SYSTEM_NAME}")

# if(WIN32)
#     set(BOOST_DLL_PATTERN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/boost*.dll)
#     list(APPEND ZST_RUNTIMES 
#         $<BUILD_INTERFACE:$<TARGET_FILE:${CLIENT_TARGET}>>
#         $<BUILD_INTERFACE:$<TARGET_FILE:${CORE_TARGET}>>
#         $<BUILD_INTERFACE:$<TARGET_FILE:${CORE_ENTITIES_TARGET}>>
#         $<BUILD_INTERFACE:$<TARGET_FILE:${CZMQ_TARGET}>>
#     )
#     list(APPEND ZST_FLAGS $<$<CONFIG:Debug>:/Zi>)
#     list(APPEND ZST_LINK_FLAGS $<$<CONFIG:Debug>:/DEBUG>)
# endif()

# if(MACOSX OR LINUX)
#     list(APPEND ZST_FLAGS "-O0;-g;-std=c++14;")
#     foreach(lib ${SHOWTIME_PUBLIC_LINKED_LIBS})
#         list(APPEND ZST_LIBS $<BUILD_INTERFACE:$<TARGET_LINKER_FILE:${lib}>>)
#     endforeach()
#     list(APPEND ZST_LINK_FLAGS "-Wl,-rpath,${ZST_RUNTIME_LIB_DIRS}" ${ZST_LIBS})
# endif()

# # Set second pass setup.py parser
# get_filename_component(SETUPPY_PARSER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/generate-setuppy.cmake REALPATH)

# # Create python target
# add_custom_target(${PYTHON_TARGET} ALL DEPENDS ${CLIENT_TARGET})

# # Command for building second-pass setup.py
# add_custom_command(TARGET ${PYTHON_TARGET} PRE_BUILD
#                     COMMAND ${CMAKE_COMMAND} 
#                         "-DSUBST_SOURCE=${SETUP_PY_PARTIAL}" 
#                         "-DSUBST_DEST=${SETUP_PY}" 
#                         "-DZST_RUNTIMES=${ZST_RUNTIMES}" 
#                         "-DZST_BUILD_LIB_DIRS=${ZST_BUILD_LIB_DIRS}" 
#                         "-DZST_RUNTIME_LIB_DIRS=${ZST_RUNTIME_LIB_DIRS}"
#                         "-DZST_LIBS=${ZST_LIBS}" 
#                         "-DZST_DEFINITIONS=${ZST_DEFINITIONS}"
#                         "-DZST_FLAGS=${ZST_FLAGS}"
#                         "-DZST_LINK_FLAGS=${ZST_LINK_FLAGS}"
#                         "-DZST_INCLUDE_DIRS=${ZST_INCLUDE_DIRS}"
#                         "-DZST_VERSION=${CMAKE_PROJECT_VERSION}"
#                         "-DBOOST_DLL_PATTERN=${BOOST_DLL_PATTERN}"
#                         -P ${SETUPPY_PARSER_SCRIPT}
#                     COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SWIG_OUTDIR}/showtime"
#                     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build_ext ${EXTENSION_COMPILER}
#                     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build ${EXTENSION_COMPILER}
#                     DEPENDS ${SETUP_PY_PARTIAL}
#                     VERBATIM
# )

# install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q install)")

# # Restore binding output dir
# set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR}/python)

# Restore original swig output dir
set(CMAKE_SWIG_OUTDIR ${CMAKE_SWIG_ROOT_OUTDIR})