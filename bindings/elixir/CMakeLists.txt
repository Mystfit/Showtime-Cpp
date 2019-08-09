message(STATUS "Configuring ${ELIXIR_PORTS_TARGET}")

set(PORTS_SRC "${CMAKE_CURRENT_LIST_DIR}/ShowtimePorts.cpp")

set(PORTS_DEPENDENCIES 
    ${CLIENT_TARGET}
    ${SERVER_TARGET}
)

find_package(erlang)

add_executable (${ELIXIR_PORTS_TARGET}  ${PORTS_SRC})
target_link_libraries(${ELIXIR_PORTS_TARGET} PUBLIC ${PORTS_DEPENDENCIES} erlang)
add_dependencies(${ELIXIR_PORTS_TARGET} ${PORTS_DEPENDENCIES})
