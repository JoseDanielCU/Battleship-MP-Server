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
void draw_menu(const std::string& title, const std::vector<std::string>& options) {
    system("cls"); // Limpiar pantalla (Windows)
    const int width = 40;
    std::string border = std::string(width, '=');

    std::cout << border << std::endl;
    std::cout << std::setw(width) << std::left << "  " + title << std::endl;
    std::cout << border << std::endl;

    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << options[i] << std::endl;
    }

    std::cout << border << std::endl;
    std::cout << "Seleccione una opción: ";
}

void log_event(const std::string& clientIP, const std::string& query, const std::string& response) {
    std::ofstream log_file("log.log", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de log." << std::endl;
        return;
    }

    time_t now = time(0);
    struct tm tstruct;
    char date_buf[11];   // YYYY-MM-DD
    char time_buf[9];    // HH:MM:SS
    localtime_s(&tstruct, &now);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

    std::string safe_query = query;
    std::string safe_response = response;
    std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
    std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

    log_file << date_buf << " " << time_buf << " " << clientIP << " "
             << safe_query << " " << safe_response << std::endl;
}

std::string send_message(SOCKET client_socket, const std::string& clientIP, const std::string& message) {
    if (message.empty()) return "";

    if (send(client_socket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
        log_event(clientIP, message, "ERROR|SEND_FAILED");
        throw std::runtime_error("Error enviando mensaje al servidor.");
    }

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        log_event(clientIP, message, "ERROR|RECV_FAILED");
        throw std::runtime_error("Error recibiendo respuesta del servidor o conexión perdida.");
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer, bytes_received);
    log_event(clientIP, message, response); // Registrar en log.log

    return response;
}
void show_error(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ERROR\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}

void show_success(const std::string& message) {
    system("cls");
    std::cout << "====================\n";
    std::cout << "  ÉXITO\n";
    std::cout << "====================\n";
    std::cout << message << "\n";
    std::cout << "Presione Enter para continuar...";
    std::cin.get();
}
void waiting_animation(bool& esperando) {
    const char* anim[] = {"[ / ]", "[ - ]", "[ \\ ]", "[ | ]"};
    int i = 0;
    while (esperando) {
        std::cout << "\rBuscando oponente... " << anim[i % 4] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        i++;
    }
    std::cout << "\r                                    \r" << std::flush;
}

void start_game(SOCKET client_socket, const std::string& username) {
    system("cls");
    std::cout << "==================================\n";
    std::cout << "  ¡PARTIDA INICIADA!\n";
    std::cout << "  Escribe 'ganar' para vencer.\n";
    std::cout << "==================================\n";

    bool game_active = true;

    std::thread listener_thread([&]() {
        while (game_active) {
            char buffer[1024] = {0};
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) break;
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            if (message == "GAME|WIN") {
                system("cls");
                std::cout << "==================================\n";
                std::cout << "  ¡Perdiste! El oponente fue más rápido.\n";
                std::cout << "==================================\n";
                game_active = false;
            } else if (message == "GAME|LOSE") {
                system("cls");
                std::cout << "==================================\n";
                std::cout << "  ¡GANASTE LA PARTIDA!\n";
                std::cout << "==================================\n";
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
        std::cout << "Debes escribir 'ganar' para ganar la partida.\n";
    }

    listener_thread.join();

    std::cout << "Volviendo al menú principal... Presione Enter.";
    std::cin.get();
}






bool login_menu(SOCKET client_socket, std::string& username, const std::string& clientIP) {
    while (true) {
        draw_menu("MENÚ PRINCIPAL", {"Registrarse", "Iniciar sesión", "Salir"});
        std::string input;
        std::getline(std::cin, input);

        if (input == "3") return false;

        if (input != "1" && input != "2") {
            show_error("Opción no válida.");
            continue;
        }

        system("cls");
        std::cout << "====================\n";
        std::cout << "  " << (input == "1" ? "REGISTRO" : "INICIO DE SESIÓN") << "\n";
        std::cout << "====================\n";
        std::cout << "Usuario: ";
        std::getline(std::cin, username);
        std::cout << "Contraseña: ";
        std::string password;
        std::getline(std::cin, password);

        if (username.empty() || password.empty() || username.find('|') != std::string::npos || password.find('|') != std::string::npos) {
            show_error("Usuario y contraseña no pueden estar vacíos ni contener '|'.");
            continue;
        }

        std::string query = (input == "1") ? "REGISTER|" + username + "|" + password : "LOGIN|" + username + "|" + password;
        try {
            std::string response = send_message(client_socket, clientIP, query);
            if (response.find("LOGIN|SUCCESSFUL") != std::string::npos) {
                show_success("¡Bienvenido, " + username + "!");
                return true;
            } else if (response == "LOGIN|ERROR|YA_CONECTADO") {
                show_error("Este usuario ya está conectado.");
            } else if (response == "LOGIN|ERROR") {
                show_error("Usuario o contraseña incorrectos.");
            } else if (response == "REGISTER|ERROR") {
                show_error("El usuario ya existe.");
            } else {
                show_error("Error desconocido: " + response);
            }
        } catch (const std::runtime_error& e) {
            show_error(e.what());
            return false; // Salir si hay un error de conexión
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
        draw_menu("MENÚ DE JUEGO - " + username, {
            "Ver jugadores conectados",
            "Entrar en Cola",
            "Cerrar Sesión"
        });
        std::string input;
        std::getline(std::cin, input);

        if (input == "1") {
            std::string response = send_message(client_socket, clientIP, "PLAYERS|");
            system("cls");
            std::cout << "====================\n";
            std::cout << "  JUGADORES CONECTADOS\n";
            std::cout << "====================\n";
            std::cout << response << "\n";
            std::cout << "Presione Enter para continuar...";
            std::cin.get();
        } else if (input == "2") {
            search_game(client_socket, clientIP, username);
        } else if (input == "3") {
            send_message(client_socket, clientIP, "LOGOUT|" + username);
            break;
        } else {
            std::cout << "Opción no válida. Presione Enter para continuar...";
            std::cin.get();
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

