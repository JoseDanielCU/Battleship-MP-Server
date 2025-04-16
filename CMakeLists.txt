cmake_minimum_required(VERSION 3.10)
project(Battleship_MP_Project)

set(CMAKE_CXX_STANDARD 20)

add_executable(servidor Server/server.cpp)
add_executable(cliente Client/client.cpp
        Client/game_common.h
        Client/game_common.cpp
        Client/ui.cpp
        Client/ui.h
        Client/game_manager.cpp
        Client/game_manager.h
        Client/network.cpp
        Client/network.h
        Client/game_manager.cpp
        Client/game_manager.h
        Client/network.cpp
        Client/network.h
        Client/ui.cpp
        Client/ui.h
        Client/game_common.cpp
        Client/game_common.h)
add_custom_target(run_server
        COMMAND ${CMAKE_BINARY_DIR}./servidor.exe "127.0.0.1" "8080" "${CMAKE_BINARY_DIR}/log.log"
        DEPENDS servidor
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(servidor ws2_32)
target_link_libraries(cliente ws2_32)
