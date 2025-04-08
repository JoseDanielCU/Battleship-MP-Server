#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <map>
#include <set>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 100

struct ServerState {
    std::mutex log_mutex;
    std::mutex game_mutex;

    std::map<std::string, std::string> user_db;
    std::set<std::string> active_users;
    std::set<std::string> matchmaking_queue;
    std::map<std::string, SOCKET> user_sockets;
    std::map<std::string, bool> in_game;

    std::string log_path;
};

void log_event(ServerState& state, const std::string& event) {
    std::lock_guard<std::mutex> lock(state.log_mutex);
    std::ofstream log_file(state.log_path, std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
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
    std::cout << "PARTIDA INICIADA: " << player1 << " vs " << player2 << std::endl;
    SOCKET socket1 = state.user_sockets[player1];
    SOCKET socket2 = state.user_sockets[player2];

    std::atomic<bool> has_winner(false);
    std::string winner;

    auto listen_player = [&](const std::string& player, SOCKET sock) {
        char buffer[1024];
        while (!has_winner) {
            int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;

            buffer[bytes] = '\0';
            std::string msg(buffer);
            std::cout << "Mensaje recibido de " << player << ": " << msg << std::endl;
            if (msg == "GAME|WIN") {
                has_winner = true;
                winner = player;
                break;
            }
        }
    };

    std::thread t1(listen_player, player1, socket1);
    std::thread t2(listen_player, player2, socket2);

    t1.join();
    t2.join();

    std::string loser = (winner == player1) ? player2 : player1;

    // Enviar resultados
    send(state.user_sockets[winner], "GAME|WIN", strlen("GAME|WIN"), 0);
    send(state.user_sockets[loser], "GAME|LOSE", strlen("GAME|LOSE"), 0);

    // Actualizar estado con protección de concurrencia
    std::lock_guard<std::mutex> lock(state.game_mutex);
    state.in_game[player1] = false;
    state.in_game[player2] = false;

    log_event(state, "Partida finalizada: Ganador " + winner + ", Perdedor " + loser);
}

void matchmaking(ServerState& state) {
    while (true) {
        std::lock_guard<std::mutex> lock(state.game_mutex); // Proteger acceso a la cola
        if (state.matchmaking_queue.size() >= 2) {
            auto it = state.matchmaking_queue.begin();
            std::string player1 = *it;
            state.matchmaking_queue.erase(it);
            it = state.matchmaking_queue.begin();
            std::string player2 = *it;
            state.matchmaking_queue.erase(it);

            // Marcar a los jugadores como "en juego"
            state.in_game[player1] = true;
            state.in_game[player2] = true;

            // Notificar a los jugadores del emparejamiento
            send(state.user_sockets[player1], ("MATCH|" + player2).c_str(), strlen("MATCH|FOUND"), 0);
            send(state.user_sockets[player2], ("MATCH|" + player1).c_str(), strlen("MATCH|FOUND"), 0);

            // Lanzar la partida en un hilo separado
            std::thread game_thread(start_game, player1, player2, std::ref(state));
            game_thread.detach(); // El hilo se ejecuta independientemente
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Evitar consumo excesivo de CPU
    }
}

void process_protocols(const std::string& command, const std::string& username, const std::string& password, SOCKET client_socket, std::string& logged_user, ServerState& state) {
    if (command == "REGISTER") {
        if (state.user_db.find(username) == state.user_db.end()) {
            state.user_db[username] = password;
            save_user(username, password);
            send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0);
            log_event(state, "Usuario registrado: " + username);
        } else {
            send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0);
            log_event(state, "Intento de registro fallido: " + username);
        }
    } else if (command == "LOGIN") {
        if (state.user_db.find(username) != state.user_db.end() && state.user_db[username] == password) {
            if (state.active_users.find(username) != state.active_users.end()) {
                send(client_socket, "LOGIN|ERROR|YA_CONECTADO", strlen("LOGIN|ERROR|YA_CONECTADO"), 0);
                log_event(state, "Intento de login fallido: Usuario ya conectado - " + username);
            } else {
                state.active_users.insert(username);
                state.user_sockets[username] = client_socket;
                logged_user = username;

                send(client_socket, "LOGIN|SUCCESSFUL", strlen("LOGIN|SUCCESSFUL"), 0);
                log_event(state, "Usuario logueado: " + username);
            }
        } else {
            send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0);
            log_event(state, "Intento de login fallido: Credenciales incorrectas - " + username);
        }
    } else if (command == "PLAYERS") {
        std::string players_list = "LISTA DE JUGADORES:\n";
        for (const auto& user : state.active_users) {
            std::string status = "(Conectado)";
            if (state.matchmaking_queue.find(user) != state.matchmaking_queue.end()) {
                status = "(Buscando partida)";
            } else if (state.in_game.find(user) != state.in_game.end() && state.in_game[user]) {
                status = "(En juego)";
            }
            players_list += user + " " + status + "\n";
        }
        send(client_socket, players_list.c_str(), players_list.size(), 0);
        log_event(state, "Lista de jugadores enviada a " + logged_user);
    } else if (command == "QUEUE") {
        state.matchmaking_queue.insert(logged_user);
        send(client_socket, "QUEUE|OK", strlen("QUEUE|OK"), 0);
        log_event(state, logged_user + " ha entrado en cola para jugar");
    } else if (command == "CANCEL_QUEUE") {
        state.matchmaking_queue.erase(logged_user);
        send(client_socket, "CANCEL_QUEUE|OK", strlen("CANCEL_QUEUE|OK"), 0);
        log_event(state, logged_user + " salió de la cola de emparejamiento.");
    } else if (command == "CHECK_MATCH") {
        if (state.in_game[logged_user]) {
            send(client_socket, "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        } else {
            send(client_socket, "MATCH|NOT_FOUND", strlen("MATCH|NOT_FOUND"), 0);
        }
    } else if (command == "LOGOUT") {
        state.active_users.erase(logged_user);
        state.user_sockets.erase(logged_user);
        send(client_socket, "LOGOUT|OK", strlen("LOGOUT|OK"), 0);
        log_event(state, "Usuario deslogueado: " + logged_user);
    }
}

void handle_client(SOCKET client_socket, ServerState& state) {
    char buffer[1024] = {0};
    std::string logged_user = "";

    while (true) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (!logged_user.empty()) {
                state.active_users.erase(logged_user);
                state.user_sockets.erase(logged_user);
                state.in_game.erase(logged_user);
                log_event(state, "Usuario desconectado: " + logged_user);
                std::cout << "Cliente desconectado: " << logged_user << std::endl;
            } else {
                std::cout << "Cliente anónimo desconectado." << std::endl;
            }
            closesocket(client_socket);
            return;
        }

        std::string message(buffer, bytes_read);
        std::istringstream msg_stream(message);
        std::string command, username, password;
        getline(msg_stream, command, '|');
        getline(msg_stream, username, '|');
        getline(msg_stream, password);

        process_protocols(command, username, password, client_socket, logged_user, state);
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

    WSAStartup(MAKEWORD(2, 2), new WSADATA());
    load_users(state);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = { AF_INET, htons(port), {} };
    inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr);
    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, MAX_CLIENTS);

    std::cout << "Servidor iniciado y escuchando en " << ip_address << ":" << port << std::endl;


    std::thread matchmaking_thread(matchmaking, std::ref(state));
    matchmaking_thread.detach();
    std::cout << "Hilo de matchmaking iniciado." << std::endl;


    std::vector<std::thread> client_threads;
    while (true) {
        SOCKET new_socket = accept(server_socket, nullptr, nullptr);
        std::cout << "Nuevo cliente conectado." << std::endl;
        client_threads.emplace_back(handle_client, new_socket, std::ref(state));
    }
}
