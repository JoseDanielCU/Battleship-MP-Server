#include "network.h"
#include "game_manager.h"
#include <iostream>
#include <csignal>

SOCKET global_client_socket = INVALID_SOCKET;
std::string global_username;
std::string global_clientIP;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        if (global_client_socket != INVALID_SOCKET && !global_username.empty()) {
            try {
                Network::sendMessage(global_client_socket, global_clientIP, "LOGOUT|" + global_username);
            } catch (const std::runtime_error&) {
                // Ignorar errores al enviar LOGOUT
            }
            Network::cleanup(global_client_socket);
        }
        exit(0);
    }
    return TRUE;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cin.tie(nullptr);
    const std::string server_ip = "3.210.99.80";
    const int server_port = 8080;

    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        std::cerr << "Error al configurar el manejador de consola." << std::endl;
        return 1;
    }

    try {
        global_clientIP = Network::getLocalIP();
        global_client_socket = Network::connectToServer(server_ip, server_port);

        while (true) {
            global_username.clear();
            if (!GameManager::loginMenu(global_client_socket, global_username, global_clientIP)) break;
            GameManager::menuLoggedIn(global_client_socket, global_clientIP, global_username);
        }

        if (!global_username.empty()) {
            Network::sendMessage(global_client_socket, global_clientIP, "LOGOUT|" + global_username);
        }
        Network::cleanup(global_client_socket);
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (global_client_socket != INVALID_SOCKET) {
            Network::cleanup(global_client_socket);
        }
        return 1;
    }

    return 0;
}