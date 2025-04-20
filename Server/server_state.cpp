#include "server_state.h"
// Incluye la biblioteca para manejo de archivos, como std::ofstream.
#include <cstring>
// Incluye la biblioteca para manejo de tiempo, como time(nullptr).
#include <ctime>

// Constructor que inicializa una partida con dos jugadores.
Game::Game(const std::string& p1, const std::string& p2)
    : player1(p1), player2(p2), current_turn(p1), active(true) {}

// Registra un evento en el archivo de log.
void log_event(ServerState& state, const std::string& event) {
    std::lock_guard<std::mutex> lock(state.log_mutex);
    std::ofstream log_file(state.log_path, std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(nullptr);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0'; // Elimina el salto de línea.
        log_file << "[" << dt << "] " << event << std::endl;
    }
}

// Carga la base de datos de usuarios desde un archivo.
void load_users(ServerState& state) {
    std::ifstream file("usuarios.txt");
    std::string username, password;
    while (file >> username >> password) {
        state.user_db[username] = password;
    }
}

// Guarda un nuevo usuario en la base de datos.
void save_user(const std::string& username, const std::string& password) {
    std::ofstream file("usuarios.txt", std::ios::app);
    file << username << " " << password << std::endl;
}