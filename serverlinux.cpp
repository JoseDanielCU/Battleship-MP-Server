#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <map>
#include <set>
#include <ctime>
#include <algorithm>
#include <signal.h>
#include <cstring>
#include "game_common.h"

#define MAX_CLIENTS 100

std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'},
    {"Battleship", 4, 'B'},
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'},
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'},
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}
};

struct Game {
    std::string player1, player2;
    Board board1, board2;
    std::string current_turn;
    bool active;

    Game(const std::string& p1, const std::string& p2)
        : player1(p1), player2(p2), current_turn(p1), active(true) {}
};

struct ServerState {
    std::mutex log_mutex;
    std::mutex game_mutex;
    std::mutex user_mutex;
    std::mutex matchmaking_mutex;

    std::map<std::string, std::string> user_db;
    std::set<std::string> active_users;
    std::set<std::string> matchmaking_queue;
    std::map<std::string, int> user_sockets;
    std::map<std::string, Game*> user_games;
    std::vector<Game*> games;

    std::string log_path;
};

void log_event(ServerState& state, const std::string& event) {
    std::lock_guard<std::mutex> lock(state.log_mutex);
    std::ofstream log_file(state.log_path, std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(nullptr);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0';
        log_file << "[" << dt << "] " << event << std::endl;
    }
}

void load_users(ServerState& state) {
    std::ifstream file("usuarios.txt");
    std::string username, password;
    while (file >> username >> password) {
        state.user_db[username] = password;
    }
}

void save_user(const std::string& username, const std::string& password) {
    std::ofstream file("usuarios.txt", std::ios::app);
    file << username << " " << password << std::endl;
}

void start_game(const std::string& player1, const std::string& player2, ServerState& state) {
    std::lock_guard<std::mutex> lock(state.game_mutex);
    Game* game = new Game(player1, player2);
    state.games.push_back(game);
    state.user_games[player1] = game;
    state.user_games[player2] = game;
    log_event(state, "Partida iniciada: " + player1 + " vs " + player2);

    if (state.user_sockets.count(player1)) {
        int result = send(state.user_sockets[player1], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player1 + ": " + std::string(strerror(errno)));
        }
    }
    if (state.user_sockets.count(player2)) {
        int result = send(state.user_sockets[player2], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player2 + ": " + std::string(strerror(errno)));
        }
    }
}

void matchmaking(ServerState& state) {
    while (true) {
        std::string player1, player2;
        {
            std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
            if (state.matchmaking_queue.size() >= 2) {
                auto it = state.matchmaking_queue.begin();
                player1 = *it;
                state.matchmaking_queue.erase(it);
                it = state.matchmaking_queue.begin();
                player2 = *it;
                state.matchmaking_queue.erase(it);
            }
        }
        if (!player1.empty() && !player2.empty()) {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            if (state.user_sockets.count(player1) && state.user_sockets.count(player2)) {
                start_game(player1, player2, state);
            } else {
                std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
                if (state.user_sockets.count(player1)) {
                    state.matchmaking_queue.insert(player1);
                }
                if (state.user_sockets.count(player2)) {
                    state.matchmaking_queue.insert(player2);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void process_protocols(const std::string& command, const std::string& param1, const std::string& param2, int client_socket, std::string& logged_user, ServerState& state) {
    if (command == "REGISTER") {
        std::string username = param1;
        std::string password = param2;
        std::lock_guard<std::mutex> lock(state.user_mutex);
        if (!state.user_db.count(username)) {
            state.user_db[username] = password;
            save_user(username, password);
            send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0);
            log_event(state, "Usuario registrado: " + username);
        } else {
            send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0);
            log_event(state, "Intento de registro fallido: " + username);
        }
    } else if (command == "LOGIN") {
        std::string username = param1;
        std::string password = param2;
        std::lock_guard<std::mutex> lock(state.user_mutex);
        if (!state.user_db.count(username) || state.user_db[username] != password) {
            send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0);
            log_event(state, "Intento de login fallido: Credenciales incorrectas - " + username);
        } else if (state.active_users.count(username)) {
            send(client_socket, "LOGIN|ERROR|YA_CONECTADO", strlen("LOGIN|ERROR|YA_CONECTADO"), 0);
            log_event(state, "Intento de login fallido: Usuario ya conectado - " + username);
        } else {
            state.active_users.insert(username);
            state.user_sockets[username] = client_socket;
            logged_user = username;
            send(client_socket, "LOGIN|SUCCESSFUL", strlen("LOGIN|SUCCESSFUL"), 0);
            log_event(state, "Usuario logueado: " + username);
        }
    } else if (command == "PLAYERS") {
        std::string players_list = "LISTA DE JUGADORES:\n";
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            std::lock_guard<std::mutex> game_lock(state.game_mutex);
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
            for (const auto& user : state.active_users) {
                std::string status = "(Conectado)";
                if (state.user_games.count(user)) {
                    status = "(En juego)";
                } else if (state.matchmaking_queue.count(user)) {
                    status = "(Buscando partida)";
                }
                players_list += user + " " + status + "\n";
            }
        }
        if (send(client_socket, players_list.c_str(), players_list.size(), 0) == -1) {
            log_event(state, "Error enviando lista de jugadores a " + logged_user + ": " + std::string(strerror(errno)));
        } else {
            log_event(state, "Lista de jugadores enviada a " + logged_user);
        }
    } else if (command == "QUEUE") {
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
        if (!state.user_games.count(logged_user)) {
            state.matchmaking_queue.insert(logged_user);
            send(client_socket, "QUEUE|OK", strlen("QUEUE|OK"), 0);
            log_event(state, logged_user + " ha entrado en cola para jugar");
        } else {
            send(client_socket, "QUEUE|ERROR|IN_GAME", strlen("QUEUE|ERROR|IN_GAME"), 0);
        }
    } else if (command == "CANCEL_QUEUE") {
        std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
        state.matchmaking_queue.erase(logged_user);
        send(client_socket, "CANCEL_QUEUE|OK", strlen("CANCEL_QUEUE|OK"), 0);
        log_event(state, logged_user + " salió de la cola de emparejamiento.");
    } else if (command == "CHECK_MATCH") {
        std::lock_guard<std::mutex> lock(state.game_mutex);
        if (state.user_games.count(logged_user)) {
            send(client_socket, "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        } else {
            send(client_socket, "MATCH|NOT_FOUND", strlen("MATCH|NOT_FOUND"), 0);
        }
    } else if (command == "LOGOUT") {
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        if (state.user_games.count(logged_user)) {
            Game* game = state.user_games[logged_user];
            std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1;
            if (state.user_sockets.count(opponent)) {
                send(state.user_sockets[opponent], "GAME|WIN", strlen("GAME|WIN"), 0);
                log_event(state, opponent + " gana por abandono de " + logged_user);
            }
            state.user_games.erase(game->player1);
            state.user_games.erase(game->player2);
            auto it = std::find(state.games.begin(), state.games.end(), game);
            if (it != state.games.end()) {
                state.games.erase(it);
            }
            delete game;
        }
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
            state.active_users.erase(logged_user);
            state.user_sockets.erase(logged_user);
            state.matchmaking_queue.erase(logged_user);
        }
        send(client_socket, "LOGOUT|OK", strlen("LOGOUT|OK"), 0);
        log_event(state, "Usuario deslogueado: " + logged_user);
    } else if (command == "BOARD") {
        std::string board_data = param1;
        log_event(state, "Intento de procesar BOARD de " + logged_user);

        std::string player1, player2, current_turn;
        bool both_boards_ready = false;

        {
            std::lock_guard<std::mutex> lock(state.game_mutex);
            auto game_it = state.user_games.find(logged_user);
            if (game_it == state.user_games.end()) {
                log_event(state, "Error: " + logged_user + " no está en un juego activo");
                send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0);
                return;
            }

            Game* game = game_it->second;
            log_event(state, "Tablero recibido de " + logged_user + " para juego " + game->player1 + " vs " + game->player2);

            bool was_board1_empty = game->board1.countShips() == 0;
            bool was_board2_empty = game->board2.countShips() == 0;

            if (game->player1 == logged_user) {
                if (!was_board1_empty) {
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado");
                }
                game->board1.deserialize(board_data);
            } else if (game->player2 == logged_user) {
                if (!was_board2_empty) {
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado");
                }
                game->board2.deserialize(board_data);
            } else {
                log_event(state, "Error: " + logged_user + " no es jugador en este juego");
                send(client_socket, "INVALID|NOT_PLAYER", strlen("INVALID|NOT_PLAYER"), 0);
                return;
            }

            int expected_ship_length = 0;
            for (const auto& ship : ships) {
                expected_ship_length += ship.size;
            }
            if (game->board1.countShips() == expected_ship_length && game->board2.countShips() == expected_ship_length) {
                both_boards_ready = true;
                player1 = game->player1;
                player2 = game->player2;
                current_turn = game->current_turn;
            }
        }

        if (both_boards_ready) {
            std::string turn_msg = "TURN|" + current_turn;
            log_event(state, "Enviando TURN a ambos jugadores: " + turn_msg);
            if (state.user_sockets.count(player1)) {
                int result = send(state.user_sockets[player1], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player1 + ": " + std::string(strerror(errno)));
                }
            }
            if (state.user_sockets.count(player2)) {
                int result = send(state.user_sockets[player2], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player2 + ": " + std::string(strerror(errno)));
                }
            }
            log_event(state, "Ambos tableros listos, turno inicial: " + current_turn);
        }
    } else if (command == "FIRE") {
        int x_coord = std::stoi(param1);
        int y_coord = std::stoi(param2);
        std::lock_guard<std::mutex> lock(state.game_mutex);
        if (!state.user_games.count(logged_user)) {
            send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0);
            return;
        }
        Game* game = state.user_games[logged_user];
        if (game->current_turn != logged_user || !game->active) {
            send(client_socket, "INVALID|NOT_YOUR_TURN", strlen("INVALID|NOT_YOUR_TURN"), 0);
            return;
        }

        Board& target_board = (game->player1 == logged_user) ? game->board2 : game->board1;
        std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1;

        if (state.user_sockets.count(opponent)) {
            std::string attack_msg = "ATTACKED|" + param1 + "|" + param2;
            int result = send(state.user_sockets[opponent], attack_msg.c_str(), attack_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando ATTACKED a " + opponent + ": " + std::string(strerror(errno)));
            } else {
                log_event(state, "ATTACKED enviado a " + opponent + ": " + attack_msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (target_board.isHit(x_coord, y_coord)) {
            target_board.setHit(x_coord, y_coord);

            std::string hit_msg = "HIT|" + param1 + "|" + param2;
            int result = send(client_socket, hit_msg.c_str(), hit_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando HIT a " + logged_user + ": " + std::string(strerror(errno)));
            } else {
                log_event(state, logged_user + " acertó en (" + param1 + "," + param2 + ") contra " + opponent);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            std::string sunk_ship = target_board.getSunkShipName(x_coord, y_coord);
            if (!sunk_ship.empty()) {
                std::string sunk_msg = "SUNK|" + sunk_ship;
                result = send(client_socket, sunk_msg.c_str(), sunk_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando SUNK a " + logged_user + ": " + std::string(strerror(errno)));
                } else {
                    log_event(state, logged_user + " hundió " + sunk_ship + " de " + opponent);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            if (target_board.allShipsSunk()) {
                std::string win_msg = "GAME|WIN";
                int result = send(client_socket, win_msg.c_str(), win_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando GAME|WIN a " + logged_user + ": " + std::string(strerror(errno)));
                } else {
                    log_event(state, "GAME|WIN enviado a " + logged_user);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (state.user_sockets.count(opponent)) {
                    std::string lose_msg = "GAME|LOSE";
                    result = send(state.user_sockets[opponent], lose_msg.c_str(), lose_msg.size(), 0);
                    if (result == -1) {
                        log_event(state, "Error enviando GAME|LOSE a " + opponent + ": " + std::string(strerror(errno)));
                    } else {
                        log_event(state, "GAME|LOSE enviado a " + opponent);
                    }
                }
                log_event(state, "Partida finalizada: " + logged_user + " ganó contra " + opponent);

                state.user_games.erase(game->player1);
                state.user_games.erase(game->player2);
                auto it = std::find(state.games.begin(), state.games.end(), game);
                if (it != state.games.end()) {
                    state.games.erase(it);
                }
                delete game;

                if (state.user_sockets.count(logged_user)) {
                    send(state.user_sockets[logged_user], "GAME|ENDED", strlen("GAME|ENDED"), 0);
                }
                if (state.user_sockets.count(opponent)) {
                    send(state.user_sockets[opponent], "GAME|ENDED", strlen("GAME|ENDED"), 0);
                }
                return;
            }
        } else {
            target_board.setMiss(x_coord, y_coord);
            std::string miss_msg = "MISS|" + param1 + "|" + param2;
            int result = send(client_socket, miss_msg.c_str(), miss_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando MISS a " + logged_user + ": " + std::string(strerror(errno)));
            } else {
                log_event(state, logged_user + " falló en (" + param1 + "," + param2 + ") contra " + opponent);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        game->current_turn = opponent;
        std::string turn_msg = "TURN|" + opponent;
        log_event(state, "Preparando envío de TURN a ambos jugadores: " + turn_msg);

        if (state.user_sockets.count(logged_user)) {
            int result = send(state.user_sockets[logged_user], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando TURN a " + logged_user + ": " + std::string(strerror(errno)));
            } else {
                log_event(state, "TURN enviado a " + logged_user + ": " + turn_msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (state.user_sockets.count(opponent)) {
            int result = send(state.user_sockets[opponent], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando TURN a " + opponent + ": " + std::string(strerror(errno)));
                send(state.user_sockets[logged_user], "GAME|WIN", strlen("GAME|WIN"), 0);
                state.user_games.erase(game->player1);
                state.user_games.erase(game->player2);
                auto it = std::find(state.games.begin(), state.games.end(), game);
                if (it != state.games.end()) {
                    state.games.erase(it);
                }
                delete game;
                return;
            } else {
                log_event(state, "TURN enviado a " + opponent + ": " + turn_msg);
            }
        } else {
            std::string win_msg = "GAME|WIN";
            int result = send(state.user_sockets[logged_user], win_msg.c_str(), win_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando GAME|WIN por desconexión a " + logged_user + ": " + std::string(strerror(errno)));
            }
            log_event(state, "Oponente " + opponent + " desconectado, victoria para " + logged_user);
            state.user_games.erase(game->player1);
            state.user_games.erase(game->player2);
            auto it = std::find(state.games.begin(), state.games.end(), game);
            if (it != state.games.end()) {
                state.games.erase(it);
            }
            delete game;
            return;
        }
    }
}

void handle_client(int client_socket, ServerState& state) {
    char buffer[4096] = {0};
    std::string logged_user;

    while (true) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            if (!logged_user.empty()) {
                process_protocols("LOGOUT", logged_user, "", client_socket, logged_user, state);
                log_event(state, "Usuario desconectado y deslogueado: " + logged_user + " (recv falló con " +
                          std::to_string(bytes_read) + ", error: " + std::string(strerror(errno)) + ")");
                std::cout << "Cliente desconectado y deslogueado: " << logged_user << std::endl;
            } else {
                log_event(state, "Cliente anónimo desconectado (recv falló con " + std::to_string(bytes_read) +
                          ", error: " + std::string(strerror(errno)) + ")");
                std::cout << "Cliente anónimo desconectado." << std::endl;
            }
            close(client_socket);
            return;
        }

        buffer[bytes_read] = '\0';
        std::string message(buffer, bytes_read);
        std::istringstream msg_stream(message);
        std::string command, param1, param2;
        std::getline(msg_stream, command, '|');
        std::getline(msg_stream, param1, '|');
        std::getline(msg_stream, param2);

        if (command == "LOGIN" && !logged_user.empty()) {
            int flags = fcntl(client_socket, F_GETFL, 0);
            fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        }

        process_protocols(command, param1, param2, client_socket, logged_user, state);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT> </path/log.log>" << std::endl;
        return 1;
    }

    ServerState state;

    std::string ip_address = argv[1];
    int port = std::stoi(argv[2]);
    state.log_path = argv[3];

    std::cout << "Iniciando servidor Battleship..." << std::endl;

    load_users(state);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error al crear socket: " << strerror(errno) << std::endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Error al configurar SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr);

    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "Error al bind: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        std::cerr << "Error al listen: " << strerror(errno) << std::endl;
        close(server_socket);
        return 1;
    }

    std::cout << "Servidor iniciado y escuchando en " << ip_address << ":" << port << std::endl;

    std::thread matchmaking_thread(matchmaking, std::ref(state));
    matchmaking_thread.detach();
    std::cout << "Hilo de matchmaking iniciado." << std::endl;

    std::vector<std::thread> client_threads;
    while (true) {
        int new_socket = accept(server_socket, nullptr, nullptr);
        if (new_socket == -1) {
            std::cerr << "Error al aceptar conexión: " << strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Nuevo cliente conectado." << std::endl;
        client_threads.emplace_back(handle_client, new_socket, std::ref(state));

        if (client_threads.size() > 10) {
            std::vector<std::thread> active_threads;
            for (auto& t : client_threads) {
                if (t.joinable()) {
                    t.join();
                } else {
                    active_threads.push_back(std::move(t));
                }
            }
            client_threads = std::move(active_threads);
        }
    }

    close(server_socket);
    return 0;
}