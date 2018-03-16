find_package(PythonInterp REQUIRED)
add_test(NAME TestPython
         COMMAND ${PYTHON_EXECUTABLE}
         ${CMAKE_CURRENT_BINARY_DIR}/TestPython.py
         TestPython
)
