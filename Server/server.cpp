// Inclusión de bibliotecas estándar de C++ para entrada/salida, hilos, contenedores, sincronización, archivos y utilidades
#include <iostream> // Para entrada/salida en consola
#include <thread> // Para manejo de hilos
#include <vector> // Para contenedores de tipo vector
#include <mutex> // Para sincronización con mutex
#include <fstream> // Para manejo de archivos
#include <winsock2.h> // Para sockets en Windows
#include <ws2tcpip.h> // Para funciones adicionales de sockets TCP/IP
#include <sstream> // Para manipulación de cadenas como flujos
#include <map> // Para contenedores de tipo mapa
#include <set> // Para contenedores de tipo conjunto
#include <ctime> // Para manejo de tiempo
#include <algorithm> // Para algoritmos estándar como find

// Vincula la librería Winsock de Windows al proyecto
#pragma comment(lib, "ws2_32.lib")

// Define el número máximo de clientes que el servidor puede manejar
#define MAX_CLIENTS 100
// Define el tamaño del tablero de juego (10x10)
constexpr int BOARD_SIZE = 10;
// Define el símbolo para casillas de agua
constexpr char WATER = '~';
// Define el símbolo para casillas acertadas (impacto)
constexpr char HIT = 'X';
// Define el símbolo para casillas fallidas
constexpr char MISS = 'O';

// Estructura que representa un barco con su nombre, tamaño y símbolo
struct Ship {
    std::string name; // Nombre del barco (ej. "Aircraft Carrier")
    int size; // Tamaño del barco en casillas
    char symbol; // Símbolo que representa el barco en el tablero
};

// Lista global de barcos disponibles para el juego
std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'}, // Portaaviones: 5 casillas
    {"Battleship", 4, 'B'}, // Acorazado: 4 casillas
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'}, // Dos cruceros: 3 casillas cada uno
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'}, // Dos destructores: 2 casillas cada uno
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'} // Tres submarinos: 1 casilla cada uno
};

// Clase que representa el tablero de un jugador
class Board {
public:
    // Matriz 10x10 que almacena el estado del tablero
    std::vector<std::vector<char>> grid;
    // Estructura interna para representar un barco colocado en el tablero
    struct ShipInstance {
        std::string name; // Nombre del barco
        char symbol; // Símbolo del barco
        std::vector<std::pair<int, int>> positions; // Posiciones ocupadas por el barco
        bool sunk; // Indica si el barco está hundido
        // Constructor de ShipInstance
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    // Lista de barcos colocados en el tablero
    std::vector<ShipInstance> ship_instances;

    // Constructor que inicializa el tablero
    Board() {
        // Redimensiona la matriz a 10x10 y la llena con WATER ('~')
        grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
    }

    // Convierte una cadena de datos en el estado del tablero
    void deserialize(const std::string& data) {
        // Recorre cada fila y columna del tablero
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                // Asigna el carácter correspondiente de la cadena al tablero
                grid[i][j] = data[i * BOARD_SIZE + j];
            }
        }
        // Reconstruye los barcos basándose en el nuevo estado
        rebuildShipInstances();
    }

    // Reconstruye la lista de barcos basándose en el tablero
    void rebuildShipInstances() {
        // Limpia la lista de barcos existentes
        ship_instances.clear();
        // Mapa que agrupa posiciones por símbolo de barco
        std::map<char, std::vector<std::pair<int, int>>> symbol_to_positions;

        // Recorre el tablero para identificar posiciones ocupadas por barcos
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                // Si la casilla no es agua, acierto o fallo, es un barco
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    // Agrega la posición al mapa según el símbolo
                    symbol_to_positions[grid[i][j]].emplace_back(i, j);
                }
            }
        }

        // Para cada tipo de barco definido globalmente
        for (const auto& ship : ships) {
            // Obtiene las posiciones asociadas al símbolo del barco
            auto& positions = symbol_to_positions[ship.symbol];
            // Obtiene el tamaño del barco
            int size = ship.size;

            // Mientras haya suficientes posiciones para formar un barco
            while (positions.size() >= size) {
                // Vector para almacenar las posiciones del barco actual
                std::vector<std::pair<int, int>> current_ship;
                // Itera sobre las posiciones disponibles
                for (auto it = positions.begin(); it != positions.end(); ) {
                    // Si es la primera posición del barco
                    if (current_ship.empty()) {
                        // Agrega la posición y la elimina de la lista
                        current_ship.push_back(*it);
                        it = positions.erase(it);
                    } else {
                        // Busca una posición adyacente a la última agregada
                        bool adjacent = false;
                        auto last = current_ship.back();
                        // Revisa las posiciones restantes
                        for (auto pos_it = it; pos_it != positions.end(); ++pos_it) {
                            // Verifica si la posición es adyacente horizontal o verticalmente
                            if ((pos_it->first == last.first && abs(pos_it->second - last.second) == 1) ||
                                (pos_it->second == last.second && abs(pos_it->first - last.first) == 1)) {
                                // Agrega la posición adyacente y la elimina
                                current_ship.push_back(*pos_it);
                                it = positions.erase(pos_it);
                                adjacent = true;
                                break;
                            }
                        }
                        // Si no se encontró posición adyacente, sale del bucle
                        if (!adjacent) break;
                    }
                    // Si se alcanzó el tamaño del barco, sale del bucle
                    if (current_ship.size() == size) break;
                }
                // Si se formó un barco completo
                if (current_ship.size() == size) {
                    // Agrega el barco a la lista de instancias
                    ship_instances.emplace_back(ship.name, ship.symbol, current_ship);
                } else {
                    // Si no se pudo formar, devuelve las posiciones al mapa
                    positions.insert(positions.begin(), current_ship.begin(), current_ship.end());
                    break;
                }
            }
        }
    }

    // Verifica si una posición contiene un barco no impactado
    bool isHit(int x, int y) const {
        return grid[x][y] != WATER && grid[x][y] != HIT && grid[x][y] != MISS;
    }

    // Obtiene el símbolo en una posición del tablero
    char getSymbol(int x, int y) const {
        return grid[x][y];
    }

    // Marca una posición como acertada y verifica hundimientos
    void setHit(int x, int y) {
        grid[x][y] = HIT;
        checkSunk(x, y);
    }

    // Marca una posición como fallida
    void setMiss(int x, int y) {
        grid[x][y] = MISS;
    }

    // Verifica si un barco se hundió tras un impacto
    void checkSunk(int x, int y) {
        // Recorre todos los barcos en el tablero
        for (auto& ship : ship_instances) {
            // Si el barco no está hundido
            if (!ship.sunk) {
                // Busca si la posición impactada pertenece al barco
                for (const auto& pos : ship.positions) {
                    if (pos.first == x && pos.second == y) {
                        // Verifica si todas las posiciones del barco están impactadas
                        bool all_hit = true;
                        for (const auto& p : ship.positions) {
                            if (grid[p.first][p.second] != HIT) {
                                all_hit = false;
                                break;
                            }
                        }
                        // Si todas las posiciones están impactadas, marca el barco como hundido
                        if (all_hit) {
                            ship.sunk = true;
                        }
                        return;
                    }
                }
            }
        }
    }

    // Obtiene el nombre de un barco hundido en una posición
    std::string getSunkShipName(int x, int y) {
        // Recorre los barcos
        for (const auto& ship : ship_instances) {
            // Si el barco está hundido
            if (ship.sunk) {
                // Verifica si la posición pertenece al barco
                for (const auto& pos : ship.positions) {
                    if (pos.first == x && pos.second == y) {
                        return ship.name;
                    }
                }
            }
        }
        // Retorna cadena vacía si no hay barco hundido en la posición
        return "";
    }

    // Verifica si todos los barcos están hundidos
    bool allShipsSunk() {
        // Recorre los barcos
        for (const auto& ship : ship_instances) {
            // Si algún barco no está hundido, retorna falso
            if (!ship.sunk) return false;
        }
        // Todos los barcos están hundidos
        return true;
    }

    // Cuenta el número total de casillas ocupadas por barcos
    int countShips() const {
        // Calcula el tamaño total esperado de todos los barcos
        int total_length = 0;
        for (const auto& ship : ships) {
            total_length += ship.size;
        }
        // Cuenta las casillas ocupadas por barcos en el tablero
        int current_length = 0;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    current_length++;
                }
            }
        }
        // Retorna el tamaño total si coincide con el esperado, sino 0
        return (current_length == total_length) ? total_length : 0;
    }
};

// Estructura que representa una partida entre dos jugadores
struct Game {
    std::string player1, player2; // Nombres de los jugadores
    Board board1, board2; // Tableros de los jugadores
    std::string current_turn; // Jugador con el turno actual
    bool active; // Indica si la partida está activa

    // Constructor que inicializa la partida
    Game(const std::string& p1, const std::string& p2)
        : player1(p1), player2(p2), current_turn(p1), active(true) {}
};

// Estructura que almacena el estado global del servidor
struct ServerState {
    std::mutex log_mutex; // Mutex para proteger el acceso al archivo de log
    std::mutex game_mutex; // Mutex para proteger operaciones con juegos
    std::mutex user_mutex; // Mutex para proteger operaciones con usuarios
    std::mutex matchmaking_mutex; // Mutex para proteger la cola de emparejamiento

    std::map<std::string, std::string> user_db; // Base de datos de usuarios (usuario:contraseña)
    std::set<std::string> active_users; // Conjunto de usuarios conectados
    std::set<std::string> matchmaking_queue; // Cola de usuarios buscando partida
    std::map<std::string, SOCKET> user_sockets; // Mapeo de usuarios a sus sockets
    std::map<std::string, Game*> user_games; // Mapeo de usuarios a sus partidas
    std::vector<Game*> games; // Lista de partidas activas

    std::string log_path; // Ruta del archivo de log
};

// Función para registrar eventos en el archivo de log
void log_event(ServerState& state, const std::string& event) {
    std::lock_guard<std::mutex> lock(state.log_mutex); // Bloquea el mutex del log
    std::ofstream log_file(state.log_path, std::ios::app); // Abre el archivo en modo append
    if (log_file.is_open()) { // Si el archivo se abrió correctamente
        time_t now = time(nullptr); // Obtiene la hora actual
        char* dt = ctime(&now); // Convierte la hora a cadena
        dt[strlen(dt) - 1] = '\0'; // Elimina el salto de línea
        log_file << "[" << dt << "] " << event << std::endl; // Escribe el evento
    }
}

// Carga los usuarios desde el archivo usuarios.txt
void load_users(ServerState& state) {
    std::ifstream file("usuarios.txt"); // Abre el archivo
    std::string username, password; // Variables para almacenar usuario y contraseña
    while (file >> username >> password) { // Lee cada par usuario-contraseña
        state.user_db[username] = password; // Agrega al mapa de usuarios
    }
}

// Guarda un nuevo usuario en usuarios.txt
void save_user(const std::string& username, const std::string& password) {
    std::ofstream file("usuarios.txt", std::ios::app); // Abre el archivo en modo append
    file << username << " " << password << std::endl; // Escribe el usuario y contraseña
}

// Inicia una nueva partida entre dos jugadores
void start_game(const std::string& player1, const std::string& player2, ServerState& state) {
    std::lock_guard<std::mutex> lock(state.game_mutex); // Bloquea el mutex de juegos
    Game* game = new Game(player1, player2); // Crea una nueva partida
    state.games.push_back(game); // Agrega la partida a la lista
    state.user_games[player1] = game; // Asocia la partida al primer jugador
    state.user_games[player2] = game; // Asocia la partida al segundo jugador
    log_event(state, "Partida iniciada: " + player1 + " vs " + player2); // Registra el evento

    // Notifica al primer jugador que se encontró una partida
    if (state.user_sockets.contains(player1)) {
        int result = send(state.user_sockets[player1], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == SOCKET_ERROR) { // Verifica errores al enviar
            log_event(state, "Error enviando MATCH a " + player1 + ": " + std::to_string(WSAGetLastError()));
        }
    }
    // Notifica al segundo jugador que se encontró una partida
    if (state.user_sockets.contains(player2)) {
        int result = send(state.user_sockets[player2], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        if (result == SOCKET_ERROR) { // Verifica errores al enviar
            log_event(state, "Error enviando MATCH a " + player2 + ": " + std::to_string(WSAGetLastError()));
        }
    }
}

// Función que maneja el emparejamiento de jugadores
void matchmaking(ServerState& state) {
    while (true) { // Bucle infinito
        std::string player1, player2; // Variables para almacenar los jugadores
        {
            std::lock_guard<std::mutex> lock(state.matchmaking_mutex); // Bloquea el mutex de emparejamiento
            if (state.matchmaking_queue.size() >= 2) { // Si hay al menos dos jugadores en la cola
                auto it = state.matchmaking_queue.begin(); // Obtiene el primer jugador
                player1 = *it;
                state.matchmaking_queue.erase(it); // Lo elimina de la cola
                it = state.matchmaking_queue.begin(); // Obtiene el segundo jugador
                player2 = *it;
                state.matchmaking_queue.erase(it); // Lo elimina de la cola
            }
        }
        if (!player1.empty() && !player2.empty()) { // Si se encontraron dos jugadores
            std::lock_guard<std::mutex> user_lock(state.user_mutex); // Bloquea el mutex de usuarios
            if (state.user_sockets.contains(player1) &&
                state.user_sockets.contains(player2)) { // Verifica que ambos estén conectados
                start_game(player1, player2, state); // Inicia la partida
            } else { // Si alguno no está conectado
                std::lock_guard<std::mutex> lock(state.matchmaking_mutex); // Bloquea el mutex de emparejamiento
                if (state.user_sockets.contains(player1)) { // Reinserta al primer jugador si está conectado
                    state.matchmaking_queue.insert(player1);
                }
                if (state.user_sockets.contains(player2)) { // Reinserta al segundo jugador si está conectado
                    state.matchmaking_queue.insert(player2);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Espera 1 segundo antes de la próxima iteración
    }
}

// Procesa los comandos recibidos de los clientes
void process_protocols(const std::string& command, const std::string& param1, const std::string& param2, SOCKET client_socket, std::string& logged_user, ServerState& state) {
    if (command == "REGISTER") { // Registro de un nuevo usuario
        std::string username = param1; // Nombre de usuario
        std::string password = param2; // Contraseña
        std::lock_guard<std::mutex> lock(state.user_mutex); // Bloquea el mutex de usuarios
        if (!state.user_db.contains(username)) { // Si el usuario no existe
            state.user_db[username] = password; // Agrega el usuario a la base de datos
            save_user(username, password); // Guarda en el archivo
            send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0); // Notifica éxito
            log_event(state, "Usuario registrado: " + username); // Registra el evento
        } else { // Si el usuario ya existe
            send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0); // Notifica error
            log_event(state, "Intento de registro fallido: " + username); // Registra el evento
        }
    } else if (command == "LOGIN") { // Inicio de sesión
        std::string username = param1; // Nombre de usuario
        std::string password = param2; // Contraseña
        std::lock_guard<std::mutex> lock(state.user_mutex); // Bloquea el mutex de usuarios
        if (!state.user_db.contains(username) || state.user_db[username] != password) { // Credenciales inválidas
            send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0); // Notifica error
            log_event(state, "Intento de login fallido: Credenciales incorrectas - " + username); // Registra el evento
        } else if (state.active_users.contains(username)) { // Usuario ya conectado
            send(client_socket, "LOGIN|ERROR|YA_CONECTADO", strlen("LOGIN|ERROR|YA_CONECTADO"), 0); // Notifica error
            log_event(state, "Intento de login fallido: Usuario ya conectado - " + username); // Registra el evento
        } else { // Inicio de sesión exitoso
            state.active_users.insert(username); // Agrega a usuarios activos
            state.user_sockets[username] = client_socket; // Asocia el socket
            logged_user = username; // Actualiza el usuario logueado
            send(client_socket, "LOGIN|SUCCESSFUL", strlen("LOGIN|SUCCESSFUL"), 0); // Notifica éxito
            log_event(state, "Usuario logueado: " + username); // Registra el evento
        }
    } else if (command == "PLAYERS") { // Solicitud de lista de jugadores
        std::string players_list = "LISTA DE JUGADORES:\n"; // Inicializa la lista
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex); // Bloquea mutex de usuarios
            std::lock_guard<std::mutex> game_lock(state.game_mutex); // Bloquea mutex de juegos
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex); // Bloquea mutex de emparejamiento
            for (const auto& user : state.active_users) { // Recorre usuarios activos
                std::string status = "(Conectado)"; // Estado por defecto
                if (state.user_games.contains(user)) { // Si está en un juego
                    status = "(En juego)";
                } else if (state.matchmaking_queue.contains(user)) { // Si está en la cola
                    status = "(Buscando partida)";
                }
                players_list += user + " " + status + "\n"; // Agrega usuario y estado
            }
        }
        if (send(client_socket, players_list.c_str(), players_list.size(), 0) == SOCKET_ERROR) { // Envía la lista
            log_event(state, "Error enviando lista de jugadores a " + logged_user + ": " + std::to_string(WSAGetLastError()));
        } else {
            log_event(state, "Lista de jugadores enviada a " + logged_user); // Registra el evento
        }
    } else if (command == "QUEUE") { // Entrar a la cola de emparejamiento
        std::lock_guard<std::mutex> game_lock(state.game_mutex); // Bloquea mutex de juegos
        std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex); // Bloquea mutex de emparejamiento
        if (!state.user_games.contains(logged_user)) { // Si el usuario no está en un juego
            state.matchmaking_queue.insert(logged_user); // Agrega a la cola
            send(client_socket, "QUEUE|OK", strlen("QUEUE|OK"), 0); // Notifica éxito
            log_event(state, logged_user + " ha entrado en cola para jugar"); // Registra el evento
        } else { // Si ya está en un juego
            send(client_socket, "QUEUE|ERROR|IN_GAME", strlen("QUEUE|ERROR|IN_GAME"), 0); // Notifica error
        }
    } else if (command == "CANCEL_QUEUE") { // Cancelar la cola de emparejamiento
        std::lock_guard<std::mutex> lock(state.matchmaking_mutex); // Bloquea mutex de emparejamiento
        state.matchmaking_queue.erase(logged_user); // Elimina al usuario de la cola
        send(client_socket, "CANCEL_QUEUE|OK", strlen("CANCEL_QUEUE|OK"), 0); // Notifica éxito
        log_event(state, logged_user + " salió de la cola de emparejamiento."); // Registra el evento
    } else if (command == "CHECK_MATCH") { // Verificar si se encontró una partida
        std::lock_guard<std::mutex> lock(state.game_mutex); // Bloquea mutex de juegos
        if (state.user_games.contains(logged_user)) { // Si el usuario está en un juego
            send(client_socket, "MATCH|FOUND", strlen("MATCH|FOUND"), 0); // Notifica que hay partida
        } else { // Si no está en un juego
            send(client_socket, "MATCH|NOT_FOUND", strlen("MATCH|NOT_FOUND"), 0); // Notifica que no hay partida
        }
    } else if (command == "LOGOUT") { // Cerrar sesión
        std::lock_guard<std::mutex> game_lock(state.game_mutex); // Bloquea mutex de juegos
        if (state.user_games.contains(logged_user)) { // Si el usuario está en un juego
            Game* game = state.user_games[logged_user]; // Obtiene la partida
            std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1; // Identifica al oponente
            if (state.user_sockets.contains(opponent)) { // Si el oponente está conectado
                send(state.user_sockets[opponent], "GAME|WIN", strlen("GAME|WIN"), 0); // Concede victoria al oponente
                log_event(state, opponent + " gana por abandono de " + logged_user); // Registra el evento
            }
            // Limpia el juego
            state.user_games.erase(game->player1); // Elimina la partida para el primer jugador
            state.user_games.erase(game->player2); // Elimina la partida para el segundo jugador
            auto it = std::ranges::find(state.games, game); // Busca el juego en la lista
            if (it != state.games.end()) { // Si se encontró
                state.games.erase(it); // Lo elimina
            }
            delete game; // Libera la memoria
        }
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex); // Bloquea mutex de usuarios
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex); // Bloquea mutex de emparejamiento
            state.active_users.erase(logged_user); // Elimina al usuario de activos
            state.user_sockets.erase(logged_user); // Elimina el socket del usuario
            state.matchmaking_queue.erase(logged_user); // Elimina al usuario de la cola
        }
        send(client_socket, "LOGOUT|OK", strlen("LOGOUT|OK"), 0); // Notifica éxito
        log_event(state, "Usuario deslogueado: " + logged_user); // Registra el evento
    } else if (command == "BOARD") { // Procesar el tablero enviado
        std::string board_data = param1; // Datos del tablero
        log_event(state, "Intento de procesar BOARD de " + logged_user); // Registra el evento

        std::string player1, player2, current_turn; // Variables para jugadores y turno
        bool both_boards_ready = false; // Indica si ambos tableros están listos

        {
            std::lock_guard<std::mutex> lock(state.game_mutex); // Bloquea mutex de juegos
            auto game_it = state.user_games.find(logged_user); // Busca la partida del usuario
            if (game_it == state.user_games.end()) { // Si no está en un juego
                log_event(state, "Error: " + logged_user + " no está en un juego activo"); // Registra error
                send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0); // Notifica error
                return;
            }

            Game* game = game_it->second; // Obtiene la partida
            log_event(state, "Tablero recibido de " + logged_user + " para juego " + game->player1 + " vs " + game->player2); // Registra el evento

            bool was_board1_empty = game->board1.countShips() == 0; // Verifica si el tablero 1 estaba vacío
            bool was_board2_empty = game->board2.countShips() == 0; // Verifica si el tablero 2 estaba vacío

            if (game->player1 == logged_user) { // Si el usuario es el jugador 1
                if (!was_board1_empty) { // Si el tablero ya estaba configurado
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado"); // Registra advertencia
                }
                game->board1.deserialize(board_data); // Deserializa el tablero
            } else if (game->player2 == logged_user) { // Si el usuario es el jugador 2
                if (!was_board2_empty) { // Si el tablero ya estaba configurado
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado"); // Registra advertencia
                }
                game->board2.deserialize(board_data); // Deserializa el tablero
            } else { // Si el usuario no es jugador
                log_event(state, "Error: " + logged_user + " no es jugador en este juego"); // Registra error
                send(client_socket, "INVALID|NOT_PLAYER", strlen("INVALID|NOT_PLAYER"), 0); // Notifica error
                return;
            }

            int expected_ship_length = 0; // Calcula el tamaño total esperado de barcos
            for (const auto& ship : ships) {
                expected_ship_length += ship.size;
            }
            // Verifica si ambos tableros están completos
            if (game->board1.countShips() == expected_ship_length && game->board2.countShips() == expected_ship_length) {
                both_boards_ready = true; // Marca que ambos tableros están listos
                player1 = game->player1; // Asigna el primer jugador
                player2 = game->player2; // Asigna el segundo jugador
                current_turn = game->current_turn; // Asigna el turno inicial
            }
        }

        if (both_boards_ready) { // Si ambos tableros están listos
            std::string turn_msg = "TURN|" + current_turn; // Mensaje de turno
            log_event(state, "Enviando TURN a ambos jugadores: " + turn_msg); // Registra el evento
            if (state.user_sockets.contains(player1)) { // Envía al primer jugador
                int result = send(state.user_sockets[player1], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == SOCKET_ERROR) { // Verifica errores
                    log_event(state, "Error enviando TURN a " + player1 + ": " + std::to_string(WSAGetLastError()));
                }
            }
            if (state.user_sockets.contains(player2)) { // Envía al segundo jugador
                int result = send(state.user_sockets[player2], turn_msg.c_str(), turn_msg.size(), 0);
                if (result == SOCKET_ERROR) { // Verifica errores
                    log_event(state, "Error enviando TURN a " + player2 + ": " + std::to_string(WSAGetLastError()));
                }
            }
            log_event(state, "Ambos tableros listos, turno inicial: " + current_turn); // Registra el evento
        }
    } else if (command == "FIRE") { // Procesar un disparo
        int x_coord = std::stoi(param1); // Coordenada X
        int y_coord = std::stoi(param2); // Coordenada Y
        std::lock_guard<std::mutex> lock(state.game_mutex); // Bloquea mutex de juegos
        if (!state.user_games.contains(logged_user)) { // Si no está en un juego
            send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0); // Notifica error
            return;
        }
        Game* game = state.user_games[logged_user]; // Obtiene la partida
        if (game->current_turn != logged_user || !game->active) { // Si no es el turno del usuario
            send(client_socket, "INVALID|NOT_YOUR_TURN", strlen("INVALID|NOT_YOUR_TURN"), 0); // Notifica error
            return;
        }

        Board& target_board = (game->player1 == logged_user) ? game->board2 : game->board1; // Tablero del oponente
        std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1; // Nombre del oponente

        if (state.user_sockets.contains(opponent)) { // Si el oponente está conectado
            std::string attack_msg = "ATTACKED|" + param1 + "|" + param2; // Mensaje de ataque
            int result = send(state.user_sockets[opponent], attack_msg.c_str(), attack_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando ATTACKED a " + opponent + ": " + std::to_string(WSAGetLastError()));
            } else {
                log_event(state, "ATTACKED enviado a " + opponent + ": " + attack_msg); // Registra el evento
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa
        }

        if (target_board.isHit(x_coord, y_coord)) { // Si el disparo acertó
            target_board.setHit(x_coord, y_coord); // Marca el acierto

            std::string hit_msg = "HIT|" + param1 + "|" + param2; // Mensaje de acierto
            int result = send(client_socket, hit_msg.c_str(), hit_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando HIT a " + logged_user + ": " + std::to_string(WSAGetLastError()));
            } else {
                log_event(state, logged_user + " acertó en (" + param1 + "," + param2 + ") contra " + opponent); // Registra el evento
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa

            std::string sunk_ship = target_board.getSunkShipName(x_coord, y_coord); // Verifica si un barco se hundió
            if (!sunk_ship.empty()) { // Si se hundió un barco
                std::string sunk_msg = "SUNK|" + sunk_ship; // Mensaje de hundimiento
                result = send(client_socket, sunk_msg.c_str(), sunk_msg.size(), 0);
                if (result == SOCKET_ERROR) { // Verifica errores
                    log_event(state, "Error enviando SUNK a " + logged_user + ": " + std::to_string(WSAGetLastError()));
                } else {
                    log_event(state, logged_user + " hundió " + sunk_ship + " de " + opponent); // Registra el evento
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa
            }

            if (target_board.allShipsSunk()) { // Si todos los barcos del oponente están hundidos
                std::string win_msg = "GAME|WIN"; // Mensaje de victoria
                int result = send(client_socket, win_msg.c_str(), win_msg.size(), 0);
                if (result == SOCKET_ERROR) { // Verifica errores
                    log_event(state, "Error enviando GAME|WIN a " + logged_user + ": " + std::to_string(WSAGetLastError()));
                } else {
                    log_event(state, "GAME|WIN enviado a " + logged_user); // Registra el evento
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa

                if (state.user_sockets.contains(opponent)) { // Si el oponente está conectado
                    std::string lose_msg = "GAME|LOSE"; // Mensaje de derrota
                    result = send(state.user_sockets[opponent], lose_msg.c_str(), lose_msg.size(), 0);
                    if (result == SOCKET_ERROR) { // Verifica errores
                        log_event(state, "Error enviando GAME|LOSE a " + opponent + ": " + std::to_string(WSAGetLastError()));
                    } else {
                        log_event(state, "GAME|LOSE enviado a " + opponent); // Registra el evento
                    }
                }
                log_event(state, "Partida finalizada: " + logged_user + " ganó contra " + opponent); // Registra el evento

                // Limpia el juego
                state.user_games.erase(game->player1); // Elimina la partida para el primer jugador
                state.user_games.erase(game->player2); // Elimina la partida para el segundo jugador
                auto it = std::ranges::find(state.games, game); // Busca el juego
                if (it != state.games.end()) { // Si se encontró
                    state.games.erase(it); // Lo elimina
                }
                delete game; // Libera la memoria

                // Notifica a ambos jugadores que el juego terminó
                if (state.user_sockets.contains(logged_user)) {
                    send(state.user_sockets[logged_user], "GAME|ENDED", strlen("GAME|ENDED"), 0);
                }
                if (state.user_sockets.contains(opponent)) {
                    send(state.user_sockets[opponent], "GAME|ENDED", strlen("GAME|ENDED"), 0);
                }
                return;
            }
        } else { // Si el disparo falló
            target_board.setMiss(x_coord, y_coord); // Marca el fallo
            std::string miss_msg = "MISS|" + param1 + "|" + param2; // Mensaje de fallo
            int result = send(client_socket, miss_msg.c_str(), miss_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando MISS a " + logged_user + ": " + std::to_string(WSAGetLastError()));
            } else {
                log_event(state, logged_user + " falló en (" + param1 + "," + param2 + ") contra " + opponent); // Registra el evento
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa
        }

        game->current_turn = opponent; // Cambia el turno al oponente
        std::string turn_msg = "TURN|" + opponent; // Mensaje de turno
        log_event(state, "Preparando envío de TURN a ambos jugadores: " + turn_msg); // Registra el evento

        if (state.user_sockets.contains(logged_user)) { // Notifica al usuario actual
            int result = send(state.user_sockets[logged_user], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando TURN a " + logged_user + ": " + std::to_string(WSAGetLastError()));
            } else {
                log_event(state, "TURN enviado a " + logged_user + ": " + turn_msg); // Registra el evento
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Pequeña pausa
        }

        if (state.user_sockets.contains(opponent)) { // Notifica al oponente
            int result = send(state.user_sockets[opponent], turn_msg.c_str(), turn_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando TURN a " + opponent + ": " + std::to_string(WSAGetLastError()));
                send(state.user_sockets[logged_user], "GAME|WIN", strlen("GAME|WIN"), 0); // Concede victoria al usuario
                // Limpia el juego
                state.user_games.erase(game->player1);
                state.user_games.erase(game->player2);
                auto it = std::ranges::find(state.games, game);
                if (it != state.games.end()) {
                    state.games.erase(it);
                }
                delete game;
                return;
            } else {
                log_event(state, "TURN enviado a " + opponent + ": " + turn_msg); // Registra el evento
            }
        } else { // Si el oponente se desconectó
            std::string win_msg = "GAME|WIN"; // Mensaje de victoria
            int result = send(state.user_sockets[logged_user], win_msg.c_str(), win_msg.size(), 0);
            if (result == SOCKET_ERROR) { // Verifica errores
                log_event(state, "Error enviando GAME|WIN por desconexión a " + logged_user + ": " + std::to_string(WSAGetLastError()));
            }
            log_event(state, "Oponente " + opponent + " desconectado, victoria para " + logged_user); // Registra el evento
            // Limpia el juego
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

// Maneja la comunicación con un cliente
void handle_client(SOCKET client_socket, ServerState& state) {
    char buffer[4096] = {0}; // Buffer para recibir mensajes
    std::string logged_user; // Nombre del usuario logueado

    u_long mode = 0; // Modo bloqueante
    ioctlsocket(client_socket, FIONBIO, &mode); // Configura el socket como bloqueante

    while (true) { // Bucle para recibir mensajes
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0); // Lee datos del socket
        if (bytes_read <= 0) { // Si la recepción falla (desconexión)
            if (!logged_user.empty()) { // Si hay un usuario logueado
                process_protocols("LOGOUT", logged_user, "", client_socket, logged_user, state); // Ejecuta logout
                log_event(state, "Usuario desconectado y deslogueado: " + logged_user + " (recv falló con " +
                          std::to_string(bytes_read) + ", error: " + std::to_string(WSAGetLastError()) + ")"); // Registra el evento
                std::cout << "Cliente desconectado y deslogueado: " << logged_user << std::endl; // Imprime en consola
            } else { // Si no hay usuario logueado
                log_event(state, "Cliente anónimo desconectado (recv falló con " + std::to_string(bytes_read) +
                          ", error: " + std::to_string(WSAGetLastError()) + ")"); // Registra el evento
                std::cout << "Cliente anónimo desconectado." << std::endl; // Imprime en consola
            }
            closesocket(client_socket); // Cierra el socket
            return; // Termina el hilo
        }

        buffer[bytes_read] = '\0'; // Termina la cadena
        std::string message(buffer, bytes_read); // Convierte el buffer a string
        std::istringstream msg_stream(message); // Crea un flujo para parsear
        std::string command, param1, param2; // Variables para comando y parámetros
        std::getline(msg_stream, command, '|'); // Extrae el comando
        std::getline(msg_stream, param1, '|'); // Extrae el primer parámetro
        std::getline(msg_stream, param2); // Extrae el segundo parámetro

        if (command == "LOGIN" && !logged_user.empty()) { // Si se intenta un login mientras ya está logueado
            u_long mode = 1; // Modo no bloqueante
            ioctlsocket(client_socket, FIONBIO, &mode); // Cambia el socket a no bloqueante
        }

        process_protocols(command, param1, param2, client_socket, logged_user, state); // Procesa el comando
    }
}

// Función principal del programa
int main(int argc, char* argv[]) {
    if (argc != 4) { // Verifica que se proporcionen los argumentos correctos
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT> </path/log.log>" << std::endl; // Imprime uso correcto
        return 1; // Termina con error
    }

    ServerState state; // Crea el estado del servidor

    std::string ip_address = argv[1]; // Dirección IP del servidor
    int port = std::stoi(argv[2]); // Puerto del servidor
    state.log_path = argv[3]; // Ruta del archivo de log

    std::cout << "Iniciando servidor Battleship..." << std::endl; // Imprime mensaje de inicio

    WSADATA wsaData; // Estructura para inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // Inicializa Winsock versión 2.2
        std::cerr << "Error al iniciar Winsock." << std::endl; // Imprime error
        return 1; // Termina con error
    }

    load_users(state); // Carga los usuarios desde el archivo

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0); // Crea un socket TCP
    if (server_socket == INVALID_SOCKET) { // Verifica si se creó correctamente
        std::cerr << "Error al crear socket: " << WSAGetLastError() << std::endl; // Imprime error
        WSACleanup(); // Limpia Winsock
        return 1; // Termina con error
    }

    struct sockaddr_in address = { AF_INET, htons(port), {} }; // Configura la dirección del servidor
    inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr); // Convierte la IP a formato binario
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) { // Enlaza el socket
        std::cerr << "Error al bind: " << WSAGetLastError() << std::endl; // Imprime error
        closesocket(server_socket); // Cierra el socket
        WSACleanup(); // Limpia Winsock
        return 1; // Termina con error
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) { // Pone el socket en modo escucha
        std::cerr << "Error al listen: " << WSAGetLastError() << std::endl; // Imprime error
        closesocket(server_socket); // Cierra el socket
        WSACleanup(); // Limpia Winsock
        return 1; // Termina con error
    }

    std::cout << "Servidor iniciado y escuchando en " << ip_address << ":" << port << std::endl; // Imprime estado

    std::thread matchmaking_thread(matchmaking, std::ref(state)); // Lanza el hilo de emparejamiento
    matchmaking_thread.detach(); // Desacopla el hilo
    std::cout << "Hilo de matchmaking iniciado." << std::endl; // Imprime estado

    std::vector<std::thread> client_threads; // Lista de hilos de clientes
    while (true) { // Bucle para aceptar conexiones
        SOCKET new_socket = accept(server_socket, nullptr, nullptr); // Acepta una nueva conexión
        if (new_socket == INVALID_SOCKET) { // Verifica errores
            std::cerr << "Error al aceptar conexión: " << WSAGetLastError() << std::endl; // Imprime error
            continue; // Continúa con la próxima conexión
        }
        std::cout << "Nuevo cliente conectado." << std::endl; // Imprime estado
        client_threads.emplace_back(handle_client, new_socket, std::ref(state)); // Crea un hilo para el cliente

        if (client_threads.size() > 10) { // Si hay más de 10 hilos
            std::vector<std::thread> active_threads; // Lista para hilos activos
            for (auto& t : client_threads) { // Recorre los hilos
                if (t.joinable()) { // Si el hilo puede unirse
                    t.join(); // Lo une
                } else { // Si sigue activo
                    active_threads.push_back(std::move(t)); // Lo mueve a la lista activa
                }
            }
            client_threads = std::move(active_threads); // Actualiza la lista de hilos
        }
    }

    closesocket(server_socket); // Cierra el socket del servidor
    WSACleanup(); // Limpia Winsock
    return 0; // Termina el programa
}