#ifndef GAME_COMMON_H
#define GAME_COMMON_H
#include <vector>
#include <string>

#define BOARD_SIZE 10
#define WATER '~'
#define HIT 'X'
#define MISS 'O'

struct Ship {
    std::string name;
    int size;
    char symbol;
};

extern std::vector<Ship> ships;

class Board {
public:
    std::vector<std::vector<char>> grid;
    struct ShipInstance {
        std::string name;
        char symbol;
        std::vector<std::pair<int, int>> positions;
        bool sunk;
        ShipInstance(const std::string& n, char s, const std::vector<std::pair<int, int>>& pos)
            : name(n), symbol(s), positions(pos), sunk(false) {}
    };
    std::vector<ShipInstance> ship_instances;

    Board();

    bool canPlaceShip(int x, int y, int size, char direction);
    void placeShip(int x, int y, int size, char direction, char symbol);
    void trackShip(const Ship& ship, int x, int y, char direction);
    void updateSunkStatus(int x, int y);
    std::string serialize();
};

#endif