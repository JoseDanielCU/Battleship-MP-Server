#include "protocols.h"
// Incluye la biblioteca estándar de C++ para entrada/salida, como std::cout y std::cin.
#include <iostream>
// Incluye la biblioteca para manipulación de flujos de cadenas, como std::stringstream.
#include <sstream>
// Incluye la biblioteca para algoritmos genéricos, como std::ranges::find.
#include <algorithm>
// Incluye la biblioteca para sockets, permitiendo operaciones de red como send().
#include <sys/socket.h>
// Incluye la biblioteca para manejo de cadenas de C, como strlen().
#include <cstring>
// Incluye la biblioteca para manejo de errores, como strerror().
#include <errno.h>
// Incluye la biblioteca para manejo de hilos, permitiendo el uso de std::thread.
#include <thread>

// Procesa los comandos recibidos desde un cliente.
void process_protocols(const std::string& command, const std::string& param1, const std::string& param2, int client_socket, std::string& logged_user, ServerState& state) {
    if (command == "REGISTER") {
        // Registrar un nuevo usuario.
        std::string username = param1;
        std::string password = param2;
        std::lock_guard<std::mutex> lock(state.user_mutex);
        if (!state.user_db.contains(username)) {
            state.user_db[username] = password;
            save_user(username, password);
            send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0);
            log_event(state, "Usuario registrado: " + username);
        } else {
            send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0);
            log_event(state, "Intento de registro fallido: " + username);
        }
    } else if (command == "LOGIN") {
        // Iniciar sesión de un usuario.
        std::string username = param1;
        std::string password = param2;
        std::lock_guard<std::mutex> lock(state.user_mutex);
        if (!state.user_db.contains(username) || state.user_db[username] != password) {
            send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0);
            log_event(state, "Intento de login fallido: Credenciales incorrectas - " + username);
        } else if (state.active_users.contains(username)) {
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
        // Enviar lista de jugadores conectados.
        std::string players_list = "LISTA DE JUGADORES:\n";
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            std::lock_guard<std::mutex> game_lock(state.game_mutex);
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
            for (const auto& user : state.active_users) {
                std::string status = "(Conectado)";
                if (state.user_games.contains(user)) {
                    status = "(En juego)";
                } else if (state.matchmaking_queue.contains(user)) {
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
        // Entrar en la cola de emparejamiento.
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
        if (!state.user_games.contains(logged_user)) {
            state.matchmaking_queue.insert(logged_user);
            send(client_socket, "QUEUE|OK", strlen("QUEUE|OK"), 0);
            log_event(state, logged_user + " ha entrado en cola para jugar");
        } else {
            send(client_socket, "QUEUE|ERROR|IN_GAME", strlen("QUEUE|ERROR|IN_GAME"), 0);
        }
    } else if (command == "CANCEL_QUEUE") {
        // Cancelar la cola de emparejamiento.
        std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
        state.matchmaking_queue.erase(logged_user);
        send(client_socket, "CANCEL_QUEUE|OK", strlen("CANCEL_QUEUE|OK"), 0);
        log_event(state, logged_user + " salió de la cola de emparejamiento.");
    } else if (command == "CHECK_MATCH") {
        // Verificar si se ha encontrado una partida.
        std::lock_guard<std::mutex> lock(state.game_mutex);
        if (state.user_games.contains(logged_user)) {
            send(client_socket, "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        } else {
            send(client_socket, "MATCH|NOT_FOUND", strlen("MATCH|NOT_FOUND"), 0);
        }
    } else if (command == "LOGOUT") {
        // Cerrar sesión del usuario.
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        if (state.user_games.contains(logged_user)) {
            Game* game = state.user_games[logged_user];
            std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1;
            if (state.user_sockets.contains(opponent)) {
                send(state.user_sockets[opponent], "GAME|WIN", strlen("GAME|WIN"), 0);
                log_event(state, opponent + " gana por abandono de " + logged_user);
            }
            state.user_games.erase(game->player1);
            state.user_games.erase(game->player2);
            auto it = std::ranges::find(state.games, game);
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
        // Procesar el tablero enviado por un jugador.
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
            if (state.user_sockets.contains(player1)) {
                int result = send(state.user_sockets[player1], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player1 + ": " + std::string(strerror(errno)));
                }
            }
            if (state.user_sockets.contains(player2)) {
                int result = send(state.user_sockets[player2], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player2 + ": " + std::string(strerror(errno)));
                }
            }
            log_event(state, "Ambos tableros listos, turno inicial: " + current_turn);
        }
    } else if (command == "FIRE") {
        // Procesar un disparo en el tablero del oponente.
        int x_coord = std::stoi(param1);
        int y_coord = std::stoi(param2);
        std::lock_guard<std::mutex> lock(state.game_mutex);
        if (!state.user_games.contains(logged_user)) {
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
        if (state.user_sockets.contains(opponent)) {
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
                if (state.user_sockets.contains(opponent)) {
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
                auto it = std::ranges::find(state.games, game);
                if (it != state.games.end()) {
                    state.games.erase(it);
                }
                delete game;
                if (state.user_sockets.contains(logged_user)) {
                    send(state.user_sockets[logged_user], "GAME|ENDED", strlen("GAME|ENDED"), 0);
                }
                if (state.user_sockets.contains(opponent)) {
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
        if (state.user_sockets.contains(logged_user)) {
            int result = send(state.user_sockets[logged_user], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando TURN a " + logged_user + ": " + std::string(strerror(errno)));
            } else {
                log_event(state, "TURN enviado a " + logged_user + ": " + turn_msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (state.user_sockets.contains(opponent)) {
            int result = send(state.user_sockets[opponent], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == -1) {
                log_event(state, "Error enviando TURN a " + opponent + ": " + std::string(strerror(errno)));
                send(state.user_sockets[logged_user], "GAME|WIN", strlen("GAME|WIN"), 0);
                state.user_games.erase(game->player1);
                state.user_games.erase(game->player2);
                auto it = std::ranges::find(state.games, game);
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
            auto it = std::ranges::find(state.games, game);
            if (it != state.games.end()) {
                state.games.erase(it);
            }
            delete game;
            return;
        }
    }
}