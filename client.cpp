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
    std::cout << "Mensaje enviado: " << message << std::endl;  // DEBUG

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);  // -1 para asegurar terminador nulo
    buffer[bytes_received] = '\0';  // Asegurar que el string termina correctamente

    if (bytes_received <= 0) {
        std::cerr << "Error recibiendo mensaje. Código: " << WSAGetLastError() << std::endl;
        return "";
    }

    std::string response(buffer, bytes_received);
    std::cout << "Mensaje recibido: " << response << std::endl;  // DEBUG
    log_event(clientIP, message, SERVER_IP);

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

void iniciar_partida(SOCKET client_socket, const std::string& username) {
    // 1. Mostrar el contenido del archivo "boardSetup.txt"
    std::ifstream boardFile("boardSetup.cpp");
    if (boardFile.is_open()) {
        std::stringstream buffer;
        buffer << boardFile.rdbuf();
        std::string boardSetupContent = buffer.str();
        // Enviar el contenido al cliente
        send(client_socket, boardSetupContent.c_str(), boardSetupContent.length(), 0);
        boardFile.close();
    } else {
        std::string errorMsg = "Error: No se pudo abrir el archivo boardSetup.txt.\n";
        send(client_socket, errorMsg.c_str(), errorMsg.length(), 0);
        return;
    }

    // 2. Permitir al jugador crear su tablero
    // Enviar instrucciones para que el usuario ingrese su configuración de tablero.
    std::string prompt = "\nIngrese su configuracion de tablero linea por linea.\n"
                         "Cuando finalice, escriba 'FIN' en una linea nueva.\n";
    send(client_socket, prompt.c_str(), prompt.length(), 0);

    // Recibir la configuracion del tablero desde el cliente.
    // Se utiliza un bucle simple para recibir datos hasta detectar la palabra clave "FIN".
    char buffer[1024];
    std::string boardConfig;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            // Error o desconexión
            return;
        }
        buffer[bytes_read] = '\0';
        std::string input(buffer);
        // Agregar la entrada a la configuracion
        boardConfig += input;
        // Si se detecta "FIN" en la entrada, se asume que la configuración ha finalizado.
        if (input.find("FIN") != std::string::npos) {
            break;
        }
    }

    // Opcional: Aquí podrías guardar o procesar boardConfig según lo necesites.
    std::cout << "Configuracion recibida para " << username << ":\n" << boardConfig << std::endl;

    // 3. Notificar que se espera al rival
    std::string waitingMsg = "\nConfiguracion recibida. Esperando a que el rival finalice su configuracion de tablero...\n";
    send(client_socket, waitingMsg.c_str(), waitingMsg.length(), 0);
}

bool login_menu(SOCKET client_socket, std::string& username, const std::string& clientIP) {
    while (true) {
        std::cout << "\n1. Registrarse\n2. Iniciar sesión\n3. Salir" << std::endl;
        std::cout << "Seleccione una opción: ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "3") return false;  // Salir del programa

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

    std::thread animacion(waiting_animation, std::ref(queue));
    auto start_time = std::chrono::steady_clock::now();

    while (queue) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed_time).count() >= 5) {
            queue = false;
            std::cout << "\nNo se encontró oponente, saliendo de la cola..." << std::endl;
            send_message(client_socket, clientIP, "CANCEL_QUEUE|" + username);
            break;
        }

        std::string response = send_message(client_socket, clientIP, "CHECK_MATCH|" + username);
        if (response.find("MATCH_FOUND|") != std::string::npos) {
            queue = false;
            match_found = true;
            std::cout << "\n¡Oponente encontrado! La partida comenzará." << std::endl;
            break;
        }
    }

    if (animacion.joinable()) {
        animacion.join();
    }

    if (match_found) {
        iniciar_partida(client_socket, username);
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

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
    connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    char clientIP[NI_MAXHOST];
    gethostname(clientIP, NI_MAXHOST);

    while (true) {
        std::string username;
        if (!login_menu(client_socket, username, clientIP)) break;
        menu_logged_in(client_socket, clientIP, username);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
