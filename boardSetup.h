#ifndef BOARD_SETUP_H
#define BOARD_SETUP_H

#include <vector>
#include <string>

const int BOARD_SIZE = 10;
const char WATER = '~';

struct Ship {
    std::string name;
    int size;
    char symbol;
};

extern std::vector<Ship> ships;

class Board {
public:
    std::vector<std::vector<char>> grid;

    Board();

    void display();
    bool canPlaceShip(int x, int y, int size, char direction);
    void placeShip(int x, int y, int size, char direction, char symbol);
    void manualPlacement();
    void randomPlacement();

    std::string serialize();
    void deserialize(const std::string &data);
};

#endif
