set(SETUP_PY_IN 		"${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY_PARTIAL    "${CMAKE_CURRENT_BINARY_DIR}/setup-partial.py")
set(SETUP_PY    		"${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(DEPS        		"${CMAKE_CURRENT_SOURCE_DIR}/module/__init__.py")
set(BUILD_STAMP 		"${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

set(SWIG_INTERFACES "${CMAKE_CURRENT_SOURCE_DIR}/showtime.i")
set(SWIG_INTERFACES_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/showtime.i")
configure_file(${SWIG_INTERFACES} ${SWIG_INTERFACES_DESTINATION} COPYONLY)

set(SHOWTIME_TARGET "@SHOWTIME_TARGET@")
configure_file(${SETUP_PY_IN} ${SETUP_PY_PARTIAL})
set(SHOWTIME_TARGET '$<TARGET_FILE:${PROJECT_NAME}>')
get_filename_component(SETUPPY_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/showtime-final.cmake REALPATH)

#configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT ${SETUP_PY}
					COMMAND ${CMAKE_COMMAND}
						-D SUBST_SOURCE=${SETUP_PY_PARTIAL}
						-D SUBST_DEST=${SETUP_PY}
						-D SHOWTIME_TARGET=${SHOWTIME_TARGET}
						-P ${SETUPPY_PARSER}
                	DEPENDS ${SETUP_PY_PARTIAL}
                	VERBATIM)

add_custom_command(OUTPUT ${BUILD_STAMP}
                	COMMAND ${PYTHON} ${SETUP_PY} build
                	COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
                	DEPENDS ${SETUP_PY} ${DEPS} ${PROJECT_NAME})

add_custom_target(INSTALL_PYTHON ALL DEPENDS ${BUILD_STAMP})

install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")