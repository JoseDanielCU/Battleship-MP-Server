cmake_minimum_required(VERSION 3.10)
project(Battleship_MP_Project)

set(CMAKE_CXX_STANDARD 20)

add_executable(servidor server.cpp)
add_executable(cliente client.cpp)
add_custom_target(run_server
        COMMAND ${CMAKE_BINARY_DIR}/servidor 127.0.0.1 8080 log.log
        DEPENDS servidor
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_link_libraries(servidor ws2_32)
target_link_libraries(cliente ws2_32)
