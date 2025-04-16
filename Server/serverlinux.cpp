// Incluye la biblioteca estándar de C++ para entrada/salida, como std::cout y std::cin.
#include <iostream>

// Incluye la biblioteca para manejo de hilos, permitiendo el uso de std::thread.
#include <thread>

// Incluye la biblioteca para contenedores de vectores, como std::vector.
#include <vector>

// Incluye la biblioteca para mecanismos de sincronización, como std::mutex.
#include <mutex>

// Incluye la biblioteca para operaciones con archivos, como std::ofstream y std::ifstream.
#include <fstream>

// Incluye la biblioteca para funciones de sockets en sistemas POSIX, como socket().
#include <sys/socket.h>

// Incluye la biblioteca para estructuras de red, como struct sockaddr_in.
#include <netinet/in.h>

// Incluye la biblioteca para conversiones de direcciones IP, como inet_pton().
#include <arpa/inet.h>

// Incluye la biblioteca para funciones de sistema POSIX, como close() y fcntl().
#include <unistd.h>

// Incluye la biblioteca para control de archivos, como fcntl() para configurar sockets.
#include <fcntl.h>

// Incluye la biblioteca para manejo de errores, proporcionando acceso a errno.
#include <errno.h>

// Incluye la biblioteca para manipulación de cadenas, como std::stringstream.
#include <sstream>

// Incluye la biblioteca para contenedores de mapas, como std::map.
#include <map>

// Incluye la biblioteca para contenedores de conjuntos, como std::set.
#include <set>

// Incluye la biblioteca para manejo de tiempo, como time() y ctime().
#include <ctime>

// Incluye la biblioteca para algoritmos estándar, como std::ranges::find.
#include <algorithm>

// Incluye la biblioteca para manejo de señales, como signal().
#include <signal.h>

// Incluye la biblioteca para funciones de cadenas de C, como strlen() y strerror().
#include <cstring>

// Define una constante para el número máximo de clientes que el servidor puede manejar (100).
#define MAX_CLIENTS 100

// Define una constante para el tamaño del tablero de juego (10x10).
#define BOARD_SIZE  10

// Define el símbolo que representa el agua en el tablero ('~').
#define WATER '~'

// Define el símbolo que representa un impacto acertado en el tablero ('X').
#define HIT 'X'

// Define el símbolo que representa un disparo fallado en el tablero ('O').
#define MISS 'O'

// Declara una estructura Ship para representar un barco en el juego.
struct Ship {
    // Campo para el nombre del barco (ej., "Aircraft Carrier").
    std::string name;
    // Campo para el tamaño del barco (número de casillas que ocupa).
    int size;
    // Campo para el símbolo que representa el barco en el tablero.
    char symbol;
};

// Declara un vector global de barcos predefinidos para el juego.
std::vector<Ship> ships = {
    // Define un barco "Aircraft Carrier" (Portaviones) con tamaño 5 y símbolo 'A'.
    {"Aircraft Carrier", 5, 'A'},
    // Define un barco "Battleship" (Acorazado) con tamaño 4 y símbolo 'B'.
    {"Battleship", 4, 'B'},
    // Define el primer barco "Cruiser" (Crucero) con tamaño 3 y símbolo 'C'.
    {"Cruiser", 3, 'C'},
    // Define el segundo barco "Cruiser" (Crucero) con tamaño 3 y símbolo 'C'.
    {"Cruiser", 3, 'C'},
    // Define el primer barco "Destroyer" (Destructor) con tamaño 2 y símbolo 'D'.
    {"Destroyer", 2, 'D'},
    // Define el segundo barco "Destroyer" (Destructor) con tamaño 2 y símbolo 'D'.
    {"Destroyer", 2, 'D'},
    // Define el primer barco "Submarine" (Submarino) con tamaño 1 y símbolo 'S'.
    {"Submarine", 1, 'S'},
    // Define el segundo barco "Submarine" (Submarino) con tamaño 1 y símbolo 'S'.
    {"Submarine", 1, 'S'},
    // Define el tercer barco "Submarine" (Submarino) con tamaño 1 y símbolo 'S'.
    {"Submarine", 1, 'S'}
};

// Declara la clase Board para gestionar el tablero de juego.
class Board {
public:
    // Declara un vector bidimensional de caracteres para representar el tablero (10x10).
    std::vector<std::vector<char>> grid;
    // Declara una estructura interna ShipInstance para representar instancias de barcos en el tablero.
    struct ShipInstance {
        // Campo para el nombre del barco.
        std::string name;
        // Campo para el símbolo del barco.
        char symbol;
        // Campo para las posiciones del barco en el tablero (coordenadas x, y).
        std::vector<std::pair<int, int>> positions;
        // Campo que indica si el barco ha sido hundido.
        bool sunk;
        // Constructor de ShipInstance que inicializa los campos.
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            // Inicializa los campos con los valores proporcionados y establece sunk como false.
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    // Declara un vector para almacenar las instancias de barcos en el tablero.
    std::vector<ShipInstance> ship_instances;

    // Constructor de la clase Board.
    Board() {
        // Inicializa el tablero como un vector 10x10 lleno de WATER ('~').
        grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
    }

    // Método para deserializar un tablero desde una cadena de datos.
    void deserialize(const std::string& data) {
        // Itera sobre las filas del tablero.
        for (int i = 0; i < BOARD_SIZE; i++) {
            // Itera sobre las columnas del tablero.
            for (int j = 0; j < BOARD_SIZE; j++) {
                // Asigna el carácter correspondiente de la cadena al tablero (índice calculado como i*BOARD_SIZE + j).
                grid[i][j] = data[i * BOARD_SIZE + j];
            }
        }
        // Llama a rebuildShipInstances para reconstruir las instancias de barcos basadas en el tablero.
        rebuildShipInstances();
    }

    // Método para reconstruir las instancias de barcos a partir del tablero.
    void rebuildShipInstances() {
        // Limpia el vector de instancias de barcos.
        ship_instances.clear();
        // Declara un mapa para asociar símbolos de barcos con sus posiciones en el tablero.
        std::map<char, std::vector<std::pair<int, int>>> symbol_to_positions;

        // Itera sobre las filas del tablero.
        for (int i = 0; i < BOARD_SIZE; i++) {
            // Itera sobre las columnas del tablero.
            for (int j = 0; j < BOARD_SIZE; j++) {
                // Verifica si la casilla contiene un barco (no es agua, impacto ni fallo).
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    // Agrega la posición (i, j) al vector de posiciones del símbolo correspondiente.
                    symbol_to_positions[grid[i][j]].emplace_back(i, j);
                }
            }
        }

        // Itera sobre la lista global de barcos predefinidos.
        for (const auto& ship : ships) {
            // Obtiene las posiciones asociadas al símbolo del barco actual.
            auto& positions = symbol_to_positions[ship.symbol];
            // Obtiene el tamaño del barco actual.
            int size = ship.size;

            // Procesa mientras haya suficientes posiciones para formar un barco del tamaño requerido.
            while (positions.size() >= size) {
                // Declara un vector para almacenar las posiciones del barco actual.
                std::vector<std::pair<int, int>> current_ship;
                // Itera sobre las posiciones disponibles para el símbolo del barco.
                for (auto it = positions.begin(); it != positions.end(); ) {
                    // Si el vector current_ship está vacío, agrega la primera posición.
                    if (current_ship.empty()) {
                        // Agrega la posición actual al barco.
                        current_ship.push_back(*it);
                        // Elimina la posición de la lista y avanza el iterador.
                        it = positions.erase(it);
                    } else {
                        // Declara una bandera para indicar si se encontró una posición adyacente.
                        bool adjacent = false;
                        // Obtiene la última posición agregada al barco actual.
                        auto last = current_ship.back();
                        // Itera sobre las posiciones restantes para buscar una adyacente.
                        for (auto pos_it = it; pos_it != positions.end(); ++pos_it) {
                            // Verifica si la posición es adyacente (misma fila y columna adyacente o viceversa).
                            if ((pos_it->first == last.first && abs(pos_it->second - last.second) == 1) ||
                                (pos_it->second == last.second && abs(pos_it->first - last.first) == 1)) {
                                // Agrega la posición adyacente al barco actual.
                                current_ship.push_back(*pos_it);
                                // Elimina la posición de la lista y actualiza el iterador.
                                it = positions.erase(pos_it);
                                // Marca que se encontró una posición adyacente.
                                adjacent = true;
                                // Sale del bucle interno.
                                break;
                            }
                        }
                        // Si no se encontró una posición adyacente, sale del bucle.
                        if (!adjacent) break;
                    }
                    // Si el barco actual tiene el tamaño requerido, sale del bucle.
                    if (current_ship.size() == size) break;
                }
                // Verifica si el barco actual tiene el tamaño correcto.
                if (current_ship.size() == size) {
                    // Agrega una nueva instancia de barco al vector ship_instances.
                    ship_instances.emplace_back(ship.name, ship.symbol, current_ship);
                } else {
                    // Si el barco no se completó, reinserta las posiciones usadas.
                    positions.insert(positions.begin(), current_ship.begin(), current_ship.end());
                    // Sale del bucle while.
                    break;
                }
            }
        }
    }

    // Método que verifica si una coordenada (x, y) contiene un barco (es un impacto).
    bool isHit(int x, int y) const {
        // Devuelve true si la casilla no es agua, impacto ni fallo (es un barco).
        return grid[x][y] != WATER && grid[x][y] != HIT && grid[x][y] != MISS;
    }

    // Método que obtiene el símbolo en una coordenada (x, y).
    char getSymbol(int x, int y) const {
        // Devuelve el carácter en la posición (x, y) del tablero.
        return grid[x][y];
    }

    // Método para marcar una coordenada (x, y) como impacto.
    void setHit(int x, int y) {
        // Establece la casilla (x, y) como HIT ('X').
        grid[x][y] = HIT;
        // Verifica si el impacto hundió un barco.
        checkSunk(x, y);
    }

    // Método para marcar una coordenada (x, y) como fallo.
    void setMiss(int x, int y) {
        // Establece la casilla (x, y) como MISS ('O').
        grid[x][y] = MISS;
    }

    // Método para verificar si un impacto en (x, y) hundió un barco.
    void checkSunk(int x, int y) {
        // Itera sobre todas las instancias de barcos.
        for (auto& ship : ship_instances) {
            // Verifica si el barco no está hundido.
            if (!ship.sunk) {
                // Itera sobre las posiciones del barco.
                for (const auto& pos : ship.positions) {
                    // Verifica si la posición coincide con el impacto.
                    if (pos.first == x && pos.second == y) {
                        // Declara una bandera para verificar si todas las posiciones están impactadas.
                        bool all_hit = true;
                        // Itera sobre las posiciones del barco.
                        for (const auto& p : ship.positions) {
                            // Verifica si alguna posición no está impactada.
                            if (grid[p.first][p.second] != HIT) {
                                // Marca que no todas las posiciones están impactadas.
                                all_hit = false;
                                // Sale del bucle.
                                break;
                            }
                        }
                        // Si todas las posiciones están impactadas, marca el barco como hundido.
                        if (all_hit) {
                            // Establece el barco como hundido.
                            ship.sunk = true;
                        }
                        // Sale de la función.
                        return;
                    }
                }
            }
        }
    }

    // Método para obtener el nombre de un barco hundido en la posición (x, y).
    std::string getSunkShipName(int x, int y) {
        // Itera sobre todas las instancias de barcos.
        for (const auto& ship : ship_instances) {
            // Verifica si el barco está hundido.
            if (ship.sunk) {
                // Itera sobre las posiciones del barco.
                for (const auto& pos : ship.positions) {
                    // Verifica si la posición coincide con (x, y).
                    if (pos.first == x && pos.second == y) {
                        // Devuelve el nombre del barco hundido.
                        return ship.name;
                    }
                }
            }
        }
        // Devuelve una cadena vacía si no se encuentra un barco hundido.
        return "";
    }

    // Método para verificar si todos los barcos han sido hundidos.
    bool allShipsSunk() {
        // Itera sobre todas las instancias de barcos.
        for (const auto& ship : ship_instances) {
            // Si algún barco no está hundido, devuelve false.
            if (!ship.sunk) return false;
        }
        // Devuelve true si todos los barcos están hundidos.
        return true;
    }

    // Método para contar la cantidad de casillas ocupadas por barcos.
    int countShips() const {
        // Declara una variable para la longitud total esperada de los barcos.
        int total_length = 0;
        // Itera sobre la lista global de barcos.
        for (const auto& ship : ships) {
            // Suma el tamaño de cada barco a la longitud total.
            total_length += ship.size;
        }
        // Declara una variable para la longitud actual de barcos en el tablero.
        int current_length = 0;
        // Itera sobre las filas del tablero.
        for (int i = 0; i < BOARD_SIZE; i++) {
            // Itera sobre las columnas del tablero.
            for (int j = 0; j < BOARD_SIZE; j++) {
                // Verifica si la casilla contiene un barco (no es agua, impacto ni fallo).
                if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                    // Incrementa la longitud actual.
                    current_length++;
                }
            }
        }
        // Devuelve la longitud total si coincide con la esperada, o 0 si no.
        return (current_length == total_length) ? total_length : 0;
    }
};

// Declara una estructura Game para representar una partida entre dos jugadores.
struct Game {
    // Campo para el nombre del primer jugador.
    std::string player1;
    // Campo para el nombre del segundo jugador.
    std::string player2;
    // Campo para el tablero del primer jugador.
    Board board1;
    // Campo para el tablero del segundo jugador.
    Board board2;
    // Campo para el nombre del jugador cuyo turno es actual.
    std::string current_turn;
    // Campo que indica si la partida está activa.
    bool active;

    // Constructor de la estructura Game.
    Game(const std::string& p1, const std::string& p2)
        // Inicializa los campos con los nombres de los jugadores, turno inicial y estado activo.
        : player1(p1), player2(p2), current_turn(p1), active(true) {}
};

// Declara una estructura ServerState para almacenar el estado del servidor.
struct ServerState {
    // Mutex para sincronizar el acceso al archivo de registro.
    std::mutex log_mutex;
    // Mutex para sincronizar el acceso a las partidas.
    std::mutex game_mutex;
    // Mutex para sincronizar el acceso a los datos de usuarios.
    std::mutex user_mutex;
    // Mutex para sincronizar el acceso a la cola de emparejamiento.
    std::mutex matchmaking_mutex;

    // Mapa que asocia nombres de usuario con sus contraseñas.
    std::map<std::string, std::string> user_db;
    // Conjunto de nombres de usuarios actualmente conectados.
    std::set<std::string> active_users;
    // Conjunto de nombres de usuarios en la cola de emparejamiento.
    std::set<std::string> matchmaking_queue;
    // Mapa que asocia nombres de usuario con sus descriptores de socket.
    std::map<std::string, int> user_sockets;
    // Mapa que asocia nombres de usuario con las partidas en las que están.
    std::map<std::string, Game*> user_games;
    // Vector que almacena todas las partidas activas.
    std::vector<Game*> games;

    // Campo para la ruta del archivo de registro.
    std::string log_path;
};

// Declara una función para registrar eventos en el archivo de log.
void log_event(ServerState& state, const std::string& event) {
    // Crea un guardia de bloqueo para el mutex de registro.
    std::lock_guard<std::mutex> lock(state.log_mutex);
    // Abre el archivo de registro en modo de añadir.
    std::ofstream log_file(state.log_path, std::ios::app);
    // Verifica si el archivo se abrió correctamente.
    if (log_file.is_open()) {
        // Obtiene la hora actual.
        time_t now = time(nullptr);
        // Convierte la hora a una cadena legible.
        char* dt = ctime(&now);
        // Elimina el carácter de nueva línea al final de la cadena de tiempo.
        dt[strlen(dt) - 1] = '\0';
        // Escribe el evento en el archivo con la marca de tiempo.
        log_file << "[" << dt << "] " << event << std::endl;
    }
}

// Declara una función para cargar los usuarios desde un archivo.
void load_users(ServerState& state) {
    // Abre el archivo "usuarios.txt" para lectura.
    std::ifstream file("usuarios.txt");
    // Declara variables para almacenar el nombre de usuario y la contraseña.
    std::string username, password;
    // Lee el archivo mientras haya datos (pares usuario-contraseña).
    while (file >> username >> password) {
        // Almacena el par usuario-contraseña en el mapa user_db.
        state.user_db[username] = password;
    }
}

// Declara una función para guardar un nuevo usuario en el archivo.
void save_user(const std::string& username, const std::string& password) {
    // Abre el archivo "usuarios.txt" en modo de añadir.
    std::ofstream file("usuarios.txt", std::ios::app);
    // Escribe el nombre de usuario y la contraseña en el archivo, separados por un espacio.
    file << username << " " << password << std::endl;
}

// Declara una función para iniciar una partida entre dos jugadores.
void start_game(const std::string& player1, const std::string& player2, ServerState& state) {
    // Crea un guardia de bloqueo para el mutex de partidas.
    std::lock_guard<std::mutex> lock(state.game_mutex);
    // Crea una nueva partida con los dos jugadores.
    Game* game = new Game(player1, player2);
    // Agrega la partida al vector de partidas activas.
    state.games.push_back(game);
    // Asocia la partida a ambos jugadores en el mapa user_games.
    state.user_games[player1] = game;
    state.user_games[player2] = game;
    // Registra el inicio de la partida en el log.
    log_event(state, "Partida iniciada: " + player1 + " vs " + player2);

    // Verifica si el socket del primer jugador está registrado.
    if (state.user_sockets.contains(player1)) {
        // Envía un mensaje "MATCH|FOUND" al primer jugador.
        int result = send(state.user_sockets[player1], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        // Si el envío falla, registra el error en el log.
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player1 + ": " + std::string(strerror(errno)));
        }
    }
    // Verifica si el socket del segundo jugador está registrado.
    if (state.user_sockets.contains(player2)) {
        // Envía un mensaje "MATCH|FOUND" al segundo jugador.
        int result = send(state.user_sockets[player2], "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        // Si el envío falla, registra el error en el log.
        if (result == -1) {
            log_event(state, "Error enviando MATCH a " + player2 + ": " + std::string(strerror(errno)));
        }
    }
}

// Declara una función para manejar el emparejamiento de jugadores.
void matchmaking(ServerState& state) {
    // Ejecuta un bucle infinito para procesar emparejamientos.
    while (true) {
        // Declara variables para los nombres de los dos jugadores.
        std::string player1, player2;
        // Crea un bloque con un guardia de bloqueo para el mutex de emparejamiento.
        {
            std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
            // Verifica si hay al menos dos jugadores en la cola de emparejamiento.
            if (state.matchmaking_queue.size() >= 2) {
                // Obtiene el iterador al primer jugador en la cola.
                auto it = state.matchmaking_queue.begin();
                // Asigna el nombre del primer jugador.
                player1 = *it;
                // Elimina el primer jugador de la cola.
                state.matchmaking_queue.erase(it);
                // Obtiene el iterador al siguiente jugador en la cola.
                it = state.matchmaking_queue.begin();
                // Asigna el nombre del segundo jugador.
                player2 = *it;
                // Elimina el segundo jugador de la cola.
                state.matchmaking_queue.erase(it);
            }
        }
        // Verifica si se encontraron dos jugadores válidos.
        if (!player1.empty() && !player2.empty()) {
            // Crea un guardia de bloqueo para el mutex de usuarios.
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            // Verifica si ambos jugadores están conectados (tienen sockets registrados).
            if (state.user_sockets.contains(player1) &&
                state.user_sockets.contains(player2)) {
                // Inicia una partida entre los dos jugadores.
                start_game(player1, player2, state);
            } else {
                // Crea un guardia de bloqueo para el mutex de emparejamiento.
                std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
                // Si el primer jugador sigue conectado, lo reinserta en la cola.
                if (state.user_sockets.contains(player1)) {
                    state.matchmaking_queue.insert(player1);
                }
                // Si el segundo jugador sigue conectado, lo reinserta en la cola.
                if (state.user_sockets.contains(player2)) {
                    state.matchmaking_queue.insert(player2);
                }
            }
        }
        // Pausa el hilo durante 1 segundo antes de la próxima iteración.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Declara una función para procesar los comandos del protocolo del cliente.
void process_protocols(const std::string& command, const std::string& param1, const std::string& param2, int client_socket, std::string& logged_user, ServerState& state) {
    // Verifica si el comando es "REGISTER".
    if (command == "REGISTER") {
        // Asigna el nombre de usuario del primer parámetro.
        std::string username = param1;
        // Asigna la contraseña del segundo parámetro.
        std::string password = param2;
        // Crea un guardia de bloqueo para el mutex de usuarios.
        std::lock_guard<std::mutex> lock(state.user_mutex);
        // Verifica si el usuario no está registrado.
        if (!state.user_db.contains(username)) {
            // Registra el usuario y su contraseña en la base de datos.
            state.user_db[username] = password;
            // Guarda el usuario en el archivo.
            save_user(username, password);
            // Envía un mensaje de registro exitoso al cliente.
            send(client_socket, "REGISTER|SUCCESSFUL", strlen("REGISTER|SUCCESSFUL"), 0);
            // Registra el evento en el log.
            log_event(state, "Usuario registrado: " + username);
        } else {
            // Envía un mensaje de error al cliente si el usuario ya existe.
            send(client_socket, "REGISTER|ERROR", strlen("REGISTER|ERROR"), 0);
            // Registra el intento fallido en el log.
            log_event(state, "Intento de registro fallido: " + username);
        }
    // Verifica si el comando es "LOGIN".
    } else if (command == "LOGIN") {
        // Asigna el nombre de usuario del primer parámetro.
        std::string username = param1;
        // Asigna la contraseña del segundo parámetro.
        std::string password = param2;
        // Crea un guardia de bloqueo para el mutex de usuarios.
        std::lock_guard<std::mutex> lock(state.user_mutex);
        // Verifica si el usuario no existe o la contraseña es incorrecta.
        if (!state.user_db.contains(username) || state.user_db[username] != password) {
            // Envía un mensaje de error al cliente.
            send(client_socket, "LOGIN|ERROR", strlen("LOGIN|ERROR"), 0);
            // Registra el intento fallido en el log.
            log_event(state, "Intento de login fallido: Credenciales incorrectas - " + username);
        // Verifica si el usuario ya está conectado.
        } else if (state.active_users.contains(username)) {
            // Envía un mensaje de error al cliente indicando que el usuario ya está conectado.
            send(client_socket, "LOGIN|ERROR|YA_CONECTADO", strlen("LOGIN|ERROR|YA_CONECTADO"), 0);
            // Registra el intento fallido en el log.
            log_event(state, "Intento de login fallido: Usuario ya conectado - " + username);
        } else {
            // Agrega el usuario a la lista de usuarios activos.
            state.active_users.insert(username);
            // Asocia el socket del cliente al usuario.
            state.user_sockets[username] = client_socket;
            // Establece el usuario como el usuario logueado.
            logged_user = username;
            // Envía un mensaje de login exitoso al cliente.
            send(client_socket, "LOGIN|SUCCESSFUL", strlen("LOGIN|SUCCESSFUL"), 0);
            // Registra el evento en el log.
            log_event(state, "Usuario logueado: " + username);
        }
    // Verifica si el comando es "PLAYERS".
    } else if (command == "PLAYERS") {
        // Inicializa una cadena con el encabezado de la lista de jugadores.
        std::string players_list = "LISTA DE JUGADORES:\n";
        // Crea un bloque con guardias de bloqueo para los mutex de usuarios, partidas y emparejamiento.
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            std::lock_guard<std::mutex> game_lock(state.game_mutex);
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
            // Itera sobre los usuarios activos.
            for (const auto& user : state.active_users) {
                // Establece el estado inicial como "Conectado".
                std::string status = "(Conectado)";
                // Si el usuario está en una partida, cambia el estado.
                if (state.user_games.contains(user)) {
                    status = "(En juego)";
                // Si el usuario está en la cola de emparejamiento, cambia el estado.
                } else if (state.matchmaking_queue.contains(user)) {
                    status = "(Buscando partida)";
                }
                // Agrega el usuario y su estado a la lista.
                players_list += user + " " + status + "\n";
            }
        }
        // Envía la lista de jugadores al cliente.
        if (send(client_socket, players_list.c_str(), players_list.size(), 0) == -1) {
            // Registra el error en el log si el envío falla.
            log_event(state, "Error enviando lista de jugadores a " + logged_user + ": " + std::string(strerror(errno)));
        } else {
            // Registra el envío exitoso en el log.
            log_event(state, "Lista de jugadores enviada a " + logged_user);
        }
    // Verifica si el comando es "QUEUE".
    } else if (command == "QUEUE") {
        // Crea guardias de bloqueo para los mutex de partidas y emparejamiento.
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
        // Verifica si el usuario no está en una partida.
        if (!state.user_games.contains(logged_user)) {
            // Agrega el usuario a la cola de emparejamiento.
            state.matchmaking_queue.insert(logged_user);
            // Envía un mensaje de confirmación al cliente.
            send(client_socket, "QUEUE|OK", strlen("QUEUE|OK"), 0);
            // Registra el evento en el log.
            log_event(state, logged_user + " ha entrado en cola para jugar");
        } else {
            // Envía un mensaje de error al cliente si ya está en una partida.
            send(client_socket, "QUEUE|ERROR|IN_GAME", strlen("QUEUE|ERROR|IN_GAME"), 0);
        }
    // Verifica si el comando es "CANCEL_QUEUE".
    } else if (command == "CANCEL_QUEUE") {
        // Crea un guardia de bloqueo para el mutex de emparejamiento.
        std::lock_guard<std::mutex> lock(state.matchmaking_mutex);
        // Elimina al usuario de la cola de emparejamiento.
        state.matchmaking_queue.erase(logged_user);
        // Envía un mensaje de confirmación al cliente.
        send(client_socket, "CANCEL_QUEUE|OK", strlen("CANCEL_QUEUE|OK"), 0);
        // Registra el evento en el log.
        log_event(state, logged_user + " salió de la cola de emparejamiento.");
    // Verifica si el comando es "CHECK_MATCH".
    } else if (command == "CHECK_MATCH") {
        // Crea un guardia de bloqueo para el mutex de partidas.
        std::lock_guard<std::mutex> lock(state.game_mutex);
        // Verifica si el usuario está en una partida.
        if (state.user_games.contains(logged_user)) {
            // Envía un mensaje indicando que se encontró una partida.
            send(client_socket, "MATCH|FOUND", strlen("MATCH|FOUND"), 0);
        } else {
            // Envía un mensaje indicando que no se encontró una partida.
            send(client_socket, "MATCH|NOT_FOUND", strlen("MATCH|NOT_FOUND"), 0);
        }
    // Verifica si el comando es "LOGOUT".
    } else if (command == "LOGOUT") {
        // Crea un guardia de bloqueo para el mutex de partidas.
        std::lock_guard<std::mutex> game_lock(state.game_mutex);
        // Verifica si el usuario está en una partida.
        if (state.user_games.contains(logged_user)) {
            // Obtiene la partida del usuario.
            Game* game = state.user_games[logged_user];
            // Determina el nombre del oponente.
            std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1;
            // Verifica si el oponente está conectado.
            if (state.user_sockets.contains(opponent)) {
                // Envía un mensaje de victoria al oponente por abandono.
                send(state.user_sockets[opponent], "GAME|WIN", strlen("GAME|WIN"), 0);
                // Registra la victoria en el log.
                log_event(state, opponent + " gana por abandono de " + logged_user);
            }
            // Elimina la partida de los registros de ambos jugadores.
            state.user_games.erase(game->player1);
            state.user_games.erase(game->player2);
            // Busca la partida en el vector de partidas.
            auto it = std::ranges::find(state.games, game);
            // Si la partida se encuentra, la elimina del vector.
            if (it != state.games.end()) {
                state.games.erase(it);
            }
            // Libera la memoria de la partida.
            delete game;
        }
        // Crea un bloque con guardias de bloqueo para los mutex de usuarios y emparejamiento.
        {
            std::lock_guard<std::mutex> user_lock(state.user_mutex);
            std::lock_guard<std::mutex> matchmaking_lock(state.matchmaking_mutex);
            // Elimina al usuario de la lista de usuarios activos.
            state.active_users.erase(logged_user);
            // Elimina el socket del usuario.
            state.user_sockets.erase(logged_user);
            // Elimina al usuario de la cola de emparejamiento.
            state.matchmaking_queue.erase(logged_user);
        }
        // Envía un mensaje de logout exitoso al cliente.
        send(client_socket, "LOGOUT|OK", strlen("LOGOUT|OK"), 0);
        // Registra el evento en el log.
        log_event(state, "Usuario deslogueado: " + logged_user);
    // Verifica si el comando es "BOARD".
    } else if (command == "BOARD") {
        // Asigna los datos del tablero del primer parámetro.
        std::string board_data = param1;
        // Registra el intento de procesar el tablero en el log.
        log_event(state, "Intento de procesar BOARD de " + logged_user);

        // Declara variables para los nombres de los jugadores y el turno actual.
        std::string player1, player2, current_turn;
        // Declara una bandera para indicar si ambos tableros están listos.
        bool both_boards_ready = false;

        // Crea un bloque con un guardia de bloqueo para el mutex de partidas.
        {
            std::lock_guard<std::mutex> lock(state.game_mutex);
            // Busca la partida del usuario.
            auto game_it = state.user_games.find(logged_user);
            // Verifica si el usuario no está en una partida.
            if (game_it == state.user_games.end()) {
                // Registra el error en el log.
                log_event(state, "Error: " + logged_user + " no está en un juego activo");
                // Envía un mensaje de error al cliente.
                send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0);
                // Sale de la función.
                return;
            }

            // Obtiene la partida del usuario.
            Game* game = game_it->second;
            // Registra la recepción del tablero en el log.
            log_event(state, "Tablero recibido de " + logged_user + " para juego " + game->player1 + " vs " + game->player2);

            // Verifica si el tablero del primer jugador estaba vacío.
            bool was_board1_empty = game->board1.countShips() == 0;
            // Verifica si el tablero del segundo jugador estaba vacío.
            bool was_board2_empty = game->board2.countShips() == 0;

            // Verifica si el usuario es el primer jugador.
            if (game->player1 == logged_user) {
                // Si el tablero ya estaba configurado, registra una advertencia.
                if (!was_board1_empty) {
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado");
                }
                // Deserializa los datos del tablero para el primer jugador.
                game->board1.deserialize(board_data);
            // Verifica si el usuario es el segundo jugador.
            } else if (game->player2 == logged_user) {
                // Si el tablero ya estaba configurado, registra una advertencia.
                if (!was_board2_empty) {
                    log_event(state, "Advertencia: Tablero de " + logged_user + " ya estaba configurado");
                }
                // Deserializa los datos del tablero para el segundo jugador.
                game->board2.deserialize(board_data);
            } else {
                // Registra el error en el log si el usuario no es un jugador válido.
                log_event(state, "Error: " + logged_user + " no es jugador en este juego");
                // Envía un mensaje de error al cliente.
                send(client_socket, "INVALID|NOT_PLAYER", strlen("INVALID|NOT_PLAYER"), 0);
                // Sale de la función.
                return;
            }

            // Calcula la longitud total esperada de los barcos.
            int expected_ship_length = 0;
            // Itera sobre la lista global de barcos.
            for (const auto& ship : ships) {
                // Suma el tamaño de cada barco.
                expected_ship_length += ship.size;
            }
            // Verifica si ambos tableros tienen la longitud esperada de barcos.
            if (game->board1.countShips() == expected_ship_length && game->board2.countShips() == expected_ship_length) {
                // Marca que ambos tableros están listos.
                both_boards_ready = true;
                // Asigna el nombre del primer jugador.
                player1 = game->player1;
                // Asigna el nombre del segundo jugador.
                player2 = game->player2;
                // Asigna el turno actual.
                current_turn = game->current_turn;
            }
        }

        // Verifica si ambos tableros están listos.
        if (both_boards_ready) {
            // Crea un mensaje con el turno actual.
            std::string turn_msg = "TURN|" + current_turn;
            // Registra el envío del mensaje de turno en el log.
            log_event(state, "Enviando TURN a ambos jugadores: " + turn_msg);
            // Verifica si el primer jugador está conectado.
            if (state.user_sockets.contains(player1)) {
                // Envía el mensaje de turno al primer jugador.
                int result = send(state.user_sockets[player1], turn_msg.c_str(), turn_msg.size(), 0);
                // Si el envío falla, registra el error en el log.
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player1 + ": " + std::string(strerror(errno)));
                }
            }
            // Verifica si el segundo jugador está conectado.
            if (state.user_sockets.contains(player2)) {
                // Envía el mensaje de turno al segundo jugador.
                int result = send(state.user_sockets[player2], turn_msg.c_str(), turn_msg.size(), 0);
                // Si el envío falla, registra el error en el log.
                if (result == -1) {
                    log_event(state, "Error enviando TURN a " + player2 + ": " + std::string(strerror(errno)));
                }
            }
            // Registra en el log que ambos tableros están listos y el turno inicial.
            log_event(state, "Ambos tableros listos, turno inicial: " + current_turn);
        }
    // Verifica si el comando es "FIRE".
    } else if (command == "FIRE") {
        // Convierte el primer parámetro a la coordenada x.
        int x_coord = std::stoi(param1);
        // Convierte el segundo parámetro a la coordenada y.
        int y_coord = std::stoi(param2);
        // Crea un guardia de bloqueo para el mutex de partidas.
        std::lock_guard<std::mutex> lock(state.game_mutex);
        // Verifica si el usuario no está en una partida.
        if (!state.user_games.contains(logged_user)) {
            // Envía un mensaje de error al cliente.
            send(client_socket, "INVALID|NOT_IN_GAME", strlen("INVALID|NOT_IN_GAME"), 0);
            // Sale de la función.
            return;
        }
        // Obtiene la partida del usuario.
        Game* game = state.user_games[logged_user];
        // Verifica si no es el turno del usuario o la partida no está activa.
        if (game->current_turn != logged_user || !game->active) {
            // Envía un mensaje de error al cliente.
            send(client_socket, "INVALID|NOT_YOUR_TURN", strlen("INVALID|NOT_YOUR_TURN"), 0);
            // Sale de la función.
            return;
        }

        // Selecciona el tablero del oponente (si el usuario es player1, el tablero es board2, y viceversa).
        Board& target_board = (game->player1 == logged_user) ? game->board2 : game->board1;
        // Determina el nombre del oponente.
        std::string opponent = (game->player1 == logged_user) ? game->player2 : game->player1;

        // Verifica si el oponente está conectado.
        if (state.user_sockets.contains(opponent)) {
            // Crea un mensaje con las coordenadas del ataque.
            std::string attack_msg = "ATTACKED|" + param1 + "|" + param2;
            // Envía el mensaje de ataque al oponente.
            int result = send(state.user_sockets[opponent], attack_msg.c_str(), attack_msg.size(), 0);
            // Si el envío falla, registra el error en el log.
            if (result == -1) {
                log_event(state, "Error enviando ATTACKED a " + opponent + ": " + std::string(strerror(errno)));
            } else {
                // Registra el envío exitoso en el log.
                log_event(state, "ATTACKED enviado a " + opponent + ": " + attack_msg);
            }
            // Pausa el hilo durante 50 milisegundos para evitar problemas de sincronización.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Verifica si el disparo acertó a un barco.
        if (target_board.isHit(x_coord, y_coord)) {
            // Marca la coordenada como impacto en el tablero del oponente.
            target_board.setHit(x_coord, y_coord);

            // Crea un mensaje indicando que el disparo fue un impacto.
            std::string hit_msg = "HIT|" + param1 + "|" + param2;
            // Envía el mensaje de impacto al cliente.
            int result = send(client_socket, hit_msg.c_str(), hit_msg.size(), 0);
            // Si el envío falla, registra el error en el log.
            if (result == -1) {
                log_event(state, "Error enviando HIT a " + logged_user + ": " + std::string(strerror(errno)));
            } else {
                // Registra el impacto exitoso en el log.
                log_event(state, logged_user + " acertó en (" + param1 + "," + param2 + ") contra " + opponent);
            }
            // Pausa el hilo durante 50 milisegundos.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

// Obtiene el nombre del barco hundido en las coordenadas (x_coord, y_coord) del tablero objetivo.
std::string sunk_ship = target_board.getSunkShipName(x_coord, y_coord);

// Verifica si se hundió un barco (la cadena no está vacía).
if (!sunk_ship.empty()) {
    // Crea un mensaje indicando que un barco fue hundido, incluyendo su nombre.
    std::string sunk_msg = "SUNK|" + sunk_ship;
    // Envía el mensaje de barco hundido al cliente a través del socket.
    result = send(client_socket, sunk_msg.c_str(), sunk_msg.size(), 0);
    // Si el envío falla, registra un error en el log con el mensaje de error del sistema.
    if (result == -1) {
        log_event(state, "Error enviando SUNK a " + logged_user + ": " + std::string(strerror(errno)));
    // Si el envío es exitoso, registra en el log que el usuario hundió un barco del oponente.
    } else {
        log_event(state, logged_user + " hundió " + sunk_ship + " de " + opponent);
    }
    // Pausa el hilo durante 50 milisegundos para evitar problemas de sincronización.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Verifica si todos los barcos en el tablero objetivo han sido hundidos.
if (target_board.allShipsSunk()) {
    // Crea un mensaje indicando que el usuario ganó la partida.
    std::string win_msg = "GAME|WIN";
    // Envía el mensaje de victoria al cliente.
    int result = send(client_socket, win_msg.c_str(), win_msg.size(), 0);
    // Si el envío falla, registra un error en el log.
    if (result == -1) {
        log_event(state, "Error enviando GAME|WIN a " + logged_user + ": " + std::string(strerror(errno)));
    // Si el envío es exitoso, registra en el log que el mensaje fue enviado.
    } else {
        log_event(state, "GAME|WIN enviado a " + logged_user);
    }
    // Pausa el hilo durante 50 milisegundos.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verifica si el oponente está conectado (su socket está registrado).
    if (state.user_sockets.contains(opponent)) {
        // Crea un mensaje indicando que el oponente perdió la partida.
        std::string lose_msg = "GAME|LOSE";
        // Envía el mensaje de derrota al oponente.
        result = send(state.user_sockets[opponent], lose_msg.c_str(), lose_msg.size(), 0);
        // Si el envío falla, registra un error en el log.
        if (result == -1) {
            log_event(state, "Error enviando GAME|LOSE a " + opponent + ": " + std::string(strerror(errno)));
        // Si el envío es exitoso, registra en el log que el mensaje fue enviado.
        } else {
            log_event(state, "GAME|LOSE enviado a " + opponent);
        }
    }
    // Registra en el log que la partida finalizó con la victoria del usuario.
    log_event(state, "Partida finalizada: " + logged_user + " ganó contra " + opponent);

    // Elimina la partida del registro del primer jugador.
    state.user_games.erase(game->player1);
    // Elimina la partida del registro del segundo jugador.
    state.user_games.erase(game->player2);
    // Busca la partida en el vector de partidas activas.
    auto it = std::ranges::find(state.games, game);
    // Si la partida se encuentra, la elimina del vector.
    if (it != state.games.end()) {
        state.games.erase(it);
    }
    // Libera la memoria asignada para la partida.
    delete game;

    // Verifica si el usuario está conectado.
    if (state.user_sockets.contains(logged_user)) {
        // Envía un mensaje al usuario indicando que la partida ha terminado.
        send(state.user_sockets[logged_user], "GAME|ENDED", strlen("GAME|ENDED"), 0);
    }
    // Verifica si el oponente está conectado.
    if (state.user_sockets.contains(opponent)) {
        // Envía un mensaje al oponente indicando que la partida ha terminado.
        send(state.user_sockets[opponent], "GAME|ENDED", strlen("GAME|ENDED"), 0);
    }
    // Sale de la función, terminando el procesamiento del comando.
    return;
}

// Si el disparo no acertó a un barco, ejecuta este bloque.
} else {
    // Marca la coordenada como un fallo en el tablero objetivo.
    target_board.setMiss(x_coord, y_coord);
    // Crea un mensaje indicando que el disparo falló, incluyendo las coordenadas.
    std::string miss_msg = "MISS|" + param1 + "|" + param2;
    // Envía el mensaje de fallo al cliente.
    int result = send(client_socket, miss_msg.c_str(), miss_msg.size(), 0);
    // Si el envío falla, registra un error en el log.
    if (result == -1) {
        log_event(state, "Error enviando MISS a " + logged_user + ": " + std::string(strerror(errno)));
    // Si el envío es exitoso, registra en el log que el usuario falló el disparo.
    } else {
        log_event(state, logged_user + " falló en (" + param1 + "," + param2 + ") contra " + opponent);
    }
    // Pausa el hilo durante 50 milisegundos.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Cambia el turno al oponente.
game->current_turn = opponent;
// Crea un mensaje indicando que es el turno del oponente.
std::string turn_msg = "TURN|" + opponent;
// Registra en el log que se está preparando el envío del mensaje de turno.
log_event(state, "Preparando envío de TURN a ambos jugadores: " + turn_msg);

// Verifica si el usuario está conectado.
if (state.user_sockets.contains(logged_user)) {
    // Envía el mensaje de turno al usuario.
    int result = send(state.user_sockets[logged_user], turn_msg.c_str(), turn_msg.size(), 0);
    // Si el envío falla, registra un error en el log.
    if (result == -1) {
        log_event(state, "Error enviando TURN a " + logged_user + ": " + std::string(strerror(errno)));
    // Si el envío es exitoso, registra en el log que el mensaje fue enviado.
    } else {
        log_event(state, "TURN enviado a " + logged_user + ": " + turn_msg);
    }
    // Pausa el hilo durante 50 milisegundos.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Verifica si el oponente está conectado.
if (state.user_sockets.contains(opponent)) {
    // Envía el mensaje de turno al oponente.
    int result = send(state.user_sockets[opponent], turn_msg.c_str(), turn_msg.size(), 0);
    // Si el envío falla, registra un error y concede la victoria al usuario.
    if (result == -1) {
        log_event(state, "Error enviando TURN a " + opponent + ": " + std::string(strerror(errno)));
        // Envía un mensaje de victoria al usuario.
        send(state.user_sockets[logged_user], "GAME|WIN", strlen("GAME|WIN"), 0);
        // Elimina la partida del registro del primer jugador.
        state.user_games.erase(game->player1);
        // Elimina la partida del registro del segundo jugador.
        state.user_games.erase(game->player2);
        // Busca la partida en el vector de partidas activas.
        auto it = std::ranges::find(state.games, game);
        // Si la partida se encuentra, la elimina del vector.
        if (it != state.games.end()) {
            state.games.erase(it);
        }
        // Libera la memoria asignada para la partida.
        delete game;
        // Sale de la función.
        return;
    // Si el envío es exitoso, registra en el log que el mensaje fue enviado.
    } else {
        log_event(state, "TURN enviado a " + opponent + ": " + turn_msg);
    }
// Si el oponente no está conectado, concede la victoria al usuario.
} else {
    // Crea un mensaje de victoria.
    std::string win_msg = "GAME|WIN";
    // Envía el mensaje de victoria al usuario.
    int result = send(state.user_sockets[logged_user], win_msg.c_str(), win_msg.size(), 0);
    // Si el envío falla, registra un error en el log.
    if (result == -1) {
        log_event(state, "Error enviando GAME|WIN por desconexión a " + logged_user + ": " + std::string(strerror(errno)));
    }
    // Registra en el log que el oponente se desconectó, dando la victoria al usuario.
    log_event(state, "Oponente " + opponent + " desconectado, victoria para " + logged_user);
    // Elimina la partida del registro del primer jugador.
    state.user_games.erase(game->player1);
    // Elimina la partida del registro del segundo jugador.
    state.user_games.erase(game->player2);
    // Busca la partida en el vector de partidas activas.
    auto it = std::ranges::find(state.games, game);
    // Si la partida se encuentra, la elimina del vector.
    if (it != state.games.end()) {
        state.games.erase(it);
    }
    // Libera la memoria asignada para la partida.
    delete game;
    // Sale de la función.
    return;
}

// Cierra el bloque de la función process_protocols.
}

// Declara una función para manejar la comunicación con un cliente conectado.
void handle_client(int client_socket, ServerState& state) {
    // Crea un búfer de 4096 bytes inicializado a cero para almacenar mensajes recibidos.
    char buffer[4096] = {0};
    // Declara una cadena para almacenar el nombre del usuario logueado.
    std::string logged_user;

    // Obtiene las banderas actuales del socket del cliente.
    int flags = fcntl(client_socket, F_GETFL, 0);
    // Configura el socket en modo bloqueante eliminando la bandera O_NONBLOCK.
    fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);

    // Inicia un bucle infinito para procesar mensajes del cliente.
    while (true) {
        // Lee datos del socket del cliente en el búfer.
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        // Si no se leyeron datos o hubo un error, maneja la desconexión.
        if (bytes_read <= 0) {
            // Si el usuario está logueado, procesa un comando de logout.
            if (!logged_user.empty()) {
                process_protocols("LOGOUT", logged_user, "", client_socket, logged_user, state);
                // Registra en el log que el usuario se desconectó.
                log_event(state, "Usuario desconectado y deslogueado: " + logged_user + " (recv falló con " +
                          std::to_string(bytes_read) + ", error: " + std::string(strerror(errno)) + ")");
                // Imprime en la consola que el cliente se desconectó.
                std::cout << "Cliente desconectado y deslogueado: " << logged_user << std::endl;
            // Si no hay usuario logueado, maneja la desconexión de un cliente anónimo.
            } else {
                // Registra en el log que un cliente anónimo se desconectó.
                log_event(state, "Cliente anónimo desconectado (recv falló con " + std::to_string(bytes_read) +
                          ", error: " + std::string(strerror(errno)) + ")");
                // Imprime en la consola que un cliente anónimo se desconectó.
                std::cout << "Cliente anónimo desconectado." << std::endl;
            }
            // Cierra el socket del cliente.
            close(client_socket);
            // Sale de la función.
            return;
        }

        // Agrega un terminador nulo al final de los datos recibidos.
        buffer[bytes_read] = '\0';
        // Convierte el búfer en una cadena.
        std::string message(buffer, bytes_read);
        // Crea un flujo de entrada para parsear el mensaje.
        std::istringstream msg_stream(message);
        // Declara variables para almacenar el comando y los parámetros.
        std::string command, param1, param2;
        // Lee el comando hasta el primer delimitador '|'.
        std::getline(msg_stream, command, '|');
        // Lee el primer parámetro hasta el siguiente delimitador '|'.
        std::getline(msg_stream, param1, '|');
        // Lee el segundo parámetro hasta el final.
        std::getline(msg_stream, param2);

        // Verifica si el comando es "LOGIN" y el usuario ya está logueado.
        if (command == "LOGIN" && !logged_user.empty()) {
            // Obtiene las banderas actuales del socket.
            int flags = fcntl(client_socket, F_GETFL, 0);
            // Configura el socket en modo no bloqueante.
            fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        }

        // Procesa el comando recibido usando la función process_protocols.
        process_protocols(command, param1, param2, client_socket, logged_user, state);
        // Si el comando es "LOGOUT", sale del bucle.
        if (command == "LOGOUT") {
            break;
        }
    }
}

// Declara una variable global volátil para controlar si el servidor está ejecutándose.
volatile sig_atomic_t server_running = 1;

// Declara una función para manejar señales del sistema.
void signal_handler(int sig) {
    // Establece la variable server_running a 0 para detener el servidor.
    server_running = 0;
}

// Declara la función principal del programa.
int main(int argc, char* argv[]) {
    // Verifica si se proporcionaron exactamente 4 argumentos (nombre del programa, IP, puerto, ruta de log).
    if (argc != 4) {
        // Imprime un mensaje de error con el uso correcto del programa.
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT> </path/log.log>" << std::endl;
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Crea una instancia de la estructura ServerState para almacenar el estado del servidor.
    ServerState state;
    // Asigna la dirección IP del primer argumento.
    std::string ip_address = argv[1];
    // Convierte el segundo argumento (puerto) a un entero.
    int port = std::stoi(argv[2]);
    // Asigna la ruta del archivo de log del tercer argumento.
    state.log_path = argv[3];

    // Imprime un mensaje indicando que el servidor está iniciando.
    std::cout << "Iniciando servidor Battleship..." << std::endl;

    // Configura el manejo de la señal SIGINT (Ctrl+C) para llamar a signal_handler.
    signal(SIGINT, signal_handler);
    // Configura el manejo de la señal SIGTERM para llamar a signal_handler.
    signal(SIGTERM, signal_handler);

    // Carga los usuarios desde el archivo de usuarios.
    load_users(state);

    // Crea un socket para el servidor usando IPv4 y TCP.
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Si la creación del socket falla, muestra un error.
    if (server_socket == -1) {
        std::cerr << "Error al crear socket: " << strerror(errno) << std::endl;
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Habilita la reutilización del puerto.
    int opt = 1;
    // Configura la opción SO_REUSEADDR en el socket.
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        // Si falla, muestra un error.
        std::cerr << "Error al configurar SO_REUSEADDR: " << strerror(errno) << std::endl;
        // Cierra el socket.
        close(server_socket);
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Crea una estructura sockaddr_in para la dirección del servidor.
    struct sockaddr_in address = { AF_INET, htons(port), {} };
    // Convierte la dirección IP de cadena a formato binario.
    if (inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr) <= 0) {
        // Si la conversión falla, muestra un error.
        std::cerr << "Dirección IP inválida: " << ip_address << std::endl;
        // Cierra el socket.
        close(server_socket);
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Asocia el socket a la dirección y puerto especificados.
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        // Si falla, muestra un error.
        std::cerr << "Error al bind: " << strerror(errno) << std::endl;
        // Cierra el socket.
        close(server_socket);
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Configura el socket para escuchar conexiones entrantes, hasta MAX_CLIENTS.
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        // Si falla, muestra un error.
        std::cerr << "Error al listen: " << strerror(errno) << std::endl;
        // Cierra el socket.
        close(server_socket);
        // Devuelve 1 para indicar un error.
        return 1;
    }

    // Imprime un mensaje indicando que el servidor está escuchando en la IP y puerto.
    std::cout << "Servidor iniciado y escuchando en " << ip_address << ":" << port << std::endl;

    // Crea un hilo para manejar el emparejamiento de jugadores.
    std::thread matchmaking_thread(matchmaking, std::ref(state));
    // Desacopla el hilo para que se ejecute de forma independiente.
    matchmaking_thread.detach();
    // Imprime un mensaje indicando que el hilo de emparejamiento ha iniciado.
    std::cout << "Hilo de matchmaking iniciado." << std::endl;

    // Crea un vector para almacenar los hilos de los clientes.
    std::vector<std::thread> client_threads;
    // Inicia un bucle mientras el servidor esté en ejecución.
    while (server_running) {
        // Acepta una nueva conexión de cliente.
        int new_socket = accept(server_socket, nullptr, nullptr);
        // Si la aceptación falla, muestra un error si el servidor aún está en ejecución.
        if (new_socket == -1) {
            if (server_running) {
                std::cerr << "Error al aceptar conexión: " << strerror(errno) << std::endl;
            }
            // Continúa con la siguiente iteración.
            continue;
        }
        // Imprime un mensaje indicando que se conectó un nuevo cliente.
        std::cout << "Nuevo cliente conectado." << std::endl;
        // Crea un nuevo hilo para manejar al cliente y lo agrega al vector.
        client_threads.emplace_back(handle_client, new_socket, std::ref(state));

        // Verifica si hay más de 10 hilos de clientes.
        if (client_threads.size() > 10) {
            // Crea un vector para almacenar los hilos activos.
            std::vector<std::thread> active_threads;
            // Itera sobre los hilos de clientes.
            for (auto& t : client_threads) {
                // Si el hilo es unible, lo une.
                if (t.joinable()) {
                    t.join();
                // Si no es unible, lo mueve al vector de hilos activos.
                } else {
                    active_threads.push_back(std::move(t));
                }
            }
            // Actualiza el vector de hilos con los hilos activos.
            client_threads = std::move(active_threads);
        }
    }

    // Imprime un mensaje indicando que el servidor se está cerrando.
    std::cout << "Cerrando servidor..." << std::endl;
    // Cierra el socket del servidor.
    close(server_socket);
    // Devuelve 0 para indicar que el programa terminó correctamente.
    return 0;
}}