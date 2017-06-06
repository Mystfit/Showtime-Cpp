set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/module/__init__.py")
set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

set(SWIG_INTERFACES "${CMAKE_CURRENT_SOURCE_DIR}/showtime.i")
set(SWIG_INTERFACES_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/showtime.i")

configure_file(${SETUP_PY_IN} ${SETUP_PY})
configure_file(${SWIG_INTERFACES} ${SWIG_INTERFACES_DESTINATION} COPYONLY)

add_custom_command(OUTPUT ${OUTPUT}
                   COMMAND ${PYTHON} ${SETUP_PY} build
                   COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
                   DEPENDS ${DEPS})

add_custom_target(INSTALL_PYTHON ALL DEPENDS ${OUTPUT})

install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")