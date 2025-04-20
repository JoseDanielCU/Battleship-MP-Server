#include "game_common.h"

// Inicialización del vector global de barcos con sus atributos.
std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'}, // Portaaviones, ocupa 5 casillas.
    {"Battleship", 4, 'B'},      // Acorazado, ocupa 4 casillas.
    {"Cruiser", 3, 'C'},         // Crucero, ocupa 3 casillas.
    {"Cruiser", 3, 'C'},         // Segundo crucero.
    {"Destroyer", 2, 'D'},       // Destructor, ocupa 2 casillas.
    {"Destroyer", 2, 'D'},       // Segundo destructor.
    {"Submarine", 1, 'S'},       // Submarino, ocupa 1 casilla.
    {"Submarine", 1, 'S'},       // Segundo submarino.
    {"Submarine", 1, 'S'}        // Tercer submarino.
};

// Constructor de ShipInstance para inicializar una instancia de barco.
Board::ShipInstance::ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
    : name(n), symbol(s), positions(pos), sunk(false) {}

// Constructor que inicializa el tablero con agua.
Board::Board() {
    grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
}

// Deserializa un tablero desde una cadena de datos.
void Board::deserialize(const std::string& data) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            grid[i][j] = data[i * BOARD_SIZE + j];
        }
    }
    rebuildShipInstances();
}

// Reconstruye las instancias de barcos basándose en el tablero.
void Board::rebuildShipInstances() {
    ship_instances.clear();
    std::map<char, std::vector<std::pair<int, int>>> symbol_to_positions;

    // Recorre el tablero para recolectar posiciones de barcos.
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                symbol_to_positions[grid[i][j]].emplace_back(i, j);
            }
        }
    }

    // Reconstruye instancias de barcos basándose en las posiciones recolectadas.
    for (const auto& ship : ships) {
        auto& positions = symbol_to_positions[ship.symbol];
        int size = ship.size;

        while (positions.size() >= size) {
            std::vector<std::pair<int, int>> current_ship;
            for (auto it = positions.begin(); it != positions.end(); ) {
                if (current_ship.empty()) {
                    current_ship.push_back(*it);
                    it = positions.erase(it);
                } else {
                    bool adjacent = false;
                    auto last = current_ship.back();
                    for (auto pos_it = it; pos_it != positions.end(); ++pos_it) {
                        if ((pos_it->first == last.first && abs(pos_it->second - last.second) == 1) ||
                            (pos_it->second == last.second && abs(pos_it->first - last.first) == 1)) {
                            current_ship.push_back(*pos_it);
                            it = positions.erase(pos_it);
                            adjacent = true;
                            break;
                        }
                    }
                    if (!adjacent) break;
                }
                if (current_ship.size() == size) break;
            }
            if (current_ship.size() == size) {
                ship_instances.emplace_back(ship.name, ship.symbol, current_ship);
            } else {
                positions.insert(positions.begin(), current_ship.begin(), current_ship.end());
                break;
            }
        }
    }
}

// Verifica si una posición es un impacto (contiene un barco).
bool Board::isHit(int x, int y) const {
    return grid[x][y] != WATER && grid[x][y] != HIT && grid[x][y] != MISS;
}

// Obtiene el símbolo en una posición específica.
char Board::getSymbol(int x, int y) const {
    return grid[x][y];
}

// Marca una posición como impacto.
void Board::setHit(int x, int y) {
    grid[x][y] = HIT;
    checkSunk(x, y);
}

// Marca una posición como fallo.
void Board::setMiss(int x, int y) {
    grid[x][y] = MISS;
}

// Verifica si un barco se ha hundido tras un impacto.
void Board::checkSunk(int x, int y) {
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
                    return;
                }
            }
        }
    }
}

// Obtiene el nombre de un barco hundido en una posición.
std::string Board::getSunkShipName(int x, int y) {
    for (const auto& ship : ship_instances) {
        if (ship.sunk) {
            for (const auto& pos : ship.positions) {
                if (pos.first == x && pos.second == y) {
                    return ship.name;
                }
            }
        }
    }
    return "";
}

// Verifica si todos los barcos están hundidos.
bool Board::allShipsSunk() {
    for (const auto& ship : ship_instances) {
        if (!ship.sunk) return false;
    }
    return true;
}

// Cuenta el número total de casillas ocupadas por barcos.
int Board::countShips() const {
    int total_length = 0;
    for (const auto& ship : ships) {
        total_length += ship.size;
    }
    int current_length = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (grid[i][j] != WATER && grid[i][j] != HIT && grid[i][j] != MISS) {
                current_length++;
            }
        }
    }
    return (current_length == total_length) ? total_length : 0;
}