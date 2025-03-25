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
#include <queue>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 100

std::vector<std::thread> client_threads;
std::mutex log_mutex;
std::map<std::string, std::string> user_db;
std::set<std::string> active_users;
std::queue<std::string> matchmaking_queue;
std::map<std::string, SOCKET> user_sockets;
std::map<std::string, bool> in_game;
std::map<std::string, std::string> game_sessions;
std::string log_path;

void log_event(const std::string& event) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream log_file(log_path, std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0';
        log_file << "[" << dt << "] " << event << std::endl;
        log_file.close();
    }
}


void load_users() {
    std::ifstream file("usuarios.txt");
    std::string username, password;
    while (file >> username >> password) {
        user_db[username] = password;
    }
    file.close();
}

void save_user(const std::string& username, const std::string& password) {
    std::ofstream file("usuarios.txt", std::ios::app);
    file << username << " " << password << std::endl;
    file.close();
}
void start_game(const std::string& player1, const std::string& player2) {
    log_event("Juego iniciado entre " + player1 + " y " + player2);

    std::string start_msg = "GAME_START|" + player1 + " vs " + player2;
    send(user_sockets[player1], start_msg.c_str(), start_msg.length(), 0);
    send(user_sockets[player2], start_msg.c_str(), start_msg.length(), 0);

    while (in_game[player1] && in_game[player2]) {
        char buffer[1024] = {0};
        int bytes_read = recv(user_sockets[player1], buffer, sizeof(buffer), 0);
        std::string message(buffer, bytes_read);

        if (message == "SURRENDER") {
            in_game[player1] = false;
            in_game[player2] = false;
            send(user_sockets[player1], "GAME_OVER|Te has rendido, perdiste", strlen("GAME_OVER|Te has rendido, perdiste"), 0);
            send(user_sockets[player2], "GAME_OVER|Tu oponente se ha rendido, has ganado", strlen("GAME_OVER|Tu oponente se ha rendido, has ganado"), 0);
            log_event(player1 + " se ha rendido, " + player2 + " gana");
            return;
        }
    }
}
void handle_client(SOCKET client_socket) {
    char buffer[1024] = {0};
    std::string logged_user = "";

    while (true) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            std::cout << "Cliente desconectado." << std::endl;
            if (!logged_user.empty()) {
                active_users.erase(logged_user);
                user_sockets.erase(logged_user);
                in_game.erase(logged_user);
                log_event("Usuario desconectado: " + logged_user);
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

        if (command == "REGISTER") {
            if (user_db.find(username) == user_db.end()) {
                user_db[username] = password;
                save_user(username, password);
                send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0);
                log_event("Usuario registrado: " + username);
            } else {
                send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0);
                log_event("Intento de registro fallido: " + username);
            }
        } else if (command == "LOGIN") {
            if (user_db.find(username) != user_db.end() && user_db[username] == password) {
                if (active_users.find(username) != active_users.end()) {
                    send(client_socket, "LOGIN|ERROR|YA_CONECTADO", strlen("LOGIN|ERROR|YA_CONECTADO"), 0);
                    log_event("Intento de login fallido: Usuario ya conectado - " + username);
                } else {
                    active_users.insert(username);
                    user_sockets[username] = client_socket;
                    logged_user = username;
                    send(client_socket, "LOGIN|SUCCESSFUL", strlen("LOGIN|SUCCESSFUL"), 0);
                    log_event("Usuario logueado: " + username);
                }
            } else {
                send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0);
                log_event("Intento de login fallido: Credenciales incorrectas - " + username);
            }
        } else if (command == "PLAYERS") {
            std::string players_list = "PLAYERS|";
            for (const auto& user : active_users) {
                players_list += user ;
            }
            send(client_socket, players_list.c_str(), players_list.size(), 0);
            log_event("Lista de jugadores enviada a " + logged_user);
        } else if (command == "LOGOUT") {
            active_users.erase(logged_user);
            user_sockets.erase(logged_user);
            send(client_socket, "LOGOUT|OK", strlen("LOGOUT|OK"), 0);
            log_event("Usuario deslogueado: " + logged_user);
        }
    }
    closesocket(client_socket);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT> </path/log.log>" << std::endl;
        return 1;
    }

    std::string ip_address = argv[1];
    int port = std::stoi(argv[2]);
    log_path = argv[3];

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    load_users();

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Error al enlazar el socket." << std::endl;
        return 1;
    }

    listen(server_socket, MAX_CLIENTS);
    std::cout << "Servidor en " << ip_address << ":" << port << std::endl;
    log_event("Servidor iniciado en " + ip_address + ":" + std::to_string(port));

    while (true) {
        SOCKET new_socket = accept(server_socket, nullptr, nullptr);
        client_threads.emplace_back(handle_client, new_socket);
    }
}

