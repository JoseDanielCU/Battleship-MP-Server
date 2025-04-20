#pragma once

// Incluye la biblioteca para contenedores de vectores, como std::vector.
#include <vector>
// Incluye la biblioteca para manipulación de cadenas, como std::string.
#include <string>
// Incluye la biblioteca para contenedores asociativos, como std::map.
#include <map>

// Tamaño del tablero (10x10).
#define BOARD_SIZE 10
// Símbolo para representar agua en el tablero.
#define WATER '~'
// Símbolo para representar un impacto exitoso en el tablero.
#define HIT 'X'
// Símbolo para representar un disparo fallido en el tablero.
#define MISS 'O'

// Estructura para representar un barco con su nombre, tamaño y símbolo.
struct Ship {
    std::string name; // Nombre del barco (ej. "Aircraft Carrier").
    int size;         // Tamaño del barco (número de casillas que ocupa).
    char symbol;      // Símbolo que representa el barco en el tablero.
};

// Vector global que contiene todos los barcos disponibles.
extern std::vector<Ship> ships;

// Clase que representa el tablero de juego.
class Board {
public:
    // Matriz que representa el tablero.
    std::vector<std::vector<char>> grid;
    // Estructura para representar una instancia de un barco en el tablero.
    struct ShipInstance {
        std::string name;                     // Nombre del barco.
        char symbol;                          // Símbolo del barco.
        std::vector<std::pair<int, int>> positions; // Posiciones ocupadas por el barco.
        bool sunk;                            // Indica si el barco está hundido.
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos);
    };
    // Lista de instancias de barcos en el tablero.
    std::vector<ShipInstance> ship_instances;

    // Constructor que inicializa el tablero con agua.
    Board();
    // Deserializa un tablero desde una cadena de datos.
    void deserialize(const std::string& data);
    // Reconstruye las instancias de barcos basándose en el tablero.
    void rebuildShipInstances();
    // Verifica si una posición es un impacto (contiene un barco).
    bool isHit(int x, int y) const;
    // Obtiene el símbolo en una posición específica.
    char getSymbol(int x, int y) const;
    // Marca una posición como impacto.
    void setHit(int x, int y);
    // Marca una posición como fallo.
    void setMiss(int x, int y);
    // Verifica si un barco se ha hundido tras un impacto.
    void checkSunk(int x, int y);
    // Obtiene el nombre de un barco hundido en una posición.
    std::string getSunkShipName(int x, int y);
    // Verifica si todos los barcos están hundidos.
    bool allShipsSunk();
    // Cuenta el número total de casillas ocupadas por barcos.
    int countShips() const;
};