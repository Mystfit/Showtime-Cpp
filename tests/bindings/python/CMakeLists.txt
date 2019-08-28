set(PYTHON_TEST_TARGET "TestPython")
message(STATUS "Adding test ${PYTHON_TEST_TARGET}")

if(MSVC)
	set(PYPROJ_IN "${CMAKE_CURRENT_LIST_DIR}/TestPython.pyproj.in")
	set(PYPROJ_OUT "${CMAKE_CURRENT_BINARY_DIR}/TestPython.pyproj")
	configure_file(${PYPROJ_IN} ${PYPROJ_OUT})
    if(ADD_GENERATED_MSVC_PROJECTS)
        include_external_msproject(${PYTHON_TEST_TARGET} ${PYPROJ_OUT})
    endif()
endif()

add_dependencies(${PYTHON_TEST_TARGET} ${PYTHON_TARGET})

add_test(NAME ${PYTHON_TEST_TARGET} COMMAND ${Python_EXECUTABLE} -B ${CMAKE_CURRENT_LIST_DIR}/TestPython.py)
set_tests_properties(${PYTHON_TEST_TARGET} PROPERTIES ENVIRONMENT PYTHONPATH=${CMAKE_SWIG_ROOT_OUTDIR}/python)