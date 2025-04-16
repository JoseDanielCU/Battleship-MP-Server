#include "game_manager.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include "ui.h"
#include "network.h"

void GameManager::startGame(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    Board player_board, opponent_board;
    UI::setupBoard(player_board);

    std::vector<std::string> event_log;
    auto add_event = [&event_log](const std::string& event) {
        event_log.push_back(event);
        if (event_log.size() > 5) event_log.erase(event_log.begin());
    };

    std::string board_data = player_board.serialize();
    std::string response = Network::sendMessage(client_socket, clientIP, "BOARD|" + board_data);
    if (response == "ERROR|CONNECTION_LOST") {
        UI::showError("Conexión perdida al enviar el tablero.");
        return;
    }
    add_event("Tablero enviado al servidor.");

    std::cout << "Tablero enviado. Esperando inicio del juego...\n";

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        Network::logEvent(clientIP, "INITIAL_TURN", "ERROR|CONNECTION_LOST");
        UI::showError("Conexión perdida al esperar el turno inicial.");
        return;
    }
    buffer[bytes_received] = '\0';
    std::string initial_message(buffer);
    Network::logEvent(clientIP, "INITIAL_TURN", initial_message);
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
        UI::displayBoard(player_board, false, false);
        std::cout << "\nTablero del oponente:\n";
        UI::displayBoard(opponent_board, true, true);
        std::cout << "\nÚltimos eventos:\n";
        for (const auto& event : event_log) {
            std::cout << "- " << event << "\n";
        }
        std::cout << "==================================\n";
    } else {
        UI::showError("No se recibió el turno inicial esperado. Mensaje: " + initial_message);
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
                    Network::logEvent(clientIP, "GAME_LISTEN", "ERROR|CONNECTION_LOST");
                    UI::showError("Conexión perdida durante el juego.");
                    game_active = false;
                    break;
                }
            } else if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string message(buffer);
                Network::logEvent(clientIP, "RECEIVED", message);

                std::istringstream iss(message);
                std::getline(iss, command, '|');
                std::getline(iss, arg1, '|');
                std::getline(iss, arg2, '|');

                received_message = true;
                waiting_message_displayed = false;

                if (command == "TURN") {
                    my_turn = (arg1 == username);
                } else if (command == "ATTACKED") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    if (player_board.grid[x][y] != WATER && player_board.grid[x][y] != HIT && player_board.grid[x][y] != MISS) {
                        player_board.grid[x][y] = HIT;
                        player_board.updateSunkStatus(x, y);
                        add_event("Oponente acertó en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                    } else {
                        player_board.grid[x][y] = MISS;
                        add_event("Oponente falló en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                    }
                } else if (command == "HIT") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    opponent_board.grid[x][y] = HIT;
                    opponent_board.updateSunkStatus(x, y);
                    add_event("¡Acertaste en (" + std::to_string(x) + "," + std::to_string(y) + ")!");
                } else if (command == "MISS") {
                    int x = std::stoi(arg1), y = std::stoi(arg2);
                    opponent_board.grid[x][y] = MISS;
                    add_event("Fallaste en (" + std::to_string(x) + "," + std::to_string(y) + ").");
                } else if (command == "SUNK") {
                    for (auto& ship : opponent_board.ship_instances) {
                        if (ship.name == arg1 && !ship.sunk) {
                            ship.sunk = true;
                            break;
                        }
                    }
                    add_event("¡Hundiste un " + arg1 + "!");
                } else if (command == "GAME") {
                    system("cls");
                    if (arg1 == "WIN") {
                        add_event("¡Ganaste la partida!");
                        std::cout << "==================================\n";
                        std::cout << "  ¡GANASTE LA PARTIDA!\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "LOSE") {
                        add_event("Perdiste la partida.");
                        std::cout << "==================================\n";
                        std::cout << "  ¡Perdiste! El oponente ganó.\n";
                        std::cout << "==================================\n";
                    } else if (arg1 == "ENDED") {
                        add_event("Partida finalizada.");
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

                system("cls");
                std::cout << "==================================\n";
                std::cout << (my_turn ? "  TU TURNO" : "  TURNO DEL OPONENTE") << "\n";
                std::cout << "==================================\n";
                std::cout << "Tu tablero:\n";
                UI::displayBoard(player_board, false, false);
                std::cout << "\nTablero del oponente:\n";
                UI::displayBoard(opponent_board, true, true);
                std::cout << "\nÚltimos eventos:\n";
                for (const auto& event : event_log) {
                    std::cout << "- " << event << "\n";
                }
                std::cout << "==================================\n";
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
                        Network::logEvent(clientIP, "FIRE", "ERROR|SEND_FAILED: " + std::to_string(WSAGetLastError()));
                        UI::showError("Error al enviar disparo al servidor.");
                    } else {
                        Network::logEvent(clientIP, "FIRE", fire_msg);
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

bool GameManager::loginMenu(SOCKET client_socket, std::string& username, const std::string& clientIP) {
    while (true) {
        UI::drawMenu("MENÚ PRINCIPAL", {"Registrarse", "Iniciar sesión", "Salir"});
        std::string input;
        std::getline(std::cin, input);

        if (input == "3") return false;

        if (input != "1" && input != "2") {
            UI::showError("Opción no válida.");
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
            UI::showError("Usuario y contraseña no pueden estar vacíos ni contener '|'.");
            continue;
        }

        std::string query = (input == "1") ? "REGISTER|" + username + "|" + password : "LOGIN|" + username + "|" + password;
        try {
            std::string response = Network::sendMessage(client_socket, clientIP, query);
            if (response == "LOGIN|SUCCESSFUL") {
                UI::showSuccess("¡Bienvenido, " + username + "!");
                return true;
            } else if (response == "LOGIN|ERROR|YA_CONECTADO") {
                UI::showError("Este usuario ya está conectado.");
            } else if (response == "LOGIN|ERROR") {
                UI::showError("Usuario o contraseña incorrectos.");
            } else if (response == "REGISTER|SUCCESSFUL") {
                UI::showSuccess("¡Usuario " + username + " registrado con éxito!");
            } else if (response == "REGISTER|ERROR") {
                UI::showError("El usuario ya existe.");
            } else {
                UI::showError("Error desconocido: " + response);
            }
        } catch (const std::runtime_error& e) {
            UI::showError(e.what());
            return false;
        }
    }
}

void GameManager::searchGame(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    std::string response = Network::sendMessage(client_socket, clientIP, "QUEUE|" + username);
    if (response != "QUEUE|OK") {
        UI::showError("Error al entrar en cola: " + response);
        return;
    }

    bool queue = true;
    bool match_found = false;

    std::thread animation(UI::waitingAnimation, std::ref(queue));
    auto start_time = std::chrono::steady_clock::now();

    while (queue) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed_time).count() >= 15) {
            queue = false;
            std::cout << "\nNo se encontró oponente, saliendo de la cola..." << std::endl;
            Network::sendMessage(client_socket, clientIP, "CANCEL_QUEUE|" + username);
            break;
        }

        response = Network::sendMessage(client_socket, clientIP, "CHECK_MATCH|" + username);
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
        startGame(client_socket, clientIP, username);
    }
}

void GameManager::menuLoggedIn(SOCKET client_socket, const std::string& clientIP, std::string& username) {
    while (true) {
        UI::drawMenu("MENÚ DE JUEGO - " + username, {
            "Ver jugadores conectados",
            "Entrar en Cola",
            "Cerrar Sesión"
        });
        std::string input;
        std::getline(std::cin, input);

        if (input == "1") {
            try {
                std::string response = Network::sendMessage(client_socket, clientIP, "PLAYERS|");
                if (response == "ERROR|CONNECTION_LOST") {
                    UI::showError("Conexión perdida con el servidor al intentar listar jugadores. Volviendo al menú principal...");
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
                UI::showError(std::string(e.what()) + " Volviendo al menú principal...");
                return;
            }
        } else if (input == "2") {
            searchGame(client_socket, clientIP, username);
        } else if (input == "3") {
            std::string response = Network::sendMessage(client_socket, clientIP, "LOGOUT|" + username);
            if (response == "ERROR|CONNECTION_LOST") {
                UI::showError("Conexión perdida al intentar cerrar sesión. Volviendo al menú principal...");
            }
            break;
        } else {
            std::cout << "Opción no válida. Presione Enter para continuar...";
            std::cin.get();
        }
    }
}