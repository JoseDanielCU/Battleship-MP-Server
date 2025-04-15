#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <limits>
#include "game_common.h"
#include "client_network.cpp"
#include "client_ui.cpp"

#pragma comment(lib, "ws2_32.lib")

std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'},
    {"Battleship", 4, 'B'},
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'},
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'},
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}
};

void start_game(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    Board player_board, opponent_board;
    setup_board(player_board);

    std::string board_data = player_board.serialize();
    std::string response = send_message(client_socket, clientIP, "BOARD|" + board_data);
    if (response == "ERROR|CONNECTION_LOST") {
        show_error("Conexión perdida al enviar el tablero.");
        return;
    }

    std::cout << "Tablero enviado. Esperando inicio del juego...\n";

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::ofstream log_file("log.log", std::ios::app);
        if (log_file.is_open()) {
            time_t now = time(0);
            struct tm tstruct;
            char date_buf[11];
            char time_buf[9];
            localtime_s(&tstruct, &now);
            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

            std::string safe_query = "INITIAL_TURN";
            std::string safe_response = "ERROR|CONNECTION_LOST";
            std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
            std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

            log_file << date_buf << " " << time_buf << " " << clientIP << " "
                     << safe_query << " " << safe_response << std::endl;
        }
        show_error("Conexión perdida al esperar el turno inicial.");
        return;
    }
    buffer[bytes_received] = '\0';
    std::string initial_message(buffer);
    std::istringstream iss(initial_message);
    std::string command, arg1, arg2;
    std::getline(iss, command, '|');
    std::getline(iss, arg1, '|');
    std::getline(iss, arg2, '|');

    bool game_active = true;
    bool my_turn = false;
    if (command == "TURN") {
        my_turn = (arg1 == username);
        system("cls");
        std::cout << "==================================\n";
        std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
        std::cout << "==================================\n";
        std::cout << "Tu tablero:\n";
        player_board.display();
        std::cout << "\nTablero del oponente:\n";
        opponent_board.display(true);
    } else {
        show_error("No se recibió el turno inicial esperado. Mensaje: " + initial_message);
        return;
    }

    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode);

    bool waiting_message_displayed = false;

    while (game_active) {
        bool received_message = false;
        do {
            bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received == SOCKET_ERROR) {
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    break;
                } else {
                    std::ofstream log_file("log.log", std::ios::app);
                    if (log_file.is_open()) {
                        time_t now = time(0);
                        struct tm tstruct;
                        char date_buf[11];
                        char time_buf[9];
                        localtime_s(&tstruct, &now);
                        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
                        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

                        std::string safe_query = "GAME_LISTEN";
                        std::string safe_response = "ERROR|CONNECTION_LOST";
                        std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
                        std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

                        log_file << date_buf << " " << time_buf << " " << clientIP << " "
                                 << safe_query << " " << safe_response << std::endl;
                    }
                    show_error("Conexión perdida durante el juego.");
                    game_active = false;
                    break;
                }
            } else if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string message(buffer);
                std::ofstream log_file("log.log", std::ios::app);
                if (log_file.is_open()) {
                    time_t now = time(0);
                    struct tm tstruct;
                    char date_buf[11];
                    char time_buf[9];
                    localtime_s(&tstruct, &now);
                    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
                    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

                    std::string safe_query = "RECEIVED";
                    std::string safe_response = message;
                    std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
                    std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

                    log_file << date_buf << " " << time_buf << " " << clientIP << " "
                             << safe_query << " " << safe_response << std::endl;
                }

                std::istringstream iss(message);
                std::getline(iss, command, '|');
                std::getline(iss, arg1, '|');
                std::getline(iss, arg2, '|');

                received_message = true;
                waiting_message_displayed = false;

                if (command == "TURN") {
                    my_turn = (arg1 == username);
                    system("cls");
                    std::cout << "==================================\n";
                    std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
                    std::cout << "==================================\n";
                    std::cout << "Tu tablero:\n";
                    player_board.display();
                    std::cout << "\nTablero del oponente:\n";
                    opponent_board.display(true);
                } else if (command == "ATTACKED") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    if (player_board.grid[x][y] != WATER && player_board.grid[x][y] != HIT && player_board.grid[x][y] != MISS) {
                        player_board.grid[x][y] = HIT;
                        std::cout << "El oponente acertó en (" << x << "," << y << ").\n";
                    } else {
                        player_board.grid[x][y] = MISS;
                        std::cout << "El oponente falló en (" << x << "," << y << ").\n";
                    }
                    system("cls");
                    std::cout << "==================================\n";
                    std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
                    std::cout << "==================================\n";
                    std::cout << "Tu tablero:\n";
                    player_board.display();
                    std::cout << "\nTablero del oponente:\n";
                    opponent_board.display(true);
                } else if (command == "HIT") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    opponent_board.grid[x][y] = HIT;
                    system("cls");
                    std::cout << "==================================\n";
                    std::cout << "  TU TURNO\n";
                    std::cout << "==================================\n";
                    std::cout << "¡Acertaste en (" << x << "," << y << ")!\n";
                    std::cout << "Tu tablero:\n";
                    player_board.display();
                    std::cout << "\nTablero del oponente:\n";
                    opponent_board.display(true);
                } else if (command == "MISS") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    opponent_board.grid[x][y] = MISS;
                    system("cls");
                    std::cout << "==================================\n";
                    std::cout << "  TU TURNO\n";
                    std::cout << "==================================\n";
                    std::cout << "Fallaste en (" << x << "," << y << ").\n";
                    std::cout << "Tu tablero:\n";
                    player_board.display();
                    std::cout << "\nTablero del oponente:\n";
                    opponent_board.display(true);
                } else if (command == "SUNK") {
                    system("cls");
                    std::cout << "==================================\n";
                    std::cout << "  TU TURNO\n";
                    std::cout << "==================================\n";
                    std::cout << "¡Hundiste un " << arg1 << "!\n";
                    std::cout << "Tu tablero:\n";
                    player_board.display();
                    std::cout << "\nTablero del oponente:\n";
                    opponent_board.display(true);
                } else if (command == "GAME") {
                    system("cls");
                    if (arg1 == "WIN") {
                        std::cout << "==================================\n";
                        std::cout << "  ¡GANASTE LA PARTIDA!\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "LOSE") {
                        std::cout << "==================================\n";
                        std::cout << "  ¡Perdiste! El oponente ganó.\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "ENDED") {
                        std::cout << "==================================\n";
                        std::cout << "  Partida finalizada.\n";
                        std::cout << "==================================\n";
                    }
                    std::cout << "Presione Enter para volver al menú principal...";
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cin.get();
                    game_active = false;
                    break;
                }
            }
        } while (bytes_received > 0 && game_active);

        if (my_turn) {
            std::cout << "Ingresa coordenadas para disparar (fila columna, ej: 3 4): ";
            std::string input;
            if (std::getline(std::cin, input)) {
                std::istringstream iss(input);
                int x, y;
                if (iss >> x >> y && x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && opponent_board.grid[x][y] != HIT && opponent_board.grid[x][y] != MISS) {
                    std::string fire_msg = "FIRE|" + std::to_string(x) + "|" + std::to_string(y);
                    int result = send(client_socket, fire_msg.c_str(), fire_msg.size(), 0);
                    if (result == SOCKET_ERROR) {
                        std::ofstream log_file("log.log", std::ios::app);
                        if (log_file.is_open()) {
                            time_t now = time(0);
                            struct tm tstruct;
                            char date_buf[11];
                            char time_buf[9];
                            localtime_s(&tstruct, &now);
                            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
                            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

                            std::string safe_query = "FIRE";
                            std::string safe_response = "ERROR|SEND_FAILED: " + std::to_string(WSAGetLastError());
                            std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
                            std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

                            log_file << date_buf << " " << time_buf << " " << clientIP << " "
                                     << safe_query << " " << safe_response << std::endl;
                        }
                        show_error("Error al enviar disparo al servidor.");
                    } else {
                        std::ofstream log_file("log.log", std::ios::app);
                        if (log_file.is_open()) {
                            time_t now = time(0);
                            struct tm tstruct;
                            char date_buf[11];
                            char time_buf[9];
                            localtime_s(&tstruct, &now);
                            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tstruct);
                            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tstruct);

                            std::string safe_query = "FIRE";
                            std::string safe_response = fire_msg;
                            std::replace(safe_query.begin(), safe_query.end(), ' ', '_');
                            std::replace(safe_response.begin(), safe_response.end(), ' ', '_');

                            log_file << date_buf << " " << time_buf << " " << clientIP << " "
                                     << safe_query << " " << safe_response << std::endl;
                        }
                        my_turn = false;
                    }
                } else {
                    std::cout << "Coordenadas inválidas o ya disparaste ahí. Intenta de nuevo.\n";
                }
            }
        } else if (!waiting_message_displayed && !received_message && game_active) {
            std::cout << "Esperando el turno del oponente...\n";
            waiting_message_displayed = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    mode = 0;
    ioctlsocket(client_socket, FIONBIO, &mode);

    std::cout << "Volviendo al menú principal... Presione Enter.";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
            return false;
        }
    }
}

void search_game(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    std::string response = send_message(client_socket, clientIP, "QUEUE|" + username);
    if (response != "QUEUE|OK") {
        show_error("Error al entrar en cola: " + response);
        return;
    }

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

        response = send_message(client_socket, clientIP, "CHECK_MATCH|" + username);
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
        start_game(client_socket, clientIP, username);
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
            try {
                std::string response = send_message(client_socket, clientIP, "PLAYERS|");
                if (response == "ERROR|CONNECTION_LOST") {
                    show_error("Conexión perdida con el servidor al intentar listar jugadores. Volviendo al menú principal...");
                    return;
                }
                system("cls");
                std::cout << "====================\n";
                std::cout << "  JUGADORES CONECTADOS\n";
                std::cout << "====================\n";
                std::cout << response << "\n";
                std::cout << "Presione Enter para continuar...";
                std::cin.get();
            } catch (const std::runtime_error& e) {
                show_error(std::string(e.what()) + " Volviendo al menú principal...");
                return;
            }
        } else if (input == "2") {
            search_game(client_socket, clientIP, username);
        } else if (input == "3") {
            std::string response = send_message(client_socket, clientIP, "LOGOUT|" + username);
            if (response == "ERROR|CONNECTION_LOST") {
                show_error("Conexión perdida al intentar cerrar sesión. Volviendo al menú principal...");
            }
            break;
        } else {
            std::cout << "Opción no válida. Presione Enter para continuar...";
            std::cin.get();
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cin.tie(nullptr);
    const std::string server_ip = "3.92.237.93";
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