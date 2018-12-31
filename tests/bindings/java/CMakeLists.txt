# Build java tests
set(JAVA_TEST_TARGET TestJava)
message(STATUS "Configuring ${JAVA_TEST_TARGET}")

# Add showtime java lib to the classpath
get_target_property(SHOWTIME_JAR ${JAVA_TARGET} JAR_FILE)

# Create custom target to compile test java target to .class file
add_custom_target(${JAVA_TEST_TARGET})
add_custom_command(TARGET ${JAVA_TEST_TARGET} POST_BUILD
    COMMAND ${Java_JAVAC_EXECUTABLE} -cp ${SHOWTIME_JAR} -d ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/TestJava.java
    VERBATIM
)

add_test(NAME ${JAVA_TEST_TARGET} 
    COMMAND ${Java_JAVA_EXECUTABLE} -cp ".;${SHOWTIME_JAR}" -Djava.library.path=${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${JAVA_TEST_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
