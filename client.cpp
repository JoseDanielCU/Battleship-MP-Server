#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void log_event(const std::string& clientIP, const std::string& query, const std::string& responseIP) {
    std::ofstream log_file("log.log", std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
        struct tm tstruct;
        char time_buf[80];
        localtime_s(&tstruct, &now);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %X", &tstruct);

        log_file << time_buf << " " << clientIP << " " << query << " " << responseIP << std::endl;
        log_file.close();
    }
}

std::string send_message(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    if (message.empty()) return "";
    send(client_socket, message.c_str(), message.size(), 0);

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    std::string response;
    if (bytes_received > 0) {
        response = std::string(buffer, bytes_received);
        std::cout << "Respuesta del servidor: " << response << std::endl;
        log_event(clientIP, message, SERVER_IP);
    }

    return response;
}

void animacion_espera(bool& esperando) {
    const char anim[4] = {'.', '.', '.', ' '};
    int i = 0;
    while (esperando) {
        std::cout << "\rEsperando oponente" << anim[i % 4] << " " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        i++;
    }
    std::cout << "\r                      \r" << std::flush;
}


int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error en WSAStartup" << std::endl;
        return -1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket" << std::endl;
        WSACleanup();
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Error al conectar con el servidor" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Conectado al servidor." << std::endl;

    char clientIP[NI_MAXHOST];
    gethostname(clientIP, NI_MAXHOST);

    bool running = true;

    while (running) {  // Outer loop: keeps the client running
        bool logged_in = false;
        std::string username;

        while (!logged_in) {  // Login loop
            std::cout << "\n1. Registrarse\n2. Iniciar sesión\n3. Salir" << std::endl;
            std::cout << "Seleccione una opción: ";
            std::string input;
            std::getline(std::cin, input);

            if (input.empty() || (input != "1" && input != "2" && input != "3")) {
                std::cout << "Opción no válida. Intente de nuevo." << std::endl;
                continue;
            }

            int option = std::stoi(input);
            if (option == 3) {  // Exit the entire program
                running = false;
                break;
            }

            std::cout << "Usuario: ";
            std::getline(std::cin, username);
            std::cout << "Contraseña: ";
            std::string password;
            std::getline(std::cin, password);

            if (username.empty() || password.empty()) {
                std::cout << "Error: Usuario y contraseña no pueden estar vacíos." << std::endl;
                continue;
            }

            std::string query = (option == 1) ? "REGISTER|" + username + "|" + password : "LOGIN|" + username + "|" + password;
            std::string response = send_message(client_socket, clientIP, query);

            if (response.find("LOGIN|SUCCESSFUL") != std::string::npos) {
                logged_in = true;
            }
        }

        while (logged_in) {  // Post-login loop
            std::cout << "\n1. Ver jugadores conectados\n2. Cerrar sesión" << std::endl;
            std::cout << "Seleccione una opción: ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "1") {
                send_message(client_socket, clientIP, "PLAYERS|");
            } else if (input == "2") {
                send_message(client_socket, clientIP, "LOGOUT|" + username);
                logged_in = false;  // Go back to login menu
            } else {
                std::cout << "Opción no válida." << std::endl;
            }
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
