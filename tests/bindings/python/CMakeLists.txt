message(STATUS "Adding python test")

add_test(NAME TestPython
         COMMAND ${PYTHON_EXECUTABLE}
         ${CMAKE_CURRENT_LIST_DIR}/TestPython.py
         TestPython
)
