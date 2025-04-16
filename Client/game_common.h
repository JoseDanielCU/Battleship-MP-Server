#ifndef GAME_COMMON_H // Directiva para evitar inclusión múltiple
#define GAME_COMMON_H
#include <vector> // Incluye la biblioteca para vectores
#include <string> // Incluye la biblioteca para cadenas

// Define el tamaño del tablero de juego (10x10)
#define BOARD_SIZE 10
// Define el símbolo para casillas vacías (agua)
#define WATER '~'
// Define el símbolo para impactos acertados
#define HIT 'X'
// Define el símbolo para disparos fallidos
#define MISS 'O'

// Estructura que representa un tipo de barco
struct Ship {
    std::string name; // Nombre del barco (ej. "Aircraft Carrier")
    int size; // Tamaño en casillas
    char symbol; // Símbolo que lo representa en el tablero
};

// Declaración externa del vector global de barcos
extern std::vector<Ship> ships;

// Clase que representa el tablero de juego
class Board {
public:
    // Matriz que almacena el estado del tablero
    std::vector<std::vector<char>> grid;
    // Estructura interna para rastrear instancias de barcos
    struct ShipInstance {
        std::string name; // Nombre del barco
        char symbol; // Símbolo del barco
        std::vector<std::pair<int, int>> positions; // Posiciones ocupadas
        bool sunk; // Indica si el barco está hundido
        // Constructor para inicializar una instancia de barco
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    // Vector que almacena las instancias de barcos en el tablero
    std::vector<ShipInstance> ship_instances;

    // Constructor de la clase Board
    Board();

    // Verifica si un barco puede colocarse en las coordenadas dadas
    bool canPlaceShip(int x, int y, int size, char direction);
    // Coloca un barco en el tablero
    void placeShip(int x, int y, int size, char direction, char symbol);
    // Rastrea las posiciones de un barco colocado
    void trackShip(const Ship& ship, int x, int y, char direction);
    // Actualiza el estado de hundimiento de los barcos
    void updateSunkStatus(int x, int y);
    // Serializa el tablero a una cadena para enviarlo por red
    std::string serialize();
};

#endif // Cierra la directiva de inclusión