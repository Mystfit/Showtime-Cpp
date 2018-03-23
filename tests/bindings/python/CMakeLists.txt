set(PYTHON_TEST_TARGET "TestPython")

message(STATUS "Configuring ${PYTHON_TEST_TARGET}")

add_test(NAME ${PYTHON_TEST_TARGET}
         COMMAND ${PYTHON_EXECUTABLE}
         ${CMAKE_CURRENT_LIST_DIR}/TestPython.py
         TestPython
)
