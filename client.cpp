#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")-

void log_event(const std::string& clientIP, const std::string& query, const std::string& response) {
    std::ofstream log_file("log.log", std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
        struct tm tstruct;
        char date_buf[11];   // YYYY-MM-DD
        char time_buf[9];    // HH:MM:SS
        localtime_s(&tstruct, &now);
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

        log_file << date_buf << " " << time_buf << " " << clientIP << " " << query << " " << response << std::endl;
        log_file.close();
    }
}

std::string send_message(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    if (message.empty()) return "";

    send(client_socket, message.c_str(), message.size(), 0);
    std::cout << "Mensaje enviado: " << message << std::endl;

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';

    if (bytes_received <= 0) {
        std::cerr << "Error recibiendo mensaje. Código: " << WSAGetLastError() << std::endl;
        return "";
    }

    std::string response(buffer, bytes_received);
    std::cout << "Mensaje recibido: " << response << std::endl;

    log_event(clientIP, message, response);

    return response;
}

void waiting_animation(bool& esperando) {
    const char anim[4] = {'.', '.', '.', ' '};
    int i = 0;
    while (esperando) {
        std::cout << "\rEsperando oponente" << anim[i % 4] << " " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        i++;
    }
    std::cout << "\r                      \r" << std::flush;
}

void start_game(SOCKET client_socket, const std::string& username) {
    std::cout << "\n--- La partida ha comenzado ---\n";
    bool game_active = true;

    std::thread listener_thread([&]() {
        while (game_active) {
            char buffer[1024] = {0};
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) break;
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            if (message == "GAME|WIN") {
                std::cout << "\n¡Perdiste la partida! El oponente fue más rápido.\n";
                game_active = false;
            } else if (message == "GAME|LOSE") {
                std::cout << "\n¡Ganaste la partida!\n";
                game_active = false;
            }
        }
    });

    while (game_active) {
        std::string input;
        std::getline(std::cin, input);
        if (input == "ganar") {
            send(client_socket, "GAME|WIN", strlen("GAME|WIN"), 0);
            break;
        }
        std::cout << "Debes escribir \"ganar\" para ganar la partida.\n";
    }

    listener_thread.join();

    std::cout << "\nVolviendo al menú principal...\n";
}






bool login_menu(SOCKET client_socket, std::string& username, const std::string& clientIP) {
    while (true) {
        std::cout << "\n1. Registrarse\n2. Iniciar sesión\n3. Salir" << std::endl;
        std::cout << "Seleccione una opción: ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "3") return false;

        if (input != "1" && input != "2") {
            std::cout << "Opción no válida. Intente de nuevo." << std::endl;
            continue;
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

        std::string query = (input == "1") ? "REGISTER|" + username + "|" + password : "LOGIN|" + username + "|" + password;
        std::string response = send_message(client_socket, clientIP, query);

        if (response.find("LOGIN|SUCCESSFUL") != std::string::npos) {
            return true;
        }
    }
}

void search_game(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    send_message(client_socket, clientIP, "QUEUE|" + username);
    bool queue = true;
    bool match_found = false;

    std::thread animation(waiting_animation, std::ref(queue));
    auto start_time = std::chrono::steady_clock::now();

    while (queue) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed_time).count() >= 15) {
            queue = false;
            std::cout << "\nNo se encontró oponente, saliendo de la cola..." << std::endl;
            send_message(client_socket, clientIP, "CANCEL_QUEUE|" + username);
            break;
        }

        std::string response = send_message(client_socket, clientIP, "CHECK_MATCH|" + username);
        if (response.find("MATCH|FOUND") != std::string::npos) {
            queue = false;
            match_found = true;
            std::cout << "\n¡Oponente encontrado! La partida comenzará." << std::endl;
            break;
        }
    }

    if (animation.joinable()) {
        animation.join();
    }

    if (match_found) {
        start_game(client_socket, username);
    }
}

void menu_logged_in(SOCKET client_socket, const std::string& clientIP, std::string& username) {
    while (true) {
        std::cout << "\n1. Ver jugadores conectados\n2. Entrar en Cola\n3. Cerrar Sesión" << std::endl;
        std::cout << "Seleccione una opción: ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "1") {
            send_message(client_socket, clientIP, "PLAYERS|");
        } else if (input == "2") {
            search_game(client_socket, clientIP, username);
        } else if (input == "3") {
            send_message(client_socket, clientIP, "LOGOUT|" + username);
            break;
        } else {
            std::cout << "Opción no válida." << std::endl;
        }
    }
}
std::string get_local_ip() {
    char hostname[NI_MAXHOST];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error al obtener hostname: " << WSAGetLastError() << std::endl;
        return "UNKNOWN";
    }

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET; // Solo IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0) {
        std::cerr << "Error en getaddrinfo: " << WSAGetLastError() << std::endl;
        return "UNKNOWN";
    }

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    freeaddrinfo(res);

    return std::string(ip_str);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cin.tie(nullptr);
    const std::string server_ip = "127.0.0.1";
    const int server_port = 8080;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al iniciar Winsock." << std::endl;
        return 1;
    }

    std::string clientIP = get_local_ip();

    SOCKET client_socket;
    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);

    bool connected = false;
    auto start_time = std::chrono::steady_clock::now();

    std::cout << "Intentando conectar con el servidor..." << std::endl;

    while (!connected) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == 0) {
            connected = true;
            break;
        }

        closesocket(client_socket);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 15) {
            std::cerr << "No fue posible conectar con el servidor después de 15 segundos." << std::endl;
            WSACleanup();
            return 1;
        }

        std::cout << "." << std::flush;
    }

    std::cout << "\nConexión establecida con éxito." << std::endl;

    while (true) {
        std::string username;
        if (!login_menu(client_socket, username, clientIP)) break;
        menu_logged_in(client_socket, clientIP, username);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}

