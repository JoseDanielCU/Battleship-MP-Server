#include "game_common.h" // Incluye el encabezado con definiciones comunes

// Inicializa el vector global de barcos con sus tipos, tamaños y símbolos
std::vector<Ship> ships = {
    {"Aircraft Carrier", 5, 'A'}, // Portaaviones: 5 casillas
    {"Battleship", 4, 'B'}, // Acorazado: 4 casillas
    {"Cruiser", 3, 'C'}, {"Cruiser", 3, 'C'}, // Cruceros: 3 casillas cada uno
    {"Destroyer", 2, 'D'}, {"Destroyer", 2, 'D'}, // Destructores: 2 casillas cada uno
    {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'}, {"Submarine", 1, 'S'} // Submarinos: 1 casilla cada uno
};

// Constructor de la clase Board
Board::Board() {
    // Inicializa la matriz del tablero con agua ('~')
    grid.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, WATER));
}

// Verifica si un barco puede colocarse en las coordenadas dadas
bool Board::canPlaceShip(int x, int y, int size, char direction) {
    // Si la dirección es horizontal
    if (direction == 'H') {
        // Comprueba si el barco excede los límites del tablero
        if (y + size > BOARD_SIZE) return false;
        // Verifica que todas las casillas estén vacías (agua)
        for (int i = 0; i < size; i++)
            if (grid[x][y + i] != WATER) return false;
    } else { // Dirección vertical
        // Comprueba si el barco excede los límites del tablero
        if (x + size > BOARD_SIZE) return false;
        // Verifica que todas las casillas estén vacías (agua)
        for (int i = 0; i < size; i++)
            if (grid[x + i][y] != WATER) return false;
    }
    // Si todas las verificaciones pasan, el barco puede colocarse
    return true;
}

// Coloca un barco en el tablero
void Board::placeShip(int x, int y, int size, char direction, char symbol) {
    // Itera sobre el tamaño del barco
    for (int i = 0; i < size; i++) {
        // Si la dirección es horizontal, coloca el símbolo en la fila x
        if (direction == 'H')
            grid[x][y + i] = symbol;
        // Si la dirección es vertical, coloca el símbolo en la columna y
        else
            grid[x + i][y] = symbol;
    }
}

// Rastrea las posiciones de un barco colocado
void Board::trackShip(const Ship& ship, int x, int y, char direction) {
    // Vector para almacenar las posiciones del barco
    std::vector<std::pair<int, int>> positions;
    // Itera sobre el tamaño del barco
    for (int i = 0; i < ship.size; i++) {
        // Calcula las coordenadas según la dirección
        int pos_x = (direction == 'H') ? x : x + i;
        int pos_y = (direction == 'H') ? y + i : y;
        // Agrega la posición al vector
        positions.emplace_back(pos_x, pos_y);
    }
    // Crea una instancia del barco y la agrega al vector de instancias
    ship_instances.emplace_back(ship.name, ship.symbol, positions);
}

// Actualiza el estado de hundimiento de los barcos
void Board::updateSunkStatus(int x, int y) {
    // Itera sobre las instancias de barcos
    for (auto& ship : ship_instances) {
        // Si el barco no está hundido
        if (!ship.sunk) {
            // Busca la posición atacada
            for (const auto& pos : ship.positions) {
                if (pos.first == x && pos.second == y) {
                    // Verifica si todas las posiciones del barco han sido acertadas
                    bool all_hit = true;
                    for (const auto& p : ship.positions) {
                        if (grid[p.first][p.second] != HIT) {
                            all_hit = false;
                            break;
                        }
                    }
                    // Si todas las posiciones fueron acertadas, marca el barco como hundido
                    if (all_hit) {
                        ship.sunk = true;
                    }
                    break;
                }
            }
        }
    }
}

// Serializa el tablero a una cadena para enviarlo por red
std::string Board::serialize() {
    // Crea una cadena vacía para almacenar los datos
    std::string data;
    // Itera sobre las filas del tablero
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Itera sobre las columnas del tablero
        for (int j = 0; j < BOARD_SIZE; j++) {
            // Agrega el símbolo de cada casilla a la cadena
            data += grid[i][j];
        }
    }
    // Devuelve la cadena serializada
    return data;
}