#pragma once

// Incluye la biblioteca para mecanismos de sincronización, como std::mutex.
#include <mutex>
// Incluye la biblioteca para contenedores asociativos, como std::map.
#include <map>
// Incluye la biblioteca para contenedores de conjuntos, como std::set.
#include <set>
// Incluye la biblioteca para contenedores de vectores, como std::vector.
#include <vector>
// Incluye la biblioteca para manipulación de cadenas, como std::string.
#include <string>
// Incluye la biblioteca para manejo de archivos, como std::ofstream.
#include <fstream>
#include "game_common.h"// Incluye la definición del tablero y barcos.







// Estructura que representa una partida de Battleship.
struct Game {
    std::string player1;    // Nombre del primer jugador.
    std::string player2;    // Nombre del segundo jugador.
    Board board1;           // Tablero del primer jugador.
    Board board2;           // Tablero del segundo jugador.
    std::string current_turn; // Jugador que tiene el turno actual.
    bool active;            // Indica si la partida está activa.

    // Constructor que inicializa una partida con dos jugadores.
    Game(const std::string& p1, const std::string& p2);
};
// Estructura que representa el estado global del servidor.
struct ServerState {
    std::mutex log_mutex;             // Mutex para sincronizar el acceso al log.
    std::mutex game_mutex;            // Mutex para sincronizar el acceso a los juegos.
    std::mutex user_mutex;            // Mutex para sincronizar el acceso a los usuarios.
    std::mutex matchmaking_mutex;     // Mutex para sincronizar el emparejamiento.
    std::map<std::string, std::string> user_db; // Base de datos de usuarios (username, password).
    std::set<std::string> active_users;         // Usuarios actualmente conectados.
    std::set<std::string> matchmaking_queue;    // Cola de usuarios buscando partida.
    std::map<std::string, int> user_sockets;    // Mapeo de usuarios a sus sockets.
    std::map<std::string, Game*> user_games;    // Mapeo de usuarios a sus juegos activos.
    std::vector<Game*> games;                   // Lista de juegos activos.
    std::string log_path;                       // Ruta del archivo de log.
};

// Registra un evento en el archivo de log.
void log_event(ServerState& state, const std::string& event);
// Carga la base de datos de usuarios desde un archivo.
void load_users(ServerState& state);
// Guarda un nuevo usuario en la base de datos.
void save_user(const std::string& username, const std::string& password);