#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <windows.h>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")
#define BOARD_SIZE 10
#define WATER '~'
#define HIT 'X'
#define MISS 'O'

struct Ship {
    std::string name;
    int size;
    char symbol;
};

std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'},
    {"Battleship", 4, 'B'},
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'},
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'},
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}
};

void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void resetConsoleColor() {
    setConsoleColor(7); // Blanco (color por defecto)
}

class Board {
public:
    std::vector<std::vector<char>> grid;
    struct ShipInstance {
        std::string name;
        char symbol;
        std::vector<std::pair<int, int>> positions;
        bool sunk;
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    std::vector<ShipInstance> ship_instances;

    Board() {
        grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
    }

    void display(bool hide_ships = false, bool is_opponent = false) {
        std::vector<std::string> ship_summary = getShipSummaryLines(is_opponent);
        std::cout << "\n  ";
        for (int i = 0; i < BOARD_SIZE; i++) std::cout << " " << i << " ";
        std::cout << "   | ";
        if (!ship_summary.empty()) std::cout << ship_summary[0];
        std::cout << "\n +";
        for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
        std::cout << "+  |";

        size_t ship_line = 1;
        for (int i = 0; i < BOARD_SIZE; i++) {
            std::cout << "\n" << i << "|";
            for (int j = 0; j < BOARD_SIZE; j++) {
                char c = grid[i][j];
                if (hide_ships && c != HIT && c != MISS && c != WATER) c = WATER;
                if (c == HIT) {
                    setConsoleColor(12); // Rojo
                    std::cout << " X ";
                    resetConsoleColor();
                } else if (c == MISS) {
                    setConsoleColor(9); // Azul
                    std::cout << " O ";
                    resetConsoleColor();
                } else if (c == WATER) {
                    setConsoleColor(11); // Cian
                    std::cout << " ~ ";
                    resetConsoleColor();
                } else {
                    setConsoleColor(10); // Verde
                    std::cout << " " << c << " ";
                    resetConsoleColor();
                }
            }
            std::cout << "|  |";
            if (ship_line < ship_summary.size()) {
                std::cout << ship_summary[ship_line];
                ship_line++;
            }
        }
        std::cout << "\n +";
        for (int i = 0; i < BOARD_SIZE; i++) std::cout << "---";
        std::cout << "+  |";
        for (; ship_line < ship_summary.size(); ship_line++) {
            std::cout << "\n   " << std::string(BOARD_SIZE * 3, ' ') << "   |" << ship_summary[ship_line];
        }
        std::cout << "\n";
    }

    bool canPlaceShip(int x, int y, int size, char direction) {
        if (direction == 'H') {
            if (y + size > BOARD_SIZE) return false;
            for (int i = 0; i < size; i++)
                if (grid[x][y + i] != WATER) return false;
        } else {
            if (x + size > BOARD_SIZE) return false;
            for (int i = 0; i < size; i++)
                if (grid[x + i][y] != WATER) return false;
        }
        return true;
    }

    void placeShip(int x, int y, int size, char direction, char symbol) {
        for (int i = 0; i < size; i++) {
            if (direction == 'H')
                grid[x][y + i] = symbol;
            else
                grid[x + i][y] = symbol;
        }
    }

    void trackShip(const Ship& ship, int x, int y, char direction) {
        std::vector<std::pair<int, int>> positions;
        for (int i = 0; i < ship.size; i++) {
            int pos_x = (direction == 'H') ? x : x + i;
            int pos_y = (direction == 'H') ? y + i : y;
            positions.emplace_back(pos_x, pos_y);
        }
        ship_instances.emplace_back(ship.name, ship.symbol, positions);
    }

    void updateSunkStatus(int x, int y) {
        for (auto& ship : ship_instances) {
            if (!ship.sunk) {
                for (const auto& pos : ship.positions) {
                    if (pos.first == x && pos.second == y) {
                        bool all_hit = true;
                        for (const auto& p : ship.positions) {
                            if (grid[p.first][p.second] != HIT) {
                                all_hit = false;
                                break;
                            }
                        }
                        if (all_hit) {
                            ship.sunk = true;
                        }
                        break;
                    }
                }
            }
        }
    }

    std::vector<std::string> getShipSummaryLines(bool opponent = false) {
        std::vector<std::string> lines;
        std::string header = opponent ? "Barcos del oponente:" : "Tus barcos:";
        lines.push_back(header);
        for (const auto& ship : ship_instances) {
            std::string line = ship.name + " (" + std::to_string(ship.positions.size()) + "): ";
            line += ship.sunk ? "Hundido" : "A flote";
            lines.push_back(line);
        }
        return lines;
    }

    void manualPlacement() {
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                system("cls");
                display();
                std::cout << "Colocando " << ship.name << " (" << ship.size << " casillas)." << std::endl;

                std::cout << "Ingrese fila inicial (0-9): ";
                std::string input;
                std::getline(std::cin, input);

                try {
                    x = std::stoi(input);
                    if (x < 0 || x >= BOARD_SIZE) {
                        std::cout << "Fila inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                std::cout << "Ingrese columna inicial (0-9): ";
                std::getline(std::cin, input);

                try {
                    y = std::stoi(input);
                    if (y < 0 || y >= BOARD_SIZE) {
                        std::cout << "Columna inválida. Debe estar entre 0 y 9. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                        continue;
                    }
                } catch (const std::exception&) {
                    std::cout << "Entrada no numérica. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }

                std::cout << "Ingrese dirección (H/V): ";
                std::getline(std::cin, input);
                if (input.empty()) {
                    std::cout << "Entrada vacía. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                    continue;
                }
                direction = toupper(input[0]);

                if (direction == 'H' || direction == 'V') {
                    if (canPlaceShip(x, y, ship.size, direction)) {
                        placeShip(x, y, ship.size, direction, ship.symbol);
                        trackShip(ship, x, y, direction);
                        placed = true;
                    } else {
                        std::cout << "No se puede colocar el barco ahí. Presione Enter para intentar de nuevo.";
                        std::cin.get();
                    }
                } else {
                    std::cout << "Dirección inválida. Use 'H' o 'V'. Presione Enter para intentar de nuevo.";
                    std::cin.get();
                }
            }
        }
    }

    void randomPlacement() {
        srand(time(0));
        for (const auto &ship : ships) {
            int x, y;
            char direction;
            bool placed = false;

            while (!placed) {
                x = rand() % BOARD_SIZE;
                y = rand() % BOARD_SIZE;
                direction = (rand() % 2 == 0) ? 'H' : 'V';

                if (canPlaceShip(x, y, ship.size, direction)) {
                    placeShip(x, y, ship.size, direction, ship.symbol);
                    trackShip(ship, x, y, direction);
                    placed = true;
                }
            }
        }
    }

    std::string serialize() {
        std::string data;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                data += grid[i][j];
            }
        }
        return data;
    }
};

void draw_menu(const std::string& title, const std::vector<std::string>& options) {
    system("cls");
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
    char date_buf[11];
    char time_buf[9];
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
        return "ERROR|CONNECTION_LOST";
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer, bytes_received);
    log_event(clientIP, message, response);

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

void setup_board(Board& board) {
    system("cls");
    std::cout << "==================================\n";
    std::cout << "  CONFIGURACIÓN DEL TABLERO\n";
    std::cout << "==================================\n";
    std::cout << "Desea colocar los barcos manualmente (M) o aleatoriamente (A)? ";
    std::string choice;
    std::getline(std::cin, choice);
    choice = toupper(choice.empty() ? 'A' : choice[0]);

    if (choice == "M") {
        board.manualPlacement();
    } else {
        board.randomPlacement();
    }

    system("cls");
    std::cout << "Tu tablero:\n";
    board.display();
    std::cout << "Tablero configurado. Presione Enter para continuar...";
    std::cin.get();
}

void start_game(SOCKET client_socket, const std::string& clientIP, const std::string& username) {
    Board player_board, opponent_board;
    setup_board(player_board);

    std::vector<std::string> event_log;
    auto add_event = [&event_log](const std::string& event) {
        event_log.push_back(event);
        if (event_log.size() > 5) event_log.erase(event_log.begin());
    };

    std::string board_data = player_board.serialize();
    std::string response = send_message(client_socket, clientIP, "BOARD|" + board_data);
    if (response == "ERROR|CONNECTION_LOST") {
        show_error("Conexión perdida al enviar el tablero.");
        return;
    }
    add_event("Tablero enviado al servidor.");

    std::cout << "Tablero enviado. Esperando inicio del juego...\n";

    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        log_event(clientIP, "INITIAL_TURN", "ERROR|CONNECTION_LOST");
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
        player_board.display(false, false);
        std::cout << "\nTablero del oponente:\n";
        opponent_board.display(true, true);
        std::cout << "\nÚltimos eventos:\n";
        for (const auto& event : event_log) {
            std::cout << "- " << event << "\n";
        }
        std::cout << "==================================\n";
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
                    log_event(clientIP, "GAME_LISTEN", "ERROR|CONNECTION_LOST");
                    show_error("Conexión perdida durante el juego.");
                    game_active = false;
                    break;
                }
            } else if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string message(buffer);
                log_event(clientIP, "RECEIVED", message);

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
                player_board.display(false, false);
                std::cout << "\nTablero del oponente:\n";
                opponent_board.display(true, true);
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
                        log_event(clientIP, "FIRE", "ERROR|SEND_FAILED: " + std::to_string(WSAGetLastError()));
                        show_error("Error al enviar disparo al servidor.");
                    } else {
                        log_event(clientIP, "FIRE", fire_msg);
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
            if (response == "LOGIN|SUCCESSFUL") {
                show_success("¡Bienvenido, " + username + "!");
                return true;
            } else if (response == "LOGIN|ERROR|YA_CONECTADO") {
                show_error("Este usuario ya está conectado.");
            } else if (response == "LOGIN|ERROR") {
                show_error("Usuario o contraseña incorrectos.");
            } else if (response == "REGISTER|SUCCESSFUL") {
                show_success("¡Usuario " + username + " registrado con éxito!");
                return false;
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

std::string get_local_ip() {
    char hostname[NI_MAXHOST];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error al obtener hostname: " << WSAGetLastError() << std::endl;
        return "UNKNOWN";
    }

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
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
    const std::string server_ip = "172.26.73.33";
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