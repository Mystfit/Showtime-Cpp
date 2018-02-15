# paths
set(SETUP_PY_IN 		"${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY_PARTIAL    "${CMAKE_CURRENT_BINARY_DIR}/setup-partial.py")
set(SETUP_PY    		"${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(BUILD_STAMP 		"${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

# Copy showtime module files
file(GLOB PYTHON_INTERFACES "${CMAKE_CURRENT_SOURCE_DIR}/../*.i")
list(APPEND PYTHON_INTERFACES "${CMAKE_CURRENT_SOURCE_DIR}/showtime_python.i")
file(COPY ${PYTHON_INTERFACES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#Copy required include directories
get_target_property(CLIENT_TARGET_INCLUDES ${CLIENT_TARGET} PUBLIC_INCLUDE_DIRECTORIES)

# First pass at generating setup.py
set(CLIENT_INCLUDE_DIRS "@CLIENT_INCLUDE_DIRS@")
set(CLIENT_TARGET_LOCAL "@CLIENT_TARGET@")
set(CORE_TARGET_LOCAL "@CORE_TARGET@")

configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})

# Set up second pass for generating setup.py, filling in showtime runtime paths
set(CLIENT_INCLUDE_DIRS '$<TARGET_PROPERTY:${CLIENT_TARGET},INTERFACE_INCLUDE_DIRECTORIES>')
set(CLIENT_TARGET_LOCAL '$<TARGET_FILE:${CLIENT_TARGET}>')
set(CORE_TARGET_LOCAL '$<TARGET_FILE:${CORE_TARGET}>')

get_filename_component(SETUPPY_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/showtime-final.cmake REALPATH)

if(MACOSX)
	# SET(PREFIX "LDFLAGS=\"-framework\ Showtime\"\;PATH=/Library/Frameworks\;")
	MESSAGE(STATUS "Setting python env flags to ${PREFIX}")
ENDIF()

# Command for building second-pass setup.py
add_custom_command(OUTPUT ${SETUP_PY}
					COMMAND ${CMAKE_COMMAND}
						-D SUBST_SOURCE=${SETUP_PY_PARTIAL}
						-D SUBST_DEST=${SETUP_PY}
						-D CLIENT_INCLUDE_DIRS=${CLIENT_INCLUDE_DIRS}
						-D CLIENT_TARGET=${CLIENT_TARGET_LOCAL}
						-D CORE_TARGET=${CORE_TARGET_LOCAL}
						-P ${SETUPPY_PARSER}
                	DEPENDS ${SETUP_PY_PARTIAL}
                	VERBATIM)

# Command for showtime c extension build step
add_custom_command(OUTPUT ${BUILD_STAMP}
                	COMMAND ${PREFIX} ${PYTHON_EXECUTABLE} ${SETUP_PY} build ${EXTENSION_COMPILER}
                	COMMAND ${CMAKE_COMMAND} -E touch ${BUILD_STAMP}
                	DEPENDS ${SETUP_PY} ${CLIENT_TARGET})

# Create target and install
add_custom_target(ShowtimePython ALL DEPENDS ${BUILD_STAMP})
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")
