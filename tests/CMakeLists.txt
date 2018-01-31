set(CLIENT_TEST_SOURCE_FILES 
    "TestClient.cpp"
)

set(SINK_TEST_SOURCE_FILES
    "TestSink.cpp"
)

# Boost flags
# On windows, compile with the following (stage/install):
#bjam.exe threading=multi architecture=x86 address-model=64 link=static runtime-link=static --with-atomic --with-system --with-filesystem --with-context --with-chrono --with-date_time --with-program_options --with-regex --with-thread
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.64 COMPONENTS system filesystem REQUIRED)

# Sink test executable
set(SINK_TEST_TARGET TestSink)
add_executable (${SINK_TEST_TARGET} ${SINK_TEST_SOURCE_FILES}) 
add_dependencies(${SINK_TEST_TARGET} ${CLIENT_TARGET} ${SERVER_TARGET})
target_include_directories(${SINK_TEST_TARGET} PUBLIC ${ZST_INCLUDES})
target_link_libraries(${SINK_TEST_TARGET} ${CLIENT_TARGET})
add_test(NAME sink_test COMMAND ${SINK_TEST_TARGET})

# CLient test executable
set(CLIENT_TEST_TARGET TestClient)
add_executable (${CLIENT_TEST_TARGET} ${CLIENT_TEST_SOURCE_FILES}) 
add_dependencies(${CLIENT_TEST_TARGET} ${CLIENT_TARGET} ${SINK_TEST_TARGET})
target_include_directories(${CLIENT_TEST_TARGET} PUBLIC 
    ${ZST_INCLUDES} 
    ${Boost_INCLUDE_DIRS})
target_link_libraries( ${CLIENT_TEST_TARGET}
    ${CLIENT_TARGET}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
if(WIN32)
    target_compile_definitions(${CLIENT_TEST_TARGET} PRIVATE -D_WIN32_WINNT=0x0501)
endif(WIN32)
add_test(NAME ${CLIENT_TEST_TARGET} COMMAND ${CLIENT_TEST_TARGET} WORKING_DIRECTORY $<TARGET_FILE_DIR:${CLIENT_TEST_TARGET}>)
