# paths
set(SETUP_PY_IN 		"${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY_PARTIAL    "${CMAKE_CURRENT_BINARY_DIR}/setup-partial.py")
set(SETUP_PY    		"${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(BUILD_STAMP 		"${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

# Set up swig interfaces
set(SWIG_INTERFACES "${CMAKE_CURRENT_SOURCE_DIR}/showtime.i")
set(SWIG_INTERFACES_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/showtime.i")
configure_file(${SWIG_INTERFACES} ${SWIG_INTERFACES_DESTINATION} COPYONLY)

# Copy showtime module files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/showtime.i DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../showtime_common.i DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# First pass at generating setup.py
set(SHOWTIME_TARGET "@SHOWTIME_TARGET@")
configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})

# Set up second pass for generating setup.py, filling in showtime runtime paths
set(SHOWTIME_TARGET '$<TARGET_FILE:${PROJECT_NAME}>')

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
						-D SHOWTIME_TARGET=${SHOWTIME_TARGET}
						-P ${SETUPPY_PARSER}
                	DEPENDS ${SETUP_PY_PARTIAL}
                	VERBATIM)

# Command for showtime c extension build step
add_custom_command(OUTPUT ${BUILD_STAMP}
                	COMMAND ${PREFIX} ${PYTHON_EXECUTABLE} ${SETUP_PY} build ${EXTENSION_COMPILER}
                	COMMAND ${CMAKE_COMMAND} -E touch ${BUILD_STAMP}
                	DEPENDS ${SETUP_PY} ${PROJECT_NAME})

# Create target and install
add_custom_target(INSTALL_PYTHON ALL DEPENDS ${BUILD_STAMP})
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")
