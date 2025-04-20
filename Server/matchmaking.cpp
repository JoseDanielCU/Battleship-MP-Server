#include "matchmaking.h"
// Incluye la biblioteca para manejo de hilos, permitiendo el uso de std::thread.
#include <thread>
// Incluye la biblioteca para sockets, permitiendo operaciones de red como send().
#include <sys/socket.h>
// Incluye la biblioteca para manejo de cadenas de C, como strlen().
#include <cstring>
// Incluye la biblioteca para manejo de errores, como strerror().
#include <errno.h>

// Inicia una nueva partida entre dos jugadores.
void start_game(const std::string& player1, const std::string& player2, ServerState& state) {
    std::lock_guard<std::mutex> lock(state.game_mutex);
    Game* game = new Game(player1, player2);
    state.games.push_back(game);
    state.user_games[player1] = game;
    state.user_games[player2] = game;
    log_event(state, "Partida iniciada: " + player1 + " vs " + player2);
    // Notifica a ambos jugadores que la partida ha comenzado.
    if (state.user_sockets.contains(player1)) {
        int result = send(state.user_sockets[player1], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player1 + ": " + std::string(strerror(errno)));
        }
    }
    if (state.user_sockets.contains(player2)) {
        int result = send(state.user_sockets[player2], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player2 + ": " + std::string(strerror(errno)));
        }
    }
}

// Gestiona el emparejamiento de jugadores en la cola.
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
            if (state.user_sockets.contains(player1) &&
                state.user_sockets.contains(player2)) {
                start_game(player1, player2, state);
            } else {
                std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
                if (state.user_sockets.contains(player1)) {
                    state.matchmaking_queue.insert(player1);
                }
                if (state.user_sockets.contains(player2)) {
                    state.matchmaking_queue.insert(player2);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}