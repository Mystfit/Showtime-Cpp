# Build dotnet tests
set(UNITY_TEST_TARGET TestUnity)

message(STATUS "Adding test ${UNITY_TEST_TARGET}")

add_test(NAME ${UNITY_TEST_TARGET} COMMAND ${UNITY_EXECUTABLE} -nographics -batchmode -projectPath ${UNITY_BUILD_DIR} -editorTestsResultFile ${CMAKE_CURRENT_BINARY_DIR} -runEditorTests)
